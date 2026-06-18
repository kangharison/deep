# Fprobe - Function entry/exit probe

> 출처(원문): https://docs.kernel.org/trace/fprobe.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Fprobe - Function entry/exit probe

## Introduction

Fprobe is a function entry/exit probe based on the function-graph tracing
feature in ftrace.
Instead of tracing all functions, if you want to attach callbacks on specific
function entry and exit, similar to the kprobes and kretprobes, you can
use fprobe. Compared with kprobes and kretprobes, fprobe gives faster
instrumentation for multiple functions with single handler. This document
describes how to use fprobe.

## The usage of fprobe

The fprobe is a wrapper of ftrace (+ kretprobe-like return callback) to
attach callbacks to multiple function entry and exit. User needs to set up
the [`struct fprobe`](#c.fprobe "fprobe") and pass it to [`register_fprobe()`](#c.register_fprobe "register_fprobe").

Typically, fprobe data structure is initialized with the entry\_handler
and/or exit\_handler as below.

```
struct fprobe fp = {
       .entry_handler  = my_entry_callback,
       .exit_handler   = my_exit_callback,
};
```

To enable the fprobe, call one of [`register_fprobe()`](#c.register_fprobe "register_fprobe"), [`register_fprobe_ips()`](#c.register_fprobe_ips "register_fprobe_ips"), and
[`register_fprobe_syms()`](#c.register_fprobe_syms "register_fprobe_syms"). These functions register the fprobe with different types
of parameters.

The [`register_fprobe()`](#c.register_fprobe "register_fprobe") enables a fprobe by function-name filters.
E.g. this enables @fp on “func\*()” function except “`func2()`”.:

```
register_fprobe(&fp, "func*", "func2");
```

The [`register_fprobe_ips()`](#c.register_fprobe_ips "register_fprobe_ips") enables a fprobe by ftrace-location addresses.
E.g.

```
unsigned long ips[] = { 0x.... };

register_fprobe_ips(&fp, ips, ARRAY_SIZE(ips));
```

And the [`register_fprobe_syms()`](#c.register_fprobe_syms "register_fprobe_syms") enables a fprobe by symbol names.
E.g.

```
char syms[] = {"func1", "func2", "func3"};

register_fprobe_syms(&fp, syms, ARRAY_SIZE(syms));
```

To disable (remove from functions) this fprobe, call:

```
unregister_fprobe(&fp);
```

You can temporally (soft) disable the fprobe by:

```
disable_fprobe(&fp);
```

and resume by:

```
enable_fprobe(&fp);
```

The above is defined by including the header:

```
#include <linux/fprobe.h>
```

Same as ftrace, the registered callbacks will start being called some time
after the [`register_fprobe()`](#c.register_fprobe "register_fprobe") is called and before it returns. See
[ftrace - Function Tracer](ftrace.html).

Also, the [`unregister_fprobe()`](#c.unregister_fprobe "unregister_fprobe") will guarantee that both enter and exit
handlers are no longer being called by functions after [`unregister_fprobe()`](#c.unregister_fprobe "unregister_fprobe")
returns as same as `unregister_ftrace_function()`.

## The fprobe entry/exit handler

The prototype of the entry/exit callback function are as follows:

```
int entry_callback(struct fprobe *fp, unsigned long entry_ip, unsigned long ret_ip, struct ftrace_regs *fregs, void *entry_data);

void exit_callback(struct fprobe *fp, unsigned long entry_ip, unsigned long ret_ip, struct ftrace_regs *fregs, void *entry_data);
```

Note that the @entry\_ip is saved at function entry and passed to exit
handler.
If the entry callback function returns !0, the corresponding exit callback
will be cancelled.

@fp
:   This is the address of fprobe data structure related to this handler.
    You can embed the fprobe to your data structure and get it by
    [`container_of()`](../driver-api/basics.html#c.container_of "container_of") macro from @fp. The @fp must not be NULL.

@entry\_ip
:   This is the ftrace address of the traced function (both entry and exit).
    Note that this may not be the actual entry address of the function but
    the address where the ftrace is instrumented.

@ret\_ip
:   This is the return address that the traced function will return to,
    somewhere in the caller. This can be used at both entry and exit.

@fregs
:   This is the ftrace\_regs data structure at the entry and exit. This
    includes the function parameters, or the return values. So user can
    access thos values via appropriate ftrace\_regs\_\* APIs.

@entry\_data
:   This is a local storage to share the data between entry and exit handlers.
    This storage is NULL by default. If the user specify exit\_handler field
    and entry\_data\_size field when registering the fprobe, the storage is
    allocated and passed to both entry\_handler and exit\_handler.

## Entry data size and exit handlers on the same function

Since the entry data is passed via per-task stack and it has limited size,
the entry data size per probe is limited to 15 \* sizeof(long). You also need
to take care that the different fprobes are probing on the same function, this
limit becomes smaller. The entry data size is aligned to sizeof(long) and
each fprobe which has exit handler uses a sizeof(long) space on the stack,
you should keep the number of fprobes on the same function as small as
possible.

## Share the callbacks with kprobes

Since the recursion safeness of the fprobe (and ftrace) is a bit different
from the kprobes, this may cause an issue if user wants to run the same
code from the fprobe and the kprobes.

Kprobes has per-cpu ‘current\_kprobe’ variable which protects the kprobe
handler from recursion in all cases. On the other hand, fprobe uses
only `ftrace_test_recursion_trylock()`. This allows interrupt context to
call another (or same) fprobe while the fprobe user handler is running.

This is not a matter if the common callback code has its own recursion
detection, or it can handle the recursion in the different contexts
(normal/interrupt/NMI.)
But if it relies on the ‘current\_kprobe’ recursion lock, it has to check
`kprobe_running()` and use kprobe\_busy\_\*() APIs.

Fprobe has FPROBE\_FL\_KPROBE\_SHARED flag to do this. If your common callback
code will be shared with kprobes, please set FPROBE\_FL\_KPROBE\_SHARED
*before* registering the fprobe, like:

```
fprobe.flags = FPROBE_FL_KPROBE_SHARED;

register_fprobe(&fprobe, "func*", NULL);
```

This will protect your common callback from the nested call.

## The missed counter

The fprobe data structure has fprobe::nmissed counter field as same as
kprobes.
This counter counts up when;

> * fprobe fails to take ftrace\_recursion lock. This usually means that a function
>   which is traced by other ftrace users is called from the entry\_handler.
> * fprobe fails to setup the function exit because of failing to allocate the
>   data buffer from the per-task shadow stack.

The fprobe::nmissed field counts up in both cases. Therefore, the former
skips both of entry and exit callback and the latter skips the exit
callback, but in both case the counter will increase by 1.

Note that if you set the FTRACE\_OPS\_FL\_RECURSION and/or FTRACE\_OPS\_FL\_RCU to
fprobe::ops::flags (ftrace\_ops::flags) when registering the fprobe, this
counter may not work correctly, because ftrace skips the fprobe function which
increase the counter.

## Functions and structures

struct fprobe\_hlist\_node
:   address based hash list node for fprobe.

**Definition**:

```
struct fprobe_hlist_node {
    struct rhlist_head      hlist;
    unsigned long           addr;
    struct fprobe           *fp;
};
```

**Members**

`hlist`
:   The hlist node for address search hash table.

`addr`
:   One of the probing address of **fp**.

`fp`
:   The fprobe which owns this.

struct fprobe\_hlist
:   hash list nodes for fprobe.

**Definition**:

```
struct fprobe_hlist {
    struct hlist_node               hlist;
    struct rcu_head                 rcu;
    struct fprobe                   *fp;
    int size;
    struct fprobe_hlist_node        array[];
};
```

**Members**

`hlist`
:   The hlist node for existence checking hash table.

`rcu`
:   rcu\_head for RCU deferred release.

`fp`
:   The fprobe which owns this fprobe\_hlist.

`size`
:   The size of **array**.

`array`
:   The fprobe\_hlist\_node for each address to probe.

struct fprobe
:   ftrace based probe.

**Definition**:

```
struct fprobe {
    unsigned long           nmissed;
    unsigned int            flags;
    size_t entry_data_size;
    fprobe_entry_cb entry_handler;
    fprobe_exit_cb exit_handler;
    struct fprobe_hlist     *hlist_array;
};
```

**Members**

`nmissed`
:   The counter for missing events.

`flags`
:   The status flag.

`entry_data_size`
:   The private data storage size.

`entry_handler`
:   The callback function for function entry.

`exit_handler`
:   The callback function for function exit.

`hlist_array`
:   The fprobe\_hlist for fprobe search from IP hash table.

void disable\_fprobe(struct [fprobe](#c.fprobe "fprobe") \*fp)
:   Disable fprobe

**Parameters**

`struct fprobe *fp`
:   The fprobe to be disabled.

**Description**

This will soft-disable **fp**. Note that this doesn’t remove the ftrace
hooks from the function entry.

void enable\_fprobe(struct [fprobe](#c.fprobe "fprobe") \*fp)
:   Enable fprobe

**Parameters**

`struct fprobe *fp`
:   The fprobe to be enabled.

**Description**

This will soft-enable **fp**.

int register\_fprobe(struct [fprobe](#c.fprobe "fprobe") \*fp, const char \*filter, const char \*notfilter)
:   Register fprobe to ftrace by pattern.

**Parameters**

`struct fprobe *fp`
:   A fprobe data structure to be registered.

`const char *filter`
:   A wildcard pattern of probed symbols.

`const char *notfilter`
:   A wildcard pattern of NOT probed symbols.

**Description**

Register **fp** to ftrace for enabling the probe on the symbols matched to **filter**.
If **notfilter** is not NULL, the symbols matched the **notfilter** are not probed.

Return 0 if **fp** is registered successfully, -errno if not.

int register\_fprobe\_ips(struct [fprobe](#c.fprobe "fprobe") \*fp, unsigned long \*addrs, int num)
:   Register fprobe to ftrace by address.

**Parameters**

`struct fprobe *fp`
:   A fprobe data structure to be registered.

`unsigned long *addrs`
:   An array of target function address.

`int num`
:   The number of entries of **addrs**.

**Description**

Register **fp** to ftrace for enabling the probe on the address given by **addrs**.
The **addrs** must be the addresses of ftrace location address, which may be
the symbol address + arch-dependent offset.
If you unsure what this mean, please use other registration functions.

Return 0 if **fp** is registered successfully, -errno if not.

int register\_fprobe\_syms(struct [fprobe](#c.fprobe "fprobe") \*fp, const char \*\*syms, int num)
:   Register fprobe to ftrace by symbols.

**Parameters**

`struct fprobe *fp`
:   A fprobe data structure to be registered.

`const char **syms`
:   An array of target symbols.

`int num`
:   The number of entries of **syms**.

**Description**

Register **fp** to the symbols given by **syms** array. This will be useful if
you are sure the symbols exist in the kernel.

Return 0 if **fp** is registered successfully, -errno if not.

int unregister\_fprobe\_async(struct [fprobe](#c.fprobe "fprobe") \*fp)
:   Unregister fprobe without RCU GP wait

**Parameters**

`struct fprobe *fp`
:   A fprobe data structure to be unregistered.

**Description**

Unregister fprobe (and remove ftrace hooks from the function entries).
This function will NOT wait until the fprobe is no longer used.

Return 0 if **fp** is unregistered successfully, -errno if not.

int unregister\_fprobe(struct [fprobe](#c.fprobe "fprobe") \*fp)
:   Unregister fprobe with RCU GP wait

**Parameters**

`struct fprobe *fp`
:   A fprobe data structure to be unregistered.

**Description**

Unregister fprobe (and remove ftrace hooks from the function entries).
This function will block until the fprobe is no longer used.

Return 0 if **fp** is unregistered successfully, -errno if not.
