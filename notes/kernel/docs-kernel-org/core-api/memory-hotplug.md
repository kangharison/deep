# Memory hotplug

> 출처(원문): https://docs.kernel.org/core-api/memory-hotplug.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Memory hotplug

## Memory hotplug event notifier

Hotplugging events are sent to a notification queue.

### Memory notifier

There are six types of notification defined in `include/linux/memory.h`:

MEM\_GOING\_ONLINE
:   Generated before new memory becomes available in order to be able to
    prepare subsystems to handle memory. The page allocator is still unable
    to allocate from the new memory.

MEM\_CANCEL\_ONLINE
:   Generated if MEM\_GOING\_ONLINE fails.

MEM\_ONLINE
:   Generated when memory has successfully brought online. The callback may
    allocate pages from the new memory.

MEM\_GOING\_OFFLINE
:   Generated to begin the process of offlining memory. Allocations are no
    longer possible from the memory but some of the memory to be offlined
    is still in use. The callback can be used to free memory known to a
    subsystem from the indicated memory block.

MEM\_CANCEL\_OFFLINE
:   Generated if MEM\_GOING\_OFFLINE fails. Memory is available again from
    the memory block that we attempted to offline.

MEM\_OFFLINE
:   Generated after offlining memory is complete.

A callback routine can be registered by calling:

```
hotplug_memory_notifier(callback_func, priority)
```

Callback functions with higher values of priority are called before callback
functions with lower values.

A callback function must have the following prototype:

```
int callback_func(
  struct notifier_block *self, unsigned long action, void *arg);
```

The first argument of the callback function (self) is a pointer to the block
of the notifier chain that points to the callback function itself.
The second argument (action) is one of the event types described above.
The third argument (arg) passes a pointer of `struct memory_notify`:

```
struct memory_notify {
        unsigned long start_pfn;
        unsigned long nr_pages;
}
```

* start\_pfn is start\_pfn of online/offline memory.
* nr\_pages is # of pages of online/offline memory.

It is possible to get notified for MEM\_CANCEL\_ONLINE without having been notified
for MEM\_GOING\_ONLINE, and the same applies to MEM\_CANCEL\_OFFLINE and
MEM\_GOING\_OFFLINE.
This can happen when a consumer fails, meaning we break the callchain and we
stop calling the remaining consumers of the notifier.
It is then important that users of memory\_notify make no assumptions and get
prepared to handle such cases.

The callback routine shall return one of the values
NOTIFY\_DONE, NOTIFY\_OK, NOTIFY\_BAD, NOTIFY\_STOP
defined in `include/linux/notifier.h`

NOTIFY\_DONE and NOTIFY\_OK have no effect on the further processing.

NOTIFY\_BAD is used as response to the MEM\_GOING\_ONLINE, MEM\_GOING\_OFFLINE,
MEM\_ONLINE, or MEM\_OFFLINE action to cancel hotplugging. It stops
further processing of the notification queue.

NOTIFY\_STOP stops further processing of the notification queue.

### Numa node notifier

There are six types of notification defined in `include/linux/node.h`:

NODE\_ADDING\_FIRST\_MEMORY
:   Generated before memory becomes available to this node for the first time.

NODE\_CANCEL\_ADDING\_FIRST\_MEMORY
:   Generated if NODE\_ADDING\_FIRST\_MEMORY fails.

NODE\_ADDED\_FIRST\_MEMORY
:   Generated when memory has become available for this node for the first time.

NODE\_REMOVING\_LAST\_MEMORY
:   Generated when the last memory available to this node is about to be offlined.

NODE\_CANCEL\_REMOVING\_LAST\_MEMORY
:   Generated when NODE\_CANCEL\_REMOVING\_LAST\_MEMORY fails.

NODE\_REMOVED\_LAST\_MEMORY
:   Generated when the last memory available to this node has been offlined.

A callback routine can be registered by calling:

```
hotplug_node_notifier(callback_func, priority)
```

Callback functions with higher values of priority are called before callback
functions with lower values.

A callback function must have the following prototype:

```
int callback_func(

  struct notifier_block *self, unsigned long action, void *arg);
```

The first argument of the callback function (self) is a pointer to the block
of the notifier chain that points to the callback function itself.
The second argument (action) is one of the event types described above.
The third argument (arg) passes a pointer of `struct node_notify`:

```
struct node_notify {
        int nid;
}
```

* nid is the node we are adding or removing memory to.

It is possible to get notified for NODE\_CANCEL\_ADDING\_FIRST\_MEMORY without
having been notified for NODE\_ADDING\_FIRST\_MEMORY, and the same applies to
NODE\_CANCEL\_REMOVING\_LAST\_MEMORY and NODE\_REMOVING\_LAST\_MEMORY.
This can happen when a consumer fails, meaning we break the callchain and we
stop calling the remaining consumers of the notifier.
It is then important that users of node\_notify make no assumptions and get
prepared to handle such cases.

The callback routine shall return one of the values
NOTIFY\_DONE, NOTIFY\_OK, NOTIFY\_BAD, NOTIFY\_STOP
defined in `include/linux/notifier.h`

NOTIFY\_DONE and NOTIFY\_OK have no effect on the further processing.

NOTIFY\_BAD is used as response to the NODE\_ADDING\_FIRST\_MEMORY,
NODE\_REMOVING\_LAST\_MEMORY, NODE\_ADDED\_FIRST\_MEMORY or
NODE\_REMOVED\_LAST\_MEMORY action to cancel hotplugging.
It stops further processing of the notification queue.

NOTIFY\_STOP stops further processing of the notification queue.

Please note that we should not fail for NODE\_ADDED\_FIRST\_MEMORY /
NODE\_REMOVED\_FIRST\_MEMORY, as memory\_hotplug code cannot rollback at that
point anymore.

## Locking Internals

When adding/removing memory that uses memory block devices (i.e. ordinary RAM),
the device\_hotplug\_lock should be held to:

* synchronize against online/offline requests (e.g. via sysfs). This way, memory
  block devices can only be accessed (.online/.state attributes) by user
  space once memory has been fully added. And when removing memory, we
  know nobody is in critical sections.
* synchronize against CPU hotplug and similar (e.g. relevant for ACPI and PPC)

Especially, there is a possible lock inversion that is avoided using
device\_hotplug\_lock when adding memory and user space tries to online that
memory faster than expected:

* `device_online()` will first take the `device_lock()`, followed by
  mem\_hotplug\_lock
* `add_memory_resource()` will first take the mem\_hotplug\_lock, followed by
  the `device_lock()` (while creating the devices, during `bus_add_device()`).

As the device is visible to user space before taking the `device_lock()`, this
can result in a lock inversion.

onlining/offlining of memory should be done via `device_online()`/
`device_offline()` - to make sure it is properly synchronized to actions
via sysfs. Holding device\_hotplug\_lock is advised (to e.g. protect online\_type)

When adding/removing/onlining/offlining memory or adding/removing
heterogeneous/device memory, we should always hold the mem\_hotplug\_lock in
write mode to serialise memory hotplug (e.g. access to global/zone
variables).

In addition, mem\_hotplug\_lock (in contrast to device\_hotplug\_lock) in read
mode allows for a quite efficient get\_online\_mems/put\_online\_mems
implementation, so code accessing memory can protect from that memory
vanishing.
