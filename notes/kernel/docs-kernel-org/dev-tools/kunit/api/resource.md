# Resource API

> 출처(원문): https://docs.kernel.org/dev-tools/kunit/api/resource.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Resource API

This file documents the KUnit resource API.

Most users won’t need to use this API directly, power users can use it to store
state on a per-test basis, register custom cleanup actions, and more.

struct kunit\_resource
:   represents a *test managed resource*

**Definition**:

```
struct kunit_resource {
    void *data;
    const char *name;
    kunit_resource_free_t free;
};
```

**Members**

`data`
:   for the user to store arbitrary data.

`name`
:   optional name

`free`
:   a user supplied function to free the resource.

**Description**

Represents a *test managed resource*, a resource which will automatically be
cleaned up at the end of a test case. This cleanup is performed by the ‘free’
function. The [`struct kunit_resource`](#c.kunit_resource "kunit_resource") itself is freed automatically with
[`kfree()`](../../../core-api/mm-api.html#c.kfree "kfree") if it was allocated by KUnit (e.g., by [`kunit_alloc_resource()`](#c.kunit_alloc_resource "kunit_alloc_resource")), but
must be freed by the user otherwise.

Resources are reference counted so if a resource is retrieved via
[`kunit_alloc_and_get_resource()`](#c.kunit_alloc_and_get_resource "kunit_alloc_and_get_resource") or [`kunit_find_resource()`](#c.kunit_find_resource "kunit_find_resource"), we need
to call [`kunit_put_resource()`](#c.kunit_put_resource "kunit_put_resource") to reduce the resource reference count
when finished with it. Note that [`kunit_alloc_resource()`](#c.kunit_alloc_resource "kunit_alloc_resource") does not require a
`kunit_resource_put()` because it does not retrieve the resource itself.

**Example**

```
struct kunit_kmalloc_params {
        size_t size;
        gfp_t gfp;
};

static int kunit_kmalloc_init(struct kunit_resource *res, void *context)
{
        struct kunit_kmalloc_params *params = context;
        res->data = kmalloc(params->size, params->gfp);

        if (!res->data)
                return -ENOMEM;

        return 0;
}

static void kunit_kmalloc_free(struct kunit_resource *res)
{
        kfree(res->data);
}

void *kunit_kmalloc(struct kunit *test, size_t size, gfp_t gfp)
{
        struct kunit_kmalloc_params params;

        params.size = size;
        params.gfp = gfp;

        return kunit_alloc_resource(test, kunit_kmalloc_init,
                kunit_kmalloc_free, gfp, &params);
}
```

Resources can also be named, with lookup/removal done on a name
basis also. [`kunit_add_named_resource()`](#c.kunit_add_named_resource "kunit_add_named_resource"), [`kunit_find_named_resource()`](#c.kunit_find_named_resource "kunit_find_named_resource")
and `kunit_destroy_named_resource()`. Resource names must be
unique within the test instance.

void kunit\_get\_resource(struct [kunit\_resource](#c.kunit_resource "kunit_resource") \*res)
:   Hold resource for use. Should not need to be used by most users as we automatically get resources retrieved by kunit\_find\_resource\*().

**Parameters**

`struct kunit_resource *res`
:   resource

void kunit\_put\_resource(struct [kunit\_resource](#c.kunit_resource "kunit_resource") \*res)
:   When caller is done with retrieved resource, [`kunit_put_resource()`](#c.kunit_put_resource "kunit_put_resource") should be called to drop reference count. The resource list maintains a reference count on resources, so if no users are utilizing a resource and it is removed from the resource list, it will be freed via the associated free function (if any). Only needs to be used if we `alloc_and_get()` or `find()` resource.

**Parameters**

`struct kunit_resource *res`
:   resource

int \_\_kunit\_add\_resource(struct [kunit](test.html#c.kunit "kunit") \*test, kunit\_resource\_init\_t init, kunit\_resource\_free\_t free, struct [kunit\_resource](#c.kunit_resource "kunit_resource") \*res, void \*data)
:   Internal helper to add a resource.

**Parameters**

`struct kunit *test`
:   The test context object.

`kunit_resource_init_t init`
:   a user-supplied function to initialize the result (if needed). If
    none is supplied, the resource data value is simply set to **data**.
    If an init function is supplied, **data** is passed to it instead.

`kunit_resource_free_t free`
:   a user-supplied function to free the resource (if needed).

`struct kunit_resource *res`
:   The resource.

`void *data`
:   value to pass to init function or set in resource data field.

**Description**

res->should\_kfree is not initialised.

int kunit\_add\_resource(struct [kunit](test.html#c.kunit "kunit") \*test, kunit\_resource\_init\_t init, kunit\_resource\_free\_t free, struct [kunit\_resource](#c.kunit_resource "kunit_resource") \*res, void \*data)
:   Add a *test managed resource*.

**Parameters**

`struct kunit *test`
:   The test context object.

`kunit_resource_init_t init`
:   a user-supplied function to initialize the result (if needed). If
    none is supplied, the resource data value is simply set to **data**.
    If an init function is supplied, **data** is passed to it instead.

`kunit_resource_free_t free`
:   a user-supplied function to free the resource (if needed).

`struct kunit_resource *res`
:   The resource.

`void *data`
:   value to pass to init function or set in resource data field.

int kunit\_add\_named\_resource(struct [kunit](test.html#c.kunit "kunit") \*test, kunit\_resource\_init\_t init, kunit\_resource\_free\_t free, struct [kunit\_resource](#c.kunit_resource "kunit_resource") \*res, const char \*name, void \*data)
:   Add a named *test managed resource*.

**Parameters**

`struct kunit *test`
:   The test context object.

`kunit_resource_init_t init`
:   a user-supplied function to initialize the resource data, if needed.

`kunit_resource_free_t free`
:   a user-supplied function to free the resource data, if needed.

`struct kunit_resource *res`
:   The resource.

`const char *name`
:   name to be set for resource.

`void *data`
:   value to pass to init function or set in resource data field.

struct [kunit\_resource](#c.kunit_resource "kunit_resource") \*kunit\_alloc\_and\_get\_resource(struct [kunit](test.html#c.kunit "kunit") \*test, kunit\_resource\_init\_t init, kunit\_resource\_free\_t free, gfp\_t internal\_gfp, void \*context)
:   Allocates and returns a *test managed resource*.

**Parameters**

`struct kunit *test`
:   The test context object.

`kunit_resource_init_t init`
:   a user supplied function to initialize the resource.

`kunit_resource_free_t free`
:   a user supplied function to free the resource (if needed).

`gfp_t internal_gfp`
:   gfp to use for internal allocations, if unsure, use GFP\_KERNEL

`void *context`
:   for the user to pass in arbitrary data to the init function.

**Description**

Allocates a *test managed resource*, a resource which will automatically be
cleaned up at the end of a test case. See [`struct kunit_resource`](#c.kunit_resource "kunit_resource") for an
example.

This is effectively identical to kunit\_alloc\_resource, but returns the
[`struct kunit_resource`](#c.kunit_resource "kunit_resource") pointer, not just the ‘data’ pointer. It therefore
also increments the resource’s refcount, so [`kunit_put_resource()`](#c.kunit_put_resource "kunit_put_resource") should be
called when you’ve finished with it.

**Note**

KUnit needs to allocate memory for a kunit\_resource object. You must
specify an **internal\_gfp** that is compatible with the use context of your
resource.

void \*kunit\_alloc\_resource(struct [kunit](test.html#c.kunit "kunit") \*test, kunit\_resource\_init\_t init, kunit\_resource\_free\_t free, gfp\_t internal\_gfp, void \*context)
:   Allocates a *test managed resource*.

**Parameters**

`struct kunit *test`
:   The test context object.

`kunit_resource_init_t init`
:   a user supplied function to initialize the resource.

`kunit_resource_free_t free`
:   a user supplied function to free the resource (if needed).

`gfp_t internal_gfp`
:   gfp to use for internal allocations, if unsure, use GFP\_KERNEL

`void *context`
:   for the user to pass in arbitrary data to the init function.

**Description**

Allocates a *test managed resource*, a resource which will automatically be
cleaned up at the end of a test case. See [`struct kunit_resource`](#c.kunit_resource "kunit_resource") for an
example.

**Note**

KUnit needs to allocate memory for a kunit\_resource object. You must
specify an **internal\_gfp** that is compatible with the use context of your
resource.

bool kunit\_resource\_name\_match(struct [kunit](test.html#c.kunit "kunit") \*test, struct [kunit\_resource](#c.kunit_resource "kunit_resource") \*res, void \*match\_name)
:   Match a resource with the same name.

**Parameters**

`struct kunit *test`
:   Test case to which the resource belongs.

`struct kunit_resource *res`
:   The resource.

`void *match_name`
:   The name to match against.

struct [kunit\_resource](#c.kunit_resource "kunit_resource") \*kunit\_find\_resource(struct [kunit](test.html#c.kunit "kunit") \*test, kunit\_resource\_match\_t match, void \*match\_data)
:   Find a resource using match function/data.

**Parameters**

`struct kunit *test`
:   Test case to which the resource belongs.

`kunit_resource_match_t match`
:   match function to be applied to resources/match data.

`void *match_data`
:   data to be used in matching.

struct [kunit\_resource](#c.kunit_resource "kunit_resource") \*kunit\_find\_named\_resource(struct [kunit](test.html#c.kunit "kunit") \*test, const char \*name)
:   Find a resource using match name.

**Parameters**

`struct kunit *test`
:   Test case to which the resource belongs.

`const char *name`
:   match name.

int kunit\_destroy\_resource(struct [kunit](test.html#c.kunit "kunit") \*test, kunit\_resource\_match\_t match, void \*match\_data)
:   Find a kunit\_resource and destroy it.

**Parameters**

`struct kunit *test`
:   Test case to which the resource belongs.

`kunit_resource_match_t match`
:   Match function. Returns whether a given resource matches **match\_data**.

`void *match_data`
:   Data passed into **match**.

**Return**

0 if kunit\_resource is found and freed, -ENOENT if not found.

void kunit\_remove\_resource(struct [kunit](test.html#c.kunit "kunit") \*test, struct [kunit\_resource](#c.kunit_resource "kunit_resource") \*res)
:   remove resource from resource list associated with test.

**Parameters**

`struct kunit *test`
:   The test context object.

`struct kunit_resource *res`
:   The resource to be removed.

**Description**

Note that the resource will not be immediately freed since it is likely
the caller has a reference to it via `alloc_and_get()` or `find()`;
in this case a final call to [`kunit_put_resource()`](#c.kunit_put_resource "kunit_put_resource") is required.

KUNIT\_DEFINE\_ACTION\_WRAPPER

`KUNIT_DEFINE_ACTION_WRAPPER (wrapper, orig, arg_type)`

> Wrap a function for use as a deferred action.

**Parameters**

`wrapper`
:   The name of the new wrapper function define.

`orig`
:   The original function to wrap.

`arg_type`
:   The type of the argument accepted by **orig**.

**Description**

Defines a wrapper for a function which accepts a single, pointer-sized
argument. This wrapper can then be passed to [`kunit_add_action()`](#c.kunit_add_action "kunit_add_action") and
similar. This should be used in preference to casting a function
directly to kunit\_action\_t, as casting function pointers will break
control flow integrity (CFI), leading to crashes.

int kunit\_add\_action(struct [kunit](test.html#c.kunit "kunit") \*test, kunit\_action\_t \*action, void \*ctx)
:   Call a function when the test ends.

**Parameters**

`struct kunit *test`
:   Test case to associate the action with.

`kunit_action_t *action`
:   The function to run on test exit

`void *ctx`
:   Data passed into **func**

**Description**

Defer the execution of a function until the test exits, either normally or
due to a failure. **ctx** is passed as additional context. All functions
registered with [`kunit_add_action()`](#c.kunit_add_action "kunit_add_action") will execute in the opposite order to that
they were registered in.

This is useful for cleaning up allocated memory and resources, as these
functions are called even if the test aborts early due to, e.g., a failed
assertion.

See also: `devm_add_action()` for the devres equivalent.

**Return**

0 on success, an error if the action could not be deferred.

int kunit\_add\_action\_or\_reset(struct [kunit](test.html#c.kunit "kunit") \*test, kunit\_action\_t \*action, void \*ctx)
:   Call a function when the test ends.

**Parameters**

`struct kunit *test`
:   Test case to associate the action with.

`kunit_action_t *action`
:   The function to run on test exit

`void *ctx`
:   Data passed into **func**

**Description**

Defer the execution of a function until the test exits, either normally or
due to a failure. **ctx** is passed as additional context. All functions
registered with [`kunit_add_action()`](#c.kunit_add_action "kunit_add_action") will execute in the opposite order to that
they were registered in.

This is useful for cleaning up allocated memory and resources, as these
functions are called even if the test aborts early due to, e.g., a failed
assertion.

If the action cannot be created (e.g., due to the system being out of memory),
then action(ctx) will be called immediately, and an error will be returned.

See also: `devm_add_action_or_reset()` for the devres equivalent.

**Return**

0 on success, an error if the action could not be deferred.

void kunit\_remove\_action(struct [kunit](test.html#c.kunit "kunit") \*test, kunit\_action\_t \*action, void \*ctx)
:   Cancel a matching deferred action.

**Parameters**

`struct kunit *test`
:   Test case the action is associated with.

`kunit_action_t *action`
:   The deferred function to cancel.

`void *ctx`
:   The context passed to the deferred function to trigger.

**Description**

Prevent an action deferred via [`kunit_add_action()`](#c.kunit_add_action "kunit_add_action") from executing when the
test terminates.

If the function/context pair was deferred multiple times, only the most
recent one will be cancelled.

See also: `devm_remove_action()` for the devres equivalent.

void kunit\_release\_action(struct [kunit](test.html#c.kunit "kunit") \*test, kunit\_action\_t \*action, void \*ctx)
:   Run a matching action call immediately.

**Parameters**

`struct kunit *test`
:   Test case the action is associated with.

`kunit_action_t *action`
:   The deferred function to trigger.

`void *ctx`
:   The context passed to the deferred function to trigger.

**Description**

Execute a function deferred via [`kunit_add_action()`](#c.kunit_add_action "kunit_add_action")) immediately, rather than
when the test ends.

If the function/context pair was deferred multiple times, it will only be
executed once here. The most recent deferral will no longer execute when
the test ends.

kunit\_release\_action(test, func, ctx);
is equivalent to
func(ctx);
kunit\_remove\_action(test, func, ctx);

See also: [`devm_release_action()`](../../../driver-api/basics.html#c.devm_release_action "devm_release_action") for the devres equivalent.

## Managed Devices

Functions for using KUnit-managed [`struct device`](../../../driver-api/infrastructure.html#c.device "device") and [`struct device_driver`](../../../driver-api/infrastructure.html#c.device_driver "device_driver").
Include `kunit/device.h` to use these.

struct [device\_driver](../../../driver-api/infrastructure.html#c.device_driver "device_driver") \*kunit\_driver\_create(struct [kunit](test.html#c.kunit "kunit") \*test, const char \*name)
:   Create a [`struct device_driver`](../../../driver-api/infrastructure.html#c.device_driver "device_driver") attached to the kunit\_bus

**Parameters**

`struct kunit *test`
:   The test context object.

`const char *name`
:   The name to give the created driver.

**Description**

Creates a [`struct device_driver`](../../../driver-api/infrastructure.html#c.device_driver "device_driver") attached to the kunit\_bus, with the name **name**.
This driver will automatically be cleaned up on test exit.

**Return**

a stub [`struct device_driver`](../../../driver-api/infrastructure.html#c.device_driver "device_driver"), managed by KUnit, with the name **name**.

struct [device](../../../driver-api/infrastructure.html#c.device "device") \*kunit\_device\_register(struct [kunit](test.html#c.kunit "kunit") \*test, const char \*name)
:   Create a [`struct device`](../../../driver-api/infrastructure.html#c.device "device") for use in KUnit tests

**Parameters**

`struct kunit *test`
:   The test context object.

`const char *name`
:   The name to give the created device.

**Description**

Creates a `struct kunit_device` (which is a [`struct device`](../../../driver-api/infrastructure.html#c.device "device")) with the given name,
and a corresponding driver. The device and driver will be cleaned up on test
exit, or when kunit\_device\_unregister is called. See also
kunit\_device\_register\_with\_driver, if you wish to provide your own
[`struct device_driver`](../../../driver-api/infrastructure.html#c.device_driver "device_driver").

**Return**

a pointer to a [`struct device`](../../../driver-api/infrastructure.html#c.device "device") which will be cleaned up when the test
exits, or an error pointer if the device could not be allocated or registered.

struct [device](../../../driver-api/infrastructure.html#c.device "device") \*kunit\_device\_register\_with\_driver(struct [kunit](test.html#c.kunit "kunit") \*test, const char \*name, const struct [device\_driver](../../../driver-api/infrastructure.html#c.device_driver "device_driver") \*drv)
:   Create a [`struct device`](../../../driver-api/infrastructure.html#c.device "device") for use in KUnit tests

**Parameters**

`struct kunit *test`
:   The test context object.

`const char *name`
:   The name to give the created device.

`const struct device_driver *drv`
:   The [`struct device_driver`](../../../driver-api/infrastructure.html#c.device_driver "device_driver") to associate with the device.

**Description**

Creates a `struct kunit_device` (which is a [`struct device`](../../../driver-api/infrastructure.html#c.device "device")) with the given
name, and driver. The device will be cleaned up on test exit, or when
kunit\_device\_unregister is called. See also kunit\_device\_register, if you
wish KUnit to create and manage a driver for you.

**Return**

a pointer to a [`struct device`](../../../driver-api/infrastructure.html#c.device "device") which will be cleaned up when the test
exits, or an error pointer if the device could not be allocated or registered.

void kunit\_device\_unregister(struct [kunit](test.html#c.kunit "kunit") \*test, struct [device](../../../driver-api/infrastructure.html#c.device "device") \*dev)
:   Unregister a KUnit-managed device

**Parameters**

`struct kunit *test`
:   The test context object which created the device

`struct device *dev`
:   The device.

**Description**

Unregisters and destroys a [`struct device`](../../../driver-api/infrastructure.html#c.device "device") which was created with
kunit\_device\_register or kunit\_device\_register\_with\_driver. If KUnit created
a driver, cleans it up as well.
