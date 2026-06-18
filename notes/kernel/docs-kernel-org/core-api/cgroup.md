# Cgroup Kernel APIs

> 출처(원문): https://docs.kernel.org/core-api/cgroup.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Cgroup Kernel APIs

## Device Memory Cgroup API (dmemcg)

bool dmem\_cgroup\_state\_evict\_valuable(struct dmem\_cgroup\_pool\_state \*limit\_pool, struct dmem\_cgroup\_pool\_state \*test\_pool, bool ignore\_low, bool \*ret\_hit\_low)
:   Check if we should evict from test\_pool

**Parameters**

`struct dmem_cgroup_pool_state *limit_pool`
:   The pool for which we hit limits

`struct dmem_cgroup_pool_state *test_pool`
:   The pool for which to test

`bool ignore_low`
:   Whether we have to respect low watermarks.

`bool *ret_hit_low`
:   Pointer to whether it makes sense to consider low watermark.

**Description**

This function returns true if we can evict from **test\_pool**, false if not.
When returning false and **ignore\_low** is false, **ret\_hit\_low** may
be set to true to indicate this function can be retried with **ignore\_low**
set to true.

**Return**

bool

void dmem\_cgroup\_unregister\_region(struct dmem\_cgroup\_region \*region)
:   Unregister a previously registered region.

**Parameters**

`struct dmem_cgroup_region *region`
:   The region to unregister.

**Description**

This function undoes dmem\_cgroup\_register\_region.

struct dmem\_cgroup\_region \*dmem\_cgroup\_register\_region(u64 size, const char \*fmt, ...)
:   Register a regions for dev cgroup.

**Parameters**

`u64 size`
:   Size of region to register, in bytes.

`const char *fmt`
:   Region parameters to register

`...`
:   variable arguments

**Description**

This function registers a node in the dmem cgroup with the
name given. After calling this function, the region can be
used for allocations.

**Return**

NULL or a `struct on` success, PTR\_ERR on failure.

void dmem\_cgroup\_pool\_state\_put(struct dmem\_cgroup\_pool\_state \*pool)
:   Drop a reference to a dmem\_cgroup\_pool\_state

**Parameters**

`struct dmem_cgroup_pool_state *pool`
:   `dmem_cgroup_pool_state`

**Description**

Called to drop a reference to the limiting pool returned by
[`dmem_cgroup_try_charge()`](#c.dmem_cgroup_try_charge "dmem_cgroup_try_charge").

void dmem\_cgroup\_uncharge(struct dmem\_cgroup\_pool\_state \*pool, u64 size)
:   Uncharge a pool.

**Parameters**

`struct dmem_cgroup_pool_state *pool`
:   Pool to uncharge.

`u64 size`
:   Size to uncharge.

**Description**

Undoes the effects of dmem\_cgroup\_try\_charge.
Must be called with the returned pool as argument,
and same **index** and **size**.

int dmem\_cgroup\_try\_charge(struct dmem\_cgroup\_region \*region, u64 size, struct dmem\_cgroup\_pool\_state \*\*ret\_pool, struct dmem\_cgroup\_pool\_state \*\*ret\_limit\_pool)
:   Try charging a new allocation to a region.

**Parameters**

`struct dmem_cgroup_region *region`
:   dmem region to charge

`u64 size`
:   Size (in bytes) to charge.

`struct dmem_cgroup_pool_state **ret_pool`
:   On succesfull allocation, the pool that is charged.

`struct dmem_cgroup_pool_state **ret_limit_pool`
:   On a failed allocation, the limiting pool.

**Description**

This function charges the **region** region for a size of **size** bytes.

If the function succeeds, **ret\_pool** is set, which must be passed to
[`dmem_cgroup_uncharge()`](#c.dmem_cgroup_uncharge "dmem_cgroup_uncharge") when undoing the allocation.

When this function fails with -EAGAIN and **ret\_limit\_pool** is non-null, it
will be set to the pool for which the limit is hit. This can be used for
eviction as argument to `dmem_cgroup_evict_valuable()`. This reference must be freed
with **[`dmem_cgroup_pool_state_put()`](#c.dmem_cgroup_pool_state_put "dmem_cgroup_pool_state_put")**.

**Return**

0 on success, -EAGAIN on hitting a limit, or a negative errno on failure.
