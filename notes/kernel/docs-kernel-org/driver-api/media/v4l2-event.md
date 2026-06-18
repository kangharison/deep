# 3.14.V4L2 events

> 출처(원문): https://docs.kernel.org/driver-api/media/v4l2-event.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.14. V4L2 events

The V4L2 events provide a generic way to pass events to user space.
The driver must use [`v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh") to be able to support V4L2 events.

Events are subscribed per-filehandle. An event specification consists of a
`type` and is optionally associated with an object identified through the
`id` field. If unused, then the `id` is 0. So an event is uniquely
identified by the `(type, id)` tuple.

The [`v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh") `struct has` a list of subscribed events on its
`subscribed` field.

When the user subscribes to an event, a [`v4l2_subscribed_event`](#c.v4l2_subscribed_event "v4l2_subscribed_event")
`struct is` added to [`v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh")`.subscribed`, one for every
subscribed event.

Each [`v4l2_subscribed_event`](#c.v4l2_subscribed_event "v4l2_subscribed_event") `struct ends` with a
[`v4l2_kevent`](#c.v4l2_kevent "v4l2_kevent") ringbuffer, with the size given by the caller
of [`v4l2_event_subscribe()`](#c.v4l2_event_subscribe "v4l2_event_subscribe"). This ringbuffer is used to store any events
raised by the driver.

So every `(type, ID)` event tuple will have its own
[`v4l2_kevent`](#c.v4l2_kevent "v4l2_kevent") ringbuffer. This guarantees that if a driver is
generating lots of events of one type in a short time, then that will
not overwrite events of another type.

But if you get more events of one type than the size of the
[`v4l2_kevent`](#c.v4l2_kevent "v4l2_kevent") ringbuffer, then the oldest event will be dropped
and the new one added.

The [`v4l2_kevent`](#c.v4l2_kevent "v4l2_kevent") `struct links` into the `available`
list of the [`v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh") `struct so` [ioctl VIDIOC\_DQEVENT](../../userspace-api/media/v4l/vidioc-dqevent.html#vidioc-dqevent) will
know which event to dequeue first.

Finally, if the event subscription is associated with a particular object
such as a V4L2 control, then that object needs to know about that as well
so that an event can be raised by that object. So the `node` field can
be used to link the [`v4l2_subscribed_event`](#c.v4l2_subscribed_event "v4l2_subscribed_event") `struct into` a list of
such objects.

So to summarize:

* [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh") has two lists: one of the `subscribed` events,
  and one of the `available` events.
* [`struct v4l2_subscribed_event`](#c.v4l2_subscribed_event "v4l2_subscribed_event") has a ringbuffer of raised
  (pending) events of that particular type.
* If [`struct v4l2_subscribed_event`](#c.v4l2_subscribed_event "v4l2_subscribed_event") is associated with a specific
  object, then that object will have an internal list of
  [`struct v4l2_subscribed_event`](#c.v4l2_subscribed_event "v4l2_subscribed_event") so it knows who subscribed an
  event to that object.

Furthermore, the internal [`struct v4l2_subscribed_event`](#c.v4l2_subscribed_event "v4l2_subscribed_event") has
`merge()` and `replace()` callbacks which drivers can set. These
callbacks are called when a new event is raised and there is no more room.

The `replace()` callback allows you to replace the payload of the old event
with that of the new event, merging any relevant data from the old payload
into the new payload that replaces it. It is called when this event type has
a ringbuffer with size is one, i.e. only one event can be stored in the
ringbuffer.

The `merge()` callback allows you to merge the oldest event payload into
that of the second-oldest event payload. It is called when
the ringbuffer has size is greater than one.

This way no status information is lost, just the intermediate steps leading
up to that state.

A good example of these `replace`/`merge` callbacks is in v4l2-event.c:
`ctrls_replace()` and `ctrls_merge()` callbacks for the control event.

Note

these callbacks can be called from interrupt context, so they must
be fast.

In order to queue events to video device, drivers should call:

> [`v4l2_event_queue`](#c.v4l2_event_queue "v4l2_event_queue")
> ([`vdev`](v4l2-dev.html#c.video_device "video_device"), `ev`)

The driver’s only responsibility is to fill in the type and the data fields.
The other fields will be filled in by V4L2.

## 3.14.1. Event subscription

Subscribing to an event is via:

> [`v4l2_event_subscribe`](#c.v4l2_event_subscribe "v4l2_event_subscribe")
> ([`fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh"), `sub` ,
> elems, [`ops`](#c.v4l2_subscribed_event_ops "v4l2_subscribed_event_ops"))

This function is used to implement [`video_device`](v4l2-dev.html#c.video_device "video_device")->
[`ioctl_ops`](v4l2-common.html#c.v4l2_ioctl_ops "v4l2_ioctl_ops")-> `vidioc_subscribe_event`,
but the driver must check first if the driver is able to produce events
with specified event id, and then should call
[`v4l2_event_subscribe()`](#c.v4l2_event_subscribe "v4l2_event_subscribe") to subscribe the event.

The elems argument is the size of the event queue for this event. If it is 0,
then the framework will fill in a default value (this depends on the event
type).

The ops argument allows the driver to specify a number of callbacks:

| Callback | Description |
| --- | --- |
| add | called when a new listener gets added (subscribing to the same event twice will only cause this callback to get called once) |
| del | called when a listener stops listening |
| replace | replace event ‘old’ with event ‘new’. |
| merge | merge event ‘old’ into event ‘new’. |

All 4 callbacks are optional, if you don’t want to specify any callbacks
the ops argument itself maybe `NULL`.

## 3.14.2. Unsubscribing an event

Unsubscribing to an event is via:

> [`v4l2_event_unsubscribe`](#c.v4l2_event_unsubscribe "v4l2_event_unsubscribe")
> ([`fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh"), `sub`)

This function is used to implement [`video_device`](v4l2-dev.html#c.video_device "video_device")->
[`ioctl_ops`](v4l2-common.html#c.v4l2_ioctl_ops "v4l2_ioctl_ops")-> `vidioc_unsubscribe_event`.
A driver may call [`v4l2_event_unsubscribe()`](#c.v4l2_event_unsubscribe "v4l2_event_unsubscribe") directly unless it
wants to be involved in unsubscription process.

The special type `V4L2_EVENT_ALL` may be used to unsubscribe all events. The
drivers may want to handle this in a special way.

## 3.14.3. Check if there’s a pending event

Checking if there’s a pending event is via:

> [`v4l2_event_pending`](#c.v4l2_event_pending "v4l2_event_pending")
> ([`fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh"))

This function returns the number of pending events. Useful when implementing
poll.

## 3.14.4. How events work

Events are delivered to user space through the poll system call. The driver
can use [`v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh")->wait (a wait\_queue\_head\_t) as the argument for
`poll_wait()`.

There are standard and private events. New standard events must use the
smallest available event type. The drivers must allocate their events from
their own class starting from class base. Class base is
`V4L2_EVENT_PRIVATE_START` + n \* 1000 where n is the lowest available number.
The first event type in the class is reserved for future use, so the first
available event type is ‘class base + 1’.

An example on how the V4L2 events may be used can be found in the OMAP
3 ISP driver (`drivers/media/platform/ti/omap3isp`).

A subdev can directly send an event to the [`v4l2_device`](v4l2-device.html#c.v4l2_device "v4l2_device") notify
function with `V4L2_DEVICE_NOTIFY_EVENT`. This allows the bridge to map
the subdev that sends the event to the video node(s) associated with the
subdev that need to be informed about such an event.

### 3.14.4.1. V4L2 event functions and data structures

struct v4l2\_kevent
:   Internal kernel event struct.

> **Definition**:
>
> ```
> struct v4l2_kevent {
>       struct list_head        list;
>       struct v4l2_subscribed_event *sev;
>       struct v4l2_event       event;
>       u64 ts;
> };
> ```

**Members**

`list`
:   List node for the v4l2\_fh->available list.

`sev`
:   Pointer to parent v4l2\_subscribed\_event.

`event`
:   The event itself.

`ts`
:   The timestamp of the event.

struct v4l2\_subscribed\_event\_ops
:   Subscribed event operations.

> **Definition**:
>
> ```
> struct v4l2_subscribed_event_ops {
>       int (*add)(struct v4l2_subscribed_event *sev, unsigned int elems);
>       void (*del)(struct v4l2_subscribed_event *sev);
>       void (*replace)(struct v4l2_event *old, const struct v4l2_event *new);
>       void (*merge)(const struct v4l2_event *old, struct v4l2_event *new);
> };
> ```

**Members**

`add`
:   Optional callback, called when a new listener is added

`del`
:   Optional callback, called when a listener stops listening

`replace`
:   Optional callback that can replace event ‘old’ with event ‘new’.

`merge`
:   Optional callback that can merge event ‘old’ into event ‘new’.

struct v4l2\_subscribed\_event
:   Internal `struct representing` a subscribed event.

> **Definition**:
>
> ```
> struct v4l2_subscribed_event {
>       struct list_head        list;
>       u32 type;
>       u32 id;
>       u32 flags;
>       struct v4l2_fh          *fh;
>       struct list_head        node;
>       const struct v4l2_subscribed_event_ops *ops;
>       unsigned int            elems;
>       unsigned int            first;
>       unsigned int            in_use;
>       struct v4l2_kevent      events[];
> };
> ```

**Members**

`list`
:   List node for the v4l2\_fh->subscribed list.

`type`
:   Event type.

`id`
:   Associated object ID (e.g. control ID). 0 if there isn’t any.

`flags`
:   Copy of v4l2\_event\_subscription->flags.

`fh`
:   Filehandle that subscribed to this event.

`node`
:   List node that hooks into the object’s event list
    (if there is one).

`ops`
:   v4l2\_subscribed\_event\_ops

`elems`
:   The number of elements in the events array.

`first`
:   The index of the events containing the oldest available event.

`in_use`
:   The number of queued events.

`events`
:   An array of **elems** events.

int v4l2\_event\_dequeue(struct [v4l2\_fh](v4l2-fh.html#c.v4l2_fh "v4l2_fh") \*fh, struct v4l2\_event \*event, int nonblocking)
:   Dequeue events from video device.

**Parameters**

`struct v4l2_fh *fh`
:   pointer to [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh")

`struct v4l2_event *event`
:   pointer to `struct v4l2_event`

`int nonblocking`
:   if not zero, waits for an event to arrive

void v4l2\_event\_queue(struct [video\_device](v4l2-dev.html#c.video_device "video_device") \*vdev, const struct v4l2\_event \*ev)
:   Queue events to video device.

**Parameters**

`struct video_device *vdev`
:   pointer to [`struct video_device`](v4l2-dev.html#c.video_device "video_device")

`const struct v4l2_event *ev`
:   pointer to `struct v4l2_event`

**Description**

The event will be queued for all [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh") file handlers.

Note

The driver’s only responsibility is to fill in the type and the data
fields. The other fields will be filled in by V4L2.

void v4l2\_event\_queue\_fh(struct [v4l2\_fh](v4l2-fh.html#c.v4l2_fh "v4l2_fh") \*fh, const struct v4l2\_event \*ev)
:   Queue events to video device.

**Parameters**

`struct v4l2_fh *fh`
:   pointer to [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh")

`const struct v4l2_event *ev`
:   pointer to `struct v4l2_event`

**Description**

The event will be queued only for the specified [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh") file handler.

Note

The driver’s only responsibility is to fill in the type and the data
fields. The other fields will be filled in by V4L2.

void v4l2\_event\_wake\_all(struct [video\_device](v4l2-dev.html#c.video_device "video_device") \*vdev)
:   Wake all filehandles.

**Parameters**

`struct video_device *vdev`
:   pointer to [`struct video_device`](v4l2-dev.html#c.video_device "video_device")

**Description**

Used when unregistering a video device.

int v4l2\_event\_pending(struct [v4l2\_fh](v4l2-fh.html#c.v4l2_fh "v4l2_fh") \*fh)
:   Check if an event is available

**Parameters**

`struct v4l2_fh *fh`
:   pointer to [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh")

**Description**

Returns the number of pending events.

int v4l2\_event\_subscribe(struct [v4l2\_fh](v4l2-fh.html#c.v4l2_fh "v4l2_fh") \*fh, const struct v4l2\_event\_subscription \*sub, unsigned int elems, const struct [v4l2\_subscribed\_event\_ops](#c.v4l2_subscribed_event_ops "v4l2_subscribed_event_ops") \*ops)
:   Subscribes to an event

**Parameters**

`struct v4l2_fh *fh`
:   pointer to [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh")

`const struct v4l2_event_subscription *sub`
:   pointer to `struct v4l2_event_subscription`

`unsigned int elems`
:   size of the events queue

`const struct v4l2_subscribed_event_ops *ops`
:   pointer to [`v4l2_subscribed_event_ops`](#c.v4l2_subscribed_event_ops "v4l2_subscribed_event_ops")

**Description**

Note

if **elems** is zero, the framework will fill in a default value,
with is currently 1 element.

int v4l2\_event\_unsubscribe(struct [v4l2\_fh](v4l2-fh.html#c.v4l2_fh "v4l2_fh") \*fh, const struct v4l2\_event\_subscription \*sub)
:   Unsubscribes to an event

**Parameters**

`struct v4l2_fh *fh`
:   pointer to [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh")

`const struct v4l2_event_subscription *sub`
:   pointer to `struct v4l2_event_subscription`

void v4l2\_event\_unsubscribe\_all(struct [v4l2\_fh](v4l2-fh.html#c.v4l2_fh "v4l2_fh") \*fh)
:   Unsubscribes to all events

**Parameters**

`struct v4l2_fh *fh`
:   pointer to [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh")

int v4l2\_event\_subdev\_unsubscribe(struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd, struct [v4l2\_fh](v4l2-fh.html#c.v4l2_fh "v4l2_fh") \*fh, struct v4l2\_event\_subscription \*sub)
:   Subdev variant of [`v4l2_event_unsubscribe()`](#c.v4l2_event_unsubscribe "v4l2_event_unsubscribe")

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")

`struct v4l2_fh *fh`
:   pointer to [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh")

`struct v4l2_event_subscription *sub`
:   pointer to `struct v4l2_event_subscription`

**Description**

Note

This function should be used for the [`struct v4l2_subdev_core_ops`](v4l2-subdev.html#c.v4l2_subdev_core_ops "v4l2_subdev_core_ops")
`unsubscribe_event` field.

int v4l2\_src\_change\_event\_subscribe(struct [v4l2\_fh](v4l2-fh.html#c.v4l2_fh "v4l2_fh") \*fh, const struct v4l2\_event\_subscription \*sub)
:   helper function that calls [`v4l2_event_subscribe()`](#c.v4l2_event_subscribe "v4l2_event_subscribe") if the event is `V4L2_EVENT_SOURCE_CHANGE`.

**Parameters**

`struct v4l2_fh *fh`
:   pointer to [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh")

`const struct v4l2_event_subscription *sub`
:   pointer to `struct v4l2_event_subscription`

int v4l2\_src\_change\_event\_subdev\_subscribe(struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd, struct [v4l2\_fh](v4l2-fh.html#c.v4l2_fh "v4l2_fh") \*fh, struct v4l2\_event\_subscription \*sub)
:   Variant of [`v4l2_event_subscribe()`](#c.v4l2_event_subscribe "v4l2_event_subscribe"), meant to subscribe only events of the type `V4L2_EVENT_SOURCE_CHANGE`.

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")

`struct v4l2_fh *fh`
:   pointer to [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh")

`struct v4l2_event_subscription *sub`
:   pointer to `struct v4l2_event_subscription`
