# 3.6.V4L2 File handles

> 출처(원문): https://docs.kernel.org/driver-api/media/v4l2-fh.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.6. V4L2 File handles

[`struct v4l2_fh`](#c.v4l2_fh "v4l2_fh") provides a way to easily keep file handle specific data that is
used by the V4L2 framework. Its usage is mandatory in all drivers.

[`struct v4l2_fh`](#c.v4l2_fh "v4l2_fh") is allocated in the driver’s `open()` file operation handler.
It is typically embedded in a larger driver-specific structure. The
[`v4l2_fh`](#c.v4l2_fh "v4l2_fh") must be initialized with a call to [`v4l2_fh_init()`](#c.v4l2_fh_init "v4l2_fh_init"),
and added to the video device with [`v4l2_fh_add()`](#c.v4l2_fh_add "v4l2_fh_add"). This associates the
[`v4l2_fh`](#c.v4l2_fh "v4l2_fh") with the [`file`](../../filesystems/api-summary.html#c.file "file") by setting `file->private_data` to
point to the [`v4l2_fh`](#c.v4l2_fh "v4l2_fh").

Similarly, the [`struct v4l2_fh`](#c.v4l2_fh "v4l2_fh") is freed in the driver’s `release()` file
operation handler. It must be removed from the video device with
[`v4l2_fh_del()`](#c.v4l2_fh_del "v4l2_fh_del") and cleaned up with [`v4l2_fh_exit()`](#c.v4l2_fh_exit "v4l2_fh_exit") before being
freed.

Drivers must not access `file->private_data` directly. They can retrieve the
[`v4l2_fh`](#c.v4l2_fh "v4l2_fh") associated with a [`file`](../../filesystems/api-summary.html#c.file "file") by calling
[`file_to_v4l2_fh()`](#c.file_to_v4l2_fh "file_to_v4l2_fh"). Drivers can extract their own file handle structure
by using the container\_of macro.

Example:

```
struct my_fh {
        int blah;
        struct v4l2_fh fh;
};

...

int my_open(struct file *file)
{
        struct my_fh *my_fh;
        struct video_device *vfd;
        int ret;

        ...

        my_fh = kzalloc(sizeof(*my_fh), GFP_KERNEL);

        ...

        v4l2_fh_init(&my_fh->fh, vfd);

        ...

        v4l2_fh_add(&my_fh->fh, file);
        return 0;
}

int my_release(struct file *file)
{
        struct v4l2_fh *fh = file_to_v4l2_fh(file);
        struct my_fh *my_fh = container_of(fh, struct my_fh, fh);

        ...
        v4l2_fh_del(&my_fh->fh, file);
        v4l2_fh_exit(&my_fh->fh);
        kfree(my_fh);
        return 0;
}
```

Below is a short description of the [`v4l2_fh`](#c.v4l2_fh "v4l2_fh") functions used:

[`v4l2_fh_init`](#c.v4l2_fh_init "v4l2_fh_init")
([`fh`](#c.v4l2_fh "v4l2_fh"), [`vdev`](v4l2-dev.html#c.video_device "video_device"))

* Initialise the file handle. This **MUST** be performed in the driver’s
  [`v4l2_file_operations`](v4l2-dev.html#c.v4l2_file_operations "v4l2_file_operations")->open() handler.

[`v4l2_fh_add`](#c.v4l2_fh_add "v4l2_fh_add")
([`fh`](#c.v4l2_fh "v4l2_fh"), [`struct file`](../../filesystems/api-summary.html#c.file "file") \*filp)

* Add a [`v4l2_fh`](#c.v4l2_fh "v4l2_fh") to [`video_device`](v4l2-dev.html#c.video_device "video_device") file handle list.
  Must be called once the file handle is completely initialized.

[`v4l2_fh_del`](#c.v4l2_fh_del "v4l2_fh_del")
([`fh`](#c.v4l2_fh "v4l2_fh"), [`struct file`](../../filesystems/api-summary.html#c.file "file") \*filp)

* Unassociate the file handle from [`video_device`](v4l2-dev.html#c.video_device "video_device"). The file handle
  exit function may now be called.

[`v4l2_fh_exit`](#c.v4l2_fh_exit "v4l2_fh_exit")
([`fh`](#c.v4l2_fh "v4l2_fh"))

* Uninitialise the file handle. After uninitialisation the [`v4l2_fh`](#c.v4l2_fh "v4l2_fh")
  memory can be freed.

[`file_to_v4l2_fh`](#c.file_to_v4l2_fh "file_to_v4l2_fh")
([`struct file`](../../filesystems/api-summary.html#c.file "file") \*filp)

* Retrieve the [`v4l2_fh`](#c.v4l2_fh "v4l2_fh") instance associated with a [`file`](../../filesystems/api-summary.html#c.file "file").

If [`struct v4l2_fh`](#c.v4l2_fh "v4l2_fh") is not embedded, then you can use these helper functions:

[`v4l2_fh_open`](#c.v4l2_fh_open "v4l2_fh_open")
([`struct file`](../../filesystems/api-summary.html#c.file "file") \*filp)

* This allocates a [`struct v4l2_fh`](#c.v4l2_fh "v4l2_fh"), initializes it and adds it to
  the [`struct video_device`](v4l2-dev.html#c.video_device "video_device") associated with the file struct.

[`v4l2_fh_release`](#c.v4l2_fh_release "v4l2_fh_release")
([`struct file`](../../filesystems/api-summary.html#c.file "file") \*filp)

* This deletes it from the [`struct video_device`](v4l2-dev.html#c.video_device "video_device") associated with the
  file struct, uninitialised the [`v4l2_fh`](#c.v4l2_fh "v4l2_fh") and frees it.

These two functions can be plugged into the v4l2\_file\_operation’s `open()`
and `release()` ops.

Several drivers need to do something when the first file handle is opened and
when the last file handle closes. Two helper functions were added to check
whether the [`v4l2_fh`](#c.v4l2_fh "v4l2_fh") `struct is` the only open filehandle of the
associated device node:

[`v4l2_fh_is_singular`](#c.v4l2_fh_is_singular "v4l2_fh_is_singular")
([`fh`](#c.v4l2_fh "v4l2_fh"))

* Returns 1 if the file handle is the only open file handle, else 0.

[`v4l2_fh_is_singular_file`](#c.v4l2_fh_is_singular_file "v4l2_fh_is_singular_file")
([`struct file`](../../filesystems/api-summary.html#c.file "file") \*filp)

* Same, but it calls v4l2\_fh\_is\_singular with filp->private\_data.

## 3.6.1. V4L2 fh functions and data structures

struct v4l2\_fh
:   Describes a V4L2 file handler

> **Definition**:
>
> ```
> struct v4l2_fh {
>       struct list_head        list;
>       struct video_device     *vdev;
>       struct v4l2_ctrl_handler *ctrl_handler;
>       enum v4l2_priority      prio;
>       wait_queue_head_t wait;
>       struct mutex            subscribe_lock;
>       struct list_head        subscribed;
>       struct list_head        available;
>       unsigned int            navailable;
>       u32 sequence;
>       struct v4l2_m2m_ctx     *m2m_ctx;
> };
> ```

**Members**

`list`
:   list of file handlers

`vdev`
:   pointer to [`struct video_device`](v4l2-dev.html#c.video_device "video_device")

`ctrl_handler`
:   pointer to [`struct v4l2_ctrl_handler`](v4l2-controls.html#c.v4l2_ctrl_handler "v4l2_ctrl_handler")

`prio`
:   priority of the file handler, as defined by `enum v4l2_priority`

`wait`
:   event’ s wait queue

`subscribe_lock`
:   serialise changes to the subscribed list; guarantee that
    the add and del event callbacks are orderly called

`subscribed`
:   list of subscribed events

`available`
:   list of events waiting to be dequeued

`navailable`
:   number of available events at **available** list

`sequence`
:   event sequence number

`m2m_ctx`
:   pointer to [`struct v4l2_m2m_ctx`](v4l2-mem2mem.html#c.v4l2_m2m_ctx "v4l2_m2m_ctx")

struct [v4l2\_fh](#c.v4l2_fh "v4l2_fh") \*file\_to\_v4l2\_fh(struct [file](../../filesystems/api-summary.html#c.file "file") \*filp)
:   Return the v4l2\_fh associated with a [`struct file`](../../filesystems/api-summary.html#c.file "file")

**Parameters**

`struct file *filp`
:   pointer to [`struct file`](../../filesystems/api-summary.html#c.file "file")

**Description**

This function should be used by drivers to retrieve the [`struct v4l2_fh`](#c.v4l2_fh "v4l2_fh")
instance pointer stored in the file private\_data instead of accessing the
private\_data field directly.

void v4l2\_fh\_init(struct [v4l2\_fh](#c.v4l2_fh "v4l2_fh") \*fh, struct [video\_device](v4l2-dev.html#c.video_device "video_device") \*vdev)
:   Initialise the file handle.

**Parameters**

`struct v4l2_fh *fh`
:   pointer to [`struct v4l2_fh`](#c.v4l2_fh "v4l2_fh")

`struct video_device *vdev`
:   pointer to [`struct video_device`](v4l2-dev.html#c.video_device "video_device")

**Description**

Parts of the V4L2 framework using the
file handles should be initialised in this function. Must be called
from driver’s v4l2\_file\_operations->open() handler if the driver
uses [`struct v4l2_fh`](#c.v4l2_fh "v4l2_fh").

void v4l2\_fh\_add(struct [v4l2\_fh](#c.v4l2_fh "v4l2_fh") \*fh, struct [file](../../filesystems/api-summary.html#c.file "file") \*filp)
:   Add the fh to the list of file handles on a video\_device.

**Parameters**

`struct v4l2_fh *fh`
:   pointer to [`struct v4l2_fh`](#c.v4l2_fh "v4l2_fh")

`struct file *filp`
:   pointer to [`struct file`](../../filesystems/api-summary.html#c.file "file") associated with **fh**

**Description**

The function sets filp->private\_data to point to **fh**.

Note

The **fh** file handle must be initialised first.

int v4l2\_fh\_open(struct [file](../../filesystems/api-summary.html#c.file "file") \*filp)
:   Ancillary routine that can be used as the open() op of v4l2\_file\_operations.

**Parameters**

`struct file *filp`
:   pointer to [`struct file`](../../filesystems/api-summary.html#c.file "file")

**Description**

It allocates a v4l2\_fh and inits and adds it to the [`struct video_device`](v4l2-dev.html#c.video_device "video_device")
associated with the file pointer.

On error filp->private\_data will be `NULL`, otherwise it will point to
the [`struct v4l2_fh`](#c.v4l2_fh "v4l2_fh").

void v4l2\_fh\_del(struct [v4l2\_fh](#c.v4l2_fh "v4l2_fh") \*fh, struct [file](../../filesystems/api-summary.html#c.file "file") \*filp)
:   Remove file handle from the list of file handles.

**Parameters**

`struct v4l2_fh *fh`
:   pointer to [`struct v4l2_fh`](#c.v4l2_fh "v4l2_fh")

`struct file *filp`
:   pointer to [`struct file`](../../filesystems/api-summary.html#c.file "file") associated with **fh**

**Description**

The function resets filp->private\_data to NULL.

Note

Must be called in v4l2\_file\_operations->`release()` handler if the driver
uses [`struct v4l2_fh`](#c.v4l2_fh "v4l2_fh").

void v4l2\_fh\_exit(struct [v4l2\_fh](#c.v4l2_fh "v4l2_fh") \*fh)
:   Release resources related to a file handle.

**Parameters**

`struct v4l2_fh *fh`
:   pointer to [`struct v4l2_fh`](#c.v4l2_fh "v4l2_fh")

**Description**

Parts of the V4L2 framework using the v4l2\_fh must release their
resources here, too.

Note

Must be called in v4l2\_file\_operations->`release()` handler if the
driver uses [`struct v4l2_fh`](#c.v4l2_fh "v4l2_fh").

int v4l2\_fh\_release(struct [file](../../filesystems/api-summary.html#c.file "file") \*filp)
:   Ancillary routine that can be used as the `release()` op of v4l2\_file\_operations.

**Parameters**

`struct file *filp`
:   pointer to [`struct file`](../../filesystems/api-summary.html#c.file "file")

**Description**

It deletes and exits the v4l2\_fh associated with the file pointer and
frees it. It will do nothing if filp->private\_data (the pointer to the
v4l2\_fh struct) is `NULL`.

This function always returns 0.

int v4l2\_fh\_is\_singular(struct [v4l2\_fh](#c.v4l2_fh "v4l2_fh") \*fh)
:   Returns 1 if this filehandle is the only filehandle opened for the associated video\_device.

**Parameters**

`struct v4l2_fh *fh`
:   pointer to [`struct v4l2_fh`](#c.v4l2_fh "v4l2_fh")

**Description**

If **fh** is NULL, then it returns 0.

int v4l2\_fh\_is\_singular\_file(struct [file](../../filesystems/api-summary.html#c.file "file") \*filp)
:   Returns 1 if this filehandle is the only filehandle opened for the associated video\_device.

**Parameters**

`struct file *filp`
:   pointer to [`struct file`](../../filesystems/api-summary.html#c.file "file")

**Description**

This is a helper function variant of [`v4l2_fh_is_singular()`](#c.v4l2_fh_is_singular "v4l2_fh_is_singular") with uses
[`struct file`](../../filesystems/api-summary.html#c.file "file") as argument.

If filp->private\_data is `NULL`, then it will return 0.
