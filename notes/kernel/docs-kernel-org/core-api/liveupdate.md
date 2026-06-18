# Live Update Orchestrator

> 출처(원문): https://docs.kernel.org/core-api/liveupdate.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Live Update Orchestrator

Author:
:   Pasha Tatashin <[pasha.tatashin@soleen.com](mailto:pasha.tatashin%40soleen.com)>

Live Update is a specialized, kexec-based reboot process that allows a
running kernel to be updated from one version to another while preserving
the state of selected resources and keeping designated hardware devices
operational. For these devices, DMA activity may continue throughout the
kernel transition.

While the primary use case driving this work is supporting live updates of
the Linux kernel when it is used as a hypervisor in cloud environments, the
LUO framework itself is designed to be workload-agnostic. Live Update
facilitates a full kernel version upgrade for any type of system.

For example, a non-hypervisor system running an in-memory cache like
memcached with many gigabytes of data can use LUO. The userspace service
can place its cache into a memfd, have its state preserved by LUO, and
restore it immediately after the kernel kexec.

Whether the system is running virtual machines, containers, a
high-performance database, or networking services, LUO’s primary goal is to
enable a full kernel update by preserving critical userspace state and
keeping essential devices operational.

The core of LUO is a mechanism that tracks the progress of a live update,
along with a callback API that allows other kernel subsystems to participate
in the process. Example subsystems that can hook into LUO include: kvm,
iommu, interrupts, vfio, participating filesystems, and memory management.

LUO uses Kexec Handover to transfer memory state from the current kernel to
the next kernel. For more details see [Kexec Handover Subsystem](kho/index.html).

## LUO Sessions

LUO Sessions provide the core mechanism for grouping and managing [`struct
file`](../filesystems/api-summary.html#c.file "file") \* instances that need to be preserved across a kexec-based live
update. Each session acts as a named container for a set of file objects,
allowing a userspace agent to manage the lifecycle of resources critical to a
workload.

Core Concepts:

* Named Containers: Sessions are identified by a unique, user-provided name,
  which is used for both creation in the current kernel and retrieval in the
  next kernel.
* Userspace Interface: Session management is driven from userspace via
  ioctls on /dev/liveupdate.
* Serialization: Session metadata is preserved using the KHO framework. When
  a live update is triggered via kexec, an array of [`struct luo_session_ser`](#c.luo_session_ser "luo_session_ser")
  is populated and placed in a preserved memory region. An FDT node is also
  created, containing the count of sessions and the physical address of this
  array.

Session Lifecycle:

1. Creation: A userspace agent calls `luo_session_create()` to create a
   new, empty session and receives a file descriptor for it.
2. Serialization: When the reboot(LINUX\_REBOOT\_CMD\_KEXEC) syscall is
   made, `luo_session_serialize()` is called. It iterates through all
   active sessions and writes their metadata into a memory area preserved
   by KHO.
3. Deserialization (in new kernel): After kexec, `luo_session_deserialize()`
   runs, reading the serialized data and creating a list of `struct
   luo_session` objects representing the preserved sessions.
4. Retrieval: A userspace agent in the new kernel can then call
   `luo_session_retrieve()` with a session name to get a new file
   descriptor and access the preserved state.

## LUO Preserving File Descriptors

LUO provides the infrastructure to preserve specific, stateful file
descriptors across a kexec-based live update. The primary goal is to allow
workloads, such as virtual machines using vfio, memfd, or iommufd, to
retain access to their essential resources without interruption.

The framework is built around a callback-based handler model and a well-
defined lifecycle for each preserved file.

Handler Registration:
Kernel modules responsible for a specific file type (e.g., memfd, vfio)
register a [`struct liveupdate_file_handler`](#c.liveupdate_file_handler "liveupdate_file_handler"). This handler provides a set of
callbacks that LUO invokes at different stages of the update process, most
notably:

> * `can_preserve()`: A lightweight check to determine if the handler is
>   compatible with a given ‘[`struct file`](../filesystems/api-summary.html#c.file "file")’.
> * `preserve()`: The heavyweight operation that saves the file’s state and
>   returns an opaque u64 handle. This is typically performed while the
>   workload is still active to minimize the downtime during the
>   actual reboot transition.
> * `unpreserve()`: Cleans up any resources allocated by .`preserve()`, called
>   if the preservation process is aborted before the reboot (i.e. session is
>   closed).
> * `freeze()`: A final pre-reboot opportunity to prepare the state for kexec.
>   We are already in reboot syscall, and therefore userspace cannot mutate
>   the file anymore.
> * `unfreeze()`: Undoes the actions of .`freeze()`, called if the live update
>   is aborted after the freeze phase.
> * `retrieve()`: Reconstructs the file in the new kernel from the preserved
>   handle.
> * `finish()`: Performs final check and cleanup in the new kernel. After
>   succesul finish call, LUO gives up ownership to this file.

File Preservation Lifecycle happy path:

1. Preserve (Normal Operation): A userspace agent preserves files one by one
   via an ioctl. For each file, [`luo_preserve_file()`](#c.luo_preserve_file "luo_preserve_file") finds a compatible
   handler, calls its .`preserve()` operation, and creates an internal [`struct
   luo_file`](#c.luo_file "luo_file") to track the live state.
2. Freeze (Pre-Reboot): Just before the kexec, [`luo_file_freeze()`](#c.luo_file_freeze "luo_file_freeze") is called.
   It iterates through all preserved files, calls their respective .`freeze()`
   operation, and serializes their final metadata (compatible string, token,
   and data handle) into a contiguous memory block for KHO.
3. Deserialize: After kexec, [`luo_file_deserialize()`](#c.luo_file_deserialize "luo_file_deserialize") runs when session gets
   deserialized (which is when /dev/liveupdate is first opened). It reads the
   serialized data from the KHO memory region and reconstructs the in-memory
   list of [`struct luo_file`](#c.luo_file "luo_file") instances for the new kernel, linking them to
   their corresponding handlers.
4. Retrieve (New Kernel - Userspace Ready): The userspace agent can now
   restore file descriptors by providing a token. [`luo_retrieve_file()`](#c.luo_retrieve_file "luo_retrieve_file")
   searches for the matching token, calls the handler’s .`retrieve()` op to
   re-create the ‘[`struct file`](../filesystems/api-summary.html#c.file "file")’, and returns a new FD. Files can be
   retrieved in ANY order.
5. Finish (New Kernel - Cleanup): Once a session retrival is complete,
   [`luo_file_finish()`](#c.luo_file_finish "luo_file_finish") is called. It iterates through all files, invokes their
   .`finish()` operations for final cleanup, and releases all associated kernel
   resources.

File Preservation Lifecycle unhappy paths:

1. Abort Before Reboot: If the userspace agent aborts the live update
   process before calling reboot (e.g., by closing the session file
   descriptor), the session’s release handler calls
   [`luo_file_unpreserve_files()`](#c.luo_file_unpreserve_files "luo_file_unpreserve_files"). This invokes the .`unpreserve()` callback on
   all preserved files, ensuring all allocated resources are cleaned up and
   returning the system to a clean state.
2. Freeze Failure: During the `reboot()` syscall, if any handler’s .`freeze()`
   op fails, the .`unfreeze()` op is invoked on all previously *successful*
   freezes to roll back their state. The `reboot()` syscall then returns an
   error to userspace, canceling the live update.
3. Finish Failure: In the new kernel, if a handler’s .`finish()` op fails,
   the [`luo_file_finish()`](#c.luo_file_finish "luo_file_finish") operation is aborted. LUO retains ownership of
   all files within that session, including those that were not yet
   processed. The userspace agent can attempt to call the finish operation
   again later. If the issue cannot be resolved, these resources will be held
   by LUO until the next live update cycle, at which point they will be
   discarded.

## LUO File Lifecycle Bound Global Data

File-Lifecycle-Bound (FLB) objects provide a mechanism for managing global
state that is shared across multiple live-updatable files. The lifecycle of
this shared state is tied to the preservation of the files that depend on it.

An FLB represents a global resource, such as the IOMMU core state, that is
required by multiple file descriptors (e.g., all VFIO fds).

The preservation of the FLB’s state is triggered when the *first* file
depending on it is preserved. The cleanup of this state (unpreserve or
finish) is triggered when the *last* file depending on it is unpreserved or
finished.

Handler Dependency: A file handler declares its dependency on one or more
FLBs by registering them via [`liveupdate_register_flb()`](#c.liveupdate_register_flb "liveupdate_register_flb").

Callback Model: Each FLB is defined by a set of operations
([`struct liveupdate_flb_ops`](#c.liveupdate_flb_ops "liveupdate_flb_ops")) that LUO invokes at key points:

> * .`preserve()`: Called for the first file. Saves global state.
> * .`unpreserve()`: Called for the last file (if aborted pre-reboot).
> * .`retrieve()`: Called on-demand in the new kernel to restore the state.
> * .`finish()`: Called for the last file in the new kernel for cleanup.

This reference-counted approach ensures that shared state is saved exactly
once and restored exactly once, regardless of how many files depend on it,
and that its lifecycle is correctly managed across the kexec transition.

## Live Update Orchestrator ABI

Live Update Orchestrator uses the stable Application Binary Interface
defined below to pass state from a pre-update kernel to a post-update
kernel. The ABI is built upon the Kexec HandOver framework and uses a
Flattened Device Tree to describe the preserved data.

This interface is a contract. Any modification to the FDT structure, node
properties, compatible strings, or the layout of the \_\_packed serialization
structures defined here constitutes a breaking change. Such changes require
incrementing the version number in the relevant \_COMPATIBLE string to
prevent a new kernel from misinterpreting data from an old kernel.

Changes are allowed provided the compatibility version is incremented;
however, backward/forward compatibility is only guaranteed for kernels
supporting the same ABI version.

FDT Structure Overview:
:   The entire LUO state is encapsulated within a single KHO entry named “LUO”.
    This entry contains an FDT with the following layout:

    ```
    / {
        compatible = "luo-v1";
        liveupdate-number = <...>;

        luo-session {
            compatible = "luo-session-v1";
            luo-session-header = <phys_addr_of_session_header_ser>;
        };

        luo-flb {
            compatible = "luo-flb-v1";
            luo-flb-header = <phys_addr_of_flb_header_ser>;
        };
    };
    ```

Main LUO Node (/):

> * compatible: “luo-v1”
>   Identifies the overall LUO ABI version.
> * liveupdate-number: u64
>   A counter tracking the number of successful live updates performed.

Session Node (luo-session):
:   This node describes all preserved user-space sessions.

    * compatible: “luo-session-v1”
      Identifies the session ABI version.
    * luo-session-header: u64
      The physical address of a [`struct luo_session_header_ser`](#c.luo_session_header_ser "luo_session_header_ser"). This structure
      is the header for a contiguous block of memory containing an array of
      [`struct luo_session_ser`](#c.luo_session_ser "luo_session_ser"), one for each preserved session.

File-Lifecycle-Bound Node (luo-flb):
:   This node describes all preserved global objects whose lifecycle is bound
    to that of the preserved files (e.g., shared IOMMU state).

    * compatible: “luo-flb-v1”
      Identifies the FLB ABI version.
    * luo-flb-header: u64
      The physical address of a [`struct luo_flb_header_ser`](#c.luo_flb_header_ser "luo_flb_header_ser"). This structure is
      the header for a contiguous block of memory containing an array of
      [`struct luo_flb_ser`](#c.luo_flb_ser "luo_flb_ser"), one for each preserved global object.

Serialization Structures:
:   The FDT properties point to memory regions containing arrays of simple,
    \_\_packed structures. These structures contain the actual preserved state.

    * [`struct luo_session_header_ser`](#c.luo_session_header_ser "luo_session_header_ser"):
      Header for the session array. Contains the total page count of the
      preserved memory block and the number of [`struct luo_session_ser`](#c.luo_session_ser "luo_session_ser")
      entries that follow.
    * [`struct luo_session_ser`](#c.luo_session_ser "luo_session_ser"):
      Metadata for a single session, including its name and a physical pointer
      to another preserved memory block containing an array of
      [`struct luo_file_ser`](#c.luo_file_ser "luo_file_ser") for all files in that session.
    * [`struct luo_file_ser`](#c.luo_file_ser "luo_file_ser"):
      Metadata for a single preserved file. Contains the compatible string to
      find the correct handler in the new kernel, a user-provided token for
      identification, and an opaque data handle for the handler to use.
    * [`struct luo_flb_header_ser`](#c.luo_flb_header_ser "luo_flb_header_ser"):
      Header for the FLB array. Contains the total page count of the
      preserved memory block and the number of [`struct luo_flb_ser`](#c.luo_flb_ser "luo_flb_ser") entries
      that follow.
    * [`struct luo_flb_ser`](#c.luo_flb_ser "luo_flb_ser"):
      Metadata for a single preserved global object. Contains its name
      (compatible string), an opaque data handle, and the count
      number of files depending on it.

The following types of file descriptors can be preserved

* [Memfd Preservation via LUO](../mm/memfd_preservation.html)

## Public API

struct liveupdate\_file\_op\_args
:   Arguments for file operation callbacks.

**Definition**:

```
struct liveupdate_file_op_args {
    struct liveupdate_file_handler *handler;
    int retrieve_status;
    struct file *file;
    u64 serialized_data;
    void *private_data;
};
```

**Members**

`handler`
:   The file handler being called.

`retrieve_status`
:   The retrieve status for the ‘can\_finish / finish’
    operation. A value of 0 means the retrieve has not been
    attempted, a positive value means the retrieve was
    successful, and a negative value means the retrieve failed,
    and the value is the error code of the call.

`file`
:   The file object. For retrieve: [OUT] The callback sets
    this to the new file. For other ops: [IN] The caller sets
    this to the file being operated on.

`serialized_data`
:   The opaque u64 handle, preserve/prepare/freeze may update
    this field.

`private_data`
:   Private data for the file used to hold runtime state that
    is not preserved. Set by the handler’s .`preserve()`
    callback, and must be freed in the handler’s
    .`unpreserve()` callback.

**Description**

This structure bundles all parameters for the file operation callbacks.
The ‘data’ and ‘file’ fields are used for both input and output.

struct liveupdate\_file\_ops
:   Callbacks for live-updatable files.

**Definition**:

```
struct liveupdate_file_ops {
    bool (*can_preserve)(struct liveupdate_file_handler *handler, struct file *file);
    int (*preserve)(struct liveupdate_file_op_args *args);
    void (*unpreserve)(struct liveupdate_file_op_args *args);
    int (*freeze)(struct liveupdate_file_op_args *args);
    void (*unfreeze)(struct liveupdate_file_op_args *args);
    int (*retrieve)(struct liveupdate_file_op_args *args);
    bool (*can_finish)(struct liveupdate_file_op_args *args);
    void (*finish)(struct liveupdate_file_op_args *args);
    unsigned long (*get_id)(struct file *file);
    struct module *owner;
};
```

**Members**

`can_preserve`
:   Required. Lightweight check to see if this handler is
    compatible with the given file.

`preserve`
:   Required. Performs state-saving for the file.

`unpreserve`
:   Required. Cleans up any resources allocated by **preserve**.

`freeze`
:   Optional. Final actions just before kernel transition.

`unfreeze`
:   Optional. Undo freeze operations.

`retrieve`
:   Required. Restores the file in the new kernel.

`can_finish`
:   Optional. Check if this FD can finish, i.e. all restoration
    pre-requirements for this FD are satisfied. Called prior to
    finish, in order to do successful finish calls for all
    resources in the session.

`finish`
:   Required. Final cleanup in the new kernel.

`get_id`
:   Optional. Returns a unique identifier for the file.

`owner`
:   Module reference

**Description**

All operations (except can\_preserve) receive a pointer to a
‘[`struct liveupdate_file_op_args`](#c.liveupdate_file_op_args "liveupdate_file_op_args")’ containing the necessary context.

struct liveupdate\_file\_handler
:   Represents a handler for a live-updatable file type.

**Definition**:

```
struct liveupdate_file_handler {
    const struct liveupdate_file_ops *ops;
    const char compatible[LIVEUPDATE_HNDL_COMPAT_LENGTH];
};
```

**Members**

`ops`
:   Callback functions

`compatible`
:   The compatibility string (e.g., “memfd-v1”, “vfiofd-v1”)
    that uniquely identifies the file type this handler
    supports. This is matched against the compatible string
    associated with individual [`struct file`](../filesystems/api-summary.html#c.file "file") instances.

**Description**

Modules that want to support live update for specific file types should
register an instance of this structure. LUO uses this registration to
determine if a given file can be preserved and to find the appropriate
operations to manage its state across the update.

struct liveupdate\_flb\_op\_args
:   Arguments for FLB operation callbacks.

**Definition**:

```
struct liveupdate_flb_op_args {
    struct liveupdate_flb *flb;
    u64 data;
    void *obj;
};
```

**Members**

`flb`
:   The global FLB instance for which this call is performed.

`data`
:   For .`preserve()`: [OUT] The callback sets this field.
    For .`unpreserve()`: [IN] The handle from .`preserve()`.
    For .`retrieve()`: [IN] The handle from .`preserve()`.

`obj`
:   For .`preserve()`: [OUT] Sets this to the live object.
    For .`retrieve()`: [OUT] Sets this to the live object.
    For .`finish()`: [IN] The live object from .`retrieve()`.

**Description**

This structure bundles all parameters for the FLB operation callbacks.

struct liveupdate\_flb\_ops
:   Callbacks for global File-Lifecycle-Bound data.

**Definition**:

```
struct liveupdate_flb_ops {
    int (*preserve)(struct liveupdate_flb_op_args *argp);
    void (*unpreserve)(struct liveupdate_flb_op_args *argp);
    int (*retrieve)(struct liveupdate_flb_op_args *argp);
    void (*finish)(struct liveupdate_flb_op_args *argp);
    struct module *owner;
};
```

**Members**

`preserve`
:   Called when the first file using this FLB is preserved.
    The callback must save its state and return a single,
    self-contained u64 handle by setting the ‘argp->data’
    field and ‘argp->obj’.

`unpreserve`
:   Called when the last file using this FLB is unpreserved
    (aborted before reboot). Receives the handle via
    ‘argp->data’ and live object via ‘argp->obj’.

`retrieve`
:   Called on-demand in the new kernel, the first time a
    component requests access to the shared object. It receives
    the preserved handle via ‘argp->data’ and must reconstruct
    the live object, returning it by setting the ‘argp->obj’
    field.

`finish`
:   Called in the new kernel when the last file using this FLB
    is finished. Receives the live object via ‘argp->obj’ for
    cleanup.

`owner`
:   Module reference

**Description**

Operations that manage global shared data with file bound lifecycle,
triggered by the first file that uses it and concluded by the last file that
uses it, across all sessions.

struct liveupdate\_flb
:   A global definition for a shared data object.

**Definition**:

```
struct liveupdate_flb {
    const struct liveupdate_flb_ops *ops;
    const char compatible[LIVEUPDATE_FLB_COMPAT_LENGTH];
};
```

**Members**

`ops`
:   Callback functions

`compatible`
:   The compatibility string (e.g., “iommu-core-v1”
    that uniquely identifies the FLB type this handler
    supports. This is matched against the compatible string
    associated with individual [`struct liveupdate_flb`](#c.liveupdate_flb "liveupdate_flb")
    instances.

**Description**

This `struct is` the “template” that a driver registers to define a shared,
file-lifecycle-bound object. The actual runtime state (the live object,
refcount, etc.) is managed privately by the LUO core.

struct luo\_file\_ser
:   Represents the serialized preserves files.

**Definition**:

```
struct luo_file_ser {
    char compatible[LIVEUPDATE_HNDL_COMPAT_LENGTH];
    u64 data;
    u64 token;
};
```

**Members**

`compatible`
:   File handler compatible string.

`data`
:   Private data

`token`
:   User provided token for this file

**Description**

If this structure is modified, LUO\_SESSION\_COMPATIBLE must be updated.

struct luo\_file\_set\_ser
:   Represents the serialized metadata for file set

**Definition**:

```
struct luo_file_set_ser {
    u64 files;
    u64 count;
};
```

**Members**

`files`
:   The physical address of a contiguous memory block that holds
    the serialized state of files (array of luo\_file\_ser) in this file
    set.

`count`
:   The total number of files that were part of this session during
    serialization. Used for iteration and validation during
    restoration.

struct luo\_session\_header\_ser
:   Header for the serialized session data block.

**Definition**:

```
struct luo_session_header_ser {
    u64 count;
};
```

**Members**

`count`
:   The number of [`struct luo_session_ser`](#c.luo_session_ser "luo_session_ser") entries that immediately
    follow this header in the memory block.

**Description**

This structure is located at the beginning of a contiguous block of
physical memory preserved across the kexec. It provides the necessary
metadata to interpret the array of session entries that follow.

If this structure is modified, LUO\_FDT\_SESSION\_COMPATIBLE must be updated.

struct luo\_session\_ser
:   Represents the serialized metadata for a LUO session.

**Definition**:

```
struct luo_session_ser {
    char name[LIVEUPDATE_SESSION_NAME_LENGTH];
    struct luo_file_set_ser file_set_ser;
};
```

**Members**

`name`
:   The unique name of the session, provided by the userspace at
    the time of session creation.

`file_set_ser`
:   Serialized files belonging to this session,

**Description**

This structure is used to package session-specific metadata for transfer
between kernels via Kexec Handover. An array of these structures (one per
session) is created and passed to the new kernel, allowing it to reconstruct
the session context.

If this structure is modified, LUO\_FDT\_SESSION\_COMPATIBLE must be updated.

struct luo\_flb\_header\_ser
:   Header for the serialized FLB data block.

**Definition**:

```
struct luo_flb_header_ser {
    u64 pgcnt;
    u64 count;
};
```

**Members**

`pgcnt`
:   The total number of pages occupied by the entire preserved memory
    region, including this header and the subsequent array of
    [`struct luo_flb_ser`](#c.luo_flb_ser "luo_flb_ser") entries.

`count`
:   The number of [`struct luo_flb_ser`](#c.luo_flb_ser "luo_flb_ser") entries that follow this header
    in the memory block.

**Description**

This structure is located at the physical address specified by the
LUO\_FDT\_FLB\_HEADER FDT property. It provides the new kernel with the
necessary information to find and iterate over the array of preserved
File-Lifecycle-Bound objects and to manage the underlying memory.

If this structure is modified, LUO\_FDT\_FLB\_COMPATIBLE must be updated.

struct luo\_flb\_ser
:   Represents the serialized state of a single FLB object.

**Definition**:

```
struct luo_flb_ser {
    char name[LIVEUPDATE_FLB_COMPAT_LENGTH];
    u64 data;
    u64 count;
};
```

**Members**

`name`
:   The unique compatibility string of the FLB object, used to find the
    corresponding [`struct liveupdate_flb`](#c.liveupdate_flb "liveupdate_flb") handler in the new kernel.

`data`
:   The opaque u64 handle returned by the FLB’s .`preserve()` operation
    in the old kernel. This handle encapsulates the entire state needed
    for restoration.

`count`
:   The reference count at the time of serialization; i.e., the number
    of preserved files that depended on this FLB. This is used by the
    new kernel to correctly manage the FLB’s lifecycle.

**Description**

An array of these structures is created in a preserved memory region and
passed to the new kernel. Each entry allows the LUO core to restore one
global, shared object.

If this structure is modified, LUO\_FDT\_FLB\_COMPATIBLE must be updated.

## Internal API

int liveupdate\_reboot(void)
:   Kernel reboot notifier for live update final serialization.

**Parameters**

`void`
:   no arguments

**Description**

This function is invoked directly from the `reboot()` syscall pathway
if kexec is in progress.

If any callback fails, this function aborts KHO, undoes the `freeze()`
callbacks, and returns an error.

bool liveupdate\_enabled(void)
:   Check if the live update feature is enabled.

**Parameters**

`void`
:   no arguments

**Description**

This function returns the state of the live update feature flag, which
can be controlled via the `liveupdate` kernel command-line parameter.

**return** true if live update is enabled, false otherwise.

int luo\_flb\_file\_preserve(struct [liveupdate\_file\_handler](#c.liveupdate_file_handler "liveupdate_file_handler") \*fh)
:   Notifies FLBs that a file is about to be preserved.

**Parameters**

`struct liveupdate_file_handler *fh`
:   The file handler for the preserved file.

**Description**

This function iterates through all FLBs associated with the given file
handler. It increments the reference count for each FLB. If the count becomes
1, it triggers the FLB’s .`preserve()` callback to save the global state.

This operation is atomic. If any FLB’s .`preserve()` op fails, it will roll
back by calling .`unpreserve()` on any FLBs that were successfully preserved
during this call.

**Context**

Called from [`luo_preserve_file()`](#c.luo_preserve_file "luo_preserve_file")

**Return**

0 on success, or a negative errno on failure.

void luo\_flb\_file\_unpreserve(struct [liveupdate\_file\_handler](#c.liveupdate_file_handler "liveupdate_file_handler") \*fh)
:   Notifies FLBs that a dependent file was unpreserved.

**Parameters**

`struct liveupdate_file_handler *fh`
:   The file handler for the unpreserved file.

**Description**

This function iterates through all FLBs associated with the given file
handler, in reverse order of registration. It decrements the reference count
for each FLB. If the count becomes 0, it triggers the FLB’s .`unpreserve()`
callback to clean up the global state.

**Context**

Called when a preserved file is being cleaned up before reboot
(e.g., from [`luo_file_unpreserve_files()`](#c.luo_file_unpreserve_files "luo_file_unpreserve_files")).

void luo\_flb\_file\_finish(struct [liveupdate\_file\_handler](#c.liveupdate_file_handler "liveupdate_file_handler") \*fh)
:   Notifies FLBs that a dependent file has been finished.

**Parameters**

`struct liveupdate_file_handler *fh`
:   The file handler for the finished file.

**Description**

This function iterates through all FLBs associated with the given file
handler, in reverse order of registration. It decrements the incoming
reference count for each FLB. If the count becomes 0, it triggers the FLB’s
.`finish()` callback for final cleanup in the new kernel.

**Context**

Called from [`luo_file_finish()`](#c.luo_file_finish "luo_file_finish") for each file being finished.

void luo\_flb\_unregister\_all(struct [liveupdate\_file\_handler](#c.liveupdate_file_handler "liveupdate_file_handler") \*fh)
:   Unregister all FLBs associated with a file handler.

**Parameters**

`struct liveupdate_file_handler *fh`
:   The file handler whose FLBs should be unregistered.

**Description**

This function iterates through the list of FLBs associated with the given
file handler and unregisters them all one by one.

int liveupdate\_register\_flb(struct [liveupdate\_file\_handler](#c.liveupdate_file_handler "liveupdate_file_handler") \*fh, struct [liveupdate\_flb](#c.liveupdate_flb "liveupdate_flb") \*flb)
:   Associate an FLB with a file handler and register it globally.

**Parameters**

`struct liveupdate_file_handler *fh`
:   The file handler that will now depend on the FLB.

`struct liveupdate_flb *flb`
:   The File-Lifecycle-Bound object to associate.

**Description**

Establishes a dependency, informing the LUO core that whenever a file of
type **fh** is preserved, the state of **flb** must also be managed.

On the first registration of a given **flb** object, it is added to a global
registry. This function checks for duplicate registrations, both for a
specific handler and globally, and ensures the total number of unique
FLBs does not exceed the system limit.

**Context**

Typically called from a subsystem’s module init function after
both the handler and the FLB have been defined and initialized.

**Return**

0 on success. Returns a negative errno on failure:
-EINVAL if arguments are NULL or not initialized.
-ENOMEM on memory allocation failure.
-EEXIST if this FLB is already registered with this handler.
-ENOSPC if the maximum number of global FLBs has been reached.
-EOPNOTSUPP if live update is disabled or not configured.

void liveupdate\_unregister\_flb(struct [liveupdate\_file\_handler](#c.liveupdate_file_handler "liveupdate_file_handler") \*fh, struct [liveupdate\_flb](#c.liveupdate_flb "liveupdate_flb") \*flb)
:   Remove an FLB dependency from a file handler.

**Parameters**

`struct liveupdate_file_handler *fh`
:   The file handler that is currently depending on the FLB.

`struct liveupdate_flb *flb`
:   The File-Lifecycle-Bound object to remove.

**Description**

Removes the association between the specified file handler and the FLB
previously established by [`liveupdate_register_flb()`](#c.liveupdate_register_flb "liveupdate_register_flb").

This function manages the global lifecycle of the FLB. It decrements the
FLB’s usage count. If this was the last file handler referencing this FLB,
the FLB is removed from the global registry and the reference to its
owner module (acquired during registration) is released.

**Context**

It is typically called from a subsystem’s module exit function.

int liveupdate\_flb\_get\_incoming(struct [liveupdate\_flb](#c.liveupdate_flb "liveupdate_flb") \*flb, void \*\*objp)
:   Retrieve the incoming FLB object.

**Parameters**

`struct liveupdate_flb *flb`
:   The FLB definition.

`void **objp`
:   Output parameter; will be populated with the live shared object.

**Description**

Returns a pointer to its shared live object for the incoming (post-reboot)
path.

If this is the first time the object is requested in the new kernel, this
function will trigger the FLB’s .`retrieve()` callback to reconstruct the
object from its preserved state. Subsequent calls will return the same
cached object.

**Return**

0 on success, or a negative errno on failure. -ENODATA means no
incoming FLB data, -ENOENT means specific flb not found in the incoming
data, -ENODEV if the FLB’s module is unloading, and -EOPNOTSUPP when
live update is disabled or not configured.

int liveupdate\_flb\_get\_outgoing(struct [liveupdate\_flb](#c.liveupdate_flb "liveupdate_flb") \*flb, void \*\*objp)
:   Retrieve the outgoing FLB object.

**Parameters**

`struct liveupdate_flb *flb`
:   The FLB definition.

`void **objp`
:   Output parameter; will be populated with the live shared object.

**Description**

Returns a pointer to its shared live object for the outgoing (pre-reboot)
path.

This function assumes the object has already been created by the FLB’s
.`preserve()` callback, which is triggered when the first dependent file
is preserved.

**Return**

0 on success, or a negative errno on failure.

void luo\_flb\_serialize(void)
:   Serializes all active FLB objects for KHO.

**Parameters**

`void`
:   no arguments

**Description**

This function is called from the reboot path. It iterates through all
registered File-Lifecycle-Bound (FLB) objects. For each FLB that has been
preserved (i.e., its reference count is greater than zero), it writes its
metadata into the memory region designated for Kexec Handover.

The serialized data includes the FLB’s compatibility string, its opaque
data handle, and the final reference count. This allows the new kernel to
find the appropriate handler and reconstruct the FLB’s state.

**Context**

Called from [`liveupdate_reboot()`](#c.liveupdate_reboot "liveupdate_reboot") just before `kho_finalize()`.

struct luo\_session\_header
:   Header struct for managing LUO sessions.

**Definition**:

```
struct luo_session_header {
    long count;
    struct list_head list;
    struct rw_semaphore rwsem;
    struct luo_session_header_ser *header_ser;
    struct luo_session_ser *ser;
    bool active;
};
```

**Members**

`count`
:   The number of sessions currently tracked in the **list**.

`list`
:   The head of the linked list of `struct luo_session` instances.

`rwsem`
:   A read-write semaphore providing synchronized access to the
    session list and other fields in this structure.

`header_ser`
:   The header data of serialization array.

`ser`
:   The serialized session data (an array of
    [`struct luo_session_ser`](#c.luo_session_ser "luo_session_ser")).

`active`
:   Set to true when first initialized. If previous kernel did not
    send session data, active stays false for incoming.

struct luo\_session\_global
:   Global container for managing LUO sessions.

**Definition**:

```
struct luo_session_global {
    struct luo_session_header incoming;
    struct luo_session_header outgoing;
};
```

**Members**

`incoming`
:   The sessions passed from the previous kernel.

`outgoing`
:   The sessions that are going to be passed to the next kernel.

struct luo\_file
:   Represents a single preserved file instance.

**Definition**:

```
struct luo_file {
    struct liveupdate_file_handler *fh;
    struct file *file;
    u64 serialized_data;
    void *private_data;
    int retrieve_status;
    struct mutex mutex;
    struct list_head list;
    u64 token;
};
```

**Members**

`fh`
:   Pointer to the [`struct liveupdate_file_handler`](#c.liveupdate_file_handler "liveupdate_file_handler") that manages
    this type of file.

`file`
:   Pointer to the kernel’s [`struct file`](../filesystems/api-summary.html#c.file "file") that is being preserved.
    This is NULL in the new kernel until the file is successfully
    retrieved.

`serialized_data`
:   The opaque u64 handle to the serialized state of the file.
    This handle is passed back to the handler’s .`freeze()`,
    .`retrieve()`, and .`finish()` callbacks, allowing it to track
    and update its serialized state across phases.

`private_data`
:   Pointer to the private data for the file used to hold runtime
    state that is not preserved. Set by the handler’s .`preserve()`
    callback, and must be freed in the handler’s .`unpreserve()`
    callback.

`retrieve_status`
:   Status code indicating whether a user/kernel in the new kernel has
    successfully called `retrieve()` on this file. This prevents
    multiple retrieval attempts. A value of 0 means a `retrieve()`
    has not been attempted, a positive value means the `retrieve()`
    was successful, and a negative value means the `retrieve()`
    failed, and the value is the error code of the call.

`mutex`
:   A mutex that protects the fields of this specific instance
    (e.g., **retrieved**, **file**), ensuring that operations like
    retrieving or finishing a file are atomic.

`list`
:   The list\_head linking this instance into its parent
    file\_set’s list of preserved files.

`token`
:   The user-provided unique token used to identify this file.

**Description**

This structure is the core in-kernel representation of a single file being
managed through a live update. An instance is created by [`luo_preserve_file()`](#c.luo_preserve_file "luo_preserve_file")
to link a ‘[`struct file`](../filesystems/api-summary.html#c.file "file")’ to its corresponding handler, a user-provided token,
and the serialized state handle returned by the handler’s .`preserve()`
operation.

These instances are tracked in a per-file\_set list. The **serialized\_data**
field, which holds a handle to the file’s serialized state, may be updated
during the .`freeze()` callback before being serialized for the next kernel.
After reboot, these structures are recreated by [`luo_file_deserialize()`](#c.luo_file_deserialize "luo_file_deserialize") and
are finally cleaned up by [`luo_file_finish()`](#c.luo_file_finish "luo_file_finish").

int luo\_preserve\_file(struct luo\_file\_set \*file\_set, u64 token, int fd)
:   Initiate the preservation of a file descriptor.

**Parameters**

`struct luo_file_set *file_set`
:   The file\_set to which the preserved file will be added.

`u64 token`
:   A unique, user-provided identifier for the file.

`int fd`
:   The file descriptor to be preserved.

**Description**

This function orchestrates the first phase of preserving a file. Upon entry,
it takes a reference to the ‘[`struct file`](../filesystems/api-summary.html#c.file "file")’ via `fget()`, effectively making LUO
a co-owner of the file. This reference is held until the file is either
unpreserved or successfully finished in the next kernel, preventing the file
from being prematurely destroyed.

This function orchestrates the first phase of preserving a file. It performs
the following steps:

1. Validates that the **token** is not already in use within the file\_set.
2. Ensures the file\_set’s memory for files serialization is allocated
   (allocates if needed).
3. Iterates through registered handlers, calling `can_preserve()` to find one
   compatible with the given **fd**.
4. Calls the handler’s .`preserve()` operation, which saves the file’s state
   and returns an opaque private data handle.
5. Adds the new instance to the file\_set’s internal list.

On success, LUO takes a reference to the ‘[`struct file`](../filesystems/api-summary.html#c.file "file")’ and considers it
under its management until it is unpreserved or finished.

In case of any failure, all intermediate allocations (file reference, memory
for the ‘luo\_file’ struct, etc.) are cleaned up before returning an error.

**Context**

Can be called from an ioctl handler during normal system operation.

**Return**

0 on success. Returns a negative errno on failure:
-EEXIST if the token is already used.
-EBUSY if the file descriptor is already preserved by another session.
-EBADF if the file descriptor is invalid.
-ENOSPC if the file\_set is full.
-ENOENT if no compatible handler is found.
-ENOMEM on memory allocation failure.
Other erros might be returned by .`preserve()`.

void luo\_file\_unpreserve\_files(struct luo\_file\_set \*file\_set)
:   Unpreserves all files from a file\_set.

**Parameters**

`struct luo_file_set *file_set`
:   The files to be cleaned up.

**Description**

This function serves as the primary cleanup path for a file\_set. It is
invoked when the userspace agent closes the file\_set’s file descriptor.

For each file, it performs the following cleanup actions:
:   1. Calls the handler’s .`unpreserve()` callback to allow the handler to
       release any resources it allocated.
    2. Removes the file from the file\_set’s internal tracking list.
    3. Releases the reference to the ‘[`struct file`](../filesystems/api-summary.html#c.file "file")’ that was taken by
       [`luo_preserve_file()`](#c.luo_preserve_file "luo_preserve_file") via `fput()`, returning ownership.
    4. Frees the memory associated with the internal ‘[`struct luo_file`](#c.luo_file "luo_file")’.

After all individual files are unpreserved, it frees the contiguous memory
block that was allocated to hold their serialization data.

int luo\_file\_freeze(struct luo\_file\_set \*file\_set, struct [luo\_file\_set\_ser](#c.luo_file_set_ser "luo_file_set_ser") \*file\_set\_ser)
:   Freezes all preserved files and serializes their metadata.

**Parameters**

`struct luo_file_set *file_set`
:   The file\_set whose files are to be frozen.

`struct luo_file_set_ser *file_set_ser`
:   Where to put the serialized file\_set.

**Description**

This function is called from the `reboot()` syscall path, just before the
kernel transitions to the new image via kexec. Its purpose is to perform the
final preparation and serialization of all preserved files in the file\_set.

It iterates through each preserved file in FIFO order (the order of
preservation) and performs two main actions:

1. Freezes the File: It calls the handler’s .`freeze()` callback for each
   file. This gives the handler a final opportunity to quiesce the device or
   prepare its state for the upcoming reboot. The handler may update its
   private data handle during this step.
2. Serializes Metadata: After a successful freeze, it copies the final file
   metadata—the handler’s compatible string, the user token, and the final
   private data handle—into the pre-allocated contiguous memory buffer
   (file\_set->files) that will be handed over to the next kernel via KHO.

Error Handling (Rollback):
This function is atomic. If any handler’s .`freeze()` operation fails, the
entire live update is aborted. The `__luo_file_unfreeze()` helper is
immediately called to invoke the .`unfreeze()` op on all files that were
successfully frozen before the point of failure, rolling them back to a
running state. The function then returns an error, causing the `reboot()`
syscall to fail.

**Context**

Called only from the [`liveupdate_reboot()`](#c.liveupdate_reboot "liveupdate_reboot") path.

**Return**

0 on success, or a negative errno on failure.

void luo\_file\_unfreeze(struct luo\_file\_set \*file\_set, struct [luo\_file\_set\_ser](#c.luo_file_set_ser "luo_file_set_ser") \*file\_set\_ser)
:   Unfreezes all files in a file\_set and clear serialization

**Parameters**

`struct luo_file_set *file_set`
:   The file\_set whose files are to be unfrozen.

`struct luo_file_set_ser *file_set_ser`
:   Serialized file\_set.

**Description**

This function rolls back the state of all files in a file\_set after the
freeze phase has begun but must be aborted. It is the counterpart to
[`luo_file_freeze()`](#c.luo_file_freeze "luo_file_freeze").

It invokes the `__luo_file_unfreeze()` helper with a NULL argument, which
signals the helper to iterate through all files in the file\_set and call
their respective .`unfreeze()` handler callbacks.

**Context**

This is called when the live update is aborted during
the `reboot()` syscall, after [`luo_file_freeze()`](#c.luo_file_freeze "luo_file_freeze") has been called.

int luo\_retrieve\_file(struct luo\_file\_set \*file\_set, u64 token, struct [file](../filesystems/api-summary.html#c.file "file") \*\*filep)
:   Restores a preserved file from a file\_set by its token.

**Parameters**

`struct luo_file_set *file_set`
:   The file\_set from which to retrieve the file.

`u64 token`
:   The unique token identifying the file to be restored.

`struct file **filep`
:   Output parameter; on success, this is populated with a pointer
    to the newly retrieved ‘[`struct file`](../filesystems/api-summary.html#c.file "file")’.

**Description**

This function is the primary mechanism for recreating a file in the new
kernel after a live update. It searches the file\_set’s list of deserialized
files for an entry matching the provided **token**.

The operation is idempotent: if a file has already been successfully
retrieved, this function will simply return a pointer to the existing
‘[`struct file`](../filesystems/api-summary.html#c.file "file")’ and report success without re-executing the retrieve
operation. This is handled by checking the ‘retrieved’ flag under a lock.

File retrieval can happen in any order; it is not bound by the order of
preservation.

**Context**

Can be called from an ioctl or other in-kernel code in the new
kernel.

**Return**

0 on success. Returns a negative errno on failure:
-ENOENT if no file with the matching token is found.
Any error code returned by the handler’s .`retrieve()` op.

int luo\_file\_finish(struct luo\_file\_set \*file\_set)
:   Completes the lifecycle for all files in a file\_set.

**Parameters**

`struct luo_file_set *file_set`
:   The file\_set to be finalized.

**Description**

This function orchestrates the final teardown of a live update file\_set in
the new kernel. It should be called after all necessary files have been
retrieved and the userspace agent is ready to release the preserved state.

The function iterates through all tracked files. For each file, it performs
the following sequence of cleanup actions:

1. If file is not yet retrieved, retrieves it, and calls `can_finish()` on
   every file in the file\_set. If all can\_finish return true, continue to
   finish.
2. Calls the handler’s .`finish()` callback (via luo\_file\_finish\_one) to
   allow for final resource cleanup within the handler.
3. Releases LUO’s ownership reference on the ‘[`struct file`](../filesystems/api-summary.html#c.file "file")’ via `fput()`. This
   is the counterpart to the `get_file()` call in [`luo_retrieve_file()`](#c.luo_retrieve_file "luo_retrieve_file").
4. Removes the ‘[`struct luo_file`](#c.luo_file "luo_file")’ from the file\_set’s internal list.
5. Frees the memory for the ‘[`struct luo_file`](#c.luo_file "luo_file")’ instance itself.

After successfully finishing all individual files, it frees the
contiguous memory block that was used to transfer the serialized metadata
from the previous kernel.

Error Handling (Atomic Failure):
This operation is atomic. If any handler’s .`can_finish()` op fails, the entire
function aborts immediately and returns an error.

**Context**

Can be called from an ioctl handler in the new kernel.

**Return**

0 on success, or a negative errno on failure.

int luo\_file\_deserialize(struct luo\_file\_set \*file\_set, struct [luo\_file\_set\_ser](#c.luo_file_set_ser "luo_file_set_ser") \*file\_set\_ser)
:   Reconstructs the list of preserved files in the new kernel.

**Parameters**

`struct luo_file_set *file_set`
:   The incoming file\_set to fill with deserialized data.

`struct luo_file_set_ser *file_set_ser`
:   Serialized KHO file\_set data from the previous kernel.

**Description**

This function is called during the early boot process of the new kernel. It
takes the raw, contiguous memory block of ‘[`struct luo_file_ser`](#c.luo_file_ser "luo_file_ser")’ entries,
provided by the previous kernel, and transforms it back into a live,
in-memory linked list of ‘[`struct luo_file`](#c.luo_file "luo_file")’ instances.

For each serialized entry, it performs the following steps:
:   1. Reads the ‘compatible’ string.
    2. Searches the global list of registered file handlers for one that
       matches the compatible string.
    3. Allocates a new ‘[`struct luo_file`](#c.luo_file "luo_file")’.
    4. Populates the new structure with the deserialized data (token, private
       data handle) and links it to the found handler. The ‘file’ pointer is
       initialized to NULL, as the file has not been retrieved yet.
    5. Adds the new ‘[`struct luo_file`](#c.luo_file "luo_file")’ to the file\_set’s files\_list.

This prepares the file\_set for userspace, which can later call
[`luo_retrieve_file()`](#c.luo_retrieve_file "luo_retrieve_file") to restore the actual file descriptors.

**Context**

Called from session deserialization.

int liveupdate\_register\_file\_handler(struct [liveupdate\_file\_handler](#c.liveupdate_file_handler "liveupdate_file_handler") \*fh)
:   Register a file handler with LUO.

**Parameters**

`struct liveupdate_file_handler *fh`
:   Pointer to a caller-allocated [`struct liveupdate_file_handler`](#c.liveupdate_file_handler "liveupdate_file_handler").
    The caller must initialize this structure, including a unique
    ‘compatible’ string and a valid ‘fh’ callbacks. This function adds the
    handler to the global list of supported file handlers.

**Context**

Typically called during module initialization for file types that
support live update preservation.

**Return**

0 on success. Negative errno on failure.

void liveupdate\_unregister\_file\_handler(struct [liveupdate\_file\_handler](#c.liveupdate_file_handler "liveupdate_file_handler") \*fh)
:   Unregister a liveupdate file handler

**Parameters**

`struct liveupdate_file_handler *fh`
:   The file handler to unregister

**Description**

Unregisters the file handler from the liveupdate core. This function
reverses the operations of [`liveupdate_register_file_handler()`](#c.liveupdate_register_file_handler "liveupdate_register_file_handler").

## See Also

* [Live Update uAPI](../userspace-api/liveupdate.html)
* [Kexec Handover Subsystem](kho/index.html)
