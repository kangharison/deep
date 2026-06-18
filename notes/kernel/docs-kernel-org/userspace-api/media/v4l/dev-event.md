# 4.12.Event Interface

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/dev-event.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.12. Event Interface

The V4L2 event interface provides a means for a user to get immediately
notified on certain conditions taking place on a device. This might
include start of frame or loss of signal events, for example. Changes in
the value or state of a V4L2 control can also be reported through
events.

To receive events, the events the user is interested in first must be
subscribed using the
[ioctl VIDIOC\_SUBSCRIBE\_EVENT, VIDIOC\_UNSUBSCRIBE\_EVENT](vidioc-subscribe-event.html#vidioc-subscribe-event) ioctl. Once
an event is subscribed, the events of subscribed types are dequeueable
using the [ioctl VIDIOC\_DQEVENT](vidioc-dqevent.html#vidioc-dqevent) ioctl. Events may be
unsubscribed using VIDIOC\_UNSUBSCRIBE\_EVENT ioctl. The special event
type V4L2\_EVENT\_ALL may be used to unsubscribe all the events the
driver supports.

The event subscriptions and event queues are specific to file handles.
Subscribing an event on one file handle does not affect other file
handles.

The information on dequeueable events is obtained by using select or
poll system calls on video devices. The V4L2 events use POLLPRI events
on poll system call and exceptions on select system call.

Starting with kernel 3.1 certain guarantees can be given with regards to
events:

1. Each subscribed event has its own internal dedicated event queue.
   This means that flooding of one event type will not interfere with
   other event types.
2. If the internal event queue for a particular subscribed event becomes
   full, then the oldest event in that queue will be dropped.
3. Where applicable, certain event types can ensure that the payload of
   the oldest event that is about to be dropped will be merged with the
   payload of the next oldest event. Thus ensuring that no information
   is lost, but only an intermediate step leading up to that
   information. See the documentation for the event you want to
   subscribe to whether this is applicable for that event or not.
