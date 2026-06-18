# Debugging advice for driver development

> 출처(원문): https://docs.kernel.org/process/debugging/driver_development_debugging_guide.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Debugging advice for driver development](#id1)

This document serves as a general starting point and lookup for debugging
device drivers.
While this guide focuses on debugging that requires re-compiling the
module/kernel, the [userspace debugging guide](userspace_debugging_guide.html) will guide
you through tools like dynamic debug, ftrace and other tools useful for
debugging issues and behavior.
For general debugging advice, see the [general advice document](index.html).

The following sections show you the available tools.

## [printk() & friends](#id2)

These are derivatives of `printf()` with varying destinations and support for
being dynamically turned on or off, or lack thereof.

### [Simple printk()](#id3)

The classic, can be used to great effect for quick and dirty development
of new modules or to extract arbitrary necessary data for troubleshooting.

Prerequisite: `CONFIG_PRINTK` (usually enabled by default)

**Pros**:

* No need to learn anything, simple to use
* Easy to modify exactly to your needs (formatting of the data (See:
  [How to get printk format specifiers right](../../core-api/printk-formats.html)), visibility in the log)
* Can cause delays in the execution of the code (beneficial to confirm whether
  timing is a factor)

**Cons**:

* Requires rebuilding the kernel/module
* Can cause delays in the execution of the code (which can cause issues to be
  not reproducible)

For the full documentation see [Message logging with printk](../../core-api/printk-basics.html)

### [Trace\_printk](#id4)

Prerequisite: `CONFIG_DYNAMIC_FTRACE` & `#include <linux/ftrace.h>`

It is a tiny bit less comfortable to use than [`printk()`](../../core-api/printk-basics.html#c.printk "printk"), because you will have
to read the messages from the trace file (See: [Reading the ftrace log](userspace_debugging_guide.html#read-ftrace-log)
instead of from the kernel log, but very useful when [`printk()`](../../core-api/printk-basics.html#c.printk "printk") adds unwanted
delays into the code execution, causing issues to be flaky or hidden.)

If the processing of this still causes timing issues then you can try
`trace_puts()`.

For the full Documentation see `trace_printk()`

### [dev\_dbg](#id5)

Print statement, which can be targeted by
[Dynamic debug](userspace_debugging_guide.html#dynamic-debug) that contains
additional information about the device used within the context.

**When is it appropriate to leave a debug print in the code?**

Permanent debug statements have to be useful for a developer to troubleshoot
driver misbehavior. Judging that is a bit more of an art than a science, but
some guidelines are in the [Coding style guidelines](../coding-style.html#printing-kernel-messages). In almost all cases the
debug statements shouldn’t be upstreamed, as a working driver is supposed to be
silent.

### [Custom printk](#id6)

Example:

```
#define core_dbg(fmt, arg...) do { \
        if (core_debug) \
                printk(KERN_DEBUG pr_fmt("core: " fmt), ## arg); \
        } while (0)
```

**When should you do this?**

It is better to just use a [`pr_debug()`](../../core-api/printk-basics.html#c.pr_debug "pr_debug"), which can later be turned on/off with
dynamic debug. Additionally, a lot of drivers activate these prints via a
variable like `core_debug` set by a module parameter. However, Module
parameters [are not recommended anymore](https://lore.kernel.org/all/2024032757-surcharge-grime-d3dd@gregkh).

## [Ftrace](#id7)

### [Creating a custom Ftrace tracepoint](#id8)

A tracepoint adds a hook into your code that will be called and logged when the
tracepoint is enabled. This can be used, for example, to trace hitting a
conditional branch or to dump the internal state at specific points of the code
flow during a debugging session.

Here is a basic description of [how to implement new tracepoints](../../trace/tracepoints.html#usage).

For the full event tracing documentation see [Event Tracing](../../trace/events.html)

For the full Ftrace documentation see [ftrace - Function Tracer](../../trace/ftrace.html)

## [DebugFS](#id9)

Prerequisite: `` CONFIG_DEBUG_FS` & `#include <linux/debugfs.h> ``

DebugFS differs from the other approaches of debugging, as it doesn’t write
messages to the kernel log nor add traces to the code. Instead it allows the
developer to handle a set of files.
With these files you can either store values of variables or make
register/memory dumps or you can make these files writable and modify
values/settings in the driver.

Possible use-cases among others:

* Store register values
* Keep track of variables
* Store errors
* Store settings
* Toggle a setting like debug on/off
* Error injection

This is especially useful, when the size of a data dump would be hard to digest
as part of the general kernel log (for example when dumping raw bitstream data)
or when you are not interested in all the values all the time, but with the
possibility to inspect them.

The general idea is:

* Create a directory during probe (`struct dentry *parent =
  debugfs_create_dir("my_driver", NULL);`)
* Create a file (`debugfs_create_u32("my_value", 444, parent, &my_variable);`)

  + In this example the file is found in
    `/sys/kernel/debug/my_driver/my_value` (with read permissions for
    user/group/all)
  + any read of the file will return the current contents of the variable
    `my_variable`
* Clean up the directory when removing the device
  (`debugfs_remove(parent);`)

For the full documentation see [DebugFS](../../filesystems/debugfs.html).

## [KASAN, UBSAN, lockdep and other error checkers](#id10)

### [KASAN (Kernel Address Sanitizer)](#id11)

Prerequisite: `CONFIG_KASAN`

KASAN is a dynamic memory error detector that helps to find use-after-free and
out-of-bounds bugs. It uses compile-time instrumentation to check every memory
access.

For the full documentation see [Kernel Address Sanitizer (KASAN)](../../dev-tools/kasan.html).

### [UBSAN (Undefined Behavior Sanitizer)](#id12)

Prerequisite: `CONFIG_UBSAN`

UBSAN relies on compiler instrumentation and runtime checks to detect undefined
behavior. It is designed to find a variety of issues, including signed integer
overflow, array index out of bounds, and more.

For the full documentation see [Undefined Behavior Sanitizer - UBSAN](../../dev-tools/ubsan.html)

### [lockdep (Lock Dependency Validator)](#id13)

Prerequisite: `CONFIG_DEBUG_LOCKDEP`

lockdep is a runtime lock dependency validator that detects potential deadlocks
and other locking-related issues in the kernel.
It tracks lock acquisitions and releases, building a dependency graph that is
analyzed for potential deadlocks.
lockdep is especially useful for validating the correctness of lock ordering in
the kernel.

### [PSI (Pressure stall information tracking)](#id14)

Prerequisite: `CONFIG_PSI`

PSI is a measurement tool to identify excessive overcommits on hardware
resources, that can cause performance disruptions or even OOM kills.

## [device coredump](#id15)

Prerequisite: `CONFIG_DEV_COREDUMP` & `#include <linux/devcoredump.h>`

Provides the infrastructure for a driver to provide arbitrary data to userland.
It is most often used in conjunction with udev or similar userland application
to listen for kernel uevents, which indicate that the dump is ready. Udev has
rules to copy that file somewhere for long-term storage and analysis, as by
default, the data for the dump is automatically cleaned up after a default
5 minutes. That data is analyzed with driver-specific tools or GDB.

A device coredump can be created with a vmalloc area, with read/free
methods, or as a scatter/gather list.

You can find an example implementation at:
[drivers/media/platform/qcom/venus/core.c](https://elixir.bootlin.com/linux/v6.11.6/source/drivers/media/platform/qcom/venus/core.c#L30),
in the Bluetooth HCI layer, in several wireless drivers, and in several
DRM drivers.

### [devcoredump interfaces](#id16)

void dev\_coredumpm(struct [device](../../driver-api/infrastructure.html#c.device "device") \*dev, struct module \*owner, void \*data, size\_t datalen, gfp\_t gfp, ssize\_t (\*read)(char \*buffer, loff\_t offset, size\_t count, void \*data, size\_t datalen), void (\*free)(void \*data))
:   create device coredump with read/free methods

**Parameters**

`struct device *dev`
:   the [`struct device`](../../driver-api/infrastructure.html#c.device "device") for the crashed device

`struct module *owner`
:   the module that contains the read/free functions, use `THIS_MODULE`

`void *data`
:   data cookie for the **read**/**free** functions

`size_t datalen`
:   length of the data

`gfp_t gfp`
:   allocation flags

`ssize_t (*read)(char *buffer, loff_t offset, size_t count, void *data, size_t datalen)`
:   function to read from the given buffer

`void (*free)(void *data)`
:   function to free the given buffer

**Description**

Creates a new device coredump for the given device. If a previous one hasn’t
been read yet, the new coredump is discarded. The data lifetime is determined
by the device coredump framework and when it is no longer needed the **free**
function will be called to free the data.

void dev\_coredumpv(struct [device](../../driver-api/infrastructure.html#c.device "device") \*dev, void \*data, size\_t datalen, gfp\_t gfp)
:   create device coredump with vmalloc data

**Parameters**

`struct device *dev`
:   the [`struct device`](../../driver-api/infrastructure.html#c.device "device") for the crashed device

`void *data`
:   vmalloc data containing the device coredump

`size_t datalen`
:   length of the data

`gfp_t gfp`
:   allocation flags

**Description**

This function takes ownership of the vmalloc’ed data and will free
it when it is no longer used. See [`dev_coredumpm()`](#c.dev_coredumpm "dev_coredumpm") for more information.

void devcd\_free\_sgtable(void \*data)
:   free all the memory of the given scatterlist table (i.e. both pages and scatterlist instances)

**Parameters**

`void *data`
:   pointer to sg\_table to free

**NOTE**

if two tables allocated with devcd\_alloc\_sgtable and then chained
using the sg\_chain function then that function should be called only once
on the chained table

ssize\_t devcd\_read\_from\_sgtable(char \*buffer, loff\_t offset, size\_t buf\_len, void \*data, size\_t data\_len)
:   copy data from sg\_table to a given buffer and return the number of bytes read

**Parameters**

`char *buffer`
:   the buffer to copy the data to it

`loff_t offset`
:   start copy from **offset\*\*\*\*** bytes from the head of the data
    in the given scatterlist

`size_t buf_len`
:   the length of the buffer

`void *data`
:   the scatterlist table to copy from

`size_t data_len`
:   the length of the data in the sg\_table

**Return**

the number of bytes copied

void dev\_coredump\_put(struct [device](../../driver-api/infrastructure.html#c.device "device") \*dev)
:   remove device coredump

**Parameters**

`struct device *dev`
:   the [`struct device`](../../driver-api/infrastructure.html#c.device "device") for the crashed device

**Description**

[`dev_coredump_put()`](#c.dev_coredump_put "dev_coredump_put") removes coredump, if exists, for a given device from
the file system and free its associated data otherwise, does nothing.

It is useful for modules that do not want to keep coredump
available after its unload.

void dev\_coredumpm\_timeout(struct [device](../../driver-api/infrastructure.html#c.device "device") \*dev, struct module \*owner, void \*data, size\_t datalen, gfp\_t gfp, ssize\_t (\*read)(char \*buffer, loff\_t offset, size\_t count, void \*data, size\_t datalen), void (\*free)(void \*data), unsigned long timeout)
:   create device coredump with read/free methods with a custom timeout.

**Parameters**

`struct device *dev`
:   the [`struct device`](../../driver-api/infrastructure.html#c.device "device") for the crashed device

`struct module *owner`
:   the module that contains the read/free functions, use `THIS_MODULE`

`void *data`
:   data cookie for the **read**/**free** functions

`size_t datalen`
:   length of the data

`gfp_t gfp`
:   allocation flags

`ssize_t (*read)(char *buffer, loff_t offset, size_t count, void *data, size_t datalen)`
:   function to read from the given buffer

`void (*free)(void *data)`
:   function to free the given buffer

`unsigned long timeout`
:   time in jiffies to remove coredump

**Description**

Creates a new device coredump for the given device. If a previous one hasn’t
been read yet, the new coredump is discarded. The data lifetime is determined
by the device coredump framework and when it is no longer needed the **free**
function will be called to free the data.

void dev\_coredumpsg(struct [device](../../driver-api/infrastructure.html#c.device "device") \*dev, struct scatterlist \*table, size\_t datalen, gfp\_t gfp)
:   create device coredump that uses scatterlist as data parameter

**Parameters**

`struct device *dev`
:   the [`struct device`](../../driver-api/infrastructure.html#c.device "device") for the crashed device

`struct scatterlist *table`
:   the dump data

`size_t datalen`
:   length of the data

`gfp_t gfp`
:   allocation flags

**Description**

Creates a new device coredump for the given device. If a previous one hasn’t
been read yet, the new coredump is discarded. The data lifetime is determined
by the device coredump framework and when it is no longer needed
it will free the data.

**Copyright** ©2024 : Collabora
