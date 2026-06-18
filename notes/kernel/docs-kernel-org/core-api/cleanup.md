# Scope-based Cleanup Helpers

> 출처(원문): https://docs.kernel.org/core-api/cleanup.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Scope-based Cleanup Helpers

The “goto error” pattern is notorious for introducing subtle resource
leaks. It is tedious and error prone to add new resource acquisition
constraints into code paths that already have several unwind
conditions. The “cleanup” helpers enable the compiler to help with
this tedium and can aid in maintaining LIFO (last in first out)
unwind ordering to avoid unintentional leaks.

As drivers make up the majority of the kernel code base, here is an
example of using these helpers to clean up PCI drivers. The target of
the cleanups are occasions where a goto is used to unwind a device
reference ([`pci_dev_put()`](../driver-api/pci/pci.html#c.pci_dev_put "pci_dev_put")), or unlock the device (`pci_dev_unlock()`)
before returning.

The `DEFINE_FREE()` macro can arrange for PCI device references to be
dropped when the associated variable goes out of scope:

```
DEFINE_FREE(pci_dev_put, struct pci_dev *, if (_T) pci_dev_put(_T))
...
struct pci_dev *dev __free(pci_dev_put) =
        pci_get_slot(parent, PCI_DEVFN(0, 0));
```

The above will automatically call [`pci_dev_put()`](../driver-api/pci/pci.html#c.pci_dev_put "pci_dev_put") if **dev** is non-NULL
when **dev** goes out of scope (automatic variable scope). If a function
wants to invoke [`pci_dev_put()`](../driver-api/pci/pci.html#c.pci_dev_put "pci_dev_put") on error, but return **dev** (i.e. without
freeing it) on success, it can do:

```
return no_free_ptr(dev);
```

...or:

```
return_ptr(dev);
```

The `DEFINE_GUARD()` macro can arrange for the PCI device lock to be
dropped when the scope where `guard()` is invoked ends:

```
DEFINE_GUARD(pci_dev, struct pci_dev *, pci_dev_lock(_T), pci_dev_unlock(_T))
...
guard(pci_dev)(dev);
```

The lifetime of the lock obtained by the `guard()` helper follows the
scope of automatic variable declaration. Take the following example:

```
func(...)
{
        if (...) {
                ...
                guard(pci_dev)(dev); // pci_dev_lock() invoked here
                ...
        } // <- implied pci_dev_unlock() triggered here
}
```

Observe the lock is held for the remainder of the “if ()” block not
the remainder of “`func()`”.

The `ACQUIRE()` macro can be used in all places that `guard()` can be
used and additionally support conditional locks:

```
DEFINE_GUARD_COND(pci_dev, _try, pci_dev_trylock(_T))
...
ACQUIRE(pci_dev_try, lock)(dev);
rc = ACQUIRE_ERR(pci_dev_try, &lock);
if (rc)
        return rc;
// @lock is held
```

Now, when a function uses both `__free()` and `guard()`/`ACQUIRE()`, or
multiple instances of `__free()`, the LIFO order of variable definition
order matters. GCC documentation says:

“When multiple variables in the same scope have cleanup attributes,
at exit from the scope their associated cleanup functions are run in
reverse order of definition (last defined, first cleanup).”

When the unwind order matters it requires that variables be defined
mid-function scope rather than at the top of the file. Take the
following example and notice the bug highlighted by “!!”:

```
LIST_HEAD(list);
DEFINE_MUTEX(lock);

struct object {
        struct list_head node;
};

static struct object *alloc_add(void)
{
        struct object *obj;

        lockdep_assert_held(&lock);
        obj = kzalloc(sizeof(*obj), GFP_KERNEL);
        if (obj) {
                LIST_HEAD_INIT(&obj->node);
                list_add(obj->node, &list):
        }
        return obj;
}

static void remove_free(struct object *obj)
{
        lockdep_assert_held(&lock);
        list_del(&obj->node);
        kfree(obj);
}

DEFINE_FREE(remove_free, struct object *, if (_T) remove_free(_T))
static int init(void)
{
        struct object *obj __free(remove_free) = NULL;
        int err;

        guard(mutex)(&lock);
        obj = alloc_add();

        if (!obj)
                return -ENOMEM;

        err = other_init(obj);
        if (err)
                return err; // remove_free() called without the lock!!

        no_free_ptr(obj);
        return 0;
}
```

That bug is fixed by changing `init()` to call `guard()` and define +
initialize **obj** in this order:

```
guard(mutex)(&lock);
struct object *obj __free(remove_free) = alloc_add();
```

Given that the “\_\_free(...) = NULL” pattern for variables defined at
the top of the function poses this potential interdependency problem
the recommendation is to always define and assign variables in one
statement and not group variable definitions at the top of the
function when `__free()` is used.

Lastly, given that the benefit of cleanup helpers is removal of
“goto”, and that the “goto” statement can jump between scopes, the
expectation is that usage of “goto” and cleanup helpers is never
mixed in the same function. I.e. for a given routine, convert all
resources that need a “goto” cleanup to scope-based cleanup, or
convert none of them.
