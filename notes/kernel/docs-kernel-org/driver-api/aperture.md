# Managing Ownership of the Framebuffer Aperture

> 출처(원문): https://docs.kernel.org/driver-api/aperture.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Managing Ownership of the Framebuffer Aperture

A graphics device might be supported by different drivers, but only one
driver can be active at any given time. Many systems load a generic
graphics drivers, such as EFI-GOP or VESA, early during the boot process.
During later boot stages, they replace the generic driver with a dedicated,
hardware-specific driver. To take over the device, the dedicated driver
first has to remove the generic driver. Aperture functions manage
ownership of framebuffer memory and hand-over between drivers.

Graphics drivers should call [`aperture_remove_conflicting_devices()`](#c.aperture_remove_conflicting_devices "aperture_remove_conflicting_devices")
at the top of their probe function. The function removes any generic
driver that is currently associated with the given framebuffer memory.
An example for a graphics device on the platform bus is shown below.

```
static int example_probe(struct platform_device *pdev)
{
        struct resource *mem;
        resource_size_t base, size;
        int ret;

        mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        if (!mem)
                return -ENODEV;
        base = mem->start;
        size = resource_size(mem);

        ret = aperture_remove_conflicting_devices(base, size, "example");
        if (ret)
                return ret;

        // Initialize the hardware
        ...

        return 0;
}

static const struct platform_driver example_driver = {
        .probe = example_probe,
        ...
};
```

The given example reads the platform device’s I/O-memory range from the
device instance. An active framebuffer will be located within this range.
The call to [`aperture_remove_conflicting_devices()`](#c.aperture_remove_conflicting_devices "aperture_remove_conflicting_devices") releases drivers that
have previously claimed ownership of the range and are currently driving
output on the framebuffer. If successful, the new driver can take over
the device.

While the given example uses a platform device, the aperture helpers work
with every bus that has an addressable framebuffer. In the case of PCI,
device drivers can also call [`aperture_remove_conflicting_pci_devices()`](#c.aperture_remove_conflicting_pci_devices "aperture_remove_conflicting_pci_devices") and
let the function detect the apertures automatically. Device drivers without
knowledge of the framebuffer’s location can call
[`aperture_remove_all_conflicting_devices()`](#c.aperture_remove_all_conflicting_devices "aperture_remove_all_conflicting_devices"), which removes all known devices.

Drivers that are susceptible to being removed by other drivers, such as
generic EFI or VESA drivers, have to register themselves as owners of their
framebuffer apertures. Ownership of the framebuffer memory is achieved
by calling [`devm_aperture_acquire_for_platform_device()`](#c.devm_aperture_acquire_for_platform_device "devm_aperture_acquire_for_platform_device"). If successful, the
driver is the owner of the framebuffer range. The function fails if the
framebuffer is already owned by another driver. See below for an example.

```
static int generic_probe(struct platform_device *pdev)
{
        struct resource *mem;
        resource_size_t base, size;

        mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        if (!mem)
                return -ENODEV;
        base = mem->start;
        size = resource_size(mem);

        ret = devm_aperture_acquire_for_platform_device(pdev, base, size);
        if (ret)
                return ret;

        // Initialize the hardware
        ...

        return 0;
}

static int generic_remove(struct platform_device *)
{
        // Hot-unplug the device
        ...

        return 0;
}

static const struct platform_driver generic_driver = {
        .probe = generic_probe,
        .remove = generic_remove,
        ...
};
```

The similar to the previous example, the generic driver claims ownership
of the framebuffer memory from its probe function. This will fail if the
memory range, or parts of it, is already owned by another driver.

If successful, the generic driver is now subject to forced removal by
another driver. This only works for platform drivers that support hot
unplugging. When a driver calls [`aperture_remove_conflicting_devices()`](#c.aperture_remove_conflicting_devices "aperture_remove_conflicting_devices")
et al for the registered framebuffer range, the aperture helpers call
[`platform_device_unregister()`](infrastructure.html#c.platform_device_unregister "platform_device_unregister") and the generic driver unloads itself. The
generic driver also has to provide a remove function to make this work.
Once hot unplugged from hardware, it may not access the device’s
registers, framebuffer memory, ROM, etc afterwards.

int aperture\_remove\_all\_conflicting\_devices(const char \*name)
:   remove all existing framebuffers

**Parameters**

`const char *name`
:   a descriptive name of the requesting driver

**Description**

This function removes all graphics device drivers. Use this function on systems
that can have their framebuffer located anywhere in memory.

**Return**

0 on success, or a negative errno code otherwise

int devm\_aperture\_acquire\_for\_platform\_device(struct platform\_device \*pdev, resource\_size\_t base, resource\_size\_t size)
:   Acquires ownership of an aperture on behalf of a platform device.

**Parameters**

`struct platform_device *pdev`
:   the platform device to own the aperture

`resource_size_t base`
:   the aperture’s byte offset in physical memory

`resource_size_t size`
:   the aperture size in bytes

**Description**

Installs the given device as the new owner of the aperture. The function
expects the aperture to be provided by a platform device. If another
driver takes over ownership of the aperture, aperture helpers will then
unregister the platform device automatically. All acquired apertures are
released automatically when the underlying device goes away.

The function fails if the aperture, or parts of it, is currently
owned by another device. To evict current owners, callers should use
`remove_conflicting_devices()` et al. before calling this function.

**Return**

0 on success, or a negative errno value otherwise.

int aperture\_remove\_conflicting\_devices(resource\_size\_t base, resource\_size\_t size, const char \*name)
:   remove devices in the given range

**Parameters**

`resource_size_t base`
:   the aperture’s base address in physical memory

`resource_size_t size`
:   aperture size in bytes

`const char *name`
:   a descriptive name of the requesting driver

**Description**

This function removes devices that own apertures within **base** and **size**.

**Return**

0 on success, or a negative errno code otherwise

int \_\_aperture\_remove\_legacy\_vga\_devices(struct pci\_dev \*pdev)
:   remove legacy VGA devices of a PCI devices

**Parameters**

`struct pci_dev *pdev`
:   PCI device

**Description**

This function removes VGA devices provided by **pdev**, such as a VGA
framebuffer or a console. This is useful if you have a VGA-compatible
PCI graphics device with framebuffers in non-BAR locations. Drivers
should acquire ownership of those memory areas and afterwards call
this helper to release remaining VGA devices.

If your hardware has its framebuffers accessible via PCI BARS, use
[`aperture_remove_conflicting_pci_devices()`](#c.aperture_remove_conflicting_pci_devices "aperture_remove_conflicting_pci_devices") instead. The function will
release any VGA devices automatically.

WARNING: Apparently we must remove graphics drivers before calling
:   this helper. Otherwise the vga fbdev driver falls over if
    we have vgacon configured.

**Return**

0 on success, or a negative errno code otherwise

int aperture\_remove\_conflicting\_pci\_devices(struct pci\_dev \*pdev, const char \*name)
:   remove existing framebuffers for PCI devices

**Parameters**

`struct pci_dev *pdev`
:   PCI device

`const char *name`
:   a descriptive name of the requesting driver

**Description**

This function removes devices that own apertures within any of **pdev**’s
memory bars. The function assumes that PCI device with shadowed ROM
drives a primary display and therefore kicks out vga16fb as well.

**Return**

0 on success, or a negative errno code otherwise
