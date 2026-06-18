# Livepatching APIs

> 출처(원문): https://docs.kernel.org/livepatch/api.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Livepatching APIs

## Livepatch Enablement

int klp\_enable\_patch(struct [klp\_patch](#c.klp_patch "klp_patch") \*patch)
:   enable the livepatch

**Parameters**

`struct klp_patch *patch`
:   patch to be enabled

**Description**

Initializes the data structure associated with the patch, creates the sysfs
interface, performs the needed symbol lookups and code relocations,
registers the patched functions with ftrace.

This function is supposed to be called from the livepatch [`module_init()`](../driver-api/basics.html#c.module_init "module_init")
callback.

**Return**

0 on success, otherwise error

## Shadow Variables

void \*klp\_shadow\_get(void \*obj, unsigned long id)
:   retrieve a shadow variable data pointer

**Parameters**

`void *obj`
:   pointer to parent object

`unsigned long id`
:   data identifier

**Return**

the shadow variable data element, NULL on failure.

void \*klp\_shadow\_alloc(void \*obj, unsigned long id, size\_t size, gfp\_t gfp\_flags, klp\_shadow\_ctor\_t ctor, void \*ctor\_data)
:   allocate and add a new shadow variable

**Parameters**

`void *obj`
:   pointer to parent object

`unsigned long id`
:   data identifier

`size_t size`
:   size of attached data

`gfp_t gfp_flags`
:   GFP mask for allocation

`klp_shadow_ctor_t ctor`
:   custom constructor to initialize the shadow data (optional)

`void *ctor_data`
:   pointer to any data needed by **ctor** (optional)

**Description**

Allocates **size** bytes for new shadow variable data using **gfp\_flags**.
The data are zeroed by default. They are further initialized by **ctor**
function if it is not NULL. The new shadow variable is then added
to the global hashtable.

If an existing <obj, id> shadow variable can be found, this routine will
issue a WARN, exit early and return NULL.

This function guarantees that the constructor function is called only when
the variable did not exist before. The cost is that **ctor** is called
in atomic context under a spin lock.

**Return**

the shadow variable data element, NULL on duplicate or
failure.

void \*klp\_shadow\_get\_or\_alloc(void \*obj, unsigned long id, size\_t size, gfp\_t gfp\_flags, klp\_shadow\_ctor\_t ctor, void \*ctor\_data)
:   get existing or allocate a new shadow variable

**Parameters**

`void *obj`
:   pointer to parent object

`unsigned long id`
:   data identifier

`size_t size`
:   size of attached data

`gfp_t gfp_flags`
:   GFP mask for allocation

`klp_shadow_ctor_t ctor`
:   custom constructor to initialize the shadow data (optional)

`void *ctor_data`
:   pointer to any data needed by **ctor** (optional)

**Description**

Returns a pointer to existing shadow data if an <obj, id> shadow
variable is already present. Otherwise, it creates a new shadow
variable like [`klp_shadow_alloc()`](#c.klp_shadow_alloc "klp_shadow_alloc").

This function guarantees that only one shadow variable exists with the given
**id** for the given **obj**. It also guarantees that the constructor function
will be called only when the variable did not exist before. The cost is
that **ctor** is called in atomic context under a spin lock.

**Return**

the shadow variable data element, NULL on failure.

void klp\_shadow\_free(void \*obj, unsigned long id, klp\_shadow\_dtor\_t dtor)
:   detach and free a <obj, id> shadow variable

**Parameters**

`void *obj`
:   pointer to parent object

`unsigned long id`
:   data identifier

`klp_shadow_dtor_t dtor`
:   custom callback that can be used to unregister the variable
    and/or free data that the shadow variable points to (optional)

**Description**

This function releases the memory for this <obj, id> shadow variable
instance, callers should stop referencing it accordingly.

void klp\_shadow\_free\_all(unsigned long id, klp\_shadow\_dtor\_t dtor)
:   detach and free all <\_, id> shadow variables

**Parameters**

`unsigned long id`
:   data identifier

`klp_shadow_dtor_t dtor`
:   custom callback that can be used to unregister the variable
    and/or free data that the shadow variable points to (optional)

**Description**

This function releases the memory for all <\_, id> shadow variable
instances, callers should stop referencing them accordingly.

## System State Changes

struct [klp\_state](#c.klp_state "klp_state") \*klp\_get\_state(struct [klp\_patch](#c.klp_patch "klp_patch") \*patch, unsigned long id)
:   get information about system state modified by the given patch

**Parameters**

`struct klp_patch *patch`
:   livepatch that modifies the given system state

`unsigned long id`
:   custom identifier of the modified system state

**Description**

Checks whether the given patch modifies the given system state.

The function can be called either from pre/post (un)patch
callbacks or from the kernel code added by the livepatch.

**Return**

pointer to [`struct klp_state`](#c.klp_state "klp_state") when found, otherwise NULL.

struct [klp\_state](#c.klp_state "klp_state") \*klp\_get\_prev\_state(unsigned long id)
:   get information about system state modified by the already installed livepatches

**Parameters**

`unsigned long id`
:   custom identifier of the modified system state

**Description**

Checks whether already installed livepatches modify the given
system state.

The same system state can be modified by more non-cumulative
livepatches. It is expected that the latest livepatch has
the most up-to-date information.

The function can be called only during transition when a new
livepatch is being enabled or when such a transition is reverted.
It is typically called only from pre/post (un)patch
callbacks.

**Return**

pointer to the latest [`struct klp_state`](#c.klp_state "klp_state") from already
installed livepatches, NULL when not found.

## Object Types

struct klp\_func
:   function structure for live patching

**Definition**:

```
struct klp_func {
    const char *old_name;
    void *new_func;
    unsigned long old_sympos;
    void *old_func;
    struct kobject kobj;
    struct list_head node;
    struct list_head stack_node;
    unsigned long old_size, new_size;
    bool nop;
    bool patched;
    bool transition;
};
```

**Members**

`old_name`
:   name of the function to be patched

`new_func`
:   pointer to the patched function code

`old_sympos`
:   a hint indicating which symbol position the old function
    can be found (optional)

`old_func`
:   pointer to the function being patched

`kobj`
:   kobject for sysfs resources

`node`
:   list node for klp\_object func\_list

`stack_node`
:   list node for klp\_ops func\_stack list

`old_size`
:   size of the old function

`new_size`
:   size of the new function

`nop`
:   temporary patch to use the original code again; dyn. allocated

`patched`
:   the func has been added to the klp\_ops list

`transition`
:   the func is currently being applied or reverted

**Description**

The patched and transition variables define the func’s patching state. When
patching, a func is always in one of the following states:

> patched=0 transition=0: unpatched
> patched=0 transition=1: unpatched, temporary starting state
> patched=1 transition=1: patched, may be visible to some tasks
> patched=1 transition=0: patched, visible to all tasks

And when unpatching, it goes in the reverse order:

> patched=1 transition=0: patched, visible to all tasks
> patched=1 transition=1: patched, may be visible to some tasks
> patched=0 transition=1: unpatched, temporary ending state
> patched=0 transition=0: unpatched

struct klp\_object
:   kernel object structure for live patching

**Definition**:

```
struct klp_object {
    const char *name;
    struct klp_func *funcs;
    struct klp_callbacks callbacks;
    struct kobject kobj;
    struct list_head func_list;
    struct list_head node;
    struct module *mod;
    bool dynamic;
    bool patched;
};
```

**Members**

`name`
:   module name (or NULL for vmlinux)

`funcs`
:   function entries for functions to be patched in the object

`callbacks`
:   functions to be executed pre/post (un)patching

`kobj`
:   kobject for sysfs resources

`func_list`
:   dynamic list of the function entries

`node`
:   list node for klp\_patch obj\_list

`mod`
:   kernel module associated with the patched object
    (NULL for vmlinux)

`dynamic`
:   temporary object for nop functions; dynamically allocated

`patched`
:   the object’s funcs have been added to the klp\_ops list

struct klp\_state
:   state of the system modified by the livepatch

**Definition**:

```
struct klp_state {
    unsigned long id;
    unsigned int version;
    void *data;
};
```

**Members**

`id`
:   system state identifier (non-zero)

`version`
:   version of the change

`data`
:   custom data

struct klp\_patch
:   patch structure for live patching

**Definition**:

```
struct klp_patch {
    struct module *mod;
    struct klp_object *objs;
    struct klp_state *states;
    bool replace;
    struct list_head list;
    struct kobject kobj;
    struct list_head obj_list;
    bool enabled;
    bool forced;
    struct work_struct free_work;
    struct completion finish;
};
```

**Members**

`mod`
:   reference to the live patch module

`objs`
:   object entries for kernel objects to be patched

`states`
:   system states that can get modified

`replace`
:   replace all actively used patches

`list`
:   list node for global list of actively used patches

`kobj`
:   kobject for sysfs resources

`obj_list`
:   dynamic list of the object entries

`enabled`
:   the patch is enabled (but operation may be incomplete)

`forced`
:   was involved in a forced transition

`free_work`
:   patch cleanup from workqueue-context

`finish`
:   for waiting till it is safe to remove the patch module
