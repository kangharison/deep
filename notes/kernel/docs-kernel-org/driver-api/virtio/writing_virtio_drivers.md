# Writing Virtio Drivers

> 출처(원문): https://docs.kernel.org/driver-api/virtio/writing_virtio_drivers.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Writing Virtio Drivers

## Introduction

This document serves as a basic guideline for driver programmers that
need to hack a new virtio driver or understand the essentials of the
existing ones. See [Virtio on Linux](virtio.html#virtio) for a general
overview of virtio.

## Driver boilerplate

As a bare minimum, a virtio driver needs to register in the virtio bus
and configure the virtqueues for the device according to its spec, the
configuration of the virtqueues in the driver side must match the
virtqueue definitions in the device. A basic driver skeleton could look
like this:

```
#include <linux/virtio.h>
#include <linux/virtio_ids.h>
#include <linux/virtio_config.h>
#include <linux/module.h>

/* device private data (one per device) */
struct virtio_dummy_dev {
        struct virtqueue *vq;
};

static void virtio_dummy_recv_cb(struct virtqueue *vq)
{
        struct virtio_dummy_dev *dev = vq->vdev->priv;
        char *buf;
        unsigned int len;

        while ((buf = virtqueue_get_buf(dev->vq, &len)) != NULL) {
                /* process the received data */
        }
}

static int virtio_dummy_probe(struct virtio_device *vdev)
{
        struct virtio_dummy_dev *dev = NULL;

        /* initialize device data */
        dev = kzalloc(sizeof(struct virtio_dummy_dev), GFP_KERNEL);
        if (!dev)
                return -ENOMEM;

        /* the device has a single virtqueue */
        dev->vq = virtio_find_single_vq(vdev, virtio_dummy_recv_cb, "input");
        if (IS_ERR(dev->vq)) {
                kfree(dev);
                return PTR_ERR(dev->vq);

        }
        vdev->priv = dev;

        /* from this point on, the device can notify and get callbacks */
        virtio_device_ready(vdev);

        return 0;
}

static void virtio_dummy_remove(struct virtio_device *vdev)
{
        struct virtio_dummy_dev *dev = vdev->priv;

        /*
         * disable vq interrupts: equivalent to
         * vdev->config->reset(vdev)
         */
        virtio_reset_device(vdev);

        /* detach unused buffers */
        while ((buf = virtqueue_detach_unused_buf(dev->vq)) != NULL) {
                kfree(buf);
        }

        /* remove virtqueues */
        vdev->config->del_vqs(vdev);

        kfree(dev);
}

static const struct virtio_device_id id_table[] = {
        { VIRTIO_ID_DUMMY, VIRTIO_DEV_ANY_ID },
        { 0 },
};

static struct virtio_driver virtio_dummy_driver = {
        .driver.name =  KBUILD_MODNAME,
        .id_table =     id_table,
        .probe =        virtio_dummy_probe,
        .remove =       virtio_dummy_remove,
};

module_virtio_driver(virtio_dummy_driver);
MODULE_DEVICE_TABLE(virtio, id_table);
MODULE_DESCRIPTION("Dummy virtio driver");
MODULE_LICENSE("GPL");
```

The device id `VIRTIO_ID_DUMMY` here is a placeholder, virtio drivers
should be added only for devices that are defined in the spec, see
include/uapi/linux/virtio\_ids.h. Device ids need to be at least reserved
in the virtio spec before being added to that file.

If your driver doesn’t have to do anything special in its `init` and
`exit` methods, you can use the `module_virtio_driver()` helper to
reduce the amount of boilerplate code.

The `probe` method does the minimum driver setup in this case
(memory allocation for the device data) and initializes the
virtqueue. [`virtio_device_ready()`](#c.virtio_device_ready "virtio_device_ready") is used to enable the virtqueue and to
notify the device that the driver is ready to manage the device
(“DRIVER\_OK”). The virtqueues are anyway enabled automatically by the
core after `probe` returns.

void virtio\_device\_ready(struct virtio\_device \*dev)
:   enable vq use in probe function

**Parameters**

`struct virtio_device *dev`
:   the virtio device

**Description**

Driver must call this to use vqs in the probe function.

**Note**

vqs are enabled automatically after probe returns.

In any case, the virtqueues need to be enabled before adding buffers to
them.

## Sending and receiving data

The `virtio_dummy_recv_cb()` callback in the code above will be triggered
when the device notifies the driver after it finishes processing a
descriptor or descriptor chain, either for reading or writing. However,
that’s only the second half of the virtio device-driver communication
process, as the communication is always started by the driver regardless
of the direction of the data transfer.

To configure a buffer transfer from the driver to the device, first you
have to add the buffers -- packed as scatterlists -- to the
appropriate virtqueue using any of the [`virtqueue_add_inbuf()`](#c.virtqueue_add_inbuf "virtqueue_add_inbuf"),
[`virtqueue_add_outbuf()`](#c.virtqueue_add_outbuf "virtqueue_add_outbuf") or [`virtqueue_add_sgs()`](#c.virtqueue_add_sgs "virtqueue_add_sgs"), depending on whether you
need to add one input scatterlist (for the device to fill in), one
output scatterlist (for the device to consume) or multiple
scatterlists, respectively. Then, once the virtqueue is set up, a call
to `virtqueue_kick()` sends a notification that will be serviced by the
hypervisor that implements the device:

```
struct scatterlist sg[1];
sg_init_one(sg, buffer, BUFLEN);
virtqueue_add_inbuf(dev->vq, sg, 1, buffer, GFP_ATOMIC);
virtqueue_kick(dev->vq);
```

int virtqueue\_add\_inbuf(struct [virtqueue](virtio.html#c.virtqueue "virtqueue") \*vq, struct scatterlist \*sg, unsigned int num, void \*data, gfp\_t gfp)
:   expose input buffers to other end

**Parameters**

`struct virtqueue *vq`
:   the [`struct virtqueue`](virtio.html#c.virtqueue "virtqueue") we’re talking about.

`struct scatterlist *sg`
:   scatterlist (must be well-formed and terminated!)

`unsigned int num`
:   the number of entries in **sg** writable by other side

`void *data`
:   the token identifying the buffer.

`gfp_t gfp`
:   how to do memory allocations (if necessary).

**Description**

Caller must ensure we don’t call this with other virtqueue operations
at the same time (except where noted).

Returns zero or a negative error (ie. ENOSPC, ENOMEM, EIO).

int virtqueue\_add\_outbuf(struct [virtqueue](virtio.html#c.virtqueue "virtqueue") \*vq, struct scatterlist \*sg, unsigned int num, void \*data, gfp\_t gfp)
:   expose output buffers to other end

**Parameters**

`struct virtqueue *vq`
:   the [`struct virtqueue`](virtio.html#c.virtqueue "virtqueue") we’re talking about.

`struct scatterlist *sg`
:   scatterlist (must be well-formed and terminated!)

`unsigned int num`
:   the number of entries in **sg** readable by other side

`void *data`
:   the token identifying the buffer.

`gfp_t gfp`
:   how to do memory allocations (if necessary).

**Description**

Caller must ensure we don’t call this with other virtqueue operations
at the same time (except where noted).

Returns zero or a negative error (ie. ENOSPC, ENOMEM, EIO).

int virtqueue\_add\_sgs(struct [virtqueue](virtio.html#c.virtqueue "virtqueue") \*\_vq, struct scatterlist \*sgs[], unsigned int out\_sgs, unsigned int in\_sgs, void \*data, gfp\_t gfp)
:   expose buffers to other end

**Parameters**

`struct virtqueue *_vq`
:   the [`struct virtqueue`](virtio.html#c.virtqueue "virtqueue") we’re talking about.

`struct scatterlist *sgs[]`
:   array of terminated scatterlists.

`unsigned int out_sgs`
:   the number of scatterlists readable by other side

`unsigned int in_sgs`
:   the number of scatterlists which are writable (after readable ones)

`void *data`
:   the token identifying the buffer.

`gfp_t gfp`
:   how to do memory allocations (if necessary).

**Description**

Caller must ensure we don’t call this with other virtqueue operations
at the same time (except where noted).

Returns zero or a negative error (ie. ENOSPC, ENOMEM, EIO).

NB: ENOSPC is a special code that is only returned on an attempt to add a
buffer to a full VQ. It indicates that some buffers are outstanding and that
the operation can be retried after some buffers have been used.

Then, after the device has read or written the buffers prepared by the
driver and notifies it back, the driver can call `virtqueue_get_buf()` to
read the data produced by the device (if the virtqueue was set up with
input buffers) or simply to reclaim the buffers if they were already
consumed by the device:

void \*virtqueue\_get\_buf\_ctx(struct [virtqueue](virtio.html#c.virtqueue "virtqueue") \*\_vq, unsigned int \*len, void \*\*ctx)
:   get the next used buffer

**Parameters**

`struct virtqueue *_vq`
:   the [`struct virtqueue`](virtio.html#c.virtqueue "virtqueue") we’re talking about.

`unsigned int *len`
:   the length written into the buffer

`void **ctx`
:   extra context for the token

**Description**

If the device wrote data into the buffer, **len** will be set to the
amount written. This means you don’t need to clear the buffer
beforehand to ensure there’s no data leakage in the case of short
writes.

Caller must ensure we don’t call this with other virtqueue
operations at the same time (except where noted).

Returns NULL if there are no used buffers, or the “data” token
handed to virtqueue\_add\_\*().

The virtqueue callbacks can be disabled and re-enabled using the
[`virtqueue_disable_cb()`](#c.virtqueue_disable_cb "virtqueue_disable_cb") and the family of [`virtqueue_enable_cb()`](#c.virtqueue_enable_cb "virtqueue_enable_cb") functions
respectively. See drivers/virtio/virtio\_ring.c for more details:

void virtqueue\_disable\_cb(struct [virtqueue](virtio.html#c.virtqueue "virtqueue") \*\_vq)
:   disable callbacks

**Parameters**

`struct virtqueue *_vq`
:   the [`struct virtqueue`](virtio.html#c.virtqueue "virtqueue") we’re talking about.

**Description**

Note that this is not necessarily synchronous, hence unreliable and only
useful as an optimization.

Unlike other operations, this need not be serialized.

bool virtqueue\_enable\_cb(struct [virtqueue](virtio.html#c.virtqueue "virtqueue") \*\_vq)
:   restart callbacks after disable\_cb.

**Parameters**

`struct virtqueue *_vq`
:   the [`struct virtqueue`](virtio.html#c.virtqueue "virtqueue") we’re talking about.

**Description**

This re-enables callbacks; it returns “false” if there are pending
buffers in the queue, to detect a possible race between the driver
checking for more work, and enabling callbacks.

Caller must ensure we don’t call this with other virtqueue
operations at the same time (except where noted).

But note that some spurious callbacks can still be triggered under
certain scenarios. The way to disable callbacks reliably is to reset the
device or the virtqueue (`virtio_reset_device()`).

## References

[1] Virtio Spec v1.2:
<https://docs.oasis-open.org/virtio/virtio/v1.2/virtio-v1.2.html>

Check for later versions of the spec as well.
