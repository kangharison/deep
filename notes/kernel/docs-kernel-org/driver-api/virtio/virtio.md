# Virtio on Linux

> 출처(원문): https://docs.kernel.org/driver-api/virtio/virtio.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Virtio on Linux

## Introduction

Virtio is an open standard that defines a protocol for communication
between drivers and devices of different types, see Chapter 5 (“Device
Types”) of the virtio spec ([[1]](#id2)). Originally developed as a standard
for paravirtualized devices implemented by a hypervisor, it can be used
to interface any compliant device (real or emulated) with a driver.

For illustrative purposes, this document will focus on the common case
of a Linux kernel running in a virtual machine and using paravirtualized
devices provided by the hypervisor, which exposes them as virtio devices
via standard mechanisms such as PCI.

## Device - Driver communication: virtqueues

Although the virtio devices are really an abstraction layer in the
hypervisor, they’re exposed to the guest as if they are physical devices
using a specific transport method -- PCI, MMIO or CCW -- that is
orthogonal to the device itself. The virtio spec defines these transport
methods in detail, including device discovery, capabilities and
interrupt handling.

The communication between the driver in the guest OS and the device in
the hypervisor is done through shared memory (that’s what makes virtio
devices so efficient) using specialized data structures called
virtqueues, which are actually ring buffers [[1]](#f1) of buffer descriptors
similar to the ones used in a network device:

struct vring\_desc
:   Virtio ring descriptors, 16 bytes long. These can chain together via **next**.

**Definition**:

```
struct vring_desc {
    __virtio64 addr;
    __virtio32 len;
    __virtio16 flags;
    __virtio16 next;
};
```

**Members**

`addr`
:   buffer address (guest-physical)

`len`
:   buffer length

`flags`
:   descriptor flags

`next`
:   index of the next descriptor in the chain,
    if the VRING\_DESC\_F\_NEXT flag is set. We chain unused
    descriptors via this, too.

All the buffers the descriptors point to are allocated by the guest and
used by the host either for reading or for writing but not for both.

Refer to Chapter 2.5 (“Virtqueues”) of the virtio spec ([[1]](#id2)) for the
reference definitions of virtqueues and “Virtqueues and virtio ring: How
the data travels” blog post ([[2]](#id3)) for an illustrated overview of how
the host device and the guest driver communicate.

The `vring_virtqueue` `struct models` a virtqueue, including the
ring buffers and management data. Embedded in this `struct is` the
[`virtqueue`](#c.virtqueue "virtqueue") struct, which is the data structure that’s
ultimately used by virtio drivers:

struct virtqueue
:   a queue to register buffers for sending or receiving.

**Definition**:

```
struct virtqueue {
    struct list_head list;
    void (*callback)(struct virtqueue *vq);
    const char *name;
    struct virtio_device *vdev;
    unsigned int index;
    unsigned int num_free;
    unsigned int num_max;
    bool reset;
    void *priv;
};
```

**Members**

`list`
:   the chain of virtqueues for this device

`callback`
:   the function to call when buffers are consumed (can be NULL).

`name`
:   the name of this virtqueue (mainly for debugging)

`vdev`
:   the virtio device this queue was created for.

`index`
:   the zero-based ordinal number for this queue.

`num_free`
:   number of elements we expect to be able to fit.

`num_max`
:   the maximum number of elements supported by the device.

`reset`
:   vq is in reset state or not.

`priv`
:   a pointer for the virtqueue implementation to use.

**Description**

A note on **num\_free**: with indirect buffers, each buffer needs one
element in the queue, otherwise a buffer will need one element per
sg element.

The callback function pointed by this `struct is` triggered when the
device has consumed the buffers provided by the driver. More
specifically, the trigger will be an interrupt issued by the hypervisor
(see [`vring_interrupt()`](#c.vring_interrupt "vring_interrupt")). Interrupt request handlers are registered for
a virtqueue during the virtqueue setup process (transport-specific).

irqreturn\_t vring\_interrupt(int irq, void \*\_vq)
:   notify a virtqueue on an interrupt

**Parameters**

`int irq`
:   the IRQ number (ignored)

`void *_vq`
:   the [`struct virtqueue`](#c.virtqueue "virtqueue") to notify

**Description**

Calls the callback function of **\_vq** to process the virtqueue
notification.

## Device discovery and probing

In the kernel, the virtio core contains the virtio bus driver and
transport-specific drivers like virtio-pci and virtio-mmio. Then
there are individual virtio drivers for specific device types that are
registered to the virtio bus driver.

How a virtio device is found and configured by the kernel depends on how
the hypervisor defines it. Taking the [QEMU virtio-console](https://gitlab.com/qemu-project/qemu/-/blob/master/hw/char/virtio-console.c)
device as an example. When using PCI as a transport method, the device
will present itself on the PCI bus with vendor 0x1af4 (Red Hat, Inc.)
and device id 0x1003 (virtio console), as defined in the spec, so the
kernel will detect it as it would do with any other PCI device.

During the PCI enumeration process, if a device is found to match the
virtio-pci driver (according to the virtio-pci device table, any PCI
device with vendor id = 0x1af4):

```
/* Qumranet donated their vendor ID for devices 0x1000 thru 0x10FF. */
static const struct pci_device_id virtio_pci_id_table[] = {
        { PCI_DEVICE(PCI_VENDOR_ID_REDHAT_QUMRANET, PCI_ANY_ID) },
        { 0 }
};
```

then the virtio-pci driver is probed and, if the probing goes well, the
device is registered to the virtio bus:

```
static int virtio_pci_probe(struct pci_dev *pci_dev,
                            const struct pci_device_id *id)
{
        ...

        if (force_legacy) {
                rc = virtio_pci_legacy_probe(vp_dev);
                /* Also try modern mode if we can't map BAR0 (no IO space). */
                if (rc == -ENODEV || rc == -ENOMEM)
                        rc = virtio_pci_modern_probe(vp_dev);
                if (rc)
                        goto err_probe;
        } else {
                rc = virtio_pci_modern_probe(vp_dev);
                if (rc == -ENODEV)
                        rc = virtio_pci_legacy_probe(vp_dev);
                if (rc)
                        goto err_probe;
        }

        ...

        rc = register_virtio_device(&vp_dev->vdev);
```

When the device is registered to the virtio bus the kernel will look
for a driver in the bus that can handle the device and call that
driver’s `probe` method.

At this point, the virtqueues will be allocated and configured by
calling the appropriate `virtio_find` helper function, such as
`virtio_find_single_vq()` or `virtio_find_vqs()`, which will end up calling
a transport-specific `find_vqs` method.

## References

[1] Virtio Spec v1.2:
<https://docs.oasis-open.org/virtio/virtio/v1.2/virtio-v1.2.html>

[2] Virtqueues and virtio ring: How the data travels
<https://www.redhat.com/en/blog/virtqueues-and-virtio-ring-how-data-travels>

Footnotes

[[1](#id1)]

that’s why they may be also referred to as virtrings.
