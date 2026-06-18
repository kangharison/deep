# The object-lifetime debugging infrastructure

> 출처(원문): https://docs.kernel.org/core-api/debug-objects.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# The object-lifetime debugging infrastructure

Author:
:   Thomas Gleixner

## Introduction

debugobjects is a generic infrastructure to track the life time of
kernel objects and validate the operations on those.

debugobjects is useful to check for the following error patterns:

* Activation of uninitialized objects
* Initialization of active objects
* Usage of freed/destroyed objects

debugobjects is not changing the data structure of the real object so it
can be compiled in with a minimal runtime impact and enabled on demand
with a kernel command line option.

## Howto use debugobjects

A kernel subsystem needs to provide a data structure which describes the
object type and add calls into the debug code at appropriate places. The
data structure to describe the object type needs at minimum the name of
the object type. Optional functions can and should be provided to fixup
detected problems so the kernel can continue to work and the debug
information can be retrieved from a live system instead of hard core
debugging with serial consoles and stack trace transcripts from the
monitor.

The debug calls provided by debugobjects are:

* debug\_object\_init
* debug\_object\_init\_on\_stack
* debug\_object\_activate
* debug\_object\_deactivate
* debug\_object\_destroy
* debug\_object\_free
* debug\_object\_assert\_init

Each of these functions takes the address of the real object and a
pointer to the object type specific debug description structure.

Each detected error is reported in the statistics and a limited number
of errors are printk’ed including a full stack trace.

The statistics are available via /sys/kernel/debug/debug\_objects/stats.
They provide information about the number of warnings and the number of
successful fixups along with information about the usage of the internal
tracking objects and the state of the internal tracking objects pool.

## Debug functions

void debug\_object\_init(void \*addr, const struct [debug\_obj\_descr](#c.debug_obj_descr "debug_obj_descr") \*descr)
:   debug checks when an object is initialized

**Parameters**

`void *addr`
:   address of the object

`const struct debug_obj_descr *descr`
:   pointer to an object specific debug description structure

This function is called whenever the initialization function of a real
object is called.

When the real object is already tracked by debugobjects it is checked,
whether the object can be initialized. Initializing is not allowed for
active and destroyed objects. When debugobjects detects an error, then
it calls the fixup\_init function of the object type description
structure if provided by the caller. The fixup function can correct the
problem before the real initialization of the object happens. E.g. it
can deactivate an active object in order to prevent damage to the
subsystem.

When the real object is not yet tracked by debugobjects, debugobjects
allocates a tracker object for the real object and sets the tracker
object state to ODEBUG\_STATE\_INIT. It verifies that the object is not
on the callers stack. If it is on the callers stack then a limited
number of warnings including a full stack trace is printk’ed. The
calling code must use [`debug_object_init_on_stack()`](#c.debug_object_init_on_stack "debug_object_init_on_stack") and remove the
object before leaving the function which allocated it. See next section.

void debug\_object\_init\_on\_stack(void \*addr, const struct [debug\_obj\_descr](#c.debug_obj_descr "debug_obj_descr") \*descr)
:   debug checks when an object on stack is initialized

**Parameters**

`void *addr`
:   address of the object

`const struct debug_obj_descr *descr`
:   pointer to an object specific debug description structure

This function is called whenever the initialization function of a real
object which resides on the stack is called.

When the real object is already tracked by debugobjects it is checked,
whether the object can be initialized. Initializing is not allowed for
active and destroyed objects. When debugobjects detects an error, then
it calls the fixup\_init function of the object type description
structure if provided by the caller. The fixup function can correct the
problem before the real initialization of the object happens. E.g. it
can deactivate an active object in order to prevent damage to the
subsystem.

When the real object is not yet tracked by debugobjects debugobjects
allocates a tracker object for the real object and sets the tracker
object state to ODEBUG\_STATE\_INIT. It verifies that the object is on
the callers stack.

An object which is on the stack must be removed from the tracker by
calling [`debug_object_free()`](#c.debug_object_free "debug_object_free") before the function which allocates the
object returns. Otherwise we keep track of stale objects.

int debug\_object\_activate(void \*addr, const struct [debug\_obj\_descr](#c.debug_obj_descr "debug_obj_descr") \*descr)
:   debug checks when an object is activated

**Parameters**

`void *addr`
:   address of the object

`const struct debug_obj_descr *descr`
:   pointer to an object specific debug description structure
    Returns 0 for success, -EINVAL for check failed.

This function is called whenever the activation function of a real
object is called.

When the real object is already tracked by debugobjects it is checked,
whether the object can be activated. Activating is not allowed for
active and destroyed objects. When debugobjects detects an error, then
it calls the fixup\_activate function of the object type description
structure if provided by the caller. The fixup function can correct the
problem before the real activation of the object happens. E.g. it can
deactivate an active object in order to prevent damage to the subsystem.

When the real object is not yet tracked by debugobjects then the
fixup\_activate function is called if available. This is necessary to
allow the legitimate activation of statically allocated and initialized
objects. The fixup function checks whether the object is valid and calls
the `debug_objects_init()` function to initialize the tracking of this
object.

When the activation is legitimate, then the state of the associated
tracker object is set to ODEBUG\_STATE\_ACTIVE.

void debug\_object\_deactivate(void \*addr, const struct [debug\_obj\_descr](#c.debug_obj_descr "debug_obj_descr") \*descr)
:   debug checks when an object is deactivated

**Parameters**

`void *addr`
:   address of the object

`const struct debug_obj_descr *descr`
:   pointer to an object specific debug description structure

This function is called whenever the deactivation function of a real
object is called.

When the real object is tracked by debugobjects it is checked, whether
the object can be deactivated. Deactivating is not allowed for untracked
or destroyed objects.

When the deactivation is legitimate, then the state of the associated
tracker object is set to ODEBUG\_STATE\_INACTIVE.

void debug\_object\_destroy(void \*addr, const struct [debug\_obj\_descr](#c.debug_obj_descr "debug_obj_descr") \*descr)
:   debug checks when an object is destroyed

**Parameters**

`void *addr`
:   address of the object

`const struct debug_obj_descr *descr`
:   pointer to an object specific debug description structure

This function is called to mark an object destroyed. This is useful to
prevent the usage of invalid objects, which are still available in
memory: either statically allocated objects or objects which are freed
later.

When the real object is tracked by debugobjects it is checked, whether
the object can be destroyed. Destruction is not allowed for active and
destroyed objects. When debugobjects detects an error, then it calls the
fixup\_destroy function of the object type description structure if
provided by the caller. The fixup function can correct the problem
before the real destruction of the object happens. E.g. it can
deactivate an active object in order to prevent damage to the subsystem.

When the destruction is legitimate, then the state of the associated
tracker object is set to ODEBUG\_STATE\_DESTROYED.

void debug\_object\_free(void \*addr, const struct [debug\_obj\_descr](#c.debug_obj_descr "debug_obj_descr") \*descr)
:   debug checks when an object is freed

**Parameters**

`void *addr`
:   address of the object

`const struct debug_obj_descr *descr`
:   pointer to an object specific debug description structure

This function is called before an object is freed.

When the real object is tracked by debugobjects it is checked, whether
the object can be freed. Free is not allowed for active objects. When
debugobjects detects an error, then it calls the fixup\_free function of
the object type description structure if provided by the caller. The
fixup function can correct the problem before the real free of the
object happens. E.g. it can deactivate an active object in order to
prevent damage to the subsystem.

Note that debug\_object\_free removes the object from the tracker. Later
usage of the object is detected by the other debug checks.

void debug\_object\_assert\_init(void \*addr, const struct [debug\_obj\_descr](#c.debug_obj_descr "debug_obj_descr") \*descr)
:   debug checks when object should be init-ed

**Parameters**

`void *addr`
:   address of the object

`const struct debug_obj_descr *descr`
:   pointer to an object specific debug description structure

This function is called to assert that an object has been initialized.

When the real object is not tracked by debugobjects, it calls
fixup\_assert\_init of the object type description structure provided by
the caller, with the hardcoded object state ODEBUG\_NOT\_AVAILABLE. The
fixup function can correct the problem by calling debug\_object\_init
and other specific initializing functions.

When the real object is already tracked by debugobjects it is ignored.

## Fixup functions

### Debug object type description structure

struct debug\_obj
:   representation of an tracked object

**Definition**:

```
struct debug_obj {
    struct hlist_node               node;
    enum debug_obj_state            state;
    unsigned int                    astate;
    union {
        void *object;
        struct hlist_node       *batch_last;
    };
    const struct debug_obj_descr *descr;
};
```

**Members**

`node`
:   hlist node to link the object into the tracker list

`state`
:   tracked object state

`astate`
:   current active state

`{unnamed_union}`
:   anonymous

`object`
:   pointer to the real object

`batch_last`
:   pointer to the last hlist node in a batch

`descr`
:   pointer to an object type specific debug description structure

struct debug\_obj\_descr
:   object type specific debug description structure

**Definition**:

```
struct debug_obj_descr {
    const char              *name;
    void *(*debug_hint)(void *addr);
    bool (*is_static_object)(void *addr);
    bool (*fixup_init)(void *addr, enum debug_obj_state state);
    bool (*fixup_activate)(void *addr, enum debug_obj_state state);
    bool (*fixup_destroy)(void *addr, enum debug_obj_state state);
    bool (*fixup_free)(void *addr, enum debug_obj_state state);
    bool (*fixup_assert_init)(void *addr, enum debug_obj_state state);
};
```

**Members**

`name`
:   name of the object typee

`debug_hint`
:   function returning address, which have associated
    kernel symbol, to allow identify the object

`is_static_object`
:   return true if the obj is static, otherwise return false

`fixup_init`
:   fixup function, which is called when the init check
    fails. All fixup functions must return true if fixup
    was successful, otherwise return false

`fixup_activate`
:   fixup function, which is called when the activate check
    fails

`fixup_destroy`
:   fixup function, which is called when the destroy check
    fails

`fixup_free`
:   fixup function, which is called when the free check
    fails

`fixup_assert_init`
:   fixup function, which is called when the assert\_init
    check fails

### fixup\_init

This function is called from the debug code whenever a problem in
debug\_object\_init is detected. The function takes the address of the
object and the state which is currently recorded in the tracker.

Called from debug\_object\_init when the object state is:

* ODEBUG\_STATE\_ACTIVE

The function returns true when the fixup was successful, otherwise
false. The return value is used to update the statistics.

Note, that the function needs to call the [`debug_object_init()`](#c.debug_object_init "debug_object_init") function
again, after the damage has been repaired in order to keep the state
consistent.

### fixup\_activate

This function is called from the debug code whenever a problem in
debug\_object\_activate is detected.

Called from debug\_object\_activate when the object state is:

* ODEBUG\_STATE\_NOTAVAILABLE
* ODEBUG\_STATE\_ACTIVE

The function returns true when the fixup was successful, otherwise
false. The return value is used to update the statistics.

Note that the function needs to call the [`debug_object_activate()`](#c.debug_object_activate "debug_object_activate")
function again after the damage has been repaired in order to keep the
state consistent.

The activation of statically initialized objects is a special case. When
[`debug_object_activate()`](#c.debug_object_activate "debug_object_activate") has no tracked object for this object address
then `fixup_activate()` is called with object state
ODEBUG\_STATE\_NOTAVAILABLE. The fixup function needs to check whether
this is a legitimate case of a statically initialized object or not. In
case it is it calls [`debug_object_init()`](#c.debug_object_init "debug_object_init") and [`debug_object_activate()`](#c.debug_object_activate "debug_object_activate")
to make the object known to the tracker and marked active. In this case
the function should return false because this is not a real fixup.

### fixup\_destroy

This function is called from the debug code whenever a problem in
debug\_object\_destroy is detected.

Called from debug\_object\_destroy when the object state is:

* ODEBUG\_STATE\_ACTIVE

The function returns true when the fixup was successful, otherwise
false. The return value is used to update the statistics.

### fixup\_free

This function is called from the debug code whenever a problem in
debug\_object\_free is detected. Further it can be called from the debug
checks in kfree/vfree, when an active object is detected from the
`debug_check_no_obj_freed()` sanity checks.

Called from [`debug_object_free()`](#c.debug_object_free "debug_object_free") or `debug_check_no_obj_freed()` when
the object state is:

* ODEBUG\_STATE\_ACTIVE

The function returns true when the fixup was successful, otherwise
false. The return value is used to update the statistics.

### fixup\_assert\_init

This function is called from the debug code whenever a problem in
debug\_object\_assert\_init is detected.

Called from [`debug_object_assert_init()`](#c.debug_object_assert_init "debug_object_assert_init") with a hardcoded state
ODEBUG\_STATE\_NOTAVAILABLE when the object is not found in the debug
bucket.

The function returns true when the fixup was successful, otherwise
false. The return value is used to update the statistics.

Note, this function should make sure [`debug_object_init()`](#c.debug_object_init "debug_object_init") is called
before returning.

The handling of statically initialized objects is a special case. The
fixup function should check if this is a legitimate case of a statically
initialized object or not. In this case only [`debug_object_init()`](#c.debug_object_init "debug_object_init")
should be called to make the object known to the tracker. Then the
function should return false because this is not a real fixup.

## Known Bugs And Assumptions

None (knock on wood).
