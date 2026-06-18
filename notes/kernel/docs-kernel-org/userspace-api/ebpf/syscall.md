# eBPF Syscall

> 출처(원문): https://docs.kernel.org/userspace-api/ebpf/syscall.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# eBPF Syscall

Authors:
:   * Alexei Starovoitov <[ast@kernel.org](mailto:ast%40kernel.org)>
    * Joe Stringer <[joe@wand.net.nz](mailto:joe%40wand.net.nz)>
    * Michael Kerrisk <[mtk.manpages@gmail.com](mailto:mtk.manpages%40gmail.com)>

The primary info for the bpf syscall is available in the [man-pages](https://www.kernel.org/doc/man-pages/)
for [bpf(2)](https://man7.org/linux/man-pages/man2/bpf.2.html).

## bpf() subcommand reference

The operation to be performed by the **bpf**() system call is determined
by the *cmd* argument. Each operation takes an accompanying argument,
provided via *attr*, which is a pointer to a `union of` type *bpf\_attr* (see
below). The size argument is the size of the `union pointed` to by *attr*.

BPF\_MAP\_CREATE
:   Description
    :   Create a map and return a file descriptor that refers to the
        map. The close-on-exec file descriptor flag (see **fcntl**(2))
        is automatically enabled for the new file descriptor.

        Applying **close**(2) to the file descriptor returned by
        **BPF\_MAP\_CREATE** will delete the map (but see NOTES).

    Return
    :   A new file descriptor (a nonnegative integer), or -1 if an
        error occurred (in which case, *errno* is set appropriately).

BPF\_MAP\_LOOKUP\_ELEM
:   Description
    :   Look up an element with a given *key* in the map referred to
        by the file descriptor *map\_fd*.

        The *flags* argument may be specified as one of the
        following:

        **BPF\_F\_LOCK**
        :   Look up the value of a spin-locked map without
            returning the lock. This must be specified if the
            elements contain a spinlock.

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

BPF\_MAP\_UPDATE\_ELEM
:   Description
    :   Create or update an element (key/value pair) in a specified map.

        The *flags* argument should be specified as one of the
        following:

        **BPF\_ANY**
        :   Create a new element or update an existing element.

        **BPF\_NOEXIST**
        :   Create a new element only if it did not exist.

        **BPF\_EXIST**
        :   Update an existing element.

        **BPF\_F\_LOCK**
        :   Update a spin\_lock-ed map element.

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

        May set *errno* to **EINVAL**, **EPERM**, **ENOMEM**,
        **E2BIG**, **EEXIST**, or **ENOENT**.

        **E2BIG**
        :   The number of elements in the map reached the
            *max\_entries* limit specified at map creation time.

        **EEXIST**
        :   If *flags* specifies **BPF\_NOEXIST** and the element
            with *key* already exists in the map.

        **ENOENT**
        :   If *flags* specifies **BPF\_EXIST** and the element with
            *key* does not exist in the map.

BPF\_MAP\_DELETE\_ELEM
:   Description
    :   Look up and delete an element by key in a specified map.

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

BPF\_MAP\_GET\_NEXT\_KEY
:   Description
    :   Look up an element by key in a specified map and return the key
        of the next element. Can be used to iterate over all elements
        in the map.

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

        The following cases can be used to iterate over all elements of
        the map:

        * If *key* is not found, the operation returns zero and sets
          the *next\_key* pointer to the key of the first element.
        * If *key* is found, the operation returns zero and sets the
          *next\_key* pointer to the key of the next element.
        * If *key* is the last element, returns -1 and *errno* is set
          to **ENOENT**.

        May set *errno* to **ENOMEM**, **EFAULT**, **EPERM**, or
        **EINVAL** on error.

BPF\_PROG\_LOAD
:   Description
    :   Verify and load an eBPF program, returning a new file
        descriptor associated with the program.

        Applying **close**(2) to the file descriptor returned by
        **BPF\_PROG\_LOAD** will unload the eBPF program (but see NOTES).

        The close-on-exec file descriptor flag (see **fcntl**(2)) is
        automatically enabled for the new file descriptor.

    Return
    :   A new file descriptor (a nonnegative integer), or -1 if an
        error occurred (in which case, *errno* is set appropriately).

BPF\_OBJ\_PIN
:   Description
    :   Pin an eBPF program or map referred by the specified *bpf\_fd*
        to the provided *pathname* on the filesystem.

        The *pathname* argument must not contain a dot (“.”).

        On success, *pathname* retains a reference to the eBPF object,
        preventing deallocation of the object when the original
        *bpf\_fd* is closed. This allow the eBPF object to live beyond
        **close**(*bpf\_fd*), and hence the lifetime of the parent
        process.

        Applying **unlink**(2) or similar calls to the *pathname*
        unpins the object from the filesystem, removing the reference.
        If no other file descriptors or filesystem nodes refer to the
        same object, it will be deallocated (see NOTES).

        The filesystem type for the parent directory of *pathname* must
        be **BPF\_FS\_MAGIC**.

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

BPF\_OBJ\_GET
:   Description
    :   Open a file descriptor for the eBPF object pinned to the
        specified *pathname*.

    Return
    :   A new file descriptor (a nonnegative integer), or -1 if an
        error occurred (in which case, *errno* is set appropriately).

BPF\_PROG\_ATTACH
:   Description
    :   Attach an eBPF program to a *target\_fd* at the specified
        *attach\_type* hook.

        The *attach\_type* specifies the eBPF attachment point to
        attach the program to, and must be one of *bpf\_attach\_type*
        (see below).

        The *attach\_bpf\_fd* must be a valid file descriptor for a
        loaded eBPF program of a cgroup, flow dissector, LIRC, sockmap
        or sock\_ops type corresponding to the specified *attach\_type*.

        The *target\_fd* must be a valid file descriptor for a kernel
        object which depends on the attach type of *attach\_bpf\_fd*:

        **BPF\_PROG\_TYPE\_CGROUP\_DEVICE**,
        **BPF\_PROG\_TYPE\_CGROUP\_SKB**,
        **BPF\_PROG\_TYPE\_CGROUP\_SOCK**,
        **BPF\_PROG\_TYPE\_CGROUP\_SOCK\_ADDR**,
        **BPF\_PROG\_TYPE\_CGROUP\_SOCKOPT**,
        **BPF\_PROG\_TYPE\_CGROUP\_SYSCTL**,
        **BPF\_PROG\_TYPE\_SOCK\_OPS**

        > Control Group v2 hierarchy with the eBPF controller
        > enabled. Requires the kernel to be compiled with
        > **CONFIG\_CGROUP\_BPF**.

        **BPF\_PROG\_TYPE\_FLOW\_DISSECTOR**

        > Network namespace (eg /proc/self/ns/net).

        **BPF\_PROG\_TYPE\_LIRC\_MODE2**

        > LIRC device path (eg /dev/lircN). Requires the kernel
        > to be compiled with **CONFIG\_BPF\_LIRC\_MODE2**.

        **BPF\_PROG\_TYPE\_SK\_SKB**,
        **BPF\_PROG\_TYPE\_SK\_MSG**

        > eBPF map of socket type (eg **BPF\_MAP\_TYPE\_SOCKHASH**).

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

BPF\_PROG\_DETACH
:   Description
    :   Detach the eBPF program associated with the *target\_fd* at the
        hook specified by *attach\_type*. The program must have been
        previously attached using **BPF\_PROG\_ATTACH**.

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

BPF\_PROG\_TEST\_RUN
:   Description
    :   Run the eBPF program associated with the *prog\_fd* a *repeat*
        number of times against a provided program context *ctx\_in* and
        data *data\_in*, and return the modified program context
        *ctx\_out*, *data\_out* (for example, packet data), result of the
        execution *retval*, and *duration* of the test run.

        The sizes of the buffers provided as input and output
        parameters *ctx\_in*, *ctx\_out*, *data\_in*, and *data\_out* must
        be provided in the corresponding variables *ctx\_size\_in*,
        *ctx\_size\_out*, *data\_size\_in*, and/or *data\_size\_out*. If any
        of these parameters are not provided (ie set to NULL), the
        corresponding size field must be zero.

        Some program types have particular requirements:

        **BPF\_PROG\_TYPE\_SK\_LOOKUP**
        :   *data\_in* and *data\_out* must be NULL.

        **BPF\_PROG\_TYPE\_RAW\_TRACEPOINT**,
        **BPF\_PROG\_TYPE\_RAW\_TRACEPOINT\_WRITABLE**

        > *ctx\_out*, *data\_in* and *data\_out* must be NULL.
        > *repeat* must be zero.

        BPF\_PROG\_RUN is an alias for BPF\_PROG\_TEST\_RUN.

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

        **ENOSPC**
        :   Either *data\_size\_out* or *ctx\_size\_out* is too small.

        **ENOTSUPP**
        :   This command is not supported by the program type of
            the program referred to by *prog\_fd*.

BPF\_PROG\_GET\_NEXT\_ID
:   Description
    :   Fetch the next eBPF program currently loaded into the kernel.

        Looks for the eBPF program with an id greater than *start\_id*
        and updates *next\_id* on success. If no other eBPF programs
        remain with ids higher than *start\_id*, returns -1 and sets
        *errno* to **ENOENT**.

    Return
    :   Returns zero on success. On error, or when no id remains, -1
        is returned and *errno* is set appropriately.

BPF\_MAP\_GET\_NEXT\_ID
:   Description
    :   Fetch the next eBPF map currently loaded into the kernel.

        Looks for the eBPF map with an id greater than *start\_id*
        and updates *next\_id* on success. If no other eBPF maps
        remain with ids higher than *start\_id*, returns -1 and sets
        *errno* to **ENOENT**.

    Return
    :   Returns zero on success. On error, or when no id remains, -1
        is returned and *errno* is set appropriately.

BPF\_PROG\_GET\_FD\_BY\_ID
:   Description
    :   Open a file descriptor for the eBPF program corresponding to
        *prog\_id*.

    Return
    :   A new file descriptor (a nonnegative integer), or -1 if an
        error occurred (in which case, *errno* is set appropriately).

BPF\_MAP\_GET\_FD\_BY\_ID
:   Description
    :   Open a file descriptor for the eBPF map corresponding to
        *map\_id*.

    Return
    :   A new file descriptor (a nonnegative integer), or -1 if an
        error occurred (in which case, *errno* is set appropriately).

BPF\_OBJ\_GET\_INFO\_BY\_FD
:   Description
    :   Obtain information about the eBPF object corresponding to
        *bpf\_fd*.

        Populates up to *info\_len* bytes of *info*, which will be in
        one of the following formats depending on the eBPF object type
        of *bpf\_fd*:

        * **`struct bpf_prog_info`**
        * **`struct bpf_map_info`**
        * **`struct bpf_btf_info`**
        * **`struct bpf_link_info`**
        * **`struct bpf_token_info`**

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

BPF\_PROG\_QUERY
:   Description
    :   Obtain information about eBPF programs associated with the
        specified *attach\_type* hook.

        The *target\_fd* must be a valid file descriptor for a kernel
        object which depends on the attach type of *attach\_bpf\_fd*:

        **BPF\_PROG\_TYPE\_CGROUP\_DEVICE**,
        **BPF\_PROG\_TYPE\_CGROUP\_SKB**,
        **BPF\_PROG\_TYPE\_CGROUP\_SOCK**,
        **BPF\_PROG\_TYPE\_CGROUP\_SOCK\_ADDR**,
        **BPF\_PROG\_TYPE\_CGROUP\_SOCKOPT**,
        **BPF\_PROG\_TYPE\_CGROUP\_SYSCTL**,
        **BPF\_PROG\_TYPE\_SOCK\_OPS**

        > Control Group v2 hierarchy with the eBPF controller
        > enabled. Requires the kernel to be compiled with
        > **CONFIG\_CGROUP\_BPF**.

        **BPF\_PROG\_TYPE\_FLOW\_DISSECTOR**

        > Network namespace (eg /proc/self/ns/net).

        **BPF\_PROG\_TYPE\_LIRC\_MODE2**

        > LIRC device path (eg /dev/lircN). Requires the kernel
        > to be compiled with **CONFIG\_BPF\_LIRC\_MODE2**.

        **BPF\_PROG\_QUERY** always fetches the number of programs
        attached and the *attach\_flags* which were used to attach those
        programs. Additionally, if *prog\_ids* is nonzero and the number
        of attached programs is less than *prog\_cnt*, populates
        *prog\_ids* with the eBPF program ids of the programs attached
        at *target\_fd*.

        The following flags may alter the result:

        **BPF\_F\_QUERY\_EFFECTIVE**
        :   Only return information regarding programs which are
            currently effective at the specified *target\_fd*.

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

BPF\_RAW\_TRACEPOINT\_OPEN
:   Description
    :   Attach an eBPF program to a tracepoint *name* to access kernel
        internal arguments of the tracepoint in their raw form.

        The *prog\_fd* must be a valid file descriptor associated with
        a loaded eBPF program of type **BPF\_PROG\_TYPE\_RAW\_TRACEPOINT**.

        No ABI guarantees are made about the content of tracepoint
        arguments exposed to the corresponding eBPF program.

        Applying **close**(2) to the file descriptor returned by
        **BPF\_RAW\_TRACEPOINT\_OPEN** will delete the map (but see NOTES).

    Return
    :   A new file descriptor (a nonnegative integer), or -1 if an
        error occurred (in which case, *errno* is set appropriately).

BPF\_BTF\_LOAD
:   Description
    :   Verify and load BPF Type Format (BTF) metadata into the kernel,
        returning a new file descriptor associated with the metadata.
        BTF is described in more detail at
        <https://www.kernel.org/doc/html/latest/bpf/btf.html>.

        The *btf* parameter must point to valid memory providing
        *btf\_size* bytes of BTF binary metadata.

        The returned file descriptor can be passed to other **bpf**()
        subcommands such as **BPF\_PROG\_LOAD** or **BPF\_MAP\_CREATE** to
        associate the BTF with those objects.

        Similar to **BPF\_PROG\_LOAD**, **BPF\_BTF\_LOAD** has optional
        parameters to specify a *btf\_log\_buf*, *btf\_log\_size* and
        *btf\_log\_level* which allow the kernel to return freeform log
        output regarding the BTF verification process.

    Return
    :   A new file descriptor (a nonnegative integer), or -1 if an
        error occurred (in which case, *errno* is set appropriately).

BPF\_BTF\_GET\_FD\_BY\_ID
:   Description
    :   Open a file descriptor for the BPF Type Format (BTF)
        corresponding to *btf\_id*.

    Return
    :   A new file descriptor (a nonnegative integer), or -1 if an
        error occurred (in which case, *errno* is set appropriately).

BPF\_TASK\_FD\_QUERY
:   Description
    :   Obtain information about eBPF programs associated with the
        target process identified by *pid* and *fd*.

        If the *pid* and *fd* are associated with a tracepoint, kprobe
        or uprobe perf event, then the *prog\_id* and *fd\_type* will
        be populated with the eBPF program id and file descriptor type
        of type **bpf\_task\_fd\_type**. If associated with a kprobe or
        uprobe, the *probe\_offset* and *probe\_addr* will also be
        populated. Optionally, if *buf* is provided, then up to
        *buf\_len* bytes of *buf* will be populated with the name of
        the tracepoint, kprobe or uprobe.

        The resulting *prog\_id* may be introspected in deeper detail
        using **BPF\_PROG\_GET\_FD\_BY\_ID** and **BPF\_OBJ\_GET\_INFO\_BY\_FD**.

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

BPF\_MAP\_LOOKUP\_AND\_DELETE\_ELEM
:   Description
    :   Look up an element with the given *key* in the map referred to
        by the file descriptor *fd*, and if found, delete the element.

        For **BPF\_MAP\_TYPE\_QUEUE** and **BPF\_MAP\_TYPE\_STACK** map
        types, the *flags* argument needs to be set to 0, but for other
        map types, it may be specified as:

        **BPF\_F\_LOCK**
        :   Look up and delete the value of a spin-locked map
            without returning the lock. This must be specified if
            the elements contain a spinlock.

        The **BPF\_MAP\_TYPE\_QUEUE** and **BPF\_MAP\_TYPE\_STACK** map types
        implement this command as a “pop” operation, deleting the top
        element rather than one corresponding to *key*.
        The *key* and *key\_len* parameters should be zeroed when
        issuing this operation for these map types.

        This command is only valid for the following map types:
        \* **BPF\_MAP\_TYPE\_QUEUE**
        \* **BPF\_MAP\_TYPE\_STACK**
        \* **BPF\_MAP\_TYPE\_HASH**
        \* **BPF\_MAP\_TYPE\_PERCPU\_HASH**
        \* **BPF\_MAP\_TYPE\_LRU\_HASH**
        \* **BPF\_MAP\_TYPE\_LRU\_PERCPU\_HASH**

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

BPF\_MAP\_FREEZE
:   Description
    :   Freeze the permissions of the specified map.

        Write permissions may be frozen by passing zero *flags*.
        Upon success, no future syscall invocations may alter the
        map state of *map\_fd*. Write operations from eBPF programs
        are still possible for a frozen map.

        Not supported for maps of type **BPF\_MAP\_TYPE\_STRUCT\_OPS**.

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

BPF\_BTF\_GET\_NEXT\_ID
:   Description
    :   Fetch the next BPF Type Format (BTF) object currently loaded
        into the kernel.

        Looks for the BTF object with an id greater than *start\_id*
        and updates *next\_id* on success. If no other BTF objects
        remain with ids higher than *start\_id*, returns -1 and sets
        *errno* to **ENOENT**.

    Return
    :   Returns zero on success. On error, or when no id remains, -1
        is returned and *errno* is set appropriately.

BPF\_MAP\_LOOKUP\_BATCH
:   Description
    :   Iterate and fetch multiple elements in a map.

        Two opaque values are used to manage batch operations,
        *in\_batch* and *out\_batch*. Initially, *in\_batch* must be set
        to NULL to begin the batched operation. After each subsequent
        **BPF\_MAP\_LOOKUP\_BATCH**, the caller should pass the resultant
        *out\_batch* as the *in\_batch* for the next operation to
        continue iteration from the current point. Both *in\_batch* and
        *out\_batch* must point to memory large enough to hold a key,
        except for maps of type **BPF\_MAP\_TYPE\_{HASH, PERCPU\_HASH,
        LRU\_HASH, LRU\_PERCPU\_HASH}**, for which batch parameters
        must be at least 4 bytes wide regardless of key size.

        The *keys* and *values* are output parameters which must point
        to memory large enough to hold *count* items based on the key
        and value size of the map *map\_fd*. The *keys* buffer must be
        of *key\_size* \* *count*. The *values* buffer must be of
        *value\_size* \* *count*.

        The *elem\_flags* argument may be specified as one of the
        following:

        **BPF\_F\_LOCK**
        :   Look up the value of a spin-locked map without
            returning the lock. This must be specified if the
            elements contain a spinlock.

        On success, *count* elements from the map are copied into the
        user buffer, with the keys copied into *keys* and the values
        copied into the corresponding indices in *values*.

        If an error is returned and *errno* is not **EFAULT**, *count*
        is set to the number of successfully processed elements.

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

        May set *errno* to **ENOSPC** to indicate that *keys* or
        *values* is too small to dump an entire bucket during
        iteration of a hash-based map type.

BPF\_MAP\_LOOKUP\_AND\_DELETE\_BATCH
:   Description
    :   Iterate and delete all elements in a map.

        This operation has the same behavior as
        **BPF\_MAP\_LOOKUP\_BATCH** with two exceptions:

        * Every element that is successfully returned is also deleted
          from the map. This is at least *count* elements. Note that
          *count* is both an input and an output parameter.
        * Upon returning with *errno* set to **EFAULT**, up to
          *count* elements may be deleted without returning the keys
          and values of the deleted elements.

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

BPF\_MAP\_UPDATE\_BATCH
:   Description
    :   Update multiple elements in a map by *key*.

        The *keys* and *values* are input parameters which must point
        to memory large enough to hold *count* items based on the key
        and value size of the map *map\_fd*. The *keys* buffer must be
        of *key\_size* \* *count*. The *values* buffer must be of
        *value\_size* \* *count*.

        Each element specified in *keys* is sequentially updated to the
        value in the corresponding index in *values*. The *in\_batch*
        and *out\_batch* parameters are ignored and should be zeroed.

        The *elem\_flags* argument should be specified as one of the
        following:

        **BPF\_ANY**
        :   Create new elements or update a existing elements.

        **BPF\_NOEXIST**
        :   Create new elements only if they do not exist.

        **BPF\_EXIST**
        :   Update existing elements.

        **BPF\_F\_LOCK**
        :   Update spin\_lock-ed map elements. This must be
            specified if the map value contains a spinlock.

        On success, *count* elements from the map are updated.

        If an error is returned and *errno* is not **EFAULT**, *count*
        is set to the number of successfully processed elements.

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

        May set *errno* to **EINVAL**, **EPERM**, **ENOMEM**, or
        **E2BIG**. **E2BIG** indicates that the number of elements in
        the map reached the *max\_entries* limit specified at map
        creation time.

        May set *errno* to one of the following error codes under
        specific circumstances:

        **EEXIST**
        :   If *flags* specifies **BPF\_NOEXIST** and the element
            with *key* already exists in the map.

        **ENOENT**
        :   If *flags* specifies **BPF\_EXIST** and the element with
            *key* does not exist in the map.

BPF\_MAP\_DELETE\_BATCH
:   Description
    :   Delete multiple elements in a map by *key*.

        The *keys* parameter is an input parameter which must point
        to memory large enough to hold *count* items based on the key
        size of the map *map\_fd*, that is, *key\_size* \* *count*.

        Each element specified in *keys* is sequentially deleted. The
        *in\_batch*, *out\_batch*, and *values* parameters are ignored
        and should be zeroed.

        The *elem\_flags* argument may be specified as one of the
        following:

        **BPF\_F\_LOCK**
        :   Look up the value of a spin-locked map without
            returning the lock. This must be specified if the
            elements contain a spinlock.

        On success, *count* elements from the map are updated.

        If an error is returned and *errno* is not **EFAULT**, *count*
        is set to the number of successfully processed elements. If
        *errno* is **EFAULT**, up to *count* elements may be been
        deleted.

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

BPF\_LINK\_CREATE
:   Description
    :   Attach an eBPF program to a *target\_fd* at the specified
        *attach\_type* hook and return a file descriptor handle for
        managing the link.

    Return
    :   A new file descriptor (a nonnegative integer), or -1 if an
        error occurred (in which case, *errno* is set appropriately).

BPF\_LINK\_UPDATE
:   Description
    :   Update the eBPF program in the specified *link\_fd* to
        *new\_prog\_fd*.

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

BPF\_LINK\_GET\_FD\_BY\_ID
:   Description
    :   Open a file descriptor for the eBPF Link corresponding to
        *link\_id*.

    Return
    :   A new file descriptor (a nonnegative integer), or -1 if an
        error occurred (in which case, *errno* is set appropriately).

BPF\_LINK\_GET\_NEXT\_ID
:   Description
    :   Fetch the next eBPF link currently loaded into the kernel.

        Looks for the eBPF link with an id greater than *start\_id*
        and updates *next\_id* on success. If no other eBPF links
        remain with ids higher than *start\_id*, returns -1 and sets
        *errno* to **ENOENT**.

    Return
    :   Returns zero on success. On error, or when no id remains, -1
        is returned and *errno* is set appropriately.

BPF\_ENABLE\_STATS
:   Description
    :   Enable eBPF runtime statistics gathering.

        Runtime statistics gathering for the eBPF runtime is disabled
        by default to minimize the corresponding performance overhead.
        This command enables statistics globally.

        Multiple programs may independently enable statistics.
        After gathering the desired statistics, eBPF runtime statistics
        may be disabled again by calling **close**(2) for the file
        descriptor returned by this function. Statistics will only be
        disabled system-wide when all outstanding file descriptors
        returned by prior calls for this subcommand are closed.

    Return
    :   A new file descriptor (a nonnegative integer), or -1 if an
        error occurred (in which case, *errno* is set appropriately).

BPF\_ITER\_CREATE
:   Description
    :   Create an iterator on top of the specified *link\_fd* (as
        previously created using **BPF\_LINK\_CREATE**) and return a
        file descriptor that can be used to trigger the iteration.

        If the resulting file descriptor is pinned to the filesystem
        using **BPF\_OBJ\_PIN**, then subsequent **read**(2) syscalls
        for that path will trigger the iterator to read kernel state
        using the eBPF program attached to *link\_fd*.

    Return
    :   A new file descriptor (a nonnegative integer), or -1 if an
        error occurred (in which case, *errno* is set appropriately).

BPF\_LINK\_DETACH
:   Description
    :   Forcefully detach the specified *link\_fd* from its
        corresponding attachment point.

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

BPF\_PROG\_BIND\_MAP
:   Description
    :   Bind a map to the lifetime of an eBPF program.

        The map identified by *map\_fd* is bound to the program
        identified by *prog\_fd* and only released when *prog\_fd* is
        released. This may be used in cases where metadata should be
        associated with a program which otherwise does not contain any
        references to the map (for example, embedded in the eBPF
        program instructions).

    Return
    :   Returns zero on success. On error, -1 is returned and *errno*
        is set appropriately.

BPF\_TOKEN\_CREATE
:   Description
    :   Create BPF token with embedded information about what
        BPF-related functionality it allows:
        - a set of allowed [`bpf()`](../../bpf/maps.html#c.bpf "bpf") syscall commands;
        - a set of allowed BPF map types to be created with
        BPF\_MAP\_CREATE command, if BPF\_MAP\_CREATE itself is allowed;
        - a set of allowed BPF program types and BPF program attach
        types to be loaded with BPF\_PROG\_LOAD command, if
        BPF\_PROG\_LOAD itself is allowed.

        BPF token is created (derived) from an instance of BPF FS,
        assuming it has necessary delegation mount options specified.
        This BPF token can be passed as an extra parameter to various
        [`bpf()`](../../bpf/maps.html#c.bpf "bpf") syscall commands to grant BPF subsystem functionality to
        unprivileged processes.

        When created, BPF token is “associated” with the owning
        user namespace of BPF FS instance (super block) that it was
        derived from, and subsequent BPF operations performed with
        BPF token would be performing capabilities checks (i.e.,
        CAP\_BPF, CAP\_PERFMON, CAP\_NET\_ADMIN, CAP\_SYS\_ADMIN) within
        that user namespace. Without BPF token, such capabilities
        have to be granted in init user namespace, making [`bpf()`](../../bpf/maps.html#c.bpf "bpf")
        syscall incompatible with user namespace, for the most part.

    Return
    :   A new file descriptor (a nonnegative integer), or -1 if an
        error occurred (in which case, *errno* is set appropriately).

BPF\_PROG\_STREAM\_READ\_BY\_FD
:   Description
    :   Read data of a program’s BPF stream. The program is identified
        by *prog\_fd*, and the stream is identified by the *stream\_id*.
        The data is copied to a buffer pointed to by *stream\_buf*, and
        filled less than or equal to *stream\_buf\_len* bytes.

    Return
    :   Number of bytes read from the stream on success, or -1 if an
        error occurred (in which case, *errno* is set appropriately).

BPF\_PROG\_ASSOC\_STRUCT\_OPS
:   Description
    :   Associate a BPF program with a struct\_ops map. The struct\_ops
        map is identified by *map\_fd* and the BPF program is
        identified by *prog\_fd*.

    Return
    :   0 on success or -1 if an error occurred (in which case,
        *errno* is set appropriately).

NOTES
:   eBPF objects (maps and programs) can be shared between processes.

    * After **fork**(2), the child inherits file descriptors
      referring to the same eBPF objects.
    * File descriptors referring to eBPF objects can be transferred over
      **unix**(7) domain sockets.
    * File descriptors referring to eBPF objects can be duplicated in the
      usual way, using **dup**(2) and similar calls.
    * File descriptors referring to eBPF objects can be pinned to the
      filesystem using the **BPF\_OBJ\_PIN** command of **bpf**(2).

    An eBPF object is deallocated only after all file descriptors referring
    to the object have been closed and no references remain pinned to the
    filesystem or attached (for example, bound to a program or device).
