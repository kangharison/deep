# BPF cpumask kfuncs

> 출처(원문): https://docs.kernel.org/bpf/cpumasks.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# BPF cpumask kfuncs

## 1. Introduction

`struct cpumask` is a bitmap data structure in the kernel whose indices
reflect the CPUs on the system. Commonly, cpumasks are used to track which CPUs
a task is affinitized to, but they can also be used to e.g. track which cores
are associated with a scheduling domain, which cores on a machine are idle,
etc.

BPF provides programs with a set of [BPF Kernel Functions (kfuncs)](kfuncs.html#kfuncs-header-label) that can be
used to allocate, mutate, query, and free cpumasks.

## 2. BPF cpumask objects

There are two different types of cpumasks that can be used by BPF programs.

### 2.1 `struct bpf_cpumask *`

`struct bpf_cpumask *` is a cpumask that is allocated by BPF, on behalf of a
BPF program, and whose lifecycle is entirely controlled by BPF. These cpumasks
are RCU-protected, can be mutated, can be used as kptrs, and can be safely cast
to a `struct cpumask *`.

### 2.1.1 `struct bpf_cpumask *` lifecycle

A `struct bpf_cpumask *` is allocated, acquired, and released, using the
following functions:

\_\_bpf\_kfunc struct bpf\_cpumask \*bpf\_cpumask\_create(void)
:   Create a mutable BPF cpumask.

**Parameters**

`void`
:   no arguments

**Description**

Allocates a cpumask that can be queried, mutated, acquired, and released by
a BPF program. The cpumask returned by this function must either be embedded
in a map as a kptr, or freed with [`bpf_cpumask_release()`](#c.bpf_cpumask_release "bpf_cpumask_release").

[`bpf_cpumask_create()`](#c.bpf_cpumask_create "bpf_cpumask_create") allocates memory using the BPF memory allocator, and
will not block. It may return NULL if no memory is available.

**Return**

* A pointer to a new `struct bpf_cpumask` instance on success.
* NULL if the BPF memory allocator is out of memory.

\_\_bpf\_kfunc struct bpf\_cpumask \*bpf\_cpumask\_acquire(struct bpf\_cpumask \*cpumask)
:   Acquire a reference to a BPF cpumask.

**Parameters**

`struct bpf_cpumask *cpumask`
:   The BPF cpumask being acquired. The cpumask must be a trusted
    pointer.

**Description**

Acquires a reference to a BPF cpumask. The cpumask returned by this function
must either be embedded in a map as a kptr, or freed with
[`bpf_cpumask_release()`](#c.bpf_cpumask_release "bpf_cpumask_release").

**Return**

* The `struct bpf_cpumask` pointer passed to the function.

\_\_bpf\_kfunc void bpf\_cpumask\_release(struct bpf\_cpumask \*cpumask)
:   Release a previously acquired BPF cpumask.

**Parameters**

`struct bpf_cpumask *cpumask`
:   The cpumask being released.

**Description**

Releases a previously acquired reference to a BPF cpumask. When the final
reference of the BPF cpumask has been released, it is subsequently freed in
an RCU callback in the BPF memory allocator.

For example:

```
struct cpumask_map_value {
        struct bpf_cpumask __kptr * cpumask;
};

struct array_map {
        __uint(type, BPF_MAP_TYPE_ARRAY);
        __type(key, int);
        __type(value, struct cpumask_map_value);
        __uint(max_entries, 65536);
} cpumask_map SEC(".maps");

static int cpumask_map_insert(struct bpf_cpumask *mask, u32 pid)
{
        struct cpumask_map_value local, *v;
        long status;
        struct bpf_cpumask *old;
        u32 key = pid;

        local.cpumask = NULL;
        status = bpf_map_update_elem(&cpumask_map, &key, &local, 0);
        if (status) {
                bpf_cpumask_release(mask);
                return status;
        }

        v = bpf_map_lookup_elem(&cpumask_map, &key);
        if (!v) {
                bpf_cpumask_release(mask);
                return -ENOENT;
        }

        old = bpf_kptr_xchg(&v->cpumask, mask);
        if (old)
                bpf_cpumask_release(old);

        return 0;
}

/**
 * A sample tracepoint showing how a task's cpumask can be queried and
 * recorded as a kptr.
 */
SEC("tp_btf/task_newtask")
int BPF_PROG(record_task_cpumask, struct task_struct *task, u64 clone_flags)
{
        struct bpf_cpumask *cpumask;
        int ret;

        cpumask = bpf_cpumask_create();
        if (!cpumask)
                return -ENOMEM;

        if (!bpf_cpumask_full(task->cpus_ptr))
                bpf_printk("task %s has CPU affinity", task->comm);

        bpf_cpumask_copy(cpumask, task->cpus_ptr);
        return cpumask_map_insert(cpumask, task->pid);
}
```

---

### 2.1.1 `struct bpf_cpumask *` as kptrs

As mentioned and illustrated above, these `struct bpf_cpumask *` objects can
also be stored in a map and used as kptrs. If a `struct bpf_cpumask *` is in
a map, the reference can be removed from the map with `bpf_kptr_xchg()`, or
opportunistically acquired using RCU:

```
/* struct containing the struct bpf_cpumask kptr which is stored in the map. */
struct cpumasks_kfunc_map_value {
        struct bpf_cpumask __kptr * bpf_cpumask;
};

/* The map containing struct cpumasks_kfunc_map_value entries. */
struct {
        __uint(type, BPF_MAP_TYPE_ARRAY);
        __type(key, int);
        __type(value, struct cpumasks_kfunc_map_value);
        __uint(max_entries, 1);
} cpumasks_kfunc_map SEC(".maps");

/* ... */

/**
 * A simple example tracepoint program showing how a
 * struct bpf_cpumask * kptr that is stored in a map can
 * be passed to kfuncs using RCU protection.
 */
SEC("tp_btf/cgroup_mkdir")
int BPF_PROG(cgrp_ancestor_example, struct cgroup *cgrp, const char *path)
{
        struct bpf_cpumask *kptr;
        struct cpumasks_kfunc_map_value *v;
        u32 key = 0;

        /* Assume a bpf_cpumask * kptr was previously stored in the map. */
        v = bpf_map_lookup_elem(&cpumasks_kfunc_map, &key);
        if (!v)
                return -ENOENT;

        bpf_rcu_read_lock();
        /* Acquire a reference to the bpf_cpumask * kptr that's already stored in the map. */
        kptr = v->cpumask;
        if (!kptr) {
                /* If no bpf_cpumask was present in the map, it's because
                 * we're racing with another CPU that removed it with
                 * bpf_kptr_xchg() between the bpf_map_lookup_elem()
                 * above, and our load of the pointer from the map.
                 */
                bpf_rcu_read_unlock();
                return -EBUSY;
        }

        bpf_cpumask_setall(kptr);
        bpf_rcu_read_unlock();

        return 0;
}
```

---

### 2.2 `struct cpumask`

`struct cpumask` is the object that actually contains the cpumask bitmap
being queried, mutated, etc. A `struct bpf_cpumask` wraps a `struct
cpumask`, which is why it’s safe to cast it as such (note however that it is
**not** safe to cast a `struct cpumask *` to a `struct bpf_cpumask *`, and
the verifier will reject any program that tries to do so).

As we’ll see below, any kfunc that mutates its cpumask argument will take a
`struct bpf_cpumask *` as that argument. Any argument that simply queries the
cpumask will instead take a `struct cpumask *`.

## 3. cpumask kfuncs

Above, we described the kfuncs that can be used to allocate, acquire, release,
etc a `struct bpf_cpumask *`. This section of the document will describe the
kfuncs for mutating and querying cpumasks.

### 3.1 Mutating cpumasks

Some cpumask kfuncs are “read-only” in that they don’t mutate any of their
arguments, whereas others mutate at least one argument (which means that the
argument must be a `struct bpf_cpumask *`, as described above).

This section will describe all of the cpumask kfuncs which mutate at least one
argument. [3.2 Querying cpumasks](#cpumasks-querying-label) below describes the read-only kfuncs.

### 3.1.1 Setting and clearing CPUs

[`bpf_cpumask_set_cpu()`](#c.bpf_cpumask_set_cpu "bpf_cpumask_set_cpu") and [`bpf_cpumask_clear_cpu()`](#c.bpf_cpumask_clear_cpu "bpf_cpumask_clear_cpu") can be used to set and clear
a CPU in a `struct bpf_cpumask` respectively:

\_\_bpf\_kfunc void bpf\_cpumask\_set\_cpu(u32 cpu, struct bpf\_cpumask \*cpumask)
:   Set a bit for a CPU in a BPF cpumask.

**Parameters**

`u32 cpu`
:   The CPU to be set in the cpumask.

`struct bpf_cpumask *cpumask`
:   The BPF cpumask in which a bit is being set.

\_\_bpf\_kfunc void bpf\_cpumask\_clear\_cpu(u32 cpu, struct bpf\_cpumask \*cpumask)
:   Clear a bit for a CPU in a BPF cpumask.

**Parameters**

`u32 cpu`
:   The CPU to be cleared from the cpumask.

`struct bpf_cpumask *cpumask`
:   The BPF cpumask in which a bit is being cleared.

These kfuncs are pretty straightforward, and can be used, for example, as
follows:

```
/**
 * A sample tracepoint showing how a cpumask can be queried.
 */
SEC("tp_btf/task_newtask")
int BPF_PROG(test_set_clear_cpu, struct task_struct *task, u64 clone_flags)
{
        struct bpf_cpumask *cpumask;

        cpumask = bpf_cpumask_create();
        if (!cpumask)
                return -ENOMEM;

        bpf_cpumask_set_cpu(0, cpumask);
        if (!bpf_cpumask_test_cpu(0, cast(cpumask)))
                /* Should never happen. */
                goto release_exit;

        bpf_cpumask_clear_cpu(0, cpumask);
        if (bpf_cpumask_test_cpu(0, cast(cpumask)))
                /* Should never happen. */
                goto release_exit;

        /* struct cpumask * pointers such as task->cpus_ptr can also be queried. */
        if (bpf_cpumask_test_cpu(0, task->cpus_ptr))
                bpf_printk("task %s can use CPU %d", task->comm, 0);

release_exit:
        bpf_cpumask_release(cpumask);
        return 0;
}
```

---

[`bpf_cpumask_test_and_set_cpu()`](#c.bpf_cpumask_test_and_set_cpu "bpf_cpumask_test_and_set_cpu") and [`bpf_cpumask_test_and_clear_cpu()`](#c.bpf_cpumask_test_and_clear_cpu "bpf_cpumask_test_and_clear_cpu") are
complementary kfuncs that allow callers to atomically test and set (or clear)
CPUs:

\_\_bpf\_kfunc bool bpf\_cpumask\_test\_and\_set\_cpu(u32 cpu, struct bpf\_cpumask \*cpumask)
:   Atomically test and set a CPU in a BPF cpumask.

**Parameters**

`u32 cpu`
:   The CPU being set and queried for.

`struct bpf_cpumask *cpumask`
:   The BPF cpumask being set and queried for containing a CPU.

**Return**

* true - **cpu** is set in the cpumask
* false - **cpu** was not set in the cpumask, or **cpu** is invalid.

\_\_bpf\_kfunc bool bpf\_cpumask\_test\_and\_clear\_cpu(u32 cpu, struct bpf\_cpumask \*cpumask)
:   Atomically test and clear a CPU in a BPF cpumask.

**Parameters**

`u32 cpu`
:   The CPU being cleared and queried for.

`struct bpf_cpumask *cpumask`
:   The BPF cpumask being cleared and queried for containing a CPU.

**Return**

* true - **cpu** is set in the cpumask
* false - **cpu** was not set in the cpumask, or **cpu** is invalid.

---

We can also set and clear entire `struct bpf_cpumask *` objects in one
operation using [`bpf_cpumask_setall()`](#c.bpf_cpumask_setall "bpf_cpumask_setall") and [`bpf_cpumask_clear()`](#c.bpf_cpumask_clear "bpf_cpumask_clear"):

\_\_bpf\_kfunc void bpf\_cpumask\_setall(struct bpf\_cpumask \*cpumask)
:   Set all of the bits in a BPF cpumask.

**Parameters**

`struct bpf_cpumask *cpumask`
:   The BPF cpumask having all of its bits set.

\_\_bpf\_kfunc void bpf\_cpumask\_clear(struct bpf\_cpumask \*cpumask)
:   Clear all of the bits in a BPF cpumask.

**Parameters**

`struct bpf_cpumask *cpumask`
:   The BPF cpumask being cleared.

### 3.1.2 Operations between cpumasks

In addition to setting and clearing individual CPUs in a single cpumask,
callers can also perform bitwise operations between multiple cpumasks using
[`bpf_cpumask_and()`](#c.bpf_cpumask_and "bpf_cpumask_and"), [`bpf_cpumask_or()`](#c.bpf_cpumask_or "bpf_cpumask_or"), and [`bpf_cpumask_xor()`](#c.bpf_cpumask_xor "bpf_cpumask_xor"):

\_\_bpf\_kfunc bool bpf\_cpumask\_and(struct bpf\_cpumask \*dst, const struct cpumask \*src1, const struct cpumask \*src2)
:   AND two cpumasks and store the result.

**Parameters**

`struct bpf_cpumask *dst`
:   The BPF cpumask where the result is being stored.

`const struct cpumask *src1`
:   The first input.

`const struct cpumask *src2`
:   The second input.

**Return**

* true - **dst** has at least one bit set following the operation
* false - **dst** is empty following the operation

**Description**

`struct bpf_cpumask` pointers may be safely passed to **src1** and **src2**.

\_\_bpf\_kfunc void bpf\_cpumask\_or(struct bpf\_cpumask \*dst, const struct cpumask \*src1, const struct cpumask \*src2)
:   OR two cpumasks and store the result.

**Parameters**

`struct bpf_cpumask *dst`
:   The BPF cpumask where the result is being stored.

`const struct cpumask *src1`
:   The first input.

`const struct cpumask *src2`
:   The second input.

**Description**

`struct bpf_cpumask` pointers may be safely passed to **src1** and **src2**.

\_\_bpf\_kfunc void bpf\_cpumask\_xor(struct bpf\_cpumask \*dst, const struct cpumask \*src1, const struct cpumask \*src2)
:   XOR two cpumasks and store the result.

**Parameters**

`struct bpf_cpumask *dst`
:   The BPF cpumask where the result is being stored.

`const struct cpumask *src1`
:   The first input.

`const struct cpumask *src2`
:   The second input.

**Description**

`struct bpf_cpumask` pointers may be safely passed to **src1** and **src2**.

The following is an example of how they may be used. Note that some of the
kfuncs shown in this example will be covered in more detail below.

```
/**
 * A sample tracepoint showing how a cpumask can be mutated using
   bitwise operators (and queried).
 */
SEC("tp_btf/task_newtask")
int BPF_PROG(test_and_or_xor, struct task_struct *task, u64 clone_flags)
{
        struct bpf_cpumask *mask1, *mask2, *dst1, *dst2;

        mask1 = bpf_cpumask_create();
        if (!mask1)
                return -ENOMEM;

        mask2 = bpf_cpumask_create();
        if (!mask2) {
                bpf_cpumask_release(mask1);
                return -ENOMEM;
        }

        // ...Safely create the other two masks... */

        bpf_cpumask_set_cpu(0, mask1);
        bpf_cpumask_set_cpu(1, mask2);
        bpf_cpumask_and(dst1, (const struct cpumask *)mask1, (const struct cpumask *)mask2);
        if (!bpf_cpumask_empty((const struct cpumask *)dst1))
                /* Should never happen. */
                goto release_exit;

        bpf_cpumask_or(dst1, (const struct cpumask *)mask1, (const struct cpumask *)mask2);
        if (!bpf_cpumask_test_cpu(0, (const struct cpumask *)dst1))
                /* Should never happen. */
                goto release_exit;

        if (!bpf_cpumask_test_cpu(1, (const struct cpumask *)dst1))
                /* Should never happen. */
                goto release_exit;

        bpf_cpumask_xor(dst2, (const struct cpumask *)mask1, (const struct cpumask *)mask2);
        if (!bpf_cpumask_equal((const struct cpumask *)dst1,
                               (const struct cpumask *)dst2))
                /* Should never happen. */
                goto release_exit;

 release_exit:
        bpf_cpumask_release(mask1);
        bpf_cpumask_release(mask2);
        bpf_cpumask_release(dst1);
        bpf_cpumask_release(dst2);
        return 0;
}
```

---

The contents of an entire cpumask may be copied to another using
[`bpf_cpumask_copy()`](#c.bpf_cpumask_copy "bpf_cpumask_copy"):

\_\_bpf\_kfunc void bpf\_cpumask\_copy(struct bpf\_cpumask \*dst, const struct cpumask \*src)
:   Copy the contents of a cpumask into a BPF cpumask.

**Parameters**

`struct bpf_cpumask *dst`
:   The BPF cpumask being copied into.

`const struct cpumask *src`
:   The cpumask being copied.

**Description**

A `struct bpf_cpumask` pointer may be safely passed to **src**.

---

### 3.2 Querying cpumasks

In addition to the above kfuncs, there is also a set of read-only kfuncs that
can be used to query the contents of cpumasks.

\_\_bpf\_kfunc u32 bpf\_cpumask\_first(const struct [cpumask](#c.bpf_cpumask_first "cpumask") \*cpumask)
:   Get the index of the first nonzero bit in the cpumask.

**Parameters**

`const struct cpumask *cpumask`
:   The cpumask being queried.

**Description**

Find the index of the first nonzero bit of the cpumask. A `struct bpf_cpumask`
pointer may be safely passed to this function.

**Return**

* The index of the first nonzero bit in the `struct cpumask`.

\_\_bpf\_kfunc u32 bpf\_cpumask\_first\_zero(const struct [cpumask](#c.bpf_cpumask_first_zero "cpumask") \*cpumask)
:   Get the index of the first unset bit in the cpumask.

**Parameters**

`const struct cpumask *cpumask`
:   The cpumask being queried.

**Description**

Find the index of the first unset bit of the cpumask. A `struct bpf_cpumask`
pointer may be safely passed to this function.

**Return**

* The index of the first zero bit in the `struct cpumask`.

\_\_bpf\_kfunc u32 bpf\_cpumask\_first\_and(const struct cpumask \*src1, const struct cpumask \*src2)
:   Return the index of the first nonzero bit from the AND of two cpumasks.

**Parameters**

`const struct cpumask *src1`
:   The first cpumask.

`const struct cpumask *src2`
:   The second cpumask.

**Description**

Find the index of the first nonzero bit of the AND of two cpumasks.
`struct bpf_cpumask` pointers may be safely passed to **src1** and **src2**.

**Return**

* The index of the first bit that is nonzero in both cpumask instances.

\_\_bpf\_kfunc bool bpf\_cpumask\_test\_cpu(u32 cpu, const struct [cpumask](#c.bpf_cpumask_test_cpu "cpumask") \*cpumask)
:   Test whether a CPU is set in a cpumask.

**Parameters**

`u32 cpu`
:   The CPU being queried for.

`const struct cpumask *cpumask`
:   The cpumask being queried for containing a CPU.

**Return**

* true - **cpu** is set in the cpumask
* false - **cpu** was not set in the cpumask, or **cpu** is an invalid cpu.

\_\_bpf\_kfunc u32 bpf\_cpumask\_weight(const struct [cpumask](#c.bpf_cpumask_weight "cpumask") \*cpumask)
:   Return the number of bits in **cpumask**.

**Parameters**

`const struct cpumask *cpumask`
:   The cpumask being queried.

**Description**

Count the number of set bits in the given cpumask.

**Return**

* The number of bits set in the mask.

\_\_bpf\_kfunc bool bpf\_cpumask\_equal(const struct cpumask \*src1, const struct cpumask \*src2)
:   Check two cpumasks for equality.

**Parameters**

`const struct cpumask *src1`
:   The first input.

`const struct cpumask *src2`
:   The second input.

**Return**

* true - **src1** and **src2** have the same bits set.
* false - **src1** and **src2** differ in at least one bit.

**Description**

`struct bpf_cpumask` pointers may be safely passed to **src1** and **src2**.

\_\_bpf\_kfunc bool bpf\_cpumask\_intersects(const struct cpumask \*src1, const struct cpumask \*src2)
:   Check two cpumasks for overlap.

**Parameters**

`const struct cpumask *src1`
:   The first input.

`const struct cpumask *src2`
:   The second input.

**Return**

* true - **src1** and **src2** have at least one of the same bits set.
* false - **src1** and **src2** don’t have any of the same bits set.

**Description**

`struct bpf_cpumask` pointers may be safely passed to **src1** and **src2**.

\_\_bpf\_kfunc bool bpf\_cpumask\_subset(const struct cpumask \*src1, const struct cpumask \*src2)
:   Check if a cpumask is a subset of another.

**Parameters**

`const struct cpumask *src1`
:   The first cpumask being checked as a subset.

`const struct cpumask *src2`
:   The second cpumask being checked as a superset.

**Return**

* true - All of the bits of **src1** are set in **src2**.
* false - At least one bit in **src1** is not set in **src2**.

**Description**

`struct bpf_cpumask` pointers may be safely passed to **src1** and **src2**.

\_\_bpf\_kfunc bool bpf\_cpumask\_empty(const struct [cpumask](#c.bpf_cpumask_empty "cpumask") \*cpumask)
:   Check if a cpumask is empty.

**Parameters**

`const struct cpumask *cpumask`
:   The cpumask being checked.

**Return**

* true - None of the bits in **cpumask** are set.
* false - At least one bit in **cpumask** is set.

**Description**

A `struct bpf_cpumask` pointer may be safely passed to **cpumask**.

\_\_bpf\_kfunc bool bpf\_cpumask\_full(const struct [cpumask](#c.bpf_cpumask_full "cpumask") \*cpumask)
:   Check if a cpumask has all bits set.

**Parameters**

`const struct cpumask *cpumask`
:   The cpumask being checked.

**Return**

* true - All of the bits in **cpumask** are set.
* false - At least one bit in **cpumask** is cleared.

**Description**

A `struct bpf_cpumask` pointer may be safely passed to **cpumask**.

\_\_bpf\_kfunc u32 bpf\_cpumask\_any\_distribute(const struct [cpumask](#c.bpf_cpumask_any_distribute "cpumask") \*cpumask)
:   Return a random set CPU from a cpumask.

**Parameters**

`const struct cpumask *cpumask`
:   The cpumask being queried.

**Return**

* A random set bit within [0, num\_cpus) if at least one bit is set.
* >= num\_cpus if no bit is set.

**Description**

A `struct bpf_cpumask` pointer may be safely passed to **src**.

\_\_bpf\_kfunc u32 bpf\_cpumask\_any\_and\_distribute(const struct cpumask \*src1, const struct cpumask \*src2)
:   Return a random set CPU from the AND of two cpumasks.

**Parameters**

`const struct cpumask *src1`
:   The first cpumask.

`const struct cpumask *src2`
:   The second cpumask.

**Return**

* A random set bit within [0, num\_cpus) from the AND of two cpumasks, if at
  least one bit is set.
* >= num\_cpus if no bit is set.

**Description**

`struct bpf_cpumask` pointers may be safely passed to **src1** and **src2**.

---

Some example usages of these querying kfuncs were shown above. We will not
replicate those examples here. Note, however, that all of the aforementioned
kfuncs are tested in [tools/testing/selftests/bpf/progs/cpumask\_success.c](https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/tools/testing/selftests/bpf/progs/cpumask_success.c), so
please take a look there if you’re looking for more examples of how they can be
used.

## 4. Adding BPF cpumask kfuncs

The set of supported BPF cpumask kfuncs are not (yet) a 1-1 match with the
cpumask operations in include/linux/cpumask.h. Any of those cpumask operations
could easily be encapsulated in a new kfunc if and when required. If you’d like
to support a new cpumask operation, please feel free to submit a patch. If you
do add a new cpumask kfunc, please document it here, and add any relevant
selftest testcases to the cpumask selftest suite.
