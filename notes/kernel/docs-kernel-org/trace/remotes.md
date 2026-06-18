# Tracing Remotes

> 출처(원문): https://docs.kernel.org/trace/remotes.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Tracing Remotes

Author:
:   Vincent Donnefort <[vdonnefort@google.com](mailto:vdonnefort%40google.com)>

## Overview

Firmware and hypervisors are black boxes to the kernel. Having a way to see what
they are doing can be useful to debug both. This is where remote tracing buffers
come in. A remote tracing buffer is a ring buffer executed by the firmware or
hypervisor into memory that is memory mapped to the host kernel. This is similar
to how user space memory maps the kernel ring buffer but in this case the kernel
is acting like user space and the firmware or hypervisor is the “kernel” side.
With a trace remote ring buffer, the firmware and hypervisor can record events
for which the host kernel can see and expose to user space.

## Register a remote

A remote must provide a set of callbacks [`struct trace_remote_callbacks`](#c.trace_remote_callbacks "trace_remote_callbacks") whom
description can be found below. Those callbacks allows Tracefs to enable and
disable tracing and events, to load and unload a tracing buffer (a set of
ring-buffers) and to swap a reader page with the head page, which enables
consuming reading.

struct trace\_remote\_callbacks
:   Callbacks used by Tracefs to control the remote

**Definition**:

```
struct trace_remote_callbacks {
    int (*init)(struct dentry *d, void *priv);
    struct trace_buffer_desc *(*load_trace_buffer)(unsigned long size, void *priv);
    void (*unload_trace_buffer)(struct trace_buffer_desc *desc, void *priv);
    int (*enable_tracing)(bool enable, void *priv);
    int (*swap_reader_page)(unsigned int cpu, void *priv);
    int (*reset)(unsigned int cpu, void *priv);
    int (*enable_event)(unsigned short id, bool enable, void *priv);
};
```

**Members**

`init`
:   Called once the remote has been registered. Allows the
    caller to extend the Tracefs remote directory

`load_trace_buffer`
:   Called before Tracefs accesses the trace buffer for the first
    time. Must return a `trace_buffer_desc`
    (most likely filled with `trace_remote_alloc_buffer()`)

`unload_trace_buffer`
:   Called once Tracefs has no use for the trace buffer
    (most likely call `trace_remote_free_buffer()`)

`enable_tracing`
:   Called on Tracefs tracing\_on. It is expected from the
    remote to allow writing.

`swap_reader_page`
:   Called when Tracefs consumes a new page from a
    ring-buffer. It is expected from the remote to isolate a

`reset`
:   Called on echo 0 > trace. It is expected from the
    remote to reset all ring-buffer pages.
    new reader-page from the **cpu** ring-buffer.

`enable_event`
:   Called on events/event\_name/enable. It is expected from
    the remote to allow the writing event **id**.

Once registered, an instance will appear for this remote in the Tracefs
directory **remotes/**. Buffers can then be read using the usual Tracefs files
**trace\_pipe** and **trace**.

## Declare a remote event

Macros are provided to ease the declaration of remote events, in a similar
fashion to in-kernel events. A declaration must provide an ID, a description of
the event arguments and how to print the event:

```
REMOTE_EVENT(foo, EVENT_FOO_ID,
        RE_STRUCT(
                re_field(u64, bar)
        ),
        RE_PRINTK("bar=%lld", __entry->bar)
);
```

Then those events must be declared in a C file with the following:

```
#define REMOTE_EVENT_INCLUDE_FILE foo_events.h
#include <trace/define_remote_events.h>
```

This will provide a `struct remote_event` remote\_event\_foo that can be given to
trace\_remote\_register.

Registered events appear in the remote directory under **events/**.

## Simple ring-buffer

A simple implementation for a ring-buffer writer can be found in
kernel/trace/simple\_ring\_buffer.c.
