# Buffer Sharing and Synchronization (dma-buf)

> 출처(원문): https://docs.kernel.org/driver-api/dma-buf.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Buffer Sharing and Synchronization (dma-buf)

The dma-buf subsystem provides the framework for sharing buffers for
hardware (DMA) access across multiple device drivers and subsystems, and
for synchronizing asynchronous hardware access.

As an example, it is used extensively by the DRM subsystem to exchange
buffers between processes, contexts, library APIs within the same
process, and also to exchange buffers with other subsystems such as
V4L2.

This document describes the way in which kernel subsystems can use and
interact with the three main primitives offered by dma-buf:

> * dma-buf, representing a sg\_table and exposed to userspace as a file
>   descriptor to allow passing between processes, subsystems, devices,
>   etc;
> * dma-fence, providing a mechanism to signal when an asynchronous
>   hardware operation has completed; and
> * dma-resv, which manages a set of dma-fences for a particular dma-buf
>   allowing implicit (kernel-ordered) synchronization of work to
>   preserve the illusion of coherent access

## Userspace API principles and use

For more details on how to design your subsystem’s API for dma-buf use, please
see [Exchanging pixel buffers](../userspace-api/dma-buf-alloc-exchange.html).

## Shared DMA Buffers

This document serves as a guide to device-driver writers on what is the dma-buf
buffer sharing API, how to use it for exporting and using shared buffers.

Any device driver which wishes to be a part of DMA buffer sharing, can do so as
either the ‘exporter’ of buffers, or the ‘user’ or ‘importer’ of buffers.

Say a driver A wants to use buffers created by driver B, then we call B as the
exporter, and A as buffer-user/importer.

The exporter

> * implements and manages operations in [`struct dma_buf_ops`](#c.dma_buf_ops "dma_buf_ops") for the buffer,
> * allows other users to share the buffer by using dma\_buf sharing APIs,
> * manages the details of buffer allocation, wrapped in a [`struct
>   dma_buf`](#c.dma_buf "dma_buf"),
> * decides about the actual backing storage where this allocation happens,
> * and takes care of any migration of scatterlist - for all (shared) users of
>   this buffer.

The buffer-user

> * is one of (many) sharing users of the buffer.
> * doesn’t need to worry about how the buffer is allocated, or where.
> * and needs a mechanism to get access to the scatterlist that makes up this
>   buffer in memory, mapped into its own address space, so it can access the
>   same area of memory. This interface is provided by [`struct
>   dma_buf_attachment`](#c.dma_buf_attachment "dma_buf_attachment").

Any exporters or users of the dma-buf buffer sharing framework must have a
‘select DMA\_SHARED\_BUFFER’ in their respective Kconfigs.

### Userspace Interface Notes

Mostly a DMA buffer file descriptor is simply an opaque object for userspace,
and hence the generic interface exposed is very minimal. There’s a few things to
consider though:

* Since kernel 3.12 the dma-buf FD supports the llseek system call, but only
  with offset=0 and whence=SEEK\_END|SEEK\_SET. SEEK\_SET is supported to allow
  the usual size discover pattern size = SEEK\_END(0); SEEK\_SET(0). Every other
  llseek operation will report -EINVAL.

  If llseek on dma-buf FDs isn’t supported the kernel will report -ESPIPE for all
  cases. Userspace can use this to detect support for discovering the dma-buf
  size using llseek.
* In order to avoid fd leaks on exec, the FD\_CLOEXEC flag must be set
  on the file descriptor. This is not just a resource leak, but a
  potential security hole. It could give the newly exec’d application
  access to buffers, via the leaked fd, to which it should otherwise
  not be permitted access.

  The problem with doing this via a separate fcntl() call, versus doing it
  atomically when the fd is created, is that this is inherently racy in a
  multi-threaded app[3]. The issue is made worse when it is library code
  opening/creating the file descriptor, as the application may not even be
  aware of the fd’s.

  To avoid this problem, userspace must have a way to request O\_CLOEXEC
  flag be set when the dma-buf fd is created. So any API provided by
  the exporting driver to create a dmabuf fd must provide a way to let
  userspace control setting of O\_CLOEXEC flag passed in to [`dma_buf_fd()`](#c.dma_buf_fd "dma_buf_fd").
* Memory mapping the contents of the DMA buffer is also supported. See the
  discussion below on [CPU Access to DMA Buffer Objects](#cpu-access-to-dma-buffer-objects) for the full details.
* The DMA buffer FD is also pollable, see [Implicit Fence Poll Support](#implicit-fence-poll-support) below for
  details.
* The DMA buffer FD also supports a few dma-buf-specific ioctls, see
  [DMA Buffer ioctls](#dma-buffer-ioctls) below for details.

### Basic Operation and Device DMA Access

For device DMA access to a shared DMA buffer the usual sequence of operations
is fairly simple:

1. The exporter defines his exporter instance using
   [`DEFINE_DMA_BUF_EXPORT_INFO()`](#c.DEFINE_DMA_BUF_EXPORT_INFO "DEFINE_DMA_BUF_EXPORT_INFO") and calls [`dma_buf_export()`](#c.dma_buf_export "dma_buf_export") to wrap a private
   buffer object into a [`dma_buf`](#c.dma_buf "dma_buf"). It then exports that [`dma_buf`](#c.dma_buf "dma_buf") to userspace
   as a file descriptor by calling [`dma_buf_fd()`](#c.dma_buf_fd "dma_buf_fd").
2. Userspace passes this file-descriptors to all drivers it wants this buffer
   to share with: First the file descriptor is converted to a [`dma_buf`](#c.dma_buf "dma_buf") using
   [`dma_buf_get()`](#c.dma_buf_get "dma_buf_get"). Then the buffer is attached to the device using
   [`dma_buf_attach()`](#c.dma_buf_attach "dma_buf_attach").

   Up to this stage the exporter is still free to migrate or reallocate the
   backing storage.
3. Once the buffer is attached to all devices userspace can initiate DMA
   access to the shared buffer. In the kernel this is done by calling
   [`dma_buf_map_attachment()`](#c.dma_buf_map_attachment "dma_buf_map_attachment") and [`dma_buf_unmap_attachment()`](#c.dma_buf_unmap_attachment "dma_buf_unmap_attachment").
4. Once a driver is done with a shared buffer it needs to call
   [`dma_buf_detach()`](#c.dma_buf_detach "dma_buf_detach") (after cleaning up any mappings) and then release the
   reference acquired with [`dma_buf_get()`](#c.dma_buf_get "dma_buf_get") by calling [`dma_buf_put()`](#c.dma_buf_put "dma_buf_put").

For the detailed semantics exporters are expected to implement see
[`dma_buf_ops`](#c.dma_buf_ops "dma_buf_ops").

### CPU Access to DMA Buffer Objects

There are multiple reasons for supporting CPU access to a dma buffer object:

* Fallback operations in the kernel, for example when a device is connected
  over USB and the kernel needs to shuffle the data around first before
  sending it away. Cache coherency is handled by bracketing any transactions
  with calls to [`dma_buf_begin_cpu_access()`](#c.dma_buf_begin_cpu_access "dma_buf_begin_cpu_access") and [`dma_buf_end_cpu_access()`](#c.dma_buf_end_cpu_access "dma_buf_end_cpu_access")
  access.

  Since for most kernel internal dma-buf accesses need the entire buffer, a
  vmap interface is introduced. Note that on very old 32-bit architectures
  vmalloc space might be limited and result in vmap calls failing.

  Interfaces:

  ```
  void *dma_buf_vmap(struct dma_buf *dmabuf, struct iosys_map *map)
  void dma_buf_vunmap(struct dma_buf *dmabuf, struct iosys_map *map)
  ```

  The vmap call can fail if there is no vmap support in the exporter, or if
  it runs out of vmalloc space. Note that the dma-buf layer keeps a reference
  count for all vmap access and calls down into the exporter’s vmap function
  only when no vmapping exists, and only unmaps it once. Protection against
  concurrent vmap/vunmap calls is provided by taking the [`dma_buf.lock`](#c.dma_buf "dma_buf") mutex.
* For full compatibility on the importer side with existing userspace
  interfaces, which might already support mmap’ing buffers. This is needed in
  many processing pipelines (e.g. feeding a software rendered image into a
  hardware pipeline, thumbnail creation, snapshots, ...). Also, Android’s ION
  framework already supported this and for DMA buffer file descriptors to
  replace ION buffers mmap support was needed.

  There is no special interfaces, userspace simply calls mmap on the dma-buf
  fd. But like for CPU access there’s a need to bracket the actual access,
  which is handled by the ioctl (DMA\_BUF\_IOCTL\_SYNC). Note that
  DMA\_BUF\_IOCTL\_SYNC can fail with -EAGAIN or -EINTR, in which case it must
  be restarted.

  Some systems might need some sort of cache coherency management e.g. when
  CPU and GPU domains are being accessed through dma-buf at the same time.
  To circumvent this problem there are begin/end coherency markers, that
  forward directly to existing dma-buf device drivers vfunc hooks. Userspace
  can make use of those markers through the DMA\_BUF\_IOCTL\_SYNC ioctl. The
  sequence would be used like following:

  > > + mmap dma-buf fd
  > > + for each drawing/upload cycle in CPU 1. SYNC\_START ioctl, 2. read/write
  > >   to mmap area 3. SYNC\_END ioctl. This can be repeated as often as you
  > >   want (with the new data being consumed by say the GPU or the scanout
  > >   device)
  > > + munmap once you don’t need the buffer any more
  >
  > For correctness and optimal performance, it is always required to use
  > SYNC\_START and SYNC\_END before and after, respectively, when accessing the
  > mapped address. Userspace cannot rely on coherent access, even when there
  > are systems where it just works without calling these ioctls.
* And as a CPU fallback in userspace processing pipelines.

  Similar to the motivation for kernel cpu access it is again important that
  the userspace code of a given importing subsystem can use the same
  interfaces with a imported dma-buf buffer object as with a native buffer
  object. This is especially important for drm where the userspace part of
  contemporary OpenGL, X, and other drivers is huge, and reworking them to
  use a different way to mmap a buffer rather invasive.

  The assumption in the current dma-buf interfaces is that redirecting the
  initial mmap is all that’s needed. A survey of some of the existing
  subsystems shows that no driver seems to do any nefarious thing like
  syncing up with outstanding asynchronous processing on the device or
  allocating special resources at fault time. So hopefully this is good
  enough, since adding interfaces to intercept pagefaults and allow pte
  shootdowns would increase the complexity quite a bit.

  Interface:

  ```
  int dma_buf_mmap(struct dma_buf *, struct vm_area_struct *, unsigned long);
  ```

  If the importing subsystem simply provides a special-purpose mmap call to
  set up a mapping in userspace, calling do\_mmap with [`dma_buf.file`](#c.dma_buf "dma_buf") will
  equally achieve that for a dma-buf object.

### Implicit Fence Poll Support

To support cross-device and cross-driver synchronization of buffer access
implicit fences (represented internally in the kernel with [`struct dma_fence`](#c.dma_fence "dma_fence"))
can be attached to a [`dma_buf`](#c.dma_buf "dma_buf"). The glue for that and a few related things are
provided in the [`dma_resv`](#c.dma_resv "dma_resv") structure.

Userspace can query the state of these implicitly tracked fences using poll()
and related system calls:

* Checking for EPOLLIN, i.e. read access, can be use to query the state of the
  most recent write or exclusive fence.
* Checking for EPOLLOUT, i.e. write access, can be used to query the state of
  all attached fences, shared and exclusive ones.

Note that this only signals the completion of the respective fences, i.e. the
DMA transfers are complete. Cache flushing and any other necessary
preparations before CPU access can begin still need to happen.

As an alternative to poll(), the set of fences on DMA buffer can be
exported as a [`sync_file`](#c.sync_file "sync_file") using `dma_buf_sync_file_export`.

### DMA Buffer ioctls

struct dma\_buf\_sync
:   Synchronize with CPU access.

**Definition**:

```
struct dma_buf_sync {
    __u64 flags;
};
```

**Members**

`flags`
:   Set of access flags

    DMA\_BUF\_SYNC\_START:
    :   Indicates the start of a map access session.

    DMA\_BUF\_SYNC\_END:
    :   Indicates the end of a map access session.

    DMA\_BUF\_SYNC\_READ:
    :   Indicates that the mapped DMA buffer will be read by the
        client via the CPU map.

    DMA\_BUF\_SYNC\_WRITE:
    :   Indicates that the mapped DMA buffer will be written by the
        client via the CPU map.

    DMA\_BUF\_SYNC\_RW:
    :   An alias for DMA\_BUF\_SYNC\_READ | DMA\_BUF\_SYNC\_WRITE.

**Description**

When a DMA buffer is accessed from the CPU via mmap, it is not always
possible to guarantee coherency between the CPU-visible map and underlying
memory. To manage coherency, DMA\_BUF\_IOCTL\_SYNC must be used to bracket
any CPU access to give the kernel the chance to shuffle memory around if
needed.

Prior to accessing the map, the client must call DMA\_BUF\_IOCTL\_SYNC
with DMA\_BUF\_SYNC\_START and the appropriate read/write flags. Once the
access is complete, the client should call DMA\_BUF\_IOCTL\_SYNC with
DMA\_BUF\_SYNC\_END and the same read/write flags.

The synchronization provided via DMA\_BUF\_IOCTL\_SYNC only provides cache
coherency. It does not prevent other processes or devices from
accessing the memory at the same time. If synchronization with a GPU or
other device driver is required, it is the client’s responsibility to
wait for buffer to be ready for reading or writing before calling this
ioctl with DMA\_BUF\_SYNC\_START. Likewise, the client must ensure that
follow-up work is not submitted to GPU or other device driver until
after this ioctl has been called with DMA\_BUF\_SYNC\_END?

If the driver or API with which the client is interacting uses implicit
synchronization, waiting for prior work to complete can be done via
poll() on the DMA buffer file descriptor. If the driver or API requires
explicit synchronization, the client may have to wait on a sync\_file or
other synchronization primitive outside the scope of the DMA buffer API.

struct dma\_buf\_export\_sync\_file
:   Get a sync\_file from a dma-buf

**Definition**:

```
struct dma_buf_export_sync_file {
    __u32 flags;
    __s32 fd;
};
```

**Members**

`flags`
:   Read/write flags

    Must be DMA\_BUF\_SYNC\_READ, DMA\_BUF\_SYNC\_WRITE, or both.

    If DMA\_BUF\_SYNC\_READ is set and DMA\_BUF\_SYNC\_WRITE is not set,
    the returned sync file waits on any writers of the dma-buf to
    complete. Waiting on the returned sync file is equivalent to
    poll() with POLLIN.

    If DMA\_BUF\_SYNC\_WRITE is set, the returned sync file waits on
    any users of the dma-buf (read or write) to complete. Waiting
    on the returned sync file is equivalent to poll() with POLLOUT.
    If both DMA\_BUF\_SYNC\_WRITE and DMA\_BUF\_SYNC\_READ are set, this
    is equivalent to just DMA\_BUF\_SYNC\_WRITE.

`fd`
:   Returned sync file descriptor

**Description**

Userspace can perform a DMA\_BUF\_IOCTL\_EXPORT\_SYNC\_FILE to retrieve the
current set of fences on a dma-buf file descriptor as a sync\_file. CPU
waits via poll() or other driver-specific mechanisms typically wait on
whatever fences are on the dma-buf at the time the wait begins. This
is similar except that it takes a snapshot of the current fences on the
dma-buf for waiting later instead of waiting immediately. This is
useful for modern graphics APIs such as Vulkan which assume an explicit
synchronization model but still need to inter-operate with dma-buf.

The intended usage pattern is the following:

> 1. Export a sync\_file with flags corresponding to the expected GPU usage
>    via DMA\_BUF\_IOCTL\_EXPORT\_SYNC\_FILE.
> 2. Submit rendering work which uses the dma-buf. The work should wait on
>    the exported sync file before rendering and produce another sync\_file
>    when complete.
> 3. Import the rendering-complete sync\_file into the dma-buf with flags
>    corresponding to the GPU usage via DMA\_BUF\_IOCTL\_IMPORT\_SYNC\_FILE.

Unlike doing implicit synchronization via a GPU kernel driver’s exec ioctl,
the above is not a single atomic operation. If userspace wants to ensure
ordering via these fences, it is the respnosibility of userspace to use
locks or other mechanisms to ensure that no other context adds fences or
submits work between steps 1 and 3 above.

struct dma\_buf\_import\_sync\_file
:   Insert a sync\_file into a dma-buf

**Definition**:

```
struct dma_buf_import_sync_file {
    __u32 flags;
    __s32 fd;
};
```

**Members**

`flags`
:   Read/write flags

    Must be DMA\_BUF\_SYNC\_READ, DMA\_BUF\_SYNC\_WRITE, or both.

    If DMA\_BUF\_SYNC\_READ is set and DMA\_BUF\_SYNC\_WRITE is not set,
    this inserts the sync\_file as a read-only fence. Any subsequent
    implicitly synchronized writes to this dma-buf will wait on this
    fence but reads will not.

    If DMA\_BUF\_SYNC\_WRITE is set, this inserts the sync\_file as a
    write fence. All subsequent implicitly synchronized access to
    this dma-buf will wait on this fence.

`fd`
:   Sync file descriptor

**Description**

Userspace can perform a DMA\_BUF\_IOCTL\_IMPORT\_SYNC\_FILE to insert a
sync\_file into a dma-buf for the purposes of implicit synchronization
with other dma-buf consumers. This allows clients using explicitly
synchronized APIs such as Vulkan to inter-op with dma-buf consumers
which expect implicit synchronization such as OpenGL or most media
drivers/video.

### DMA-BUF locking convention

In order to avoid deadlock situations between dma-buf exports and importers,
all dma-buf API users must follow the common dma-buf locking convention.

Convention for importers

1. Importers must hold the dma-buf reservation lock when calling these
   functions:

   > * [`dma_buf_pin()`](#c.dma_buf_pin "dma_buf_pin")
   > * [`dma_buf_unpin()`](#c.dma_buf_unpin "dma_buf_unpin")
   > * [`dma_buf_map_attachment()`](#c.dma_buf_map_attachment "dma_buf_map_attachment")
   > * [`dma_buf_unmap_attachment()`](#c.dma_buf_unmap_attachment "dma_buf_unmap_attachment")
   > * [`dma_buf_vmap()`](#c.dma_buf_vmap "dma_buf_vmap")
   > * [`dma_buf_vunmap()`](#c.dma_buf_vunmap "dma_buf_vunmap")
2. Importers must not hold the dma-buf reservation lock when calling these
   functions:

   > * [`dma_buf_attach()`](#c.dma_buf_attach "dma_buf_attach")
   > * [`dma_buf_dynamic_attach()`](#c.dma_buf_dynamic_attach "dma_buf_dynamic_attach")
   > * [`dma_buf_detach()`](#c.dma_buf_detach "dma_buf_detach")
   > * [`dma_buf_export()`](#c.dma_buf_export "dma_buf_export")
   > * [`dma_buf_fd()`](#c.dma_buf_fd "dma_buf_fd")
   > * [`dma_buf_get()`](#c.dma_buf_get "dma_buf_get")
   > * [`dma_buf_put()`](#c.dma_buf_put "dma_buf_put")
   > * [`dma_buf_mmap()`](#c.dma_buf_mmap "dma_buf_mmap")
   > * [`dma_buf_begin_cpu_access()`](#c.dma_buf_begin_cpu_access "dma_buf_begin_cpu_access")
   > * [`dma_buf_end_cpu_access()`](#c.dma_buf_end_cpu_access "dma_buf_end_cpu_access")
   > * [`dma_buf_map_attachment_unlocked()`](#c.dma_buf_map_attachment_unlocked "dma_buf_map_attachment_unlocked")
   > * [`dma_buf_unmap_attachment_unlocked()`](#c.dma_buf_unmap_attachment_unlocked "dma_buf_unmap_attachment_unlocked")
   > * [`dma_buf_vmap_unlocked()`](#c.dma_buf_vmap_unlocked "dma_buf_vmap_unlocked")
   > * [`dma_buf_vunmap_unlocked()`](#c.dma_buf_vunmap_unlocked "dma_buf_vunmap_unlocked")

Convention for exporters

1. These [`dma_buf_ops`](#c.dma_buf_ops "dma_buf_ops") callbacks are invoked with unlocked dma-buf
   reservation and exporter can take the lock:

   > * [`dma_buf_ops.attach()`](#c.dma_buf_ops "dma_buf_ops")
   > * [`dma_buf_ops.detach()`](#c.dma_buf_ops "dma_buf_ops")
   > * [`dma_buf_ops.release()`](#c.dma_buf_ops "dma_buf_ops")
   > * [`dma_buf_ops.begin_cpu_access()`](#c.dma_buf_ops "dma_buf_ops")
   > * [`dma_buf_ops.end_cpu_access()`](#c.dma_buf_ops "dma_buf_ops")
   > * [`dma_buf_ops.mmap()`](#c.dma_buf_ops "dma_buf_ops")
2. These [`dma_buf_ops`](#c.dma_buf_ops "dma_buf_ops") callbacks are invoked with locked dma-buf
   reservation and exporter can’t take the lock:

   > * [`dma_buf_ops.pin()`](#c.dma_buf_ops "dma_buf_ops")
   > * [`dma_buf_ops.unpin()`](#c.dma_buf_ops "dma_buf_ops")
   > * [`dma_buf_ops.map_dma_buf()`](#c.dma_buf_ops "dma_buf_ops")
   > * [`dma_buf_ops.unmap_dma_buf()`](#c.dma_buf_ops "dma_buf_ops")
   > * [`dma_buf_ops.vmap()`](#c.dma_buf_ops "dma_buf_ops")
   > * [`dma_buf_ops.vunmap()`](#c.dma_buf_ops "dma_buf_ops")
3. Exporters must hold the dma-buf reservation lock when calling these
   functions:

   > * [`dma_buf_invalidate_mappings()`](#c.dma_buf_invalidate_mappings "dma_buf_invalidate_mappings")

### Kernel Functions and Structures Reference

struct [dma\_buf](#c.dma_buf "dma_buf") \*dma\_buf\_export(const struct [dma\_buf\_export\_info](#c.dma_buf_export_info "dma_buf_export_info") \*exp\_info)
:   Creates a new dma\_buf, and associates an anon file with this buffer, so it can be exported. Also connect the allocator specific data and ops to the buffer. Additionally, provide a name string for exporter; useful in debugging.

**Parameters**

`const struct dma_buf_export_info *exp_info`
:   [in] holds all the export related information provided
    by the exporter. see [`struct dma_buf_export_info`](#c.dma_buf_export_info "dma_buf_export_info")
    for further details.

**Description**

Returns, on success, a newly created [`struct dma_buf`](#c.dma_buf "dma_buf") object, which wraps the
supplied private data and operations for [`struct dma_buf_ops`](#c.dma_buf_ops "dma_buf_ops"). On either
missing ops, or error in allocating [`struct dma_buf`](#c.dma_buf "dma_buf"), will return negative
error.

For most cases the easiest way to create **exp\_info** is through the
`DEFINE_DMA_BUF_EXPORT_INFO` macro.

int dma\_buf\_fd(struct [dma\_buf](#c.dma_buf "dma_buf") \*dmabuf, int flags)
:   returns a file descriptor for the given [`struct dma_buf`](#c.dma_buf "dma_buf")

**Parameters**

`struct dma_buf *dmabuf`
:   [in] pointer to dma\_buf for which fd is required.

`int flags`
:   [in] flags to give to fd

**Description**

On success, returns an associated ‘fd’. Else, returns error.

struct [dma\_buf](#c.dma_buf "dma_buf") \*dma\_buf\_get(int fd)
:   returns the [`struct dma_buf`](#c.dma_buf "dma_buf") related to an fd

**Parameters**

`int fd`
:   [in] fd associated with the [`struct dma_buf`](#c.dma_buf "dma_buf") to be returned

**Description**

On success, returns the [`struct dma_buf`](#c.dma_buf "dma_buf") associated with an fd; uses
file’s refcounting done by fget to increase refcount. returns ERR\_PTR
otherwise.

void dma\_buf\_put(struct [dma\_buf](#c.dma_buf "dma_buf") \*dmabuf)
:   decreases refcount of the buffer

**Parameters**

`struct dma_buf *dmabuf`
:   [in] buffer to reduce refcount of

**Description**

Uses file’s refcounting done implicitly by `fput()`.

If, as a result of this call, the refcount becomes 0, the ‘release’ file
operation related to this fd is called. It calls [`dma_buf_ops.release`](#c.dma_buf_ops "dma_buf_ops") vfunc
in turn, and frees the memory allocated for dmabuf when exported.

struct [dma\_buf\_attachment](#c.dma_buf_attachment "dma_buf_attachment") \*dma\_buf\_dynamic\_attach(struct [dma\_buf](#c.dma_buf "dma_buf") \*dmabuf, struct [device](infrastructure.html#c.device "device") \*dev, const struct [dma\_buf\_attach\_ops](#c.dma_buf_attach_ops "dma_buf_attach_ops") \*importer\_ops, void \*importer\_priv)
:   Add the device to dma\_buf’s attachments list

**Parameters**

`struct dma_buf *dmabuf`
:   [in] buffer to attach device to.

`struct device *dev`
:   [in] device to be attached.

`const struct dma_buf_attach_ops *importer_ops`
:   [in] importer operations for the attachment

`void *importer_priv`
:   [in] importer private pointer for the attachment

**Description**

Returns [`struct dma_buf_attachment`](#c.dma_buf_attachment "dma_buf_attachment") pointer for this attachment. Attachments
must be cleaned up by calling [`dma_buf_detach()`](#c.dma_buf_detach "dma_buf_detach").

Optionally this calls [`dma_buf_ops.attach`](#c.dma_buf_ops "dma_buf_ops") to allow device-specific attach
functionality.

A pointer to newly created [`dma_buf_attachment`](#c.dma_buf_attachment "dma_buf_attachment") on success, or a negative
error code wrapped into a pointer on failure.

Note that this can fail if the backing storage of **dmabuf** is in a place not
accessible to **dev**, and cannot be moved to a more suitable place. This is
indicated with the error code -EBUSY.

struct [dma\_buf\_attachment](#c.dma_buf_attachment "dma_buf_attachment") \*dma\_buf\_attach(struct [dma\_buf](#c.dma_buf "dma_buf") \*dmabuf, struct [device](infrastructure.html#c.device "device") \*dev)
:   Wrapper for dma\_buf\_dynamic\_attach

**Parameters**

`struct dma_buf *dmabuf`
:   [in] buffer to attach device to.

`struct device *dev`
:   [in] device to be attached.

**Description**

Wrapper to call [`dma_buf_dynamic_attach()`](#c.dma_buf_dynamic_attach "dma_buf_dynamic_attach") for drivers which still use a static
mapping.

void dma\_buf\_detach(struct [dma\_buf](#c.dma_buf "dma_buf") \*dmabuf, struct [dma\_buf\_attachment](#c.dma_buf_attachment "dma_buf_attachment") \*attach)
:   Remove the given attachment from dmabuf’s attachments list

**Parameters**

`struct dma_buf *dmabuf`
:   [in] buffer to detach from.

`struct dma_buf_attachment *attach`
:   [in] attachment to be detached; is free’d after this call.

**Description**

Clean up a device attachment obtained by calling [`dma_buf_attach()`](#c.dma_buf_attach "dma_buf_attach").

Optionally this calls [`dma_buf_ops.detach`](#c.dma_buf_ops "dma_buf_ops") for device-specific detach.

int dma\_buf\_pin(struct [dma\_buf\_attachment](#c.dma_buf_attachment "dma_buf_attachment") \*attach)
:   Lock down the DMA-buf

**Parameters**

`struct dma_buf_attachment *attach`
:   [in] attachment which should be pinned

**Description**

Only dynamic importers (who set up **attach** with [`dma_buf_dynamic_attach()`](#c.dma_buf_dynamic_attach "dma_buf_dynamic_attach")) may
call this, and only for limited use cases like scanout and not for temporary
pin operations. It is not permitted to allow userspace to pin arbitrary
amounts of buffers through this interface.

Buffers must be unpinned by calling [`dma_buf_unpin()`](#c.dma_buf_unpin "dma_buf_unpin").

**Return**

0 on success, negative error code on failure.

void dma\_buf\_unpin(struct [dma\_buf\_attachment](#c.dma_buf_attachment "dma_buf_attachment") \*attach)
:   Unpin a DMA-buf

**Parameters**

`struct dma_buf_attachment *attach`
:   [in] attachment which should be unpinned

**Description**

This unpins a buffer pinned by [`dma_buf_pin()`](#c.dma_buf_pin "dma_buf_pin") and allows the exporter to move
any mapping of **attach** again and inform the importer through
[`dma_buf_attach_ops.invalidate_mappings`](#c.dma_buf_attach_ops "dma_buf_attach_ops").

struct sg\_table \*dma\_buf\_map\_attachment(struct [dma\_buf\_attachment](#c.dma_buf_attachment "dma_buf_attachment") \*attach, enum dma\_data\_direction direction)
:   Returns the scatterlist table of the attachment; mapped into \_device\_ address space. Is a wrapper for `map_dma_buf()` of the dma\_buf\_ops.

**Parameters**

`struct dma_buf_attachment *attach`
:   [in] attachment whose scatterlist is to be returned

`enum dma_data_direction direction`
:   [in] direction of DMA transfer

**Description**

Returns sg\_table containing the scatterlist to be returned; returns ERR\_PTR
on error. May return -EINTR if it is interrupted by a signal.

On success, the DMA addresses and lengths in the returned scatterlist are
PAGE\_SIZE aligned.

A mapping must be unmapped by using [`dma_buf_unmap_attachment()`](#c.dma_buf_unmap_attachment "dma_buf_unmap_attachment"). Note that
the underlying backing storage is pinned for as long as a mapping exists,
therefore users/importers should not hold onto a mapping for undue amounts of
time.

Important: Dynamic importers must wait for the exclusive fence of the [`struct
dma_resv`](#c.dma_resv "dma_resv") attached to the DMA-BUF first.

struct sg\_table \*dma\_buf\_map\_attachment\_unlocked(struct [dma\_buf\_attachment](#c.dma_buf_attachment "dma_buf_attachment") \*attach, enum dma\_data\_direction direction)
:   Returns the scatterlist table of the attachment; mapped into \_device\_ address space. Is a wrapper for `map_dma_buf()` of the dma\_buf\_ops.

**Parameters**

`struct dma_buf_attachment *attach`
:   [in] attachment whose scatterlist is to be returned

`enum dma_data_direction direction`
:   [in] direction of DMA transfer

**Description**

Unlocked variant of [`dma_buf_map_attachment()`](#c.dma_buf_map_attachment "dma_buf_map_attachment").

void dma\_buf\_unmap\_attachment(struct [dma\_buf\_attachment](#c.dma_buf_attachment "dma_buf_attachment") \*attach, struct [sg\_table](#c.dma_buf_unmap_attachment "sg_table") \*sg\_table, enum dma\_data\_direction direction)
:   unmaps and decreases usecount of the buffer;might deallocate the scatterlist associated. Is a wrapper for `unmap_dma_buf()` of dma\_buf\_ops.

**Parameters**

`struct dma_buf_attachment *attach`
:   [in] attachment to unmap buffer from

`struct sg_table *sg_table`
:   [in] scatterlist info of the buffer to unmap

`enum dma_data_direction direction`
:   [in] direction of DMA transfer

**Description**

This unmaps a DMA mapping for **attached** obtained by [`dma_buf_map_attachment()`](#c.dma_buf_map_attachment "dma_buf_map_attachment").

void dma\_buf\_unmap\_attachment\_unlocked(struct [dma\_buf\_attachment](#c.dma_buf_attachment "dma_buf_attachment") \*attach, struct [sg\_table](#c.dma_buf_unmap_attachment_unlocked "sg_table") \*sg\_table, enum dma\_data\_direction direction)
:   unmaps and decreases usecount of the buffer;might deallocate the scatterlist associated. Is a wrapper for `unmap_dma_buf()` of dma\_buf\_ops.

**Parameters**

`struct dma_buf_attachment *attach`
:   [in] attachment to unmap buffer from

`struct sg_table *sg_table`
:   [in] scatterlist info of the buffer to unmap

`enum dma_data_direction direction`
:   [in] direction of DMA transfer

**Description**

Unlocked variant of [`dma_buf_unmap_attachment()`](#c.dma_buf_unmap_attachment "dma_buf_unmap_attachment").

bool dma\_buf\_attach\_revocable(struct [dma\_buf\_attachment](#c.dma_buf_attachment "dma_buf_attachment") \*attach)
:   check if a DMA-buf importer implements revoke semantics.

**Parameters**

`struct dma_buf_attachment *attach`
:   the DMA-buf attachment to check

**Description**

Returns true if the DMA-buf importer can support the revoke sequence
explained in [`dma_buf_invalidate_mappings()`](#c.dma_buf_invalidate_mappings "dma_buf_invalidate_mappings") within bounded time. Meaning the
importer implements `invalidate_mappings()` and ensures that unmap is called as
a result.

void dma\_buf\_invalidate\_mappings(struct [dma\_buf](#c.dma_buf "dma_buf") \*dmabuf)
:   notify attachments that DMA-buf is moving

**Parameters**

`struct dma_buf *dmabuf`
:   [in] buffer which is moving

**Description**

Informs all attachments that they need to destroy and recreate all their
mappings. If the attachment is dynamic then the dynamic importer is expected
to invalidate any caches it has of the mapping result and perform a new
mapping request before allowing HW to do any further DMA.

If the attachment is pinned then this informs the pinned importer that the
underlying mapping is no longer available. Pinned importers may take this is
as a permanent revocation and never establish new mappings so exporters
should not trigger it lightly.

Upon return importers may continue to access the DMA-buf memory. The caller
must do two additional waits to ensure that the memory is no longer being
accessed:

> 1. Until [`dma_resv_wait_timeout()`](#c.dma_resv_wait_timeout "dma_resv_wait_timeout") retires fences the importer is allowed to
>    fully access the memory.
> 2. Until the importer calls unmap it is allowed to speculatively
>    read-and-discard the memory. It must not write to the memory.

A caller wishing to use [`dma_buf_invalidate_mappings()`](#c.dma_buf_invalidate_mappings "dma_buf_invalidate_mappings") to fully stop access to
the DMA-buf must wait for both. Dynamic callers can often use just the first.

All importers providing a `invalidate_mappings()` op must ensure that unmap is
called within bounded time after the op.

Pinned importers that do not support a `invalidate_mappings()` op will
eventually perform unmap when they are done with the buffer, which may be an
ubounded time from calling this function. [`dma_buf_attach_revocable()`](#c.dma_buf_attach_revocable "dma_buf_attach_revocable") can be
used to prevent such importers from attaching.

Importers are free to request a new mapping in parallel as this function
returns.

int dma\_buf\_begin\_cpu\_access(struct [dma\_buf](#c.dma_buf "dma_buf") \*dmabuf, enum dma\_data\_direction direction)
:   Must be called before accessing a dma\_buf from the cpu in the kernel context. Calls begin\_cpu\_access to allow exporter-specific preparations. Coherency is only guaranteed in the specified range for the specified access direction.

**Parameters**

`struct dma_buf *dmabuf`
:   [in] buffer to prepare cpu access for.

`enum dma_data_direction direction`
:   [in] direction of access.

**Description**

After the cpu access is complete the caller should call
[`dma_buf_end_cpu_access()`](#c.dma_buf_end_cpu_access "dma_buf_end_cpu_access"). Only when cpu access is bracketed by both calls is
it guaranteed to be coherent with other DMA access.

This function will also wait for any DMA transactions tracked through
implicit synchronization in [`dma_buf.resv`](#c.dma_buf "dma_buf"). For DMA transactions with explicit
synchronization this function will only ensure cache coherency, callers must
ensure synchronization with such DMA transactions on their own.

Can return negative error values, returns 0 on success.

int dma\_buf\_end\_cpu\_access(struct [dma\_buf](#c.dma_buf "dma_buf") \*dmabuf, enum dma\_data\_direction direction)
:   Must be called after accessing a dma\_buf from the cpu in the kernel context. Calls end\_cpu\_access to allow exporter-specific actions. Coherency is only guaranteed in the specified range for the specified access direction.

**Parameters**

`struct dma_buf *dmabuf`
:   [in] buffer to complete cpu access for.

`enum dma_data_direction direction`
:   [in] direction of access.

**Description**

This terminates CPU access started with [`dma_buf_begin_cpu_access()`](#c.dma_buf_begin_cpu_access "dma_buf_begin_cpu_access").

Can return negative error values, returns 0 on success.

int dma\_buf\_mmap(struct [dma\_buf](#c.dma_buf "dma_buf") \*dmabuf, struct vm\_area\_struct \*vma, unsigned long pgoff)
:   Setup up a userspace mmap with the given vma

**Parameters**

`struct dma_buf *dmabuf`
:   [in] buffer that should back the vma

`struct vm_area_struct *vma`
:   [in] vma for the mmap

`unsigned long pgoff`
:   [in] offset in pages where this mmap should start within the
    dma-buf buffer.

**Description**

This function adjusts the passed in vma so that it points at the file of the
dma\_buf operation. It also adjusts the starting pgoff and does bounds
checking on the size of the vma. Then it calls the exporters mmap function to
set up the mapping.

Can return negative error values, returns 0 on success.

int dma\_buf\_vmap(struct [dma\_buf](#c.dma_buf "dma_buf") \*dmabuf, struct [iosys\_map](device-io.html#c.iosys_map "iosys_map") \*map)
:   Create virtual mapping for the buffer object into kernel address space. Same restrictions as for vmap and friends apply.

**Parameters**

`struct dma_buf *dmabuf`
:   [in] buffer to vmap

`struct iosys_map *map`
:   [out] returns the vmap pointer

**Description**

This call may fail due to lack of virtual mapping address space.
These calls are optional in drivers. The intended use for them
is for mapping objects linear in kernel space for high use objects.

To ensure coherency users must call [`dma_buf_begin_cpu_access()`](#c.dma_buf_begin_cpu_access "dma_buf_begin_cpu_access") and
[`dma_buf_end_cpu_access()`](#c.dma_buf_end_cpu_access "dma_buf_end_cpu_access") around any cpu access performed through this
mapping.

Returns 0 on success, or a negative errno code otherwise.

int dma\_buf\_vmap\_unlocked(struct [dma\_buf](#c.dma_buf "dma_buf") \*dmabuf, struct [iosys\_map](device-io.html#c.iosys_map "iosys_map") \*map)
:   Create virtual mapping for the buffer object into kernel address space. Same restrictions as for vmap and friends apply.

**Parameters**

`struct dma_buf *dmabuf`
:   [in] buffer to vmap

`struct iosys_map *map`
:   [out] returns the vmap pointer

**Description**

Unlocked version of [`dma_buf_vmap()`](#c.dma_buf_vmap "dma_buf_vmap")

Returns 0 on success, or a negative errno code otherwise.

void dma\_buf\_vunmap(struct [dma\_buf](#c.dma_buf "dma_buf") \*dmabuf, struct [iosys\_map](device-io.html#c.iosys_map "iosys_map") \*map)
:   Unmap a vmap obtained by dma\_buf\_vmap.

**Parameters**

`struct dma_buf *dmabuf`
:   [in] buffer to vunmap

`struct iosys_map *map`
:   [in] vmap pointer to vunmap

void dma\_buf\_vunmap\_unlocked(struct [dma\_buf](#c.dma_buf "dma_buf") \*dmabuf, struct [iosys\_map](device-io.html#c.iosys_map "iosys_map") \*map)
:   Unmap a vmap obtained by dma\_buf\_vmap.

**Parameters**

`struct dma_buf *dmabuf`
:   [in] buffer to vunmap

`struct iosys_map *map`
:   [in] vmap pointer to vunmap

struct dma\_buf\_ops
:   operations possible on [`struct dma_buf`](#c.dma_buf "dma_buf")

**Definition**:

```
struct dma_buf_ops {
    int (*attach)(struct dma_buf *, struct dma_buf_attachment *);
    void (*detach)(struct dma_buf *, struct dma_buf_attachment *);
    int (*pin)(struct dma_buf_attachment *attach);
    void (*unpin)(struct dma_buf_attachment *attach);
    struct sg_table * (*map_dma_buf)(struct dma_buf_attachment *, enum dma_data_direction);
    void (*unmap_dma_buf)(struct dma_buf_attachment *, struct sg_table *, enum dma_data_direction);
    void (*release)(struct dma_buf *);
    int (*begin_cpu_access)(struct dma_buf *, enum dma_data_direction);
    int (*end_cpu_access)(struct dma_buf *, enum dma_data_direction);
    int (*mmap)(struct dma_buf *, struct vm_area_struct *vma);
    int (*vmap)(struct dma_buf *dmabuf, struct iosys_map *map);
    void (*vunmap)(struct dma_buf *dmabuf, struct iosys_map *map);
};
```

**Members**

`attach`
:   This is called from [`dma_buf_attach()`](#c.dma_buf_attach "dma_buf_attach") to make sure that a given
    [`dma_buf_attachment.dev`](#c.dma_buf_attachment "dma_buf_attachment") can access the provided [`dma_buf`](#c.dma_buf "dma_buf"). Exporters
    which support buffer objects in special locations like VRAM or
    device-specific carveout areas should check whether the buffer could
    be move to system memory (or directly accessed by the provided
    device), and otherwise need to fail the attach operation.

    The exporter should also in general check whether the current
    allocation fulfills the DMA constraints of the new device. If this
    is not the case, and the allocation cannot be moved, it should also
    fail the attach operation.

    Any exporter-private housekeeping data can be stored in the
    [`dma_buf_attachment.priv`](#c.dma_buf_attachment "dma_buf_attachment") pointer.

    This callback is optional.

    Returns:

    0 on success, negative error code on failure. It might return -EBUSY
    to signal that backing storage is already allocated and incompatible
    with the requirements of requesting device.

`detach`
:   This is called by [`dma_buf_detach()`](#c.dma_buf_detach "dma_buf_detach") to release a [`dma_buf_attachment`](#c.dma_buf_attachment "dma_buf_attachment").
    Provided so that exporters can clean up any housekeeping for an
    [`dma_buf_attachment`](#c.dma_buf_attachment "dma_buf_attachment").

    This callback is optional.

`pin`
:   This is called by [`dma_buf_pin()`](#c.dma_buf_pin "dma_buf_pin") and lets the exporter know that the
    DMA-buf can’t be moved any more. Ideally, the exporter should
    pin the buffer so that it is generally accessible by all
    devices.

    This is called with the `dmabuf.resv` object locked and is mutual
    exclusive with **cache\_sgt\_mapping**.

    This is called automatically for non-dynamic importers from
    [`dma_buf_attach()`](#c.dma_buf_attach "dma_buf_attach").

    Note that similar to non-dynamic exporters in their **map\_dma\_buf**
    callback the driver must guarantee that the memory is available for
    use and cleared of any old data by the time this function returns.
    Drivers which pipeline their buffer moves internally must wait for
    all moves and clears to complete.

    Returns:

    0 on success, negative error code on failure.

`unpin`
:   This is called by [`dma_buf_unpin()`](#c.dma_buf_unpin "dma_buf_unpin") and lets the exporter know that the
    DMA-buf can be moved again.

    This is called with the dmabuf->resv object locked and is mutual
    exclusive with **cache\_sgt\_mapping**.

    This callback is optional.

`map_dma_buf`
:   This is called by [`dma_buf_map_attachment()`](#c.dma_buf_map_attachment "dma_buf_map_attachment") and is used to map a
    shared [`dma_buf`](#c.dma_buf "dma_buf") into device address space, and it is mandatory. It
    can only be called if **attach** has been called successfully.

    This call may sleep, e.g. when the backing storage first needs to be
    allocated, or moved to a location suitable for all currently attached
    devices.

    Note that any specific buffer attributes required for this function
    should get added to device\_dma\_parameters accessible via
    [`device.dma_params`](infrastructure.html#c.device "device") from the [`dma_buf_attachment`](#c.dma_buf_attachment "dma_buf_attachment"). The **attach** callback
    should also check these constraints.

    If this is being called for the first time, the exporter can now
    choose to scan through the list of attachments for this buffer,
    collate the requirements of the attached devices, and choose an
    appropriate backing storage for the buffer.

    Based on `enum dma_data_direction`, it might be possible to have
    multiple users accessing at the same time (for reading, maybe), or
    any other kind of sharing that the exporter might wish to make
    available to buffer-users.

    This is always called with the dmabuf->resv object locked when
    the dynamic\_mapping flag is true.

    Note that for non-dynamic exporters the driver must guarantee that
    that the memory is available for use and cleared of any old data by
    the time this function returns. Drivers which pipeline their buffer
    moves internally must wait for all moves and clears to complete.
    Dynamic exporters do not need to follow this rule: For non-dynamic
    importers the buffer is already pinned through **pin**, which has the
    same requirements. Dynamic importers otoh are required to obey the
    dma\_resv fences.

    Returns:

    A `sg_table` scatter list of the backing storage of the DMA buffer,
    already mapped into the device address space of the [`device`](infrastructure.html#c.device "device") attached
    with the provided [`dma_buf_attachment`](#c.dma_buf_attachment "dma_buf_attachment"). The addresses and lengths in
    the scatter list are PAGE\_SIZE aligned.

    On failure, returns a negative error value wrapped into a pointer.
    May also return -EINTR when a signal was received while being
    blocked.

    Note that exporters should not try to cache the scatter list, or
    return the same one for multiple calls. Caching is done either by the
    DMA-BUF code (for non-dynamic importers) or the importer. Ownership
    of the scatter list is transferred to the caller, and returned by
    **unmap\_dma\_buf**.

`unmap_dma_buf`
:   This is called by [`dma_buf_unmap_attachment()`](#c.dma_buf_unmap_attachment "dma_buf_unmap_attachment") and should unmap and
    release the `sg_table` allocated in **map\_dma\_buf**, and it is mandatory.
    For static dma\_buf handling this might also unpin the backing
    storage if this is the last mapping of the DMA buffer.

`release`
:   Called after the last dma\_buf\_put to release the [`dma_buf`](#c.dma_buf "dma_buf"), and
    mandatory.

`begin_cpu_access`
:   This is called from [`dma_buf_begin_cpu_access()`](#c.dma_buf_begin_cpu_access "dma_buf_begin_cpu_access") and allows the
    exporter to ensure that the memory is actually coherent for cpu
    access. The exporter also needs to ensure that cpu access is coherent
    for the access direction. The direction can be used by the exporter
    to optimize the cache flushing, i.e. access with a different
    direction (read instead of write) might return stale or even bogus
    data (e.g. when the exporter needs to copy the data to temporary
    storage).

    Note that this is both called through the DMA\_BUF\_IOCTL\_SYNC IOCTL
    command for userspace mappings established through **mmap**, and also
    for kernel mappings established with **vmap**.

    This callback is optional.

    Returns:

    0 on success or a negative error code on failure. This can for
    example fail when the backing storage can’t be allocated. Can also
    return -ERESTARTSYS or -EINTR when the call has been interrupted and
    needs to be restarted.

`end_cpu_access`
:   This is called from [`dma_buf_end_cpu_access()`](#c.dma_buf_end_cpu_access "dma_buf_end_cpu_access") when the importer is
    done accessing the CPU. The exporter can use this to flush caches and
    undo anything else done in **begin\_cpu\_access**.

    This callback is optional.

    Returns:

    0 on success or a negative error code on failure. Can return
    -ERESTARTSYS or -EINTR when the call has been interrupted and needs
    to be restarted.

`mmap`
:   This callback is used by the [`dma_buf_mmap()`](#c.dma_buf_mmap "dma_buf_mmap") function

    Note that the mapping needs to be incoherent, userspace is expected
    to bracket CPU access using the DMA\_BUF\_IOCTL\_SYNC interface.

    Because dma-buf buffers have invariant size over their lifetime, the
    dma-buf core checks whether a vma is too large and rejects such
    mappings. The exporter hence does not need to duplicate this check.
    Drivers do not need to check this themselves.

    If an exporter needs to manually flush caches and hence needs to fake
    coherency for mmap support, it needs to be able to zap all the ptes
    pointing at the backing storage. Now linux mm needs a [`struct
    address_space`](../filesystems/api-summary.html#c.address_space "address_space") associated with the [`struct file`](../filesystems/api-summary.html#c.file "file") stored in vma->vm\_file
    to do that with the function unmap\_mapping\_range. But the dma\_buf
    framework only backs every dma\_buf fd with the anon\_file [`struct file`](../filesystems/api-summary.html#c.file "file"),
    i.e. all dma\_bufs share the same file.

    Hence exporters need to setup their own file (and address\_space)
    association by setting vma->vm\_file and adjusting vma->vm\_pgoff in
    the dma\_buf mmap callback. In the specific case of a gem driver the
    exporter could use the shmem file already provided by gem (and set
    vm\_pgoff = 0). Exporters can then zap ptes by unmapping the
    corresponding range of the [`struct address_space`](../filesystems/api-summary.html#c.address_space "address_space") associated with their
    own file.

    This callback is optional.

    Returns:

    0 on success or a negative error code on failure.

`vmap`
:   [optional] creates a virtual mapping for the buffer into kernel
    address space. Same restrictions as for vmap and friends apply.

`vunmap`
:   [optional] unmaps a vmap from the buffer

struct dma\_buf
:   shared buffer object

**Definition**:

```
struct dma_buf {
    size_t size;
    struct file *file;
    struct list_head attachments;
    const struct dma_buf_ops *ops;
    unsigned vmapping_counter;
    struct iosys_map vmap_ptr;
    const char *exp_name;
    const char *name;
    spinlock_t name_lock;
    struct module *owner;
    struct list_head list_node;
    void *priv;
    struct dma_resv *resv;
    wait_queue_head_t poll;
    struct dma_buf_poll_cb_t {
        struct dma_fence_cb cb;
        wait_queue_head_t *poll;
        __poll_t active;
    } cb_in, cb_out;
};
```

**Members**

`size`
:   Size of the buffer; invariant over the lifetime of the buffer.

`file`
:   File pointer used for sharing buffers across, and for refcounting.
    See [`dma_buf_get()`](#c.dma_buf_get "dma_buf_get") and [`dma_buf_put()`](#c.dma_buf_put "dma_buf_put").

`attachments`
:   List of dma\_buf\_attachment that denotes all devices attached,
    protected by [`dma_resv`](#c.dma_resv "dma_resv") lock **resv**.

`ops`
:   dma\_buf\_ops associated with this buffer object.

`vmapping_counter`
:   Used internally to refcnt the vmaps returned by [`dma_buf_vmap()`](#c.dma_buf_vmap "dma_buf_vmap").
    Protected by **resv**.

`vmap_ptr`
:   The current vmap ptr if **vmapping\_counter** > 0. Protected by **resv**.

`exp_name`
:   Name of the exporter; useful for debugging. Must not be NULL

`name`
:   Userspace-provided name. Default value is NULL. If not NULL,
    length cannot be longer than DMA\_BUF\_NAME\_LEN, including NIL
    char. Useful for accounting and debugging. Read/Write accesses
    are protected by **name\_lock**

    See the IOCTLs DMA\_BUF\_SET\_NAME or DMA\_BUF\_SET\_NAME\_A/B

`name_lock`
:   Spinlock to protect name access for read access.

`owner`
:   Pointer to exporter module; used for refcounting when exporter is a
    kernel module.

`list_node`
:   node for dma\_buf accounting and debugging.

`priv`
:   exporter specific private data for this buffer object.

`resv`
:   Reservation object linked to this dma-buf.

    IMPLICIT SYNCHRONIZATION RULES:

    Drivers which support implicit synchronization of buffer access as
    e.g. exposed in [Implicit Fence Poll Support](#implicit-fence-poll-support) must follow the
    below rules.

    * Drivers must add a read fence through [`dma_resv_add_fence()`](#c.dma_resv_add_fence "dma_resv_add_fence") with the
      DMA\_RESV\_USAGE\_READ flag for anything the userspace API considers a
      read access. This highly depends upon the API and window system.
    * Similarly drivers must add a write fence through
      [`dma_resv_add_fence()`](#c.dma_resv_add_fence "dma_resv_add_fence") with the DMA\_RESV\_USAGE\_WRITE flag for
      anything the userspace API considers write access.
    * Drivers may just always add a write fence, since that only
      causes unnecessary synchronization, but no correctness issues.
    * Some drivers only expose a synchronous userspace API with no
      pipelining across drivers. These do not set any fences for their
      access. An example here is v4l.
    * Driver should use [`dma_resv_usage_rw()`](#c.dma_resv_usage_rw "dma_resv_usage_rw") when retrieving fences as
      dependency for implicit synchronization.

    DYNAMIC IMPORTER RULES:

    Dynamic importers, see `dma_buf_attachment_is_dynamic()`, have
    additional constraints on how they set up fences:

    * Dynamic importers must obey the write fences and wait for them to
      signal before allowing access to the buffer’s underlying storage
      through the device.
    * Dynamic importers should set fences for any access that they can’t
      disable immediately from their [`dma_buf_attach_ops.invalidate_mappings`](#c.dma_buf_attach_ops "dma_buf_attach_ops")
      callback.

    IMPORTANT:

    All drivers and memory management related functions must obey the
    [`struct dma_resv`](#c.dma_resv "dma_resv") rules, specifically the rules for updating and
    obeying fences. See [`enum dma_resv_usage`](#c.dma_resv_usage "dma_resv_usage") for further descriptions.

`poll`
:   for userspace poll support

`cb_in`
:   for userspace poll support

`cb_out`
:   for userspace poll support

**Description**

This represents a shared buffer, created by calling [`dma_buf_export()`](#c.dma_buf_export "dma_buf_export"). The
userspace representation is a normal file descriptor, which can be created by
calling [`dma_buf_fd()`](#c.dma_buf_fd "dma_buf_fd").

Shared dma buffers are reference counted using [`dma_buf_put()`](#c.dma_buf_put "dma_buf_put") and
[`get_dma_buf()`](#c.get_dma_buf "get_dma_buf").

Device DMA access is handled by the separate [`struct dma_buf_attachment`](#c.dma_buf_attachment "dma_buf_attachment").

struct dma\_buf\_attach\_ops
:   importer operations for an attachment

**Definition**:

```
struct dma_buf_attach_ops {
    bool allow_peer2peer;
    void (*invalidate_mappings)(struct dma_buf_attachment *attach);
};
```

**Members**

`allow_peer2peer`
:   If this is set to true the importer must be able to handle peer
    resources without `struct pages`.

`invalidate_mappings`
:   [optional] notification that the DMA-buf is moving

    If this callback is provided the framework can avoid pinning the
    backing store while mappings exists.

    This callback is called with the lock of the reservation object
    associated with the dma\_buf held and the mapping function must be
    called with this lock held as well. This makes sure that no mapping
    is created concurrently with an ongoing move operation.

    See the kdoc for [`dma_buf_invalidate_mappings()`](#c.dma_buf_invalidate_mappings "dma_buf_invalidate_mappings") for details on the
    required behavior.

**Description**

Attachment operations implemented by the importer.

struct dma\_buf\_attachment
:   holds device-buffer attachment data

**Definition**:

```
struct dma_buf_attachment {
    struct dma_buf *dmabuf;
    struct device *dev;
    struct list_head node;
    bool peer2peer;
    const struct dma_buf_attach_ops *importer_ops;
    void *importer_priv;
    void *priv;
};
```

**Members**

`dmabuf`
:   buffer for this attachment.

`dev`
:   device attached to the buffer.

`node`
:   list of dma\_buf\_attachment, protected by dma\_resv lock of the dmabuf.

`peer2peer`
:   true if the importer can handle peer resources without pages.

`importer_ops`
:   importer operations for this attachment, if provided
    dma\_buf\_map/`unmap_attachment()` must be called with the dma\_resv lock held.

`importer_priv`
:   importer specific attachment data.

`priv`
:   exporter specific attachment data.

**Description**

This structure holds the attachment information between the dma\_buf buffer
and its user device(s). The list contains one attachment `struct per` device
attached to the buffer.

An attachment is created by calling [`dma_buf_attach()`](#c.dma_buf_attach "dma_buf_attach"), and released again by
calling [`dma_buf_detach()`](#c.dma_buf_detach "dma_buf_detach"). The DMA mapping itself needed to initiate a
transfer is created by [`dma_buf_map_attachment()`](#c.dma_buf_map_attachment "dma_buf_map_attachment") and freed again by calling
[`dma_buf_unmap_attachment()`](#c.dma_buf_unmap_attachment "dma_buf_unmap_attachment").

struct dma\_buf\_export\_info
:   holds information needed to export a dma\_buf

**Definition**:

```
struct dma_buf_export_info {
    const char *exp_name;
    struct module *owner;
    const struct dma_buf_ops *ops;
    size_t size;
    int flags;
    struct dma_resv *resv;
    void *priv;
};
```

**Members**

`exp_name`
:   name of the exporter - useful for debugging.

`owner`
:   pointer to exporter module - used for refcounting kernel module

`ops`
:   Attach allocator-defined dma buf ops to the new buffer

`size`
:   Size of the buffer - invariant over the lifetime of the buffer

`flags`
:   mode flags for the file

`resv`
:   reservation-object, NULL to allocate default one

`priv`
:   Attach private data of allocator to this buffer

**Description**

This structure holds the information required to export the buffer. Used
with [`dma_buf_export()`](#c.dma_buf_export "dma_buf_export") only.

DEFINE\_DMA\_BUF\_EXPORT\_INFO

`DEFINE_DMA_BUF_EXPORT_INFO (name)`

> helper macro for exporters

**Parameters**

`name`
:   export-info name

**Description**

DEFINE\_DMA\_BUF\_EXPORT\_INFO macro defines the [`struct dma_buf_export_info`](#c.dma_buf_export_info "dma_buf_export_info"),
zeroes it out and pre-populates exp\_name in it.

void get\_dma\_buf(struct [dma\_buf](#c.dma_buf "dma_buf") \*dmabuf)
:   convenience wrapper for get\_file.

**Parameters**

`struct dma_buf *dmabuf`
:   [in] pointer to dma\_buf

**Description**

Increments the reference count on the dma-buf, needed in case of drivers
that either need to create additional references to the dmabuf on the
kernel side. For example, an exporter that needs to keep a dmabuf ptr
so that subsequent exports don’t create a new dmabuf.

bool dma\_buf\_is\_dynamic(struct [dma\_buf](#c.dma_buf "dma_buf") \*dmabuf)
:   check if a DMA-buf uses dynamic mappings.

**Parameters**

`struct dma_buf *dmabuf`
:   the DMA-buf to check

**Description**

Returns true if a DMA-buf exporter wants to be called with the dma\_resv
locked for the map/unmap callbacks, false if it doesn’t wants to be called
with the lock held.

## Reservation Objects

The reservation object provides a mechanism to manage a container of
dma\_fence object associated with a resource. A reservation object
can have any number of fences attaches to it. Each fence carries an usage
parameter determining how the operation represented by the fence is using the
resource. The RCU mechanism is used to protect read access to fences from
locked write-side updates.

See [`struct dma_resv`](#c.dma_resv "dma_resv") for more details.

void dma\_resv\_init(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj)
:   initialize a reservation object

**Parameters**

`struct dma_resv *obj`
:   the reservation object

void dma\_resv\_fini(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj)
:   destroys a reservation object

**Parameters**

`struct dma_resv *obj`
:   the reservation object

int dma\_resv\_reserve\_fences(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj, unsigned int num\_fences)
:   Reserve space to add fences to a dma\_resv object.

**Parameters**

`struct dma_resv *obj`
:   reservation object

`unsigned int num_fences`
:   number of fences we want to add

**Description**

Should be called before [`dma_resv_add_fence()`](#c.dma_resv_add_fence "dma_resv_add_fence"). Must be called with **obj**
locked through [`dma_resv_lock()`](#c.dma_resv_lock "dma_resv_lock").

Note that the preallocated slots need to be re-reserved if **obj** is unlocked
at any time before calling [`dma_resv_add_fence()`](#c.dma_resv_add_fence "dma_resv_add_fence"). This is validated when
CONFIG\_DEBUG\_MUTEXES is enabled.

RETURNS
Zero for success, or -errno

void dma\_resv\_reset\_max\_fences(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj)
:   reset fences for debugging

**Parameters**

`struct dma_resv *obj`
:   the dma\_resv object to reset

**Description**

Reset the number of pre-reserved fence slots to test that drivers do
correct slot allocation using [`dma_resv_reserve_fences()`](#c.dma_resv_reserve_fences "dma_resv_reserve_fences"). See also
`dma_resv_list.max_fences`.

void dma\_resv\_add\_fence(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj, struct [dma\_fence](#c.dma_fence "dma_fence") \*fence, enum [dma\_resv\_usage](#c.dma_resv_usage "dma_resv_usage") usage)
:   Add a fence to the dma\_resv obj

**Parameters**

`struct dma_resv *obj`
:   the reservation object

`struct dma_fence *fence`
:   the fence to add

`enum dma_resv_usage usage`
:   how the fence is used, see [`enum dma_resv_usage`](#c.dma_resv_usage "dma_resv_usage")

**Description**

Add a fence to a slot, **obj** must be locked with [`dma_resv_lock()`](#c.dma_resv_lock "dma_resv_lock"), and
[`dma_resv_reserve_fences()`](#c.dma_resv_reserve_fences "dma_resv_reserve_fences") has been called.

See also [`dma_resv.fence`](#c.dma_resv "dma_resv") for a discussion of the semantics.

void dma\_resv\_replace\_fences(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj, uint64\_t context, struct [dma\_fence](#c.dma_fence "dma_fence") \*replacement, enum [dma\_resv\_usage](#c.dma_resv_usage "dma_resv_usage") usage)
:   replace fences in the dma\_resv obj

**Parameters**

`struct dma_resv *obj`
:   the reservation object

`uint64_t context`
:   the context of the fences to replace

`struct dma_fence *replacement`
:   the new fence to use instead

`enum dma_resv_usage usage`
:   how the new fence is used, see [`enum dma_resv_usage`](#c.dma_resv_usage "dma_resv_usage")

**Description**

Replace fences with a specified context with a new fence. Only valid if the
operation represented by the original fence has no longer access to the
resources represented by the dma\_resv object when the new fence completes.

And example for using this is replacing a preemption fence with a page table
update fence which makes the resource inaccessible.

struct [dma\_fence](#c.dma_fence "dma_fence") \*dma\_resv\_iter\_first\_unlocked(struct [dma\_resv\_iter](#c.dma_resv_iter "dma_resv_iter") \*cursor)
:   first fence in an unlocked dma\_resv obj.

**Parameters**

`struct dma_resv_iter *cursor`
:   the cursor with the current position

**Description**

Subsequent fences are iterated with [`dma_resv_iter_next_unlocked()`](#c.dma_resv_iter_next_unlocked "dma_resv_iter_next_unlocked").

Beware that the iterator can be restarted. Code which accumulates statistics
or similar needs to check for this with [`dma_resv_iter_is_restarted()`](#c.dma_resv_iter_is_restarted "dma_resv_iter_is_restarted"). For
this reason prefer the locked [`dma_resv_iter_first()`](#c.dma_resv_iter_first "dma_resv_iter_first") whenever possible.

Returns the first fence from an unlocked dma\_resv obj.

struct [dma\_fence](#c.dma_fence "dma_fence") \*dma\_resv\_iter\_next\_unlocked(struct [dma\_resv\_iter](#c.dma_resv_iter "dma_resv_iter") \*cursor)
:   next fence in an unlocked dma\_resv obj.

**Parameters**

`struct dma_resv_iter *cursor`
:   the cursor with the current position

**Description**

Beware that the iterator can be restarted. Code which accumulates statistics
or similar needs to check for this with [`dma_resv_iter_is_restarted()`](#c.dma_resv_iter_is_restarted "dma_resv_iter_is_restarted"). For
this reason prefer the locked [`dma_resv_iter_next()`](#c.dma_resv_iter_next "dma_resv_iter_next") whenever possible.

Returns the next fence from an unlocked dma\_resv obj.

struct [dma\_fence](#c.dma_fence "dma_fence") \*dma\_resv\_iter\_first(struct [dma\_resv\_iter](#c.dma_resv_iter "dma_resv_iter") \*cursor)
:   first fence from a locked dma\_resv object

**Parameters**

`struct dma_resv_iter *cursor`
:   cursor to record the current position

**Description**

Subsequent fences are iterated with [`dma_resv_iter_next_unlocked()`](#c.dma_resv_iter_next_unlocked "dma_resv_iter_next_unlocked").

Return the first fence in the dma\_resv object while holding the
[`dma_resv.lock`](#c.dma_resv "dma_resv").

struct [dma\_fence](#c.dma_fence "dma_fence") \*dma\_resv\_iter\_next(struct [dma\_resv\_iter](#c.dma_resv_iter "dma_resv_iter") \*cursor)
:   next fence from a locked dma\_resv object

**Parameters**

`struct dma_resv_iter *cursor`
:   cursor to record the current position

**Description**

Return the next fences from the dma\_resv object while holding the
[`dma_resv.lock`](#c.dma_resv "dma_resv").

int dma\_resv\_copy\_fences(struct [dma\_resv](#c.dma_resv "dma_resv") \*dst, struct [dma\_resv](#c.dma_resv "dma_resv") \*src)
:   Copy all fences from src to dst.

**Parameters**

`struct dma_resv *dst`
:   the destination reservation object

`struct dma_resv *src`
:   the source reservation object

**Description**

Copy all fences from src to dst. dst-lock must be held.

int dma\_resv\_get\_fences(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj, enum [dma\_resv\_usage](#c.dma_resv_usage "dma_resv_usage") usage, unsigned int \*num\_fences, struct [dma\_fence](#c.dma_fence "dma_fence") \*\*\*fences)
:   Get an object’s fences fences without update side lock held

**Parameters**

`struct dma_resv *obj`
:   the reservation object

`enum dma_resv_usage usage`
:   controls which fences to include, see [`enum dma_resv_usage`](#c.dma_resv_usage "dma_resv_usage").

`unsigned int *num_fences`
:   the number of fences returned

`struct dma_fence ***fences`
:   the array of fence ptrs returned (array is krealloc’d to the
    required size, and must be freed by caller)

**Description**

Retrieve all fences from the reservation object.
Returns either zero or -ENOMEM.

int dma\_resv\_get\_singleton(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj, enum [dma\_resv\_usage](#c.dma_resv_usage "dma_resv_usage") usage, struct [dma\_fence](#c.dma_fence "dma_fence") \*\*fence)
:   Get a single fence for all the fences

**Parameters**

`struct dma_resv *obj`
:   the reservation object

`enum dma_resv_usage usage`
:   controls which fences to include, see [`enum dma_resv_usage`](#c.dma_resv_usage "dma_resv_usage").

`struct dma_fence **fence`
:   the resulting fence

**Description**

Get a single fence representing all the fences inside the resv object.
Returns either 0 for success or -ENOMEM.

Warning: This can’t be used like this when adding the fence back to the resv
object since that can lead to stack corruption when finalizing the
dma\_fence\_array.

Returns 0 on success and negative error values on failure.

long dma\_resv\_wait\_timeout(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj, enum [dma\_resv\_usage](#c.dma_resv_usage "dma_resv_usage") usage, bool intr, unsigned long timeout)
:   Wait on reservation’s objects fences

**Parameters**

`struct dma_resv *obj`
:   the reservation object

`enum dma_resv_usage usage`
:   controls which fences to include, see [`enum dma_resv_usage`](#c.dma_resv_usage "dma_resv_usage").

`bool intr`
:   if true, do interruptible wait

`unsigned long timeout`
:   timeout value in jiffies or zero to return immediately

**Description**

Callers are not required to hold specific locks, but maybe hold
[`dma_resv_lock()`](#c.dma_resv_lock "dma_resv_lock") already
RETURNS
Returns -ERESTARTSYS if interrupted, 0 if the wait timed out, or
greater than zero on success.

void dma\_resv\_set\_deadline(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj, enum [dma\_resv\_usage](#c.dma_resv_usage "dma_resv_usage") usage, ktime\_t deadline)
:   Set a deadline on reservation’s objects fences

**Parameters**

`struct dma_resv *obj`
:   the reservation object

`enum dma_resv_usage usage`
:   controls which fences to include, see [`enum dma_resv_usage`](#c.dma_resv_usage "dma_resv_usage").

`ktime_t deadline`
:   the requested deadline (MONOTONIC)

**Description**

May be called without holding the dma\_resv lock. Sets **deadline** on
all fences filtered by **usage**.

bool dma\_resv\_test\_signaled(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj, enum [dma\_resv\_usage](#c.dma_resv_usage "dma_resv_usage") usage)
:   Test if a reservation object’s fences have been signaled.

**Parameters**

`struct dma_resv *obj`
:   the reservation object

`enum dma_resv_usage usage`
:   controls which fences to include, see [`enum dma_resv_usage`](#c.dma_resv_usage "dma_resv_usage").

**Description**

Callers are not required to hold specific locks, but maybe hold
[`dma_resv_lock()`](#c.dma_resv_lock "dma_resv_lock") already.

RETURNS

True if all fences signaled, else false.

void dma\_resv\_describe(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj, struct seq\_file \*seq)
:   Dump description of the resv object into seq\_file

**Parameters**

`struct dma_resv *obj`
:   the reservation object

`struct seq_file *seq`
:   the seq\_file to dump the description into

**Description**

Dump a textual description of the fences inside an dma\_resv object into the
seq\_file.

enum dma\_resv\_usage
:   how the fences from a dma\_resv obj are used

**Constants**

`DMA_RESV_USAGE_KERNEL`
:   For in kernel memory management only.

    This should only be used for things like copying or clearing memory
    with a DMA hardware engine for the purpose of kernel memory
    management.

    Drivers *always* must wait for those fences before accessing the
    resource protected by the dma\_resv object. The only exception for
    that is when the resource is known to be locked down in place by
    pinning it previously.

`DMA_RESV_USAGE_WRITE`
:   Implicit write synchronization.

    This should only be used for userspace command submissions which add
    an implicit write dependency.

`DMA_RESV_USAGE_READ`
:   Implicit read synchronization.

    This should only be used for userspace command submissions which add
    an implicit read dependency.

`DMA_RESV_USAGE_BOOKKEEP`
:   No implicit sync.

    This should be used by submissions which don’t want to participate in
    any implicit synchronization.

    The most common cases are preemption fences, page table updates, TLB
    flushes as well as explicitly synced user submissions.

    Explicitly synced user submissions can be promoted to
    DMA\_RESV\_USAGE\_READ or DMA\_RESV\_USAGE\_WRITE as needed using
    [`dma_buf_import_sync_file()`](#c.dma_buf_import_sync_file "dma_buf_import_sync_file") when implicit synchronization should
    become necessary after initial adding of the fence.

**Description**

This `enum describes` the different use cases for a dma\_resv object and
controls which fences are returned when queried.

An important fact is that there is the order KERNEL<WRITE<READ<BOOKKEEP and
when the dma\_resv object is asked for fences for one use case the fences
for the lower use case are returned as well.

For example when asking for WRITE fences then the KERNEL fences are returned
as well. Similar when asked for READ fences then both WRITE and KERNEL
fences are returned as well.

Already used fences can be promoted in the sense that a fence with
DMA\_RESV\_USAGE\_BOOKKEEP could become DMA\_RESV\_USAGE\_READ by adding it again
with this usage. But fences can never be degraded in the sense that a fence
with DMA\_RESV\_USAGE\_WRITE could become DMA\_RESV\_USAGE\_READ.

enum [dma\_resv\_usage](#c.dma_resv_usage "dma_resv_usage") dma\_resv\_usage\_rw(bool write)
:   helper for implicit sync

**Parameters**

`bool write`
:   true if we create a new implicit sync write

**Description**

This returns the implicit synchronization usage for write or read accesses,
see [`enum dma_resv_usage`](#c.dma_resv_usage "dma_resv_usage") and [`dma_buf.resv`](#c.dma_buf "dma_buf").

struct dma\_resv
:   a reservation object manages fences for a buffer

**Definition**:

```
struct dma_resv {
    struct ww_mutex lock;
    struct dma_resv_list *fences;
};
```

**Members**

`lock`
:   Update side lock. Don’t use directly, instead use the wrapper
    functions like [`dma_resv_lock()`](#c.dma_resv_lock "dma_resv_lock") and [`dma_resv_unlock()`](#c.dma_resv_unlock "dma_resv_unlock").

    Drivers which use the reservation object to manage memory dynamically
    also use this lock to protect buffer object state like placement,
    allocation policies or throughout command submission.

`fences`
:   Array of fences which where added to the dma\_resv object

    A new fence is added by calling [`dma_resv_add_fence()`](#c.dma_resv_add_fence "dma_resv_add_fence"). Since this
    often needs to be done past the point of no return in command
    submission it cannot fail, and therefore sufficient slots need to be
    reserved by calling [`dma_resv_reserve_fences()`](#c.dma_resv_reserve_fences "dma_resv_reserve_fences").

**Description**

This is a container for dma\_fence objects which needs to handle multiple use
cases.

One use is to synchronize cross-driver access to a [`struct dma_buf`](#c.dma_buf "dma_buf"), either for
dynamic buffer management or just to handle implicit synchronization between
different users of the buffer in userspace. See [`dma_buf.resv`](#c.dma_buf "dma_buf") for a more
in-depth discussion.

The other major use is to manage access and locking within a driver in a
buffer based memory manager. `struct ttm_buffer_object` is the canonical
example here, since this is where reservation objects originated from. But
use in drivers is spreading and some drivers also manage [`struct
drm_gem_object`](../gpu/drm-mm.html#c.drm_gem_object "drm_gem_object") with the same scheme.

struct dma\_resv\_iter
:   current position into the dma\_resv fences

**Definition**:

```
struct dma_resv_iter {
    struct dma_resv *obj;
    enum dma_resv_usage usage;
    struct dma_fence *fence;
    enum dma_resv_usage fence_usage;
    unsigned int index;
    struct dma_resv_list *fences;
    unsigned int num_fences;
    bool is_restarted;
};
```

**Members**

`obj`
:   The dma\_resv object we iterate over

`usage`
:   Return fences with this usage or lower.

`fence`
:   the currently handled fence

`fence_usage`
:   the usage of the current fence

`index`
:   index into the shared fences

`fences`
:   the shared fences; private, *MUST* not dereference

`num_fences`
:   number of fences

`is_restarted`
:   true if this is the first returned fence

**Description**

Don’t touch this directly in the driver, use the accessor function instead.

IMPORTANT

When using the lockless iterators like [`dma_resv_iter_next_unlocked()`](#c.dma_resv_iter_next_unlocked "dma_resv_iter_next_unlocked") or
[`dma_resv_for_each_fence_unlocked()`](#c.dma_resv_for_each_fence_unlocked "dma_resv_for_each_fence_unlocked") beware that the iterator can be restarted.
Code which accumulates statistics or similar needs to check for this with
[`dma_resv_iter_is_restarted()`](#c.dma_resv_iter_is_restarted "dma_resv_iter_is_restarted").

void dma\_resv\_iter\_begin(struct [dma\_resv\_iter](#c.dma_resv_iter "dma_resv_iter") \*cursor, struct [dma\_resv](#c.dma_resv "dma_resv") \*obj, enum [dma\_resv\_usage](#c.dma_resv_usage "dma_resv_usage") usage)
:   initialize a dma\_resv\_iter object

**Parameters**

`struct dma_resv_iter *cursor`
:   The dma\_resv\_iter object to initialize

`struct dma_resv *obj`
:   The dma\_resv object which we want to iterate over

`enum dma_resv_usage usage`
:   controls which fences to include, see [`enum dma_resv_usage`](#c.dma_resv_usage "dma_resv_usage").

void dma\_resv\_iter\_end(struct [dma\_resv\_iter](#c.dma_resv_iter "dma_resv_iter") \*cursor)
:   cleanup a dma\_resv\_iter object

**Parameters**

`struct dma_resv_iter *cursor`
:   the dma\_resv\_iter object which should be cleaned up

**Description**

Make sure that the reference to the fence in the cursor is properly
dropped.

enum [dma\_resv\_usage](#c.dma_resv_usage "dma_resv_usage") dma\_resv\_iter\_usage(struct [dma\_resv\_iter](#c.dma_resv_iter "dma_resv_iter") \*cursor)
:   Return the usage of the current fence

**Parameters**

`struct dma_resv_iter *cursor`
:   the cursor of the current position

**Description**

Returns the usage of the currently processed fence.

bool dma\_resv\_iter\_is\_restarted(struct [dma\_resv\_iter](#c.dma_resv_iter "dma_resv_iter") \*cursor)
:   test if this is the first fence after a restart

**Parameters**

`struct dma_resv_iter *cursor`
:   the cursor with the current position

**Description**

Return true if this is the first fence in an iteration after a restart.

dma\_resv\_for\_each\_fence\_unlocked

`dma_resv_for_each_fence_unlocked (cursor, fence)`

> unlocked fence iterator

**Parameters**

`cursor`
:   a [`struct dma_resv_iter`](#c.dma_resv_iter "dma_resv_iter") pointer

`fence`
:   the current fence

**Description**

Iterate over the fences in a [`struct dma_resv`](#c.dma_resv "dma_resv") object without holding the
[`dma_resv.lock`](#c.dma_resv "dma_resv") and using RCU instead. The cursor needs to be initialized
with [`dma_resv_iter_begin()`](#c.dma_resv_iter_begin "dma_resv_iter_begin") and cleaned up with [`dma_resv_iter_end()`](#c.dma_resv_iter_end "dma_resv_iter_end"). Inside
the iterator a reference to the dma\_fence is held and the RCU lock dropped.

Beware that the iterator can be restarted when the [`struct dma_resv`](#c.dma_resv "dma_resv") for
**cursor** is modified. Code which accumulates statistics or similar needs to
check for this with [`dma_resv_iter_is_restarted()`](#c.dma_resv_iter_is_restarted "dma_resv_iter_is_restarted"). For this reason prefer the
lock iterator [`dma_resv_for_each_fence()`](#c.dma_resv_for_each_fence "dma_resv_for_each_fence") whenever possible.

dma\_resv\_for\_each\_fence

`dma_resv_for_each_fence (cursor, obj, usage, fence)`

> fence iterator

**Parameters**

`cursor`
:   a [`struct dma_resv_iter`](#c.dma_resv_iter "dma_resv_iter") pointer

`obj`
:   a dma\_resv object pointer

`usage`
:   controls which fences to return

`fence`
:   the current fence

**Description**

Iterate over the fences in a [`struct dma_resv`](#c.dma_resv "dma_resv") object while holding the
[`dma_resv.lock`](#c.dma_resv "dma_resv"). **all\_fences** controls if the shared fences are returned as
well. The cursor initialisation is part of the iterator and the fence stays
valid as long as the lock is held and so no extra reference to the fence is
taken.

int dma\_resv\_lock(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj, struct ww\_acquire\_ctx \*ctx)
:   lock the reservation object

**Parameters**

`struct dma_resv *obj`
:   the reservation object

`struct ww_acquire_ctx *ctx`
:   the locking context

**Description**

Locks the reservation object for exclusive access and modification. Note,
that the lock is only against other writers, readers will run concurrently
with a writer under RCU. The seqlock is used to notify readers if they
overlap with a writer.

As the reservation object may be locked by multiple parties in an
undefined order, a #ww\_acquire\_ctx is passed to unwind if a cycle
is detected. See `ww_mutex_lock()` and `ww_acquire_init()`. A reservation
object may be locked by itself by passing NULL as **ctx**.

When a die situation is indicated by returning -EDEADLK all locks held by
**ctx** must be unlocked and then [`dma_resv_lock_slow()`](#c.dma_resv_lock_slow "dma_resv_lock_slow") called on **obj**.

Unlocked by calling [`dma_resv_unlock()`](#c.dma_resv_unlock "dma_resv_unlock").

See also [`dma_resv_lock_interruptible()`](#c.dma_resv_lock_interruptible "dma_resv_lock_interruptible") for the interruptible variant.

int dma\_resv\_lock\_interruptible(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj, struct ww\_acquire\_ctx \*ctx)
:   lock the reservation object

**Parameters**

`struct dma_resv *obj`
:   the reservation object

`struct ww_acquire_ctx *ctx`
:   the locking context

**Description**

Locks the reservation object interruptible for exclusive access and
modification. Note, that the lock is only against other writers, readers
will run concurrently with a writer under RCU. The seqlock is used to
notify readers if they overlap with a writer.

As the reservation object may be locked by multiple parties in an
undefined order, a #ww\_acquire\_ctx is passed to unwind if a cycle
is detected. See `ww_mutex_lock()` and `ww_acquire_init()`. A reservation
object may be locked by itself by passing NULL as **ctx**.

When a die situation is indicated by returning -EDEADLK all locks held by
**ctx** must be unlocked and then [`dma_resv_lock_slow_interruptible()`](#c.dma_resv_lock_slow_interruptible "dma_resv_lock_slow_interruptible") called on
**obj**.

Unlocked by calling [`dma_resv_unlock()`](#c.dma_resv_unlock "dma_resv_unlock").

void dma\_resv\_lock\_slow(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj, struct ww\_acquire\_ctx \*ctx)
:   slowpath lock the reservation object

**Parameters**

`struct dma_resv *obj`
:   the reservation object

`struct ww_acquire_ctx *ctx`
:   the locking context

**Description**

Acquires the reservation object after a die case. This function
will sleep until the lock becomes available. See [`dma_resv_lock()`](#c.dma_resv_lock "dma_resv_lock") as
well.

See also [`dma_resv_lock_slow_interruptible()`](#c.dma_resv_lock_slow_interruptible "dma_resv_lock_slow_interruptible") for the interruptible variant.

int dma\_resv\_lock\_slow\_interruptible(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj, struct ww\_acquire\_ctx \*ctx)
:   slowpath lock the reservation object, interruptible

**Parameters**

`struct dma_resv *obj`
:   the reservation object

`struct ww_acquire_ctx *ctx`
:   the locking context

**Description**

Acquires the reservation object interruptible after a die case. This function
will sleep until the lock becomes available. See
[`dma_resv_lock_interruptible()`](#c.dma_resv_lock_interruptible "dma_resv_lock_interruptible") as well.

bool dma\_resv\_trylock(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj)
:   trylock the reservation object

**Parameters**

`struct dma_resv *obj`
:   the reservation object

**Description**

Tries to lock the reservation object for exclusive access and modification.
Note, that the lock is only against other writers, readers will run
concurrently with a writer under RCU. The seqlock is used to notify readers
if they overlap with a writer.

Also note that since no context is provided, no deadlock protection is
possible, which is also not needed for a trylock.

Returns true if the lock was acquired, false otherwise.

bool dma\_resv\_is\_locked(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj)
:   is the reservation object locked

**Parameters**

`struct dma_resv *obj`
:   the reservation object

**Description**

Returns true if the mutex is locked, false if unlocked.

struct ww\_acquire\_ctx \*dma\_resv\_locking\_ctx(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj)
:   returns the context used to lock the object

**Parameters**

`struct dma_resv *obj`
:   the reservation object

**Description**

Returns the context used to lock a reservation object or NULL if no context
was used or the object is not locked at all.

WARNING: This interface is pretty horrible, but TTM needs it because it
doesn’t pass the `struct ww_acquire_ctx` around in some very long callchains.
Everyone else just uses it to check whether they’re holding a reservation or
not.

void dma\_resv\_unlock(struct [dma\_resv](#c.dma_resv "dma_resv") \*obj)
:   unlock the reservation object

**Parameters**

`struct dma_resv *obj`
:   the reservation object

**Description**

Unlocks the reservation object following exclusive access.

## DMA Fences

DMA fences, represented by [`struct dma_fence`](#c.dma_fence "dma_fence"), are the kernel internal
synchronization primitive for DMA operations like GPU rendering, video
encoding/decoding, or displaying buffers on a screen.

A fence is initialized using [`dma_fence_init()`](#c.dma_fence_init "dma_fence_init") and completed using
[`dma_fence_signal()`](#c.dma_fence_signal "dma_fence_signal"). Fences are associated with a context, allocated through
[`dma_fence_context_alloc()`](#c.dma_fence_context_alloc "dma_fence_context_alloc"), and all fences on the same context are
fully ordered.

Since the purposes of fences is to facilitate cross-device and
cross-application synchronization, there’s multiple ways to use one:

* Individual fences can be exposed as a [`sync_file`](#c.sync_file "sync_file"), accessed as a file
  descriptor from userspace, created by calling [`sync_file_create()`](#c.sync_file_create "sync_file_create"). This is
  called explicit fencing, since userspace passes around explicit
  synchronization points.
* Some subsystems also have their own explicit fencing primitives, like
  [`drm_syncobj`](../gpu/drm-mm.html#c.drm_syncobj "drm_syncobj"). Compared to [`sync_file`](#c.sync_file "sync_file"), a [`drm_syncobj`](../gpu/drm-mm.html#c.drm_syncobj "drm_syncobj") allows the underlying
  fence to be updated.
* Then there’s also implicit fencing, where the synchronization points are
  implicitly passed around as part of shared [`dma_buf`](#c.dma_buf "dma_buf") instances. Such
  implicit fences are stored in [`struct dma_resv`](#c.dma_resv "dma_resv") through the
  [`dma_buf.resv`](#c.dma_buf "dma_buf") pointer.

### DMA Fence Cross-Driver Contract

Since [`dma_fence`](#c.dma_fence "dma_fence") provide a cross driver contract, all drivers must follow the
same rules:

* Fences must complete in a reasonable time. Fences which represent kernels
  and shaders submitted by userspace, which could run forever, must be backed
  up by timeout and gpu hang recovery code. Minimally that code must prevent
  further command submission and force complete all in-flight fences, e.g.
  when the driver or hardware do not support gpu reset, or if the gpu reset
  failed for some reason. Ideally the driver supports gpu recovery which only
  affects the offending userspace context, and no other userspace
  submissions.
* Drivers may have different ideas of what completion within a reasonable
  time means. Some hang recovery code uses a fixed timeout, others a mix
  between observing forward progress and increasingly strict timeouts.
  Drivers should not try to second guess timeout handling of fences from
  other drivers.
* To ensure there’s no deadlocks of [`dma_fence_wait()`](#c.dma_fence_wait "dma_fence_wait") against other locks
  drivers should annotate all code required to reach [`dma_fence_signal()`](#c.dma_fence_signal "dma_fence_signal"),
  which completes the fences, with [`dma_fence_begin_signalling()`](#c.dma_fence_begin_signalling "dma_fence_begin_signalling") and
  [`dma_fence_end_signalling()`](#c.dma_fence_end_signalling "dma_fence_end_signalling").
* Drivers are allowed to call [`dma_fence_wait()`](#c.dma_fence_wait "dma_fence_wait") while holding [`dma_resv_lock()`](#c.dma_resv_lock "dma_resv_lock").
  This means any code required for fence completion cannot acquire a
  [`dma_resv`](#c.dma_resv "dma_resv") lock. Note that this also pulls in the entire established
  locking hierarchy around [`dma_resv_lock()`](#c.dma_resv_lock "dma_resv_lock") and [`dma_resv_unlock()`](#c.dma_resv_unlock "dma_resv_unlock").
* Drivers are allowed to call [`dma_fence_wait()`](#c.dma_fence_wait "dma_fence_wait") from their `shrinker`
  callbacks. This means any code required for fence completion cannot
  allocate memory with GFP\_KERNEL.
* Drivers are allowed to call [`dma_fence_wait()`](#c.dma_fence_wait "dma_fence_wait") from their `mmu_notifier`
  respectively `mmu_interval_notifier` callbacks. This means any code required
  for fence completion cannot allocate memory with GFP\_NOFS or GFP\_NOIO.
  Only GFP\_ATOMIC is permissible, which might fail.

Note that only GPU drivers have a reasonable excuse for both requiring
`mmu_interval_notifier` and `shrinker` callbacks at the same time as having to
track asynchronous compute work using [`dma_fence`](#c.dma_fence "dma_fence"). No driver outside of
drivers/gpu should ever call [`dma_fence_wait()`](#c.dma_fence_wait "dma_fence_wait") in such contexts.

### DMA Fence Signalling Annotations

Proving correctness of all the kernel code around [`dma_fence`](#c.dma_fence "dma_fence") through code
review and testing is tricky for a few reasons:

* It is a cross-driver contract, and therefore all drivers must follow the
  same rules for lock nesting order, calling contexts for various functions
  and anything else significant for in-kernel interfaces. But it is also
  impossible to test all drivers in a single machine, hence brute-force N vs.
  N testing of all combinations is impossible. Even just limiting to the
  possible combinations is infeasible.
* There is an enormous amount of driver code involved. For render drivers
  there’s the tail of command submission, after fences are published,
  scheduler code, interrupt and workers to process job completion,
  and timeout, gpu reset and gpu hang recovery code. Plus for integration
  with core mm with have `mmu_notifier`, respectively `mmu_interval_notifier`,
  and `shrinker`. For modesetting drivers there’s the commit tail functions
  between when fences for an atomic modeset are published, and when the
  corresponding vblank completes, including any interrupt processing and
  related workers. Auditing all that code, across all drivers, is not
  feasible.
* Due to how many other subsystems are involved and the locking hierarchies
  this pulls in there is extremely thin wiggle-room for driver-specific
  differences. [`dma_fence`](#c.dma_fence "dma_fence") interacts with almost all of the core memory
  handling through page fault handlers via [`dma_resv`](#c.dma_resv "dma_resv"), [`dma_resv_lock()`](#c.dma_resv_lock "dma_resv_lock") and
  [`dma_resv_unlock()`](#c.dma_resv_unlock "dma_resv_unlock"). On the other side it also interacts through all
  allocation sites through `mmu_notifier` and `shrinker`.

Furthermore lockdep does not handle cross-release dependencies, which means
any deadlocks between [`dma_fence_wait()`](#c.dma_fence_wait "dma_fence_wait") and [`dma_fence_signal()`](#c.dma_fence_signal "dma_fence_signal") can’t be caught
at runtime with some quick testing. The simplest example is one thread
waiting on a [`dma_fence`](#c.dma_fence "dma_fence") while holding a lock:

```
lock(A);
dma_fence_wait(B);
unlock(A);
```

while the other thread is stuck trying to acquire the same lock, which
prevents it from signalling the fence the previous thread is stuck waiting
on:

```
lock(A);
unlock(A);
dma_fence_signal(B);
```

By manually annotating all code relevant to signalling a [`dma_fence`](#c.dma_fence "dma_fence") we can
teach lockdep about these dependencies, which also helps with the validation
headache since now lockdep can check all the rules for us:

```
cookie = dma_fence_begin_signalling();
lock(A);
unlock(A);
dma_fence_signal(B);
dma_fence_end_signalling(cookie);
```

For using [`dma_fence_begin_signalling()`](#c.dma_fence_begin_signalling "dma_fence_begin_signalling") and [`dma_fence_end_signalling()`](#c.dma_fence_end_signalling "dma_fence_end_signalling") to
annotate critical sections the following rules need to be observed:

* All code necessary to complete a [`dma_fence`](#c.dma_fence "dma_fence") must be annotated, from the
  point where a fence is accessible to other threads, to the point where
  [`dma_fence_signal()`](#c.dma_fence_signal "dma_fence_signal") is called. Un-annotated code can contain deadlock issues,
  and due to the very strict rules and many corner cases it is infeasible to
  catch these just with review or normal stress testing.
* [`struct dma_resv`](#c.dma_resv "dma_resv") deserves a special note, since the readers are only
  protected by rcu. This means the signalling critical section starts as soon
  as the new fences are installed, even before [`dma_resv_unlock()`](#c.dma_resv_unlock "dma_resv_unlock") is called.
* The only exception are fast paths and opportunistic signalling code, which
  calls [`dma_fence_signal()`](#c.dma_fence_signal "dma_fence_signal") purely as an optimization, but is not required to
  guarantee completion of a [`dma_fence`](#c.dma_fence "dma_fence"). The usual example is a wait IOCTL
  which calls [`dma_fence_signal()`](#c.dma_fence_signal "dma_fence_signal"), while the mandatory completion path goes
  through a hardware interrupt and possible job completion worker.
* To aid composability of code, the annotations can be freely nested, as long
  as the overall locking hierarchy is consistent. The annotations also work
  both in interrupt and process context. Due to implementation details this
  requires that callers pass an opaque cookie from
  [`dma_fence_begin_signalling()`](#c.dma_fence_begin_signalling "dma_fence_begin_signalling") to [`dma_fence_end_signalling()`](#c.dma_fence_end_signalling "dma_fence_end_signalling").
* Validation against the cross driver contract is implemented by priming
  lockdep with the relevant hierarchy at boot-up. This means even just
  testing with a single device is enough to validate a driver, at least as
  far as deadlocks with [`dma_fence_wait()`](#c.dma_fence_wait "dma_fence_wait") against [`dma_fence_signal()`](#c.dma_fence_signal "dma_fence_signal") are
  concerned.

### DMA Fence Deadline Hints

In an ideal world, it would be possible to pipeline a workload sufficiently
that a utilization based device frequency governor could arrive at a minimum
frequency that meets the requirements of the use-case, in order to minimize
power consumption. But in the real world there are many workloads which
defy this ideal. For example, but not limited to:

* Workloads that ping-pong between device and CPU, with alternating periods
  of CPU waiting for device, and device waiting on CPU. This can result in
  devfreq and cpufreq seeing idle time in their respective domains and in
  result reduce frequency.
* Workloads that interact with a periodic time based deadline, such as double
  buffered GPU rendering vs vblank sync’d page flipping. In this scenario,
  missing a vblank deadline results in an *increase* in idle time on the GPU
  (since it has to wait an additional vblank period), sending a signal to
  the GPU’s devfreq to reduce frequency, when in fact the opposite is what is
  needed.

To this end, deadline hint(s) can be set on a [`dma_fence`](#c.dma_fence "dma_fence") via [`dma_fence_set_deadline`](#c.dma_fence_set_deadline "dma_fence_set_deadline")
(or indirectly via userspace facing ioctls like [`sync_set_deadline`](#c.sync_set_deadline "sync_set_deadline")).
The deadline hint provides a way for the waiting driver, or userspace, to
convey an appropriate sense of urgency to the signaling driver.

A deadline hint is given in absolute ktime (CLOCK\_MONOTONIC for userspace
facing APIs). The time could either be some point in the future (such as
the vblank based deadline for page-flipping, or the start of a compositor’s
composition cycle), or the current time to indicate an immediate deadline
hint (Ie. forward progress cannot be made until this fence is signaled).

Multiple deadlines may be set on a given fence, even in parallel. See the
documentation for [`dma_fence_ops.set_deadline`](#c.dma_fence_ops "dma_fence_ops").

The deadline hint is just that, a hint. The driver that created the fence
may react by increasing frequency, making different scheduling choices, etc.
Or doing nothing at all.

### DMA Fences Functions Reference

struct [dma\_fence](#c.dma_fence "dma_fence") \*dma\_fence\_get\_stub(void)
:   return a signaled fence

**Parameters**

`void`
:   no arguments

**Description**

Return a stub fence which is already signaled. The fence’s timestamp
corresponds to the initialisation time of the linux kernel.

struct [dma\_fence](#c.dma_fence "dma_fence") \*dma\_fence\_allocate\_private\_stub(ktime\_t timestamp)
:   return a private, signaled fence

**Parameters**

`ktime_t timestamp`
:   timestamp when the fence was signaled

**Description**

Return a newly allocated and signaled stub fence.

u64 dma\_fence\_context\_alloc(unsigned num)
:   allocate an array of fence contexts

**Parameters**

`unsigned num`
:   amount of contexts to allocate

**Description**

This function will return the first index of the number of fence contexts
allocated. The fence context is used for setting [`dma_fence.context`](#c.dma_fence "dma_fence") to a
unique number by passing the context to [`dma_fence_init()`](#c.dma_fence_init "dma_fence_init").

bool dma\_fence\_begin\_signalling(void)
:   begin a critical DMA fence signalling section

**Parameters**

`void`
:   no arguments

**Description**

Drivers should use this to annotate the beginning of any code section
required to eventually complete [`dma_fence`](#c.dma_fence "dma_fence") by calling [`dma_fence_signal()`](#c.dma_fence_signal "dma_fence_signal").

The end of these critical sections are annotated with
[`dma_fence_end_signalling()`](#c.dma_fence_end_signalling "dma_fence_end_signalling").

Opaque cookie needed by the implementation, which needs to be passed to
[`dma_fence_end_signalling()`](#c.dma_fence_end_signalling "dma_fence_end_signalling").

void dma\_fence\_end\_signalling(bool cookie)
:   end a critical DMA fence signalling section

**Parameters**

`bool cookie`
:   opaque cookie from [`dma_fence_begin_signalling()`](#c.dma_fence_begin_signalling "dma_fence_begin_signalling")

**Description**

Closes a critical section annotation opened by [`dma_fence_begin_signalling()`](#c.dma_fence_begin_signalling "dma_fence_begin_signalling").

void dma\_fence\_signal\_timestamp\_locked(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence, ktime\_t timestamp)
:   signal completion of a fence

**Parameters**

`struct dma_fence *fence`
:   the fence to signal

`ktime_t timestamp`
:   fence signal timestamp in kernel’s CLOCK\_MONOTONIC time domain

**Description**

Signal completion for software callbacks on a fence, this will unblock
[`dma_fence_wait()`](#c.dma_fence_wait "dma_fence_wait") calls and run all the callbacks added with
[`dma_fence_add_callback()`](#c.dma_fence_add_callback "dma_fence_add_callback"). Can be called multiple times, but since a fence
can only go from the unsignaled to the signaled state and not back, it will
only be effective the first time. Set the timestamp provided as the fence
signal timestamp.

Unlike [`dma_fence_signal_timestamp()`](#c.dma_fence_signal_timestamp "dma_fence_signal_timestamp"), this function must be called with
[`dma_fence.lock`](#c.dma_fence "dma_fence") held.

void dma\_fence\_signal\_timestamp(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence, ktime\_t timestamp)
:   signal completion of a fence

**Parameters**

`struct dma_fence *fence`
:   the fence to signal

`ktime_t timestamp`
:   fence signal timestamp in kernel’s CLOCK\_MONOTONIC time domain

**Description**

Signal completion for software callbacks on a fence, this will unblock
[`dma_fence_wait()`](#c.dma_fence_wait "dma_fence_wait") calls and run all the callbacks added with
[`dma_fence_add_callback()`](#c.dma_fence_add_callback "dma_fence_add_callback"). Can be called multiple times, but since a fence
can only go from the unsignaled to the signaled state and not back, it will
only be effective the first time. Set the timestamp provided as the fence
signal timestamp.

void dma\_fence\_signal\_locked(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   signal completion of a fence

**Parameters**

`struct dma_fence *fence`
:   the fence to signal

**Description**

Signal completion for software callbacks on a fence, this will unblock
[`dma_fence_wait()`](#c.dma_fence_wait "dma_fence_wait") calls and run all the callbacks added with
[`dma_fence_add_callback()`](#c.dma_fence_add_callback "dma_fence_add_callback"). Can be called multiple times, but since a fence
can only go from the unsignaled to the signaled state and not back, it will
only be effective the first time.

Unlike [`dma_fence_signal()`](#c.dma_fence_signal "dma_fence_signal"), this function must be called with [`dma_fence.lock`](#c.dma_fence "dma_fence")
held.

bool dma\_fence\_check\_and\_signal\_locked(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   signal the fence if it’s not yet signaled

**Parameters**

`struct dma_fence *fence`
:   the fence to check and signal

**Description**

Checks whether a fence was signaled and signals it if it was not yet signaled.

Unlike [`dma_fence_check_and_signal()`](#c.dma_fence_check_and_signal "dma_fence_check_and_signal"), this function must be called with
[`struct dma_fence`](#c.dma_fence "dma_fence").lock being held.

**Return**

true if fence has been signaled already, false otherwise.

bool dma\_fence\_check\_and\_signal(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   signal the fence if it’s not yet signaled

**Parameters**

`struct dma_fence *fence`
:   the fence to check and signal

**Description**

Checks whether a fence was signaled and signals it if it was not yet signaled.
All this is done in a race-free manner.

**Return**

true if fence has been signaled already, false otherwise.

void dma\_fence\_signal(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   signal completion of a fence

**Parameters**

`struct dma_fence *fence`
:   the fence to signal

**Description**

Signal completion for software callbacks on a fence, this will unblock
[`dma_fence_wait()`](#c.dma_fence_wait "dma_fence_wait") calls and run all the callbacks added with
[`dma_fence_add_callback()`](#c.dma_fence_add_callback "dma_fence_add_callback"). Can be called multiple times, but since a fence
can only go from the unsignaled to the signaled state and not back, it will
only be effective the first time.

signed long dma\_fence\_wait\_timeout(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence, bool intr, signed long timeout)
:   sleep until the fence gets signaled or until timeout elapses

**Parameters**

`struct dma_fence *fence`
:   the fence to wait on

`bool intr`
:   if true, do an interruptible wait

`signed long timeout`
:   timeout value in jiffies, or MAX\_SCHEDULE\_TIMEOUT

**Description**

Returns -ERESTARTSYS if interrupted, 0 if the wait timed out, or the
remaining timeout in jiffies on success. Other error values may be
returned on custom implementations.

Performs a synchronous wait on this fence. It is assumed the caller
directly or indirectly (buf-mgr between reservation and committing)
holds a reference to the fence, otherwise the fence might be
freed before return, resulting in undefined behavior.

See also [`dma_fence_wait()`](#c.dma_fence_wait "dma_fence_wait") and [`dma_fence_wait_any_timeout()`](#c.dma_fence_wait_any_timeout "dma_fence_wait_any_timeout").

void dma\_fence\_release(struct [kref](#c.dma_fence_release "kref") \*kref)
:   default release function for fences

**Parameters**

`struct kref *kref`
:   [`dma_fence.recfount`](#c.dma_fence "dma_fence")

**Description**

This is the default release functions for [`dma_fence`](#c.dma_fence "dma_fence"). Drivers shouldn’t call
this directly, but instead call [`dma_fence_put()`](#c.dma_fence_put "dma_fence_put").

void dma\_fence\_free(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   default release function for [`dma_fence`](#c.dma_fence "dma_fence").

**Parameters**

`struct dma_fence *fence`
:   fence to release

**Description**

This is the default implementation for [`dma_fence_ops.release`](#c.dma_fence_ops "dma_fence_ops"). It calls
[`kfree_rcu()`](../core-api/kernel-api.html#c.kfree_rcu "kfree_rcu") on **fence**.

void dma\_fence\_enable\_sw\_signaling(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   enable signaling on fence

**Parameters**

`struct dma_fence *fence`
:   the fence to enable

**Description**

This will request for sw signaling to be enabled, to make the fence
complete as soon as possible. This calls [`dma_fence_ops.enable_signaling`](#c.dma_fence_ops "dma_fence_ops")
internally.

int dma\_fence\_add\_callback(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence, struct [dma\_fence\_cb](#c.dma_fence_cb "dma_fence_cb") \*cb, dma\_fence\_func\_t func)
:   add a callback to be called when the fence is signaled

**Parameters**

`struct dma_fence *fence`
:   the fence to wait on

`struct dma_fence_cb *cb`
:   the callback to register

`dma_fence_func_t func`
:   the function to call

**Description**

Add a software callback to the fence. The caller should keep a reference to
the fence.

**cb** will be initialized by [`dma_fence_add_callback()`](#c.dma_fence_add_callback "dma_fence_add_callback"), no initialization
by the caller is required. Any number of callbacks can be registered
to a fence, but a callback can only be registered to one fence at a time.

If fence is already signaled, this function will return -ENOENT (and
*not* call the callback).

Note that the callback can be called from an atomic context or irq context.

Returns 0 in case of success, -ENOENT if the fence is already signaled
and -EINVAL in case of error.

int dma\_fence\_get\_status(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   returns the status upon completion

**Parameters**

`struct dma_fence *fence`
:   the dma\_fence to query

**Description**

This wraps [`dma_fence_get_status_locked()`](#c.dma_fence_get_status_locked "dma_fence_get_status_locked") to return the error status
condition on a signaled fence. See [`dma_fence_get_status_locked()`](#c.dma_fence_get_status_locked "dma_fence_get_status_locked") for more
details.

Returns 0 if the fence has not yet been signaled, 1 if the fence has
been signaled without an error condition, or a negative error code
if the fence has been completed in err.

bool dma\_fence\_remove\_callback(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence, struct [dma\_fence\_cb](#c.dma_fence_cb "dma_fence_cb") \*cb)
:   remove a callback from the signaling list

**Parameters**

`struct dma_fence *fence`
:   the fence to wait on

`struct dma_fence_cb *cb`
:   the callback to remove

**Description**

Remove a previously queued callback from the fence. This function returns
true if the callback is successfully removed, or false if the fence has
already been signaled.

*WARNING*:
Cancelling a callback should only be done if you really know what you’re
doing, since deadlocks and race conditions could occur all too easily. For
this reason, it should only ever be done on hardware lockup recovery,
with a reference held to the fence.

Behaviour is undefined if **cb** has not been added to **fence** using
[`dma_fence_add_callback()`](#c.dma_fence_add_callback "dma_fence_add_callback") beforehand.

signed long dma\_fence\_default\_wait(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence, bool intr, signed long timeout)
:   default sleep until the fence gets signaled or until timeout elapses

**Parameters**

`struct dma_fence *fence`
:   the fence to wait on

`bool intr`
:   if true, do an interruptible wait

`signed long timeout`
:   timeout value in jiffies, or MAX\_SCHEDULE\_TIMEOUT

**Description**

Returns -ERESTARTSYS if interrupted, 0 if the wait timed out, or the
remaining timeout in jiffies on success. If timeout is zero the value one is
returned if the fence is already signaled for consistency with other
functions taking a jiffies timeout.

signed long dma\_fence\_wait\_any\_timeout(struct [dma\_fence](#c.dma_fence "dma_fence") \*\*fences, uint32\_t count, bool intr, signed long timeout, uint32\_t \*idx)
:   sleep until any fence gets signaled or until timeout elapses

**Parameters**

`struct dma_fence **fences`
:   array of fences to wait on

`uint32_t count`
:   number of fences to wait on

`bool intr`
:   if true, do an interruptible wait

`signed long timeout`
:   timeout value in jiffies, or MAX\_SCHEDULE\_TIMEOUT

`uint32_t *idx`
:   used to store the first signaled fence index, meaningful only on
    positive return

**Description**

Returns -EINVAL on custom fence wait implementation, -ERESTARTSYS if
interrupted, 0 if the wait timed out, or the remaining timeout in jiffies
on success.

Synchronous waits for the first fence in the array to be signaled. The
caller needs to hold a reference to all fences in the array, otherwise a
fence might be freed before return, resulting in undefined behavior.

See also [`dma_fence_wait()`](#c.dma_fence_wait "dma_fence_wait") and [`dma_fence_wait_timeout()`](#c.dma_fence_wait_timeout "dma_fence_wait_timeout").

void dma\_fence\_set\_deadline(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence, ktime\_t deadline)
:   set desired fence-wait deadline hint

**Parameters**

`struct dma_fence *fence`
:   the fence that is to be waited on

`ktime_t deadline`
:   the time by which the waiter hopes for the fence to be
    signaled

**Description**

Give the fence signaler a hint about an upcoming deadline, such as
vblank, by which point the waiter would prefer the fence to be
signaled by. This is intended to give feedback to the fence signaler
to aid in power management decisions, such as boosting GPU frequency
if a periodic vblank deadline is approaching but the fence is not
yet signaled..

void dma\_fence\_describe(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence, struct seq\_file \*seq)
:   Dump fence description into seq\_file

**Parameters**

`struct dma_fence *fence`
:   the fence to describe

`struct seq_file *seq`
:   the seq\_file to put the textual description into

**Description**

Dump a textual description of the fence and it’s state into the seq\_file.

void dma\_fence\_init(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence, const struct [dma\_fence\_ops](#c.dma_fence_ops "dma_fence_ops") \*ops, spinlock\_t \*lock, u64 context, u64 seqno)
:   Initialize a custom fence.

**Parameters**

`struct dma_fence *fence`
:   the fence to initialize

`const struct dma_fence_ops *ops`
:   the dma\_fence\_ops for operations on this fence

`spinlock_t *lock`
:   optional irqsafe spinlock to use for locking this fence

`u64 context`
:   the execution context this fence is run on

`u64 seqno`
:   a linear increasing sequence number for this context

**Description**

Initializes an allocated fence, the caller doesn’t have to keep its
refcount after committing with this fence, but it will need to hold a
refcount again if [`dma_fence_ops.enable_signaling`](#c.dma_fence_ops "dma_fence_ops") gets called.

context and seqno are used for easy comparison between fences, allowing
to check which fence is later by simply using [`dma_fence_later()`](#c.dma_fence_later "dma_fence_later").

It is strongly discouraged to provide an external lock because this couples
lock and fence life time. This is only allowed for legacy use cases when
multiple fences need to be prevented from signaling out of order.

void dma\_fence\_init64(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence, const struct [dma\_fence\_ops](#c.dma_fence_ops "dma_fence_ops") \*ops, spinlock\_t \*lock, u64 context, u64 seqno)
:   Initialize a custom fence with 64-bit seqno support.

**Parameters**

`struct dma_fence *fence`
:   the fence to initialize

`const struct dma_fence_ops *ops`
:   the dma\_fence\_ops for operations on this fence

`spinlock_t *lock`
:   optional irqsafe spinlock to use for locking this fence

`u64 context`
:   the execution context this fence is run on

`u64 seqno`
:   a linear increasing sequence number for this context

**Description**

Initializes an allocated fence, the caller doesn’t have to keep its
refcount after committing with this fence, but it will need to hold a
refcount again if [`dma_fence_ops.enable_signaling`](#c.dma_fence_ops "dma_fence_ops") gets called.

Context and seqno are used for easy comparison between fences, allowing
to check which fence is later by simply using [`dma_fence_later()`](#c.dma_fence_later "dma_fence_later").

It is strongly discouraged to provide an external lock because this couples
lock and fence life time. This is only allowed for legacy use cases when
multiple fences need to be prevented from signaling out of order.

const char \_\_rcu \*dma\_fence\_driver\_name(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   Access the driver name

**Parameters**

`struct dma_fence *fence`
:   the fence to query

**Description**

Returns a driver name backing the dma-fence implementation.

IMPORTANT CONSIDERATION:
Dma-fence contract stipulates that access to driver provided data (data not
directly embedded into the object itself), such as the [`dma_fence.lock`](#c.dma_fence "dma_fence") and
memory potentially accessed by the [`dma_fence.ops`](#c.dma_fence "dma_fence") functions, is forbidden
after the fence has been signalled. Drivers are allowed to free that data,
and some do.

To allow safe access drivers are mandated to guarantee a RCU grace period
between signalling the fence and freeing said data.

As such access to the driver name is only valid inside a RCU locked section.
The pointer MUST be both queried and USED ONLY WITHIN a SINGLE block guarded
by the [`rcu_read_lock`](../core-api/kernel-api.html#c.rcu_read_lock "rcu_read_lock") and [`rcu_read_unlock`](../core-api/kernel-api.html#c.rcu_read_unlock "rcu_read_unlock") pair.

const char \_\_rcu \*dma\_fence\_timeline\_name(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   Access the timeline name

**Parameters**

`struct dma_fence *fence`
:   the fence to query

**Description**

Returns a timeline name provided by the dma-fence implementation.

IMPORTANT CONSIDERATION:
Dma-fence contract stipulates that access to driver provided data (data not
directly embedded into the object itself), such as the [`dma_fence.lock`](#c.dma_fence "dma_fence") and
memory potentially accessed by the [`dma_fence.ops`](#c.dma_fence "dma_fence") functions, is forbidden
after the fence has been signalled. Drivers are allowed to free that data,
and some do.

To allow safe access drivers are mandated to guarantee a RCU grace period
between signalling the fence and freeing said data.

As such access to the driver name is only valid inside a RCU locked section.
The pointer MUST be both queried and USED ONLY WITHIN a SINGLE block guarded
by the [`rcu_read_lock`](../core-api/kernel-api.html#c.rcu_read_lock "rcu_read_lock") and [`rcu_read_unlock`](../core-api/kernel-api.html#c.rcu_read_unlock "rcu_read_unlock") pair.

struct dma\_fence
:   software synchronization primitive

**Definition**:

```
struct dma_fence {
    union {
        spinlock_t *extern_lock;
        spinlock_t inline_lock;
    };
    const struct dma_fence_ops *ops;
    union {
        struct list_head cb_list;
        ktime_t timestamp;
        struct rcu_head rcu;
    };
    u64 context;
    u64 seqno;
    unsigned long flags;
    struct kref refcount;
    int error;
};
```

**Members**

`{unnamed_union}`
:   anonymous

`extern_lock`
:   external spin\_lock\_irqsave used for locking (deprecated)

`inline_lock`
:   alternative internal spin\_lock\_irqsave used for locking

`ops`
:   dma\_fence\_ops associated with this fence

`{unnamed_union}`
:   anonymous

`cb_list`
:   list of all callbacks to call

`timestamp`
:   Timestamp when the fence was signaled.

`rcu`
:   used for releasing fence with kfree\_rcu

`context`
:   execution context this fence belongs to, returned by
    [`dma_fence_context_alloc()`](#c.dma_fence_context_alloc "dma_fence_context_alloc")

`seqno`
:   the sequence number of this fence inside the execution context,
    can be compared to decide which fence would be signaled later.

`flags`
:   A mask of DMA\_FENCE\_FLAG\_\* defined below

`refcount`
:   refcount for this fence

`error`
:   Optional, only valid if < 0, must be set before calling
    dma\_fence\_signal, indicates that the fence has completed with an error.

**Description**

the flags member must be manipulated and read using the appropriate
atomic ops (bit\_\*), so taking the spinlock will not be needed most
of the time.

DMA\_FENCE\_FLAG\_INITIALIZED\_BIT - fence was initialized
DMA\_FENCE\_FLAG\_INLINE\_LOCK\_BIT - use inline spinlock instead of external one
DMA\_FENCE\_FLAG\_SIGNALED\_BIT - fence is already signaled
DMA\_FENCE\_FLAG\_TIMESTAMP\_BIT - timestamp recorded for fence signaling
DMA\_FENCE\_FLAG\_ENABLE\_SIGNAL\_BIT - enable\_signaling might have been called
DMA\_FENCE\_FLAG\_USER\_BITS - start of the unused bits, can be used by the
implementer of the fence for its own purposes. Can be used in different
ways by different fence implementers, so do not rely on this.

Since atomic bitops are used, this is not guaranteed to be the case.
Particularly, if the bit was set, but dma\_fence\_signal was called right
before this bit was set, it would have been able to set the
DMA\_FENCE\_FLAG\_SIGNALED\_BIT, before enable\_signaling was called.
Adding a check for DMA\_FENCE\_FLAG\_SIGNALED\_BIT after setting
DMA\_FENCE\_FLAG\_ENABLE\_SIGNAL\_BIT closes this race, and makes sure that
after dma\_fence\_signal was called, any enable\_signaling call will have either
been completed, or never called at all.

struct dma\_fence\_cb
:   callback for [`dma_fence_add_callback()`](#c.dma_fence_add_callback "dma_fence_add_callback")

**Definition**:

```
struct dma_fence_cb {
    struct list_head node;
    dma_fence_func_t func;
};
```

**Members**

`node`
:   used by [`dma_fence_add_callback()`](#c.dma_fence_add_callback "dma_fence_add_callback") to append this `struct to` fence::cb\_list

`func`
:   dma\_fence\_func\_t to call

**Description**

This `struct will` be initialized by [`dma_fence_add_callback()`](#c.dma_fence_add_callback "dma_fence_add_callback"), additional
data can be passed along by embedding dma\_fence\_cb in another struct.

struct dma\_fence\_ops
:   operations implemented for fence

**Definition**:

```
struct dma_fence_ops {
    const char * (*get_driver_name)(struct dma_fence *fence);
    const char * (*get_timeline_name)(struct dma_fence *fence);
    bool (*enable_signaling)(struct dma_fence *fence);
    bool (*signaled)(struct dma_fence *fence);
    signed long (*wait)(struct dma_fence *fence, bool intr, signed long timeout);
    void (*release)(struct dma_fence *fence);
    void (*set_deadline)(struct dma_fence *fence, ktime_t deadline);
};
```

**Members**

`get_driver_name`
:   Returns the driver name. This is a callback to allow drivers to
    compute the name at runtime, without having it to store permanently
    for each fence, or build a cache of some sort.

    This callback is mandatory.

`get_timeline_name`
:   Return the name of the context this fence belongs to. This is a
    callback to allow drivers to compute the name at runtime, without
    having it to store permanently for each fence, or build a cache of
    some sort.

    This callback is mandatory.

`enable_signaling`
:   Enable software signaling of fence.

    For fence implementations that have the capability for hw->hw
    signaling, they can implement this op to enable the necessary
    interrupts, or insert commands into cmdstream, etc, to avoid these
    costly operations for the common case where only hw->hw
    synchronization is required. This is called in the first
    [`dma_fence_wait()`](#c.dma_fence_wait "dma_fence_wait") or [`dma_fence_add_callback()`](#c.dma_fence_add_callback "dma_fence_add_callback") path to let the fence
    implementation know that there is another driver waiting on the
    signal (ie. hw->sw case).

    This is called with irq’s disabled, so only spinlocks which disable
    IRQ’s can be used in the code outside of this callback.

    A return value of false indicates the fence already passed,
    or some failure occurred that made it impossible to enable
    signaling. True indicates successful enabling.

    [`dma_fence.error`](#c.dma_fence "dma_fence") may be set in enable\_signaling, but only when false
    is returned.

    Since many implementations can call [`dma_fence_signal()`](#c.dma_fence_signal "dma_fence_signal") even when before
    **enable\_signaling** has been called there’s a race window, where the
    [`dma_fence_signal()`](#c.dma_fence_signal "dma_fence_signal") might result in the final fence reference being
    released and its memory freed. To avoid this, implementations of this
    callback should grab their own reference using [`dma_fence_get()`](#c.dma_fence_get "dma_fence_get"), to be
    released when the fence is signalled (through e.g. the interrupt
    handler).

    This callback is optional. If this callback is not present, then the
    driver must always have signaling enabled.

`signaled`
:   Peek whether the fence is signaled, as a fastpath optimization for
    e.g. [`dma_fence_wait()`](#c.dma_fence_wait "dma_fence_wait") or [`dma_fence_add_callback()`](#c.dma_fence_add_callback "dma_fence_add_callback"). Note that this
    callback does not need to make any guarantees beyond that a fence
    once indicates as signalled must always return true from this
    callback. This callback may return false even if the fence has
    completed already, in this case information hasn’t propogated throug
    the system yet. See also [`dma_fence_is_signaled()`](#c.dma_fence_is_signaled "dma_fence_is_signaled").

    May set [`dma_fence.error`](#c.dma_fence "dma_fence") if returning true.

    This callback is optional.

`wait`
:   Custom wait implementation, defaults to [`dma_fence_default_wait()`](#c.dma_fence_default_wait "dma_fence_default_wait") if
    not set.

    Deprecated and should not be used by new implementations. Only used
    by existing implementations which need special handling for their
    hardware reset procedure.

    Must return -ERESTARTSYS if the wait is intr = true and the wait was
    interrupted, and remaining jiffies if fence has signaled, or 0 if wait
    timed out. Can also return other error values on custom implementations,
    which should be treated as if the fence is signaled. For example a hardware
    lockup could be reported like that.

    Implementing this callback prevents the fence from detaching after
    signaling and so it is necessary for the module providing the
    dma\_fence\_ops to stay loaded as long as the dma\_fence exists.

`release`
:   Called on destruction of fence to release additional resources.
    Can be called from irq context. This callback is optional. If it is
    NULL, then [`dma_fence_free()`](#c.dma_fence_free "dma_fence_free") is instead called as the default
    implementation.

    Implementing this callback prevents the fence from detaching after
    signaling and so it is necessary for the module providing the
    dma\_fence\_ops to stay loaded as long as the dma\_fence exists.

    If the callback is implemented the memory backing the dma\_fence
    object must be freed RCU safe.

`set_deadline`
:   Callback to allow a fence waiter to inform the fence signaler of
    an upcoming deadline, such as vblank, by which point the waiter
    would prefer the fence to be signaled by. This is intended to
    give feedback to the fence signaler to aid in power management
    decisions, such as boosting GPU frequency.

    This is called without [`dma_fence.lock`](#c.dma_fence "dma_fence") held, it can be called
    multiple times and from any context. Locking is up to the callee
    if it has some state to manage. If multiple deadlines are set,
    the expectation is to track the soonest one. If the deadline is
    before the current time, it should be interpreted as an immediate
    deadline.

    This callback is optional.

bool dma\_fence\_was\_initialized(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   test if fence was initialized

**Parameters**

`struct dma_fence *fence`
:   fence to test

**Return**

True if fence was ever initialized, false otherwise. Works correctly
only when memory backing the fence structure is zero initialized on
allocation.

void dma\_fence\_put(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   decreases refcount of the fence

**Parameters**

`struct dma_fence *fence`
:   fence to reduce refcount of

struct [dma\_fence](#c.dma_fence "dma_fence") \*dma\_fence\_get(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   increases refcount of the fence

**Parameters**

`struct dma_fence *fence`
:   fence to increase refcount of

**Description**

Returns the same fence, with refcount increased by 1.

struct [dma\_fence](#c.dma_fence "dma_fence") \*dma\_fence\_get\_rcu(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   get a fence from a dma\_resv\_list with rcu read lock

**Parameters**

`struct dma_fence *fence`
:   fence to increase refcount of

**Description**

Function returns NULL if no refcount could be obtained, or the fence.

struct [dma\_fence](#c.dma_fence "dma_fence") \*dma\_fence\_get\_rcu\_safe(struct [dma\_fence](#c.dma_fence "dma_fence") \_\_rcu \*\*fencep)
:   acquire a reference to an RCU tracked fence

**Parameters**

`struct dma_fence __rcu **fencep`
:   pointer to fence to increase refcount of

**Description**

Function returns NULL if no refcount could be obtained, or the fence.
This function handles acquiring a reference to a fence that may be
reallocated within the RCU grace period (such as with SLAB\_TYPESAFE\_BY\_RCU),
so long as the caller is using RCU on the pointer to the fence.

An alternative mechanism is to employ a seqlock to protect a bunch of
fences, such as used by [`struct dma_resv`](#c.dma_resv "dma_resv"). When using a seqlock,
the seqlock must be taken before and checked after a reference to the
fence is acquired (as shown here).

The caller is required to hold the RCU read lock.

spinlock\_t \*dma\_fence\_spinlock(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   return pointer to the spinlock protecting the fence

**Parameters**

`struct dma_fence *fence`
:   the fence to get the lock from

**Description**

Return either the pointer to the embedded or the external spin lock.

dma\_fence\_lock\_irqsave

`dma_fence_lock_irqsave (fence, flags)`

> irqsave lock the fence

**Parameters**

`fence`
:   the fence to lock

`flags`
:   where to store the CPU flags.

**Description**

Lock the fence, preventing it from changing to the signaled state.

dma\_fence\_unlock\_irqrestore

`dma_fence_unlock_irqrestore (fence, flags)`

> unlock the fence and irqrestore

**Parameters**

`fence`
:   the fence to unlock

`flags`
:   the CPU flags to restore

**Description**

Unlock the fence, allowing it to change its state to signaled again.

dma\_fence\_assert\_held

`dma_fence_assert_held (fence)`

> lockdep assertion that fence is locked

**Parameters**

`fence`
:   the fence which should be locked

bool dma\_fence\_is\_signaled\_locked(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   Return an indication if the fence is signaled yet.

**Parameters**

`struct dma_fence *fence`
:   the fence to check

**Description**

Returns true if the fence was already signaled, false if not. Since this
function doesn’t enable signaling, it is not guaranteed to ever return
true if [`dma_fence_add_callback()`](#c.dma_fence_add_callback "dma_fence_add_callback"), [`dma_fence_wait()`](#c.dma_fence_wait "dma_fence_wait") or
[`dma_fence_enable_sw_signaling()`](#c.dma_fence_enable_sw_signaling "dma_fence_enable_sw_signaling") haven’t been called before.

This function requires [`dma_fence.lock`](#c.dma_fence "dma_fence") to be held.

See also [`dma_fence_is_signaled()`](#c.dma_fence_is_signaled "dma_fence_is_signaled").

bool dma\_fence\_is\_signaled(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   Return an indication if the fence is signaled yet.

**Parameters**

`struct dma_fence *fence`
:   the fence to check

**Description**

Returns true if the fence was already signaled, false if not. Since this
function doesn’t enable signaling, it is not guaranteed to ever return
true if [`dma_fence_add_callback()`](#c.dma_fence_add_callback "dma_fence_add_callback"), [`dma_fence_wait()`](#c.dma_fence_wait "dma_fence_wait") or
[`dma_fence_enable_sw_signaling()`](#c.dma_fence_enable_sw_signaling "dma_fence_enable_sw_signaling") haven’t been called before.

It’s recommended for seqno fences to call dma\_fence\_signal when the
operation is complete, it makes it possible to prevent issues from
wraparound between time of issue and time of use by checking the return
value of this function before calling hardware-specific wait instructions.

See also [`dma_fence_is_signaled_locked()`](#c.dma_fence_is_signaled_locked "dma_fence_is_signaled_locked").

bool \_\_dma\_fence\_is\_later(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence, u64 f1, u64 f2)
:   return if f1 is chronologically later than f2

**Parameters**

`struct dma_fence *fence`
:   fence in whose context to do the comparison

`u64 f1`
:   the first fence’s seqno

`u64 f2`
:   the second fence’s seqno from the same context

**Description**

Returns true if f1 is chronologically later than f2. Both fences must be
from the same context, since a seqno is not common across contexts.

bool dma\_fence\_is\_later(struct [dma\_fence](#c.dma_fence "dma_fence") \*f1, struct [dma\_fence](#c.dma_fence "dma_fence") \*f2)
:   return if f1 is chronologically later than f2

**Parameters**

`struct dma_fence *f1`
:   the first fence from the same context

`struct dma_fence *f2`
:   the second fence from the same context

**Description**

Returns true if f1 is chronologically later than f2. Both fences must be
from the same context, since a seqno is not re-used across contexts.

bool dma\_fence\_is\_later\_or\_same(struct [dma\_fence](#c.dma_fence "dma_fence") \*f1, struct [dma\_fence](#c.dma_fence "dma_fence") \*f2)
:   return true if f1 is later or same as f2

**Parameters**

`struct dma_fence *f1`
:   the first fence from the same context

`struct dma_fence *f2`
:   the second fence from the same context

**Description**

Returns true if f1 is chronologically later than f2 or the same fence. Both
fences must be from the same context, since a seqno is not re-used across
contexts.

struct [dma\_fence](#c.dma_fence "dma_fence") \*dma\_fence\_later(struct [dma\_fence](#c.dma_fence "dma_fence") \*f1, struct [dma\_fence](#c.dma_fence "dma_fence") \*f2)
:   return the chronologically later fence

**Parameters**

`struct dma_fence *f1`
:   the first fence from the same context

`struct dma_fence *f2`
:   the second fence from the same context

**Description**

Returns NULL if both fences are signaled, otherwise the fence that would be
signaled last. Both fences must be from the same context, since a seqno is
not re-used across contexts.

int dma\_fence\_get\_status\_locked(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   returns the status upon completion

**Parameters**

`struct dma_fence *fence`
:   the dma\_fence to query

**Description**

Drivers can supply an optional error status condition before they signal
the fence (to indicate whether the fence was completed due to an error
rather than success). The value of the status condition is only valid
if the fence has been signaled, [`dma_fence_get_status_locked()`](#c.dma_fence_get_status_locked "dma_fence_get_status_locked") first checks
the signal state before reporting the error status.

Returns 0 if the fence has not yet been signaled, 1 if the fence has
been signaled without an error condition, or a negative error code
if the fence has been completed in err.

void dma\_fence\_set\_error(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence, int error)
:   flag an error condition on the fence

**Parameters**

`struct dma_fence *fence`
:   the dma\_fence

`int error`
:   the error to store

**Description**

Drivers can supply an optional error status condition before they signal
the fence, to indicate that the fence was completed due to an error
rather than success. This must be set before signaling (so that the value
is visible before any waiters on the signal callback are woken). This
helper exists to help catching erroneous setting of #dma\_fence.error.

Examples of error codes which drivers should use:

* `-ENODATA` This operation produced no data, no other operation affected.
* `-ECANCELED` All operations from the same context have been canceled.
* `-ETIME` Operation caused a timeout and potentially device reset.

ktime\_t dma\_fence\_timestamp(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   helper to get the completion timestamp of a fence

**Parameters**

`struct dma_fence *fence`
:   fence to get the timestamp from.

**Description**

After a fence is signaled the timestamp is updated with the signaling time,
but setting the timestamp can race with tasks waiting for the signaling. This
helper busy waits for the correct timestamp to appear.

signed long dma\_fence\_wait(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence, bool intr)
:   sleep until the fence gets signaled

**Parameters**

`struct dma_fence *fence`
:   the fence to wait on

`bool intr`
:   if true, do an interruptible wait

**Description**

This function will return -ERESTARTSYS if interrupted by a signal,
or 0 if the fence was signaled. Other error values may be
returned on custom implementations.

Performs a synchronous wait on this fence. It is assumed the caller
directly or indirectly holds a reference to the fence, otherwise the
fence might be freed before return, resulting in undefined behavior.

See also [`dma_fence_wait_timeout()`](#c.dma_fence_wait_timeout "dma_fence_wait_timeout") and [`dma_fence_wait_any_timeout()`](#c.dma_fence_wait_any_timeout "dma_fence_wait_any_timeout").

bool dma\_fence\_is\_array(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   check if a fence is from the array subclass

**Parameters**

`struct dma_fence *fence`
:   the fence to test

**Description**

Return true if it is a dma\_fence\_array and false otherwise.

bool dma\_fence\_is\_chain(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   check if a fence is from the chain subclass

**Parameters**

`struct dma_fence *fence`
:   the fence to test

**Description**

Return true if it is a dma\_fence\_chain and false otherwise.

bool dma\_fence\_is\_container(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   check if a fence is a container for other fences

**Parameters**

`struct dma_fence *fence`
:   the fence to test

**Description**

Return true if this fence is a container for other fences, false otherwise.
This is important since we can’t build up large fence structure or otherwise
we run into recursion during operation on those fences.

### DMA Fence Array

struct [dma\_fence\_array](#c.dma_fence_array "dma_fence_array") \*dma\_fence\_array\_alloc(int num\_fences)
:   Allocate a custom fence array

**Parameters**

`int num_fences`
:   [in] number of fences to add in the array

**Description**

Return dma fence array on success, NULL on failure

void dma\_fence\_array\_init(struct [dma\_fence\_array](#c.dma_fence_array "dma_fence_array") \*array, int num\_fences, struct [dma\_fence](#c.dma_fence "dma_fence") \*\*fences, u64 context, unsigned seqno, bool signal\_on\_any)
:   Init a custom fence array

**Parameters**

`struct dma_fence_array *array`
:   [in] dma fence array to arm

`int num_fences`
:   [in] number of fences to add in the array

`struct dma_fence **fences`
:   [in] array containing the fences

`u64 context`
:   [in] fence context to use

`unsigned seqno`
:   [in] sequence number to use

`bool signal_on_any`
:   [in] signal on any fence in the array

**Description**

Implementation of **dma\_fence\_array\_create** without allocation. Useful to init
a preallocated dma fence array in the path of reclaim or dma fence signaling.

struct [dma\_fence\_array](#c.dma_fence_array "dma_fence_array") \*dma\_fence\_array\_create(int num\_fences, struct [dma\_fence](#c.dma_fence "dma_fence") \*\*fences, u64 context, unsigned seqno, bool signal\_on\_any)
:   Create a custom fence array

**Parameters**

`int num_fences`
:   [in] number of fences to add in the array

`struct dma_fence **fences`
:   [in] array containing the fences

`u64 context`
:   [in] fence context to use

`unsigned seqno`
:   [in] sequence number to use

`bool signal_on_any`
:   [in] signal on any fence in the array

**Description**

Allocate a dma\_fence\_array object and initialize the base fence with
[`dma_fence_init()`](#c.dma_fence_init "dma_fence_init").
In case of error it returns NULL.

The caller should allocate the fences array with num\_fences size
and fill it with the fences it wants to add to the object. Ownership of this
array is taken and [`dma_fence_put()`](#c.dma_fence_put "dma_fence_put") is used on each fence on release.

If **signal\_on\_any** is true the fence array signals if any fence in the array
signals, otherwise it signals when all fences in the array signal.

bool dma\_fence\_match\_context(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence, u64 context)
:   Check if all fences are from the given context

**Parameters**

`struct dma_fence *fence`
:   [in] fence or fence array

`u64 context`
:   [in] fence context to check all fences against

**Description**

Checks the provided fence or, for a fence array, all fences in the array
against the given context. Returns false if any fence is from a different
context.

struct dma\_fence\_array\_cb
:   callback helper for fence array

**Definition**:

```
struct dma_fence_array_cb {
    struct dma_fence_cb cb;
    struct dma_fence_array *array;
};
```

**Members**

`cb`
:   fence callback structure for signaling

`array`
:   reference to the parent fence array object

struct dma\_fence\_array
:   fence to represent an array of fences

**Definition**:

```
struct dma_fence_array {
    struct dma_fence base;
    unsigned num_fences;
    atomic_t num_pending;
    struct dma_fence **fences;
    struct irq_work work;
    struct dma_fence_array_cb callbacks[];
};
```

**Members**

`base`
:   fence base class

`num_fences`
:   number of fences in the array

`num_pending`
:   fences in the array still pending

`fences`
:   array of the fences

`work`
:   internal irq\_work function

`callbacks`
:   array of callback helpers

struct [dma\_fence\_array](#c.dma_fence_array "dma_fence_array") \*to\_dma\_fence\_array(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   cast a fence to a dma\_fence\_array

**Parameters**

`struct dma_fence *fence`
:   fence to cast to a dma\_fence\_array

**Description**

Returns NULL if the fence is not a dma\_fence\_array,
or the dma\_fence\_array otherwise.

dma\_fence\_array\_for\_each

`dma_fence_array_for_each (fence, index, head)`

> iterate over all fences in array

**Parameters**

`fence`
:   current fence

`index`
:   index into the array

`head`
:   potential dma\_fence\_array object

**Description**

Test if **array** is a dma\_fence\_array object and if yes iterate over all fences
in the array. If not just iterate over the fence in **array** itself.

For a deep dive iterator see [`dma_fence_unwrap_for_each()`](#c.dma_fence_unwrap_for_each "dma_fence_unwrap_for_each").

### DMA Fence Chain

struct [dma\_fence](#c.dma_fence "dma_fence") \*dma\_fence\_chain\_walk(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   chain walking function

**Parameters**

`struct dma_fence *fence`
:   current chain node

**Description**

Walk the chain to the next node. Returns the next fence or NULL if we are at
the end of the chain. Garbage collects chain nodes which are already
signaled.

int dma\_fence\_chain\_find\_seqno(struct [dma\_fence](#c.dma_fence "dma_fence") \*\*pfence, uint64\_t seqno)
:   find fence chain node by seqno

**Parameters**

`struct dma_fence **pfence`
:   pointer to the chain node where to start

`uint64_t seqno`
:   the sequence number to search for

**Description**

Advance the fence pointer to the chain node which will signal this sequence
number. If no sequence number is provided then this is a no-op.

Returns EINVAL if the fence is not a chain node or the sequence number has
not yet advanced far enough.

void dma\_fence\_chain\_init(struct [dma\_fence\_chain](#c.dma_fence_chain "dma_fence_chain") \*chain, struct [dma\_fence](#c.dma_fence "dma_fence") \*prev, struct [dma\_fence](#c.dma_fence "dma_fence") \*fence, uint64\_t seqno)
:   initialize a fence chain

**Parameters**

`struct dma_fence_chain *chain`
:   the chain node to initialize

`struct dma_fence *prev`
:   the previous fence

`struct dma_fence *fence`
:   the current fence

`uint64_t seqno`
:   the sequence number to use for the fence chain

**Description**

Initialize a new chain node and either start a new chain or add the node to
the existing chain of the previous fence.

struct dma\_fence\_chain
:   fence to represent an node of a fence chain

**Definition**:

```
struct dma_fence_chain {
    struct dma_fence base;
    struct dma_fence *prev;
    u64 prev_seqno;
    struct dma_fence *fence;
    union {
        struct dma_fence_cb cb;
        struct irq_work work;
    };
};
```

**Members**

`base`
:   fence base class

`prev`
:   previous fence of the chain

`prev_seqno`
:   original previous seqno before garbage collection

`fence`
:   encapsulated fence

`{unnamed_union}`
:   anonymous

`cb`
:   callback for signaling

    This is used to add the callback for signaling the
    complection of the fence chain. Never used at the same time
    as the irq work.

`work`
:   irq work item for signaling

    Irq work structure to allow us to add the callback without
    running into lock inversion. Never used at the same time as
    the callback.

struct [dma\_fence\_chain](#c.dma_fence_chain "dma_fence_chain") \*to\_dma\_fence\_chain(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   cast a fence to a dma\_fence\_chain

**Parameters**

`struct dma_fence *fence`
:   fence to cast to a dma\_fence\_array

**Description**

Returns NULL if the fence is not a dma\_fence\_chain,
or the dma\_fence\_chain otherwise.

struct [dma\_fence](#c.dma_fence "dma_fence") \*dma\_fence\_chain\_contained(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   return the contained fence

**Parameters**

`struct dma_fence *fence`
:   the fence to test

**Description**

If the fence is a dma\_fence\_chain the function returns the fence contained
inside the chain object, otherwise it returns the fence itself.

dma\_fence\_chain\_alloc

`dma_fence_chain_alloc ()`

> **Description**
>
> Returns a new [`struct dma_fence_chain`](#c.dma_fence_chain "dma_fence_chain") object or NULL on failure.
>
> This specialized allocator has to be a macro for its allocations to be
> accounted separately (to have a separate alloc\_tag). The typecast is
> intentional to enforce typesafety.

void dma\_fence\_chain\_free(struct [dma\_fence\_chain](#c.dma_fence_chain "dma_fence_chain") \*chain)

**Parameters**

`struct dma_fence_chain *chain`
:   chain node to free

**Description**

Frees up an allocated but not used [`struct dma_fence_chain`](#c.dma_fence_chain "dma_fence_chain") object. This
doesn’t need an RCU grace period since the fence was never initialized nor
published. After [`dma_fence_chain_init()`](#c.dma_fence_chain_init "dma_fence_chain_init") has been called the fence must be
released by calling [`dma_fence_put()`](#c.dma_fence_put "dma_fence_put"), and not through this function.

dma\_fence\_chain\_for\_each

`dma_fence_chain_for_each (iter, head)`

> iterate over all fences in chain

**Parameters**

`iter`
:   current fence

`head`
:   starting point

**Description**

Iterate over all fences in the chain. We keep a reference to the current
fence while inside the loop which must be dropped when breaking out.

For a deep dive iterator see [`dma_fence_unwrap_for_each()`](#c.dma_fence_unwrap_for_each "dma_fence_unwrap_for_each").

### DMA Fence unwrap

struct dma\_fence\_unwrap
:   cursor into the container structure

**Definition**:

```
struct dma_fence_unwrap {
    struct dma_fence *chain;
    struct dma_fence *array;
    unsigned int index;
};
```

**Members**

`chain`
:   potential dma\_fence\_chain, but can be other fence as well

`array`
:   potential dma\_fence\_array, but can be other fence as well

`index`
:   last returned index if **array** is really a dma\_fence\_array

**Description**

Should be used with [`dma_fence_unwrap_for_each()`](#c.dma_fence_unwrap_for_each "dma_fence_unwrap_for_each") iterator macro.

dma\_fence\_unwrap\_for\_each

`dma_fence_unwrap_for_each (fence, cursor, head)`

> iterate over all fences in containers

**Parameters**

`fence`
:   current fence

`cursor`
:   current position inside the containers

`head`
:   starting point for the iterator

**Description**

Unwrap dma\_fence\_chain and dma\_fence\_array containers and deep dive into all
potential fences in them. If **head** is just a normal fence only that one is
returned.

dma\_fence\_unwrap\_merge

`dma_fence_unwrap_merge (...)`

> unwrap and merge fences

**Parameters**

`...`
:   variable arguments

**Description**

All fences given as parameters are unwrapped and merged back together as flat
dma\_fence\_array. Useful if multiple containers need to be merged together.

Implemented as a macro to allocate the necessary arrays on the stack and
account the stack frame size to the caller.

Returns NULL on memory allocation failure, a dma\_fence object representing
all the given fences otherwise.

### DMA Fence Sync File

struct [sync\_file](#c.sync_file "sync_file") \*sync\_file\_create(struct [dma\_fence](#c.dma_fence "dma_fence") \*fence)
:   creates a sync file

**Parameters**

`struct dma_fence *fence`
:   fence to add to the sync\_fence

**Description**

Creates a sync\_file containg **fence**. This function acquires and additional
reference of **fence** for the newly-created [`sync_file`](#c.sync_file "sync_file"), if it succeeds. The
sync\_file can be released with fput(sync\_file->file). Returns the
sync\_file or NULL in case of error.

struct [dma\_fence](#c.dma_fence "dma_fence") \*sync\_file\_get\_fence(int fd)
:   get the fence related to the sync\_file fd

**Parameters**

`int fd`
:   sync\_file fd to get the fence from

**Description**

Ensures **fd** references a valid sync\_file and returns a fence that
represents all fence in the sync\_file. On error NULL is returned.

struct sync\_file
:   sync file to export to the userspace

**Definition**:

```
struct sync_file {
    struct file             *file;
    char user_name[32];
#ifdef CONFIG_DEBUG_FS;
    struct list_head        sync_file_list;
#endif;
    wait_queue_head_t wq;
    unsigned long           flags;
    struct dma_fence        *fence;
    struct dma_fence_cb cb;
};
```

**Members**

`file`
:   file representing this fence

`user_name`
:   Name of the sync file provided by userspace, for merged fences.
    Otherwise generated through driver callbacks (in which case the
    entire array is 0).

`sync_file_list`
:   membership in global file list

`wq`
:   wait queue for fence signaling

`flags`
:   flags for the sync\_file

`fence`
:   fence with the fences in the sync\_file

`cb`
:   fence callback information

**Description**

flags:
POLL\_ENABLED: whether userspace is currently poll()’ing or not

### DMA Fence Sync File uABI

struct sync\_merge\_data
:   SYNC\_IOC\_MERGE: merge two fences

**Definition**:

```
struct sync_merge_data {
    char name[32];
    __s32 fd2;
    __s32 fence;
    __u32 flags;
    __u32 pad;
};
```

**Members**

`name`
:   name of new fence

`fd2`
:   file descriptor of second fence

`fence`
:   returns the fd of the new fence to userspace

`flags`
:   merge\_data flags

`pad`
:   padding for 64-bit alignment, should always be zero

**Description**

Creates a new fence containing copies of the sync\_pts in both
the calling fd and sync\_merge\_data.fd2. Returns the new fence’s
fd in sync\_merge\_data.fence

struct sync\_fence\_info
:   detailed fence information

**Definition**:

```
struct sync_fence_info {
    char obj_name[32];
    char driver_name[32];
    __s32 status;
    __u32 flags;
    __u64 timestamp_ns;
};
```

**Members**

`obj_name`
:   name of parent sync\_timeline

`driver_name`
:   name of driver implementing the parent

`status`
:   status of the fence 0:active 1:signaled <0:error

`flags`
:   fence\_info flags

`timestamp_ns`
:   timestamp of status change in nanoseconds

struct sync\_file\_info
:   SYNC\_IOC\_FILE\_INFO: get detailed information on a sync\_file

**Definition**:

```
struct sync_file_info {
    char name[32];
    __s32 status;
    __u32 flags;
    __u32 num_fences;
    __u32 pad;
    __u64 sync_fence_info;
};
```

**Members**

`name`
:   name of fence

`status`
:   status of fence. 1: signaled 0:active <0:error

`flags`
:   sync\_file\_info flags

`num_fences`
:   number of fences in the sync\_file

`pad`
:   padding for 64-bit alignment, should always be zero

`sync_fence_info`
:   pointer to array of struct [`sync_fence_info`](#c.sync_fence_info "sync_fence_info") with all
    fences in the sync\_file

**Description**

Takes a [`struct sync_file_info`](#c.sync_file_info "sync_file_info"). If num\_fences is 0, the field is updated
with the actual number of fences. If num\_fences is > 0, the system will
use the pointer provided on sync\_fence\_info to return up to num\_fences of
[`struct sync_fence_info`](#c.sync_fence_info "sync_fence_info"), with detailed fence information.

struct sync\_set\_deadline
:   SYNC\_IOC\_SET\_DEADLINE - set a deadline hint on a fence

**Definition**:

```
struct sync_set_deadline {
    __u64 deadline_ns;
    __u64 pad;
};
```

**Members**

`deadline_ns`
:   absolute time of the deadline

`pad`
:   must be zero

**Description**

Allows userspace to set a deadline on a fence, see [`dma_fence_set_deadline`](#c.dma_fence_set_deadline "dma_fence_set_deadline")

The timebase for the deadline is CLOCK\_MONOTONIC (same as vblank). For
example

> clock\_gettime(CLOCK\_MONOTONIC, `t`);
> deadline\_ns = (t.tv\_sec \* 1000000000L) + t.tv\_nsec + ns\_until\_deadline

### Indefinite DMA Fences

At various times [`struct dma_fence`](#c.dma_fence "dma_fence") with an indefinite time until [`dma_fence_wait()`](#c.dma_fence_wait "dma_fence_wait")
finishes have been proposed. Examples include:

* Future fences, used in HWC1 to signal when a buffer isn’t used by the display
  any longer, and created with the screen update that makes the buffer visible.
  The time this fence completes is entirely under userspace’s control.
* Proxy fences, proposed to handle &drm\_syncobj for which the fence has not yet
  been set. Used to asynchronously delay command submission.
* Userspace fences or gpu futexes, fine-grained locking within a command buffer
  that userspace uses for synchronization across engines or with the CPU, which
  are then imported as a DMA fence for integration into existing winsys
  protocols.
* Long-running compute command buffers, while still using traditional end of
  batch DMA fences for memory management instead of context preemption DMA
  fences which get reattached when the compute job is rescheduled.

Common to all these schemes is that userspace controls the dependencies of these
fences and controls when they fire. Mixing indefinite fences with normal
in-kernel DMA fences does not work, even when a fallback timeout is included to
protect against malicious userspace:

* Only the kernel knows about all DMA fence dependencies, userspace is not aware
  of dependencies injected due to memory management or scheduler decisions.
* Only userspace knows about all dependencies in indefinite fences and when
  exactly they will complete, the kernel has no visibility.

Furthermore the kernel has to be able to hold up userspace command submission
for memory management needs, which means we must support indefinite fences being
dependent upon DMA fences. If the kernel also support indefinite fences in the
kernel like a DMA fence, like any of the above proposal would, there is the
potential for deadlocks.

![Indefinite Fencing Dependency Cycle](../_images/DOT-e8ff13d1f6d4fbb7ed4e8bcd73fc8bed4777de4f.svg)

Indefinite Fencing Dependency Cycle

This means that the kernel might accidentally create deadlocks
through memory management dependencies which userspace is unaware of, which
randomly hangs workloads until the timeout kicks in. Workloads, which from
userspace’s perspective, do not contain a deadlock. In such a mixed fencing
architecture there is no single entity with knowledge of all dependencies.
Therefore preventing such deadlocks from within the kernel is not possible.

The only solution to avoid dependencies loops is by not allowing indefinite
fences in the kernel. This means:

* No future fences, proxy fences or userspace fences imported as DMA fences,
  with or without a timeout.
* No DMA fences that signal end of batchbuffer for command submission where
  userspace is allowed to use userspace fencing or long running compute
  workloads. This also means no implicit fencing for shared buffers in these
  cases.

### Recoverable Hardware Page Faults Implications

Modern hardware supports recoverable page faults, which has a lot of
implications for DMA fences.

First, a pending page fault obviously holds up the work that’s running on the
accelerator and a memory allocation is usually required to resolve the fault.
But memory allocations are not allowed to gate completion of DMA fences, which
means any workload using recoverable page faults cannot use DMA fences for
synchronization. Synchronization fences controlled by userspace must be used
instead.

On GPUs this poses a problem, because current desktop compositor protocols on
Linux rely on DMA fences, which means without an entirely new userspace stack
built on top of userspace fences, they cannot benefit from recoverable page
faults. Specifically this means implicit synchronization will not be possible.
The exception is when page faults are only used as migration hints and never to
on-demand fill a memory request. For now this means recoverable page
faults on GPUs are limited to pure compute workloads.

Furthermore GPUs usually have shared resources between the 3D rendering and
compute side, like compute units or command submission engines. If both a 3D
job with a DMA fence and a compute workload using recoverable page faults are
pending they could deadlock:

* The 3D workload might need to wait for the compute job to finish and release
  hardware resources first.
* The compute workload might be stuck in a page fault, because the memory
  allocation is waiting for the DMA fence of the 3D workload to complete.

There are a few options to prevent this problem, one of which drivers need to
ensure:

* Compute workloads can always be preempted, even when a page fault is pending
  and not yet repaired. Not all hardware supports this.
* DMA fence workloads and workloads which need page fault handling have
  independent hardware resources to guarantee forward progress. This could be
  achieved through e.g. through dedicated engines and minimal compute unit
  reservations for DMA fence workloads.
* The reservation approach could be further refined by only reserving the
  hardware resources for DMA fence workloads when they are in-flight. This must
  cover the time from when the DMA fence is visible to other threads up to
  moment when fence is completed through [`dma_fence_signal()`](#c.dma_fence_signal "dma_fence_signal").
* As a last resort, if the hardware provides no useful reservation mechanics,
  all workloads must be flushed from the GPU when switching between jobs
  requiring DMA fences or jobs requiring page fault handling: This means all DMA
  fences must complete before a compute job with page fault handling can be
  inserted into the scheduler queue. And vice versa, before a DMA fence can be
  made visible anywhere in the system, all compute workloads must be preempted
  to guarantee all pending GPU page faults are flushed.
* Only a fairly theoretical option would be to untangle these dependencies when
  allocating memory to repair hardware page faults, either through separate
  memory blocks or runtime tracking of the full dependency graph of all DMA
  fences. This results very wide impact on the kernel, since resolving the page
  on the CPU side can itself involve a page fault. It is much more feasible and
  robust to limit the impact of handling hardware page faults to the specific
  driver.

Note that workloads that run on independent hardware like copy engines or other
GPUs do not have any impact. This allows us to keep using DMA fences internally
in the kernel even for resolving hardware page faults, e.g. by using copy
engines to clear or copy memory needed to resolve the page fault.

In some ways this page fault problem is a special case of the Infinite DMA
Fences discussions: Infinite fences from compute workloads are allowed to
depend on DMA fences, but not the other way around. And not even the page fault
problem is new, because some other CPU thread in userspace might
hit a page fault which holds up a userspace fence - supporting page faults on
GPUs doesn’t anything fundamentally new.
