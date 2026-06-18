# Device drivers infrastructure

> 출처(원문): https://docs.kernel.org/driver-api/infrastructure.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Device drivers infrastructure

## The Basic Device Driver-Model Structures

struct subsys\_interface
:   interfaces to device functions

**Definition**:

```
struct subsys_interface {
    const char *name;
    const struct bus_type *subsys;
    struct list_head node;
    int (*add_dev)(struct device *dev, struct subsys_interface *sif);
    void (*remove_dev)(struct device *dev, struct subsys_interface *sif);
};
```

**Members**

`name`
:   name of the device function

`subsys`
:   subsystem of the devices to attach to

`node`
:   the list of functions registered at the subsystem

`add_dev`
:   device hookup to device function handler

`remove_dev`
:   device hookup to device function handler

**Description**

Simple interfaces attached to a subsystem. Multiple interfaces can
attach to a subsystem and its devices. Unlike drivers, they do not
exclusively claim or control devices. Interfaces usually represent
a specific functionality of a subsystem/class of devices.

struct device\_attribute
:   Interface for exporting device attributes.

**Definition**:

```
struct device_attribute {
    struct attribute        attr;
    ssize_t (*show)(struct device *dev, struct device_attribute *attr, char *buf);
    ssize_t (*store)(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
};
```

**Members**

`attr`
:   sysfs attribute definition.

`show`
:   Show handler.

`store`
:   Store handler.

struct dev\_ext\_attribute
:   Exported device attribute with extra context.

**Definition**:

```
struct dev_ext_attribute {
    struct device_attribute attr;
    void *var;
};
```

**Members**

`attr`
:   Exported device attribute.

`var`
:   Pointer to context.

DEVICE\_ATTR

`DEVICE_ATTR (_name, _mode, _show, _store)`

> Define a device attribute.

**Parameters**

`_name`
:   Attribute name.

`_mode`
:   File mode.

`_show`
:   Show handler. Optional, but mandatory if attribute is readable.

`_store`
:   Store handler. Optional, but mandatory if attribute is writable.

**Description**

Convenience macro for defining a [`struct device_attribute`](#c.device_attribute "device_attribute").

For example, `DEVICE_ATTR(foo, 0644, foo_show, foo_store);` expands to:

```
struct device_attribute dev_attr_foo = {
        .attr   = { .name = "foo", .mode = 0644 },
        .show   = foo_show,
        .store  = foo_store,
};
```

DEVICE\_ATTR\_PREALLOC

`DEVICE_ATTR_PREALLOC (_name, _mode, _show, _store)`

> Define a preallocated device attribute.

**Parameters**

`_name`
:   Attribute name.

`_mode`
:   File mode.

`_show`
:   Show handler. Optional, but mandatory if attribute is readable.

`_store`
:   Store handler. Optional, but mandatory if attribute is writable.

**Description**

Like [`DEVICE_ATTR()`](#c.DEVICE_ATTR "DEVICE_ATTR"), but `SYSFS_PREALLOC` is set on **\_mode**.

DEVICE\_ATTR\_RW

`DEVICE_ATTR_RW (_name)`

> Define a read-write device attribute.

**Parameters**

`_name`
:   Attribute name.

**Description**

Like [`DEVICE_ATTR()`](#c.DEVICE_ATTR "DEVICE_ATTR"), but **\_mode** is 0644, **\_show** is <\_name>\_show,
and **\_store** is <\_name>\_store.

DEVICE\_ATTR\_ADMIN\_RW

`DEVICE_ATTR_ADMIN_RW (_name)`

> Define an admin-only read-write device attribute.

**Parameters**

`_name`
:   Attribute name.

**Description**

Like [`DEVICE_ATTR_RW()`](#c.DEVICE_ATTR_RW "DEVICE_ATTR_RW"), but **\_mode** is 0600.

DEVICE\_ATTR\_RW\_NAMED

`DEVICE_ATTR_RW_NAMED (_name, _attrname)`

> Define a read-write device attribute with a sysfs name that differs from the function name.

**Parameters**

`_name`
:   Attribute function preface

`_attrname`
:   Attribute name as it wil be exposed in the sysfs.

**Description**

Like [`DEVICE_ATTR_RW()`](#c.DEVICE_ATTR_RW "DEVICE_ATTR_RW"), but allows for reusing names under separate paths in
the same driver.

DEVICE\_ATTR\_RO

`DEVICE_ATTR_RO (_name)`

> Define a readable device attribute.

**Parameters**

`_name`
:   Attribute name.

**Description**

Like [`DEVICE_ATTR()`](#c.DEVICE_ATTR "DEVICE_ATTR"), but **\_mode** is 0444 and **\_show** is <\_name>\_show.

DEVICE\_ATTR\_ADMIN\_RO

`DEVICE_ATTR_ADMIN_RO (_name)`

> Define an admin-only readable device attribute.

**Parameters**

`_name`
:   Attribute name.

**Description**

Like [`DEVICE_ATTR_RO()`](#c.DEVICE_ATTR_RO "DEVICE_ATTR_RO"), but **\_mode** is 0400.

DEVICE\_ATTR\_RO\_NAMED

`DEVICE_ATTR_RO_NAMED (_name, _attrname)`

> Define a read-only device attribute with a sysfs name that differs from the function name.

**Parameters**

`_name`
:   Attribute function preface

`_attrname`
:   Attribute name as it wil be exposed in the sysfs.

**Description**

Like [`DEVICE_ATTR_RO()`](#c.DEVICE_ATTR_RO "DEVICE_ATTR_RO"), but allows for reusing names under separate paths in
the same driver.

DEVICE\_ATTR\_WO

`DEVICE_ATTR_WO (_name)`

> Define an admin-only writable device attribute.

**Parameters**

`_name`
:   Attribute name.

**Description**

Like [`DEVICE_ATTR()`](#c.DEVICE_ATTR "DEVICE_ATTR"), but **\_mode** is 0200 and **\_store** is <\_name>\_store.

DEVICE\_ATTR\_WO\_NAMED

`DEVICE_ATTR_WO_NAMED (_name, _attrname)`

> Define a read-only device attribute with a sysfs name that differs from the function name.

**Parameters**

`_name`
:   Attribute function preface

`_attrname`
:   Attribute name as it wil be exposed in the sysfs.

**Description**

Like [`DEVICE_ATTR_WO()`](#c.DEVICE_ATTR_WO "DEVICE_ATTR_WO"), but allows for reusing names under separate paths in
the same driver.

DEVICE\_ULONG\_ATTR

`DEVICE_ULONG_ATTR (_name, _mode, _var)`

> Define a device attribute backed by an unsigned long.

**Parameters**

`_name`
:   Attribute name.

`_mode`
:   File mode.

`_var`
:   Identifier of unsigned long.

**Description**

Like [`DEVICE_ATTR()`](#c.DEVICE_ATTR "DEVICE_ATTR"), but **\_show** and **\_store** are automatically provided
such that reads and writes to the attribute from userspace affect **\_var**.

DEVICE\_INT\_ATTR

`DEVICE_INT_ATTR (_name, _mode, _var)`

> Define a device attribute backed by an int.

**Parameters**

`_name`
:   Attribute name.

`_mode`
:   File mode.

`_var`
:   Identifier of int.

**Description**

Like [`DEVICE_ULONG_ATTR()`](#c.DEVICE_ULONG_ATTR "DEVICE_ULONG_ATTR"), but **\_var** is an int.

DEVICE\_BOOL\_ATTR

`DEVICE_BOOL_ATTR (_name, _mode, _var)`

> Define a device attribute backed by a bool.

**Parameters**

`_name`
:   Attribute name.

`_mode`
:   File mode.

`_var`
:   Identifier of bool.

**Description**

Like [`DEVICE_ULONG_ATTR()`](#c.DEVICE_ULONG_ATTR "DEVICE_ULONG_ATTR"), but **\_var** is a bool.

DEVICE\_STRING\_ATTR\_RO

`DEVICE_STRING_ATTR_RO (_name, _mode, _var)`

> Define a device attribute backed by a r/o string.

**Parameters**

`_name`
:   Attribute name.

`_mode`
:   File mode.

`_var`
:   Identifier of string.

**Description**

Like [`DEVICE_ULONG_ATTR()`](#c.DEVICE_ULONG_ATTR "DEVICE_ULONG_ATTR"), but **\_var** is a string. Because the length of the
string allocation is unknown, the attribute must be read-only.

enum dl\_dev\_state
:   Device driver presence tracking information.

**Constants**

`DL_DEV_NO_DRIVER`
:   There is no driver attached to the device.

`DL_DEV_PROBING`
:   A driver is probing.

`DL_DEV_DRIVER_BOUND`
:   The driver has been bound to the device.

`DL_DEV_UNBINDING`
:   The driver is unbinding from the device.

enum device\_removable
:   Whether the device is removable. The criteria for a device to be classified as removable is determined by its subsystem or bus.

**Constants**

`DEVICE_REMOVABLE_NOT_SUPPORTED`
:   This attribute is not supported for this
    device (default).

`DEVICE_REMOVABLE_UNKNOWN`
:   Device location is Unknown.

`DEVICE_FIXED`
:   Device is not removable by the user.

`DEVICE_REMOVABLE`
:   Device is removable by the user.

struct dev\_links\_info
:   Device data related to device links.

**Definition**:

```
struct dev_links_info {
    struct list_head suppliers;
    struct list_head consumers;
    struct list_head defer_sync;
    enum dl_dev_state status;
};
```

**Members**

`suppliers`
:   List of links to supplier devices.

`consumers`
:   List of links to consumer devices.

`defer_sync`
:   Hook to global list of devices that have deferred sync\_state.

`status`
:   Driver status information.

struct dev\_msi\_info
:   Device data related to MSI

**Definition**:

```
struct dev_msi_info {
#ifdef CONFIG_GENERIC_MSI_IRQ;
    struct irq_domain       *domain;
    struct msi_device_data  *data;
#endif;
};
```

**Members**

`domain`
:   The MSI interrupt domain associated to the device

`data`
:   Pointer to MSI device data

enum device\_physical\_location\_panel
:   Describes which panel surface of the system’s housing the device connection point resides on.

**Constants**

`DEVICE_PANEL_TOP`
:   Device connection point is on the top panel.

`DEVICE_PANEL_BOTTOM`
:   Device connection point is on the bottom panel.

`DEVICE_PANEL_LEFT`
:   Device connection point is on the left panel.

`DEVICE_PANEL_RIGHT`
:   Device connection point is on the right panel.

`DEVICE_PANEL_FRONT`
:   Device connection point is on the front panel.

`DEVICE_PANEL_BACK`
:   Device connection point is on the back panel.

`DEVICE_PANEL_UNKNOWN`
:   The panel with device connection point is unknown.

enum device\_physical\_location\_vertical\_position
:   Describes vertical position of the device connection point on the panel surface.

**Constants**

`DEVICE_VERT_POS_UPPER`
:   Device connection point is at upper part of panel.

`DEVICE_VERT_POS_CENTER`
:   Device connection point is at center part of panel.

`DEVICE_VERT_POS_LOWER`
:   Device connection point is at lower part of panel.

enum device\_physical\_location\_horizontal\_position
:   Describes horizontal position of the device connection point on the panel surface.

**Constants**

`DEVICE_HORI_POS_LEFT`
:   Device connection point is at left part of panel.

`DEVICE_HORI_POS_CENTER`
:   Device connection point is at center part of panel.

`DEVICE_HORI_POS_RIGHT`
:   Device connection point is at right part of panel.

struct device\_physical\_location
:   Device data related to physical location of the device connection point.

**Definition**:

```
struct device_physical_location {
    enum device_physical_location_panel panel;
    enum device_physical_location_vertical_position vertical_position;
    enum device_physical_location_horizontal_position horizontal_position;
    bool dock;
    bool lid;
};
```

**Members**

`panel`
:   Panel surface of the system’s housing that the device connection
    point resides on.

`vertical_position`
:   Vertical position of the device connection point within
    the panel.

`horizontal_position`
:   Horizontal position of the device connection point
    within the panel.

`dock`
:   Set if the device connection point resides in a docking station or
    port replicator.

`lid`
:   Set if this device connection point resides on the lid of laptop
    system.

enum struct\_device\_flags
:   Flags in [`struct device`](#c.device "device")

**Constants**

`DEV_FLAG_READY_TO_PROBE`
:   If set then [`device_add()`](#c.device_add "device_add") has finished enough
    initialization that probe could be called.

`DEV_FLAG_COUNT`
:   Number of defined struct\_device\_flags.

**Description**

Each flag should have a set of accessor functions created via
`__create_dev_flag_accessors()` for each access.

struct device
:   The basic device structure

**Definition**:

```
struct device {
    struct kobject kobj;
    struct device           *parent;
    struct device_private   *p;
    const char              *init_name;
    const struct device_type *type;
    const struct bus_type   *bus;
    struct device_driver *driver;
    void *platform_data;
    void *driver_data;
    struct {
        const char      *name;
        spinlock_t lock;
    } driver_override;
    struct mutex            mutex;
    struct dev_links_info   links;
    struct dev_pm_info      power;
    struct dev_pm_domain    *pm_domain;
#ifdef CONFIG_ENERGY_MODEL;
    struct em_perf_domain   *em_pd;
#endif;
#ifdef CONFIG_PINCTRL;
    struct dev_pin_info     *pins;
#endif;
    struct dev_msi_info     msi;
#ifdef CONFIG_ARCH_HAS_DMA_OPS;
    const struct dma_map_ops *dma_ops;
#endif;
    u64 *dma_mask;
    u64 coherent_dma_mask;
    u64 bus_dma_limit;
    const struct bus_dma_region *dma_range_map;
    struct device_dma_parameters *dma_parms;
    struct list_head        dma_pools;
#ifdef CONFIG_DMA_DECLARE_COHERENT;
    struct dma_coherent_mem *dma_mem;
#endif;
#ifdef CONFIG_DMA_CMA;
    struct cma *cma_area;
#endif;
#ifdef CONFIG_SWIOTLB;
    struct io_tlb_mem *dma_io_tlb_mem;
#endif;
#ifdef CONFIG_SWIOTLB_DYNAMIC;
    struct list_head dma_io_tlb_pools;
    spinlock_t dma_io_tlb_lock;
    bool dma_uses_io_tlb;
#endif;
    struct dev_archdata     archdata;
    struct device_node      *of_node;
    struct fwnode_handle    *fwnode;
#ifdef CONFIG_NUMA;
    int numa_node;
#endif;
    dev_t devt;
    u32 id;
    spinlock_t devres_lock;
    struct list_head        devres_head;
    const struct class      *class;
    const struct attribute_group **groups;
    void (*release)(struct device *dev);
    struct iommu_group      *iommu_group;
    struct dev_iommu        *iommu;
    struct device_physical_location *physical_location;
    enum device_removable   removable;
    bool offline_disabled:1;
    bool offline:1;
    bool of_node_reused:1;
    bool state_synced:1;
    bool can_match:1;
#if defined(CONFIG_ARCH_HAS_SYNC_DMA_FOR_DEVICE) ||     defined(CONFIG_ARCH_HAS_SYNC_DMA_FOR_CPU) ||     defined(CONFIG_ARCH_HAS_SYNC_DMA_FOR_CPU_ALL);
    bool dma_coherent:1;
#endif;
#ifdef CONFIG_DMA_OPS_BYPASS;
    bool dma_ops_bypass : 1;
#endif;
#ifdef CONFIG_DMA_NEED_SYNC;
    bool dma_skip_sync:1;
#endif;
#ifdef CONFIG_IOMMU_DMA;
    bool dma_iommu:1;
#endif;
    unsigned long flags[BITS_TO_LONGS( DEV_FLAG_COUNT)];
};
```

**Members**

`kobj`
:   A top-level, abstract class from which other classes are derived.

`parent`
:   The device’s “parent” device, the device to which it is attached.
    In most cases, a parent device is some sort of bus or host
    controller. If parent is NULL, the device, is a top-level device,
    which is not usually what you want.

`p`
:   Holds the private data of the driver core portions of the device.
    See the comment of the `struct device_private` for detail.

`init_name`
:   Initial name of the device.

`type`
:   The type of device.
    This identifies the device type and carries type-specific
    information.

`bus`
:   Type of bus device is on.

`driver`
:   Which driver has allocated this

`platform_data`
:   Platform data specific to the device.

`driver_data`
:   Private pointer for driver specific info.

`driver_override`
:   Driver name to force a match. Do not touch directly; use
    [`device_set_driver_override()`](#c.device_set_driver_override "device_set_driver_override") instead.

`mutex`
:   Mutex to synchronize calls to its driver.

`links`
:   Links to suppliers and consumers of this device.

`power`
:   For device power management.
    See [Device Power Management Basics](pm/devices.html) for details.

`pm_domain`
:   Provide callbacks that are executed during system suspend,
    hibernation, system resume and during runtime PM transitions
    along with subsystem-level and driver-level callbacks.

`em_pd`
:   device’s energy model performance domain

`pins`
:   For device pin management.
    See [PINCTRL (PIN CONTROL) subsystem](pin-control.html) for details.

`msi`
:   MSI related data

`dma_ops`
:   DMA mapping operations for this device.

`dma_mask`
:   Dma mask (if dma’ble device).

`coherent_dma_mask`
:   Like dma\_mask, but for alloc\_coherent mapping as not all
    hardware supports 64-bit addresses for consistent allocations
    such descriptors.

`bus_dma_limit`
:   Limit of an upstream bridge or bus which imposes a smaller
    DMA limit than the device itself supports.

`dma_range_map`
:   map for DMA memory ranges relative to that of RAM

`dma_parms`
:   A low level driver may set these to teach IOMMU code about
    segment limitations.

`dma_pools`
:   Dma pools (if dma’ble device).

`dma_mem`
:   Internal for coherent mem override.

`cma_area`
:   Contiguous memory area for dma allocations

`dma_io_tlb_mem`
:   Software IO TLB allocator. Not for driver use.

`dma_io_tlb_pools`
:   List of transient swiotlb memory pools.

`dma_io_tlb_lock`
:   Protects changes to the list of active pools.

`dma_uses_io_tlb`
:   `true` if device has used the software IO TLB.

`archdata`
:   For arch-specific additions.

`of_node`
:   Associated device tree node.

`fwnode`
:   Associated device node supplied by platform firmware.

`numa_node`
:   NUMA node this device is close to.

`devt`
:   For creating the sysfs “dev”.

`id`
:   device instance

`devres_lock`
:   Spinlock to protect the resource of the device.

`devres_head`
:   The resources list of the device.

`class`
:   The class of the device.

`groups`
:   Optional attribute groups.

`release`
:   Callback to free the device after all references have
    gone away. This should be set by the allocator of the
    device (i.e. the bus driver that discovered the device).

`iommu_group`
:   IOMMU group the device belongs to.

`iommu`
:   Per device generic IOMMU runtime data

`physical_location`
:   Describes physical location of the device connection
    point in the system housing.

`removable`
:   Whether the device can be removed from the system. This
    should be set by the subsystem / bus driver that discovered
    the device.

`offline_disabled`
:   If set, the device is permanently online.

`offline`
:   Set after successful invocation of bus type’s .`offline()`.

`of_node_reused`
:   Set if the device-tree node is shared with an ancestor
    device.

`state_synced`
:   The hardware state of this device has been synced to match
    the software state of this device by calling the driver/bus
    `sync_state()` callback.

`can_match`
:   The device has matched with a driver at least once or it is in
    a bus (like AMBA) which can’t check for matching drivers until
    other devices probe successfully.

`dma_coherent`
:   this particular device is dma coherent, even if the
    architecture supports non-coherent devices.

`dma_ops_bypass`
:   If set to `true` then the dma\_ops are bypassed for the
    streaming DMA operations (->map\_\* / ->unmap\_\* / ->sync\_\*),
    and optionall (if the coherent mask is large enough) also
    for dma allocations. This flag is managed by the dma ops
    instance from ->dma\_supported.

`dma_skip_sync`
:   DMA sync operations can be skipped for coherent buffers.

`dma_iommu`
:   Device is using default IOMMU implementation for DMA and
    doesn’t rely on dma\_ops structure.

`flags`
:   DEV\_FLAG\_XXX flags. Use atomic bitfield operations to modify.

**Example**

For devices on custom boards, as typical of embedded
:   and SOC based hardware, Linux often uses platform\_data to point
    to board-specific structures describing devices and how they
    are wired. That can include what ports are available, chip
    variants, which GPIO pins act in what additional roles, and so
    on. This shrinks the “Board Support Packages” (BSPs) and
    minimizes board-specific #ifdefs in drivers.

**Description**

At the lowest level, every device in a Linux system is represented by an
instance of [`struct device`](#c.device "device"). The device structure contains the information
that the device model core needs to model the system. Most subsystems,
however, track additional information about the devices they host. As a
result, it is rare for devices to be represented by bare device structures;
instead, that structure, like kobject structures, is usually embedded within
a higher-level representation of the device.

struct device\_link
:   Device link representation.

**Definition**:

```
struct device_link {
    struct device *supplier;
    struct list_head s_node;
    struct device *consumer;
    struct list_head c_node;
    struct device link_dev;
    enum device_link_state status;
    u32 flags;
    refcount_t rpm_active;
    struct kref kref;
    struct work_struct rm_work;
    bool supplier_preactivated;
};
```

**Members**

`supplier`
:   The device on the supplier end of the link.

`s_node`
:   Hook to the supplier device’s list of links to consumers.

`consumer`
:   The device on the consumer end of the link.

`c_node`
:   Hook to the consumer device’s list of links to suppliers.

`link_dev`
:   device used to expose link details in sysfs

`status`
:   The state of the link (with respect to the presence of drivers).

`flags`
:   Link flags.

`rpm_active`
:   Whether or not the consumer device is runtime-PM-active.

`kref`
:   Count repeated addition of the same link.

`rm_work`
:   Work structure used for removing the link.

`supplier_preactivated`
:   Supplier has been made active before consumer probe.

int device\_set\_driver\_override(struct [device](#c.device "device") \*dev, const char \*s)
:   Helper to set or clear driver override.

**Parameters**

`struct device *dev`
:   Device to change

`const char *s`
:   NUL-terminated string, new driver name to force a match, pass empty
    string to clear it (”” or “n”, where the latter is only for sysfs
    interface).

**Description**

Helper to set or clear driver override of a device.

**Return**

0 on success or a negative error code on failure.

bool device\_has\_driver\_override(struct [device](#c.device "device") \*dev)
:   Check if a driver override has been set.

**Parameters**

`struct device *dev`
:   device to check

**Description**

Returns true if a driver override has been set for this device.

int device\_match\_driver\_override(struct [device](#c.device "device") \*dev, const struct [device\_driver](#c.device_driver "device_driver") \*drv)
:   Match a driver against the device’s driver\_override.

**Parameters**

`struct device *dev`
:   device to check

`const struct device_driver *drv`
:   driver to match against

**Description**

Returns > 0 if a driver override is set and matches the given driver, 0 if a
driver override is set but does not match, or < 0 if a driver override is not
set at all.

bool device\_iommu\_mapped(struct [device](#c.device "device") \*dev)
:   Returns true when the device DMA is translated by an IOMMU

**Parameters**

`struct device *dev`
:   Device to perform the check on

const char \*dev\_name(const struct [device](#c.device "device") \*dev)
:   Return a device’s name.

**Parameters**

`const struct device *dev`
:   Device with name to get.

**Return**

The kobject name of the device, or its initial name if unavailable.

const char \*dev\_bus\_name(const struct [device](#c.device "device") \*dev)
:   Return a device’s bus/class name, if at all possible

**Parameters**

`const struct device *dev`
:   [`struct device`](#c.device "device") to get the bus/class name of

**Description**

Will return the name of the bus/class the device is attached to. If it is
not attached to a bus/class, an empty string will be returned.

struct [device](#c.device "device") \*device\_find\_child\_by\_name(struct [device](#c.device "device") \*parent, const char \*name)
:   device iterator for locating a child device.

**Parameters**

`struct device *parent`
:   parent [`struct device`](#c.device "device")

`const char *name`
:   name of the child device

**Description**

This is similar to the [`device_find_child()`](#c.device_find_child "device_find_child") function above, but it
returns a reference to a device that has the name **name**.

**NOTE**

you will need to drop the reference with [`put_device()`](#c.put_device "put_device") after use.

struct [device](#c.device "device") \*device\_find\_any\_child(struct [device](#c.device "device") \*parent)
:   device iterator for locating a child device, if any.

**Parameters**

`struct device *parent`
:   parent [`struct device`](#c.device "device")

**Description**

This is similar to the [`device_find_child()`](#c.device_find_child "device_find_child") function above, but it
returns a reference to a child device, if any.

**NOTE**

you will need to drop the reference with [`put_device()`](#c.put_device "put_device") after use.

device\_lock\_set\_class

`device_lock_set_class (dev, key)`

> Specify a temporary lock class while a device is attached to a driver

**Parameters**

`dev`
:   device to modify

`key`
:   lock class key data

**Description**

This must be called with the `device_lock()` already held, for example
from driver ->`probe()`. Take care to only override the default
lockdep\_no\_validate class.

device\_lock\_reset\_class

`device_lock_reset_class (dev)`

> Return a device to the default lockdep novalidate state

**Parameters**

`dev`
:   device to modify

**Description**

This must be called with the `device_lock()` already held, for example
from driver ->`remove()`.

struct bus\_type
:   The bus type of the device

**Definition**:

```
struct bus_type {
    const char              *name;
    const char              *dev_name;
    const struct attribute_group **bus_groups;
    const struct attribute_group **dev_groups;
    const struct attribute_group **drv_groups;
    int (*match)(struct device *dev, const struct device_driver *drv);
    int (*uevent)(const struct device *dev, struct kobj_uevent_env *env);
    int (*probe)(struct device *dev);
    void (*sync_state)(struct device *dev);
    void (*remove)(struct device *dev);
    void (*shutdown)(struct device *dev);
    const struct cpumask *(*irq_get_affinity)(struct device *dev, unsigned int irq_vec);
    int (*online)(struct device *dev);
    int (*offline)(struct device *dev);
    int (*suspend)(struct device *dev, pm_message_t state);
    int (*resume)(struct device *dev);
    int (*num_vf)(struct device *dev);
    int (*dma_configure)(struct device *dev);
    void (*dma_cleanup)(struct device *dev);
    const struct dev_pm_ops *pm;
    bool driver_override;
    bool need_parent_lock;
};
```

**Members**

`name`
:   The name of the bus.

`dev_name`
:   Used for subsystems to enumerate devices like (“foo``u``”, dev->id).

`bus_groups`
:   Default attributes of the bus.

`dev_groups`
:   Default attributes of the devices on the bus.

`drv_groups`
:   Default attributes of the device drivers on the bus.

`match`
:   Called, perhaps multiple times, whenever a new device or driver
    is added for this bus. It should return a positive value if the
    given device can be handled by the given driver and zero
    otherwise. It may also return error code if determining that
    the driver supports the device is not possible. In case of
    -EPROBE\_DEFER it will queue the device for deferred probing.

`uevent`
:   Called when a device is added, removed, or a few other things
    that generate uevents to add the environment variables.

`probe`
:   Called when a new device or driver add to this bus, and callback
    the specific driver’s probe to initial the matched device.

`sync_state`
:   Called to sync device state to software state after all the
    state tracking consumers linked to this device (present at
    the time of late\_initcall) have successfully bound to a
    driver. If the device has no consumers, this function will
    be called at late\_initcall\_sync level. If the device has
    consumers that are never bound to a driver, this function
    will never get called until they do.

`remove`
:   Called when a device removed from this bus.

`shutdown`
:   Called at shut-down time to quiesce the device.

`irq_get_affinity`
:   Get IRQ affinity mask for the device on this bus.

`online`
:   Called to put the device back online (after offlining it).

`offline`
:   Called to put the device offline for hot-removal. May fail.

`suspend`
:   Called when a device on this bus wants to go to sleep mode.

`resume`
:   Called to bring a device on this bus out of sleep mode.

`num_vf`
:   Called to find out how many virtual functions a device on this
    bus supports.

`dma_configure`
:   Called to setup DMA configuration on a device on
    this bus.

`dma_cleanup`
:   Called to cleanup DMA configuration on a device on
    this bus.

`pm`
:   Power management operations of this bus, callback the specific
    device driver’s pm-ops.

`driver_override`
:   Set to true if this bus supports the driver\_override
    mechanism, which allows userspace to force a specific
    driver to bind to a device via a sysfs attribute.

`need_parent_lock`
:   When probing or removing a device on this bus, the
    device core should lock the device’s parent.

**Note**

This callback may be invoked with or without the device
:   lock held.

**Description**

A bus is a channel between the processor and one or more devices. For the
purposes of the device model, all devices are connected via a bus, even if
it is an internal, virtual, “platform” bus. Buses can plug into each other.
A USB controller is usually a PCI device, for example. The device model
represents the actual connections between buses and the devices they control.
A bus is represented by the bus\_type structure. It contains the name, the
default attributes, the bus’ methods, PM operations, and the driver core’s
private data.

enum bus\_notifier\_event
:   Bus Notifier events that have happened

**Constants**

`BUS_NOTIFY_ADD_DEVICE`
:   device is added to this bus

`BUS_NOTIFY_DEL_DEVICE`
:   device is about to be removed from this bus

`BUS_NOTIFY_REMOVED_DEVICE`
:   device is successfully removed from this bus

`BUS_NOTIFY_BIND_DRIVER`
:   a driver is about to be bound to this device on this bus

`BUS_NOTIFY_BOUND_DRIVER`
:   a driver is successfully bound to this device on this bus

`BUS_NOTIFY_UNBIND_DRIVER`
:   a driver is about to be unbound from this device on this bus

`BUS_NOTIFY_UNBOUND_DRIVER`
:   a driver is successfully unbound from this device on this bus

`BUS_NOTIFY_DRIVER_NOT_BOUND`
:   a driver failed to be bound to this device on this bus

**Description**

These are the value passed to a bus notifier when a specific event happens.

Note that bus notifiers are likely to be called with the device lock already
held by the driver core, so be careful in any notifier callback as to what
you do with the device structure.

All bus notifiers are called with the target [`struct device`](#c.device "device") \* as an argument.

struct class
:   device classes

**Definition**:

```
struct class {
    const char              *name;
    const struct attribute_group    *const *class_groups;
    const struct attribute_group    *const *dev_groups;
    int (*dev_uevent)(const struct device *dev, struct kobj_uevent_env *env);
    char *(*devnode)(const struct device *dev, umode_t *mode);
    void (*class_release)(const struct class *class);
    void (*dev_release)(struct device *dev);
    int (*shutdown_pre)(struct device *dev);
    const struct kobj_ns_type_operations *ns_type;
    const struct ns_common *(*namespace)(const struct device *dev);
    void (*get_ownership)(const struct device *dev, kuid_t *uid, kgid_t *gid);
    const struct dev_pm_ops *pm;
};
```

**Members**

`name`
:   Name of the class.

`class_groups`
:   Default attributes of this class.

`dev_groups`
:   Default attributes of the devices that belong to the class.

`dev_uevent`
:   Called when a device is added, removed from this class, or a
    few other things that generate uevents to add the environment
    variables.

`devnode`
:   Callback to provide the devtmpfs.

`class_release`
:   Called to release this class.

`dev_release`
:   Called to release the device.

`shutdown_pre`
:   Called at shut-down time before driver shutdown.

`ns_type`
:   Callbacks so sysfs can detemine namespaces.

`namespace`
:   Namespace of the device belongs to this class.

`get_ownership`
:   Allows class to specify uid/gid of the sysfs directories
    for the devices belonging to the class. Usually tied to
    device’s namespace.

`pm`
:   The default device power management operations of this class.

**Description**

A class is a higher-level view of a device that abstracts out low-level
implementation details. Drivers may see a SCSI disk or an ATA disk, but,
at the class level, they are all simply disks. Classes allow user space
to work with devices based on what they do, rather than how they are
connected or how they work.

enum probe\_type
:   device driver probe type to try Device drivers may opt in for special handling of their respective probe routines. This tells the core what to expect and prefer.

**Constants**

`PROBE_DEFAULT_STRATEGY`
:   Used by drivers that work equally well
    whether probed synchronously or asynchronously.

`PROBE_PREFER_ASYNCHRONOUS`
:   Drivers for “slow” devices which
    probing order is not essential for booting the system may
    opt into executing their probes asynchronously.

`PROBE_FORCE_SYNCHRONOUS`
:   Use this to annotate drivers that need
    their probe routines to run synchronously with driver and
    device registration (with the exception of -EPROBE\_DEFER
    handling - re-probing always ends up being done asynchronously).

**Description**

Note that the end goal is to switch the kernel to use asynchronous
probing by default, so annotating drivers with
`PROBE_PREFER_ASYNCHRONOUS` is a temporary measure that allows us
to speed up boot process while we are validating the rest of the
drivers.

struct device\_driver
:   The basic device driver structure

**Definition**:

```
struct device_driver {
    const char              *name;
    const struct bus_type   *bus;
    struct module           *owner;
    const char              *mod_name;
    bool suppress_bind_attrs;
    enum probe_type probe_type;
    const struct of_device_id       *of_match_table;
    const struct acpi_device_id     *acpi_match_table;
    int (*probe) (struct device *dev);
    void (*sync_state)(struct device *dev);
    int (*remove) (struct device *dev);
    void (*shutdown) (struct device *dev);
    int (*suspend) (struct device *dev, pm_message_t state);
    int (*resume) (struct device *dev);
    const struct attribute_group **groups;
    const struct attribute_group **dev_groups;
    const struct dev_pm_ops *pm;
    void (*coredump) (struct device *dev);
    struct driver_private *p;
    struct {
        void (*post_unbind_rust)(struct device *dev);
    } p_cb;
};
```

**Members**

`name`
:   Name of the device driver.

`bus`
:   The bus which the device of this driver belongs to.

`owner`
:   The module owner.

`mod_name`
:   Used for built-in modules.

`suppress_bind_attrs`
:   Disables bind/unbind via sysfs.

`probe_type`
:   Type of the probe (synchronous or asynchronous) to use.

`of_match_table`
:   The open firmware table.

`acpi_match_table`
:   The ACPI match table.

`probe`
:   Called to query the existence of a specific device,
    whether this driver can work with it, and bind the driver
    to a specific device.

`sync_state`
:   Called to sync device state to software state after all the
    state tracking consumers linked to this device (present at
    the time of late\_initcall) have successfully bound to a
    driver. If the device has no consumers, this function will
    be called at late\_initcall\_sync level. If the device has
    consumers that are never bound to a driver, this function
    will never get called until they do.

`remove`
:   Called when the device is removed from the system to
    unbind a device from this driver.

`shutdown`
:   Called at shut-down time to quiesce the device.

`suspend`
:   Called to put the device to sleep mode. Usually to a
    low power state.

`resume`
:   Called to bring a device from sleep mode.

`groups`
:   Default attributes that get created by the driver core
    automatically.

`dev_groups`
:   Additional attributes attached to device instance once
    it is bound to the driver.

`pm`
:   Power management operations of the device which matched
    this driver.

`coredump`
:   Called when sysfs entry is written to. The device driver
    is expected to call the dev\_coredump API resulting in a
    uevent.

`p`
:   Driver core’s private data, no one other than the driver
    core can touch this.

`p_cb`
:   Callbacks private to the driver core; no one other than the
    driver core is allowed to touch this.

**Description**

The device driver-model tracks all of the drivers known to the system.
The main reason for this tracking is to enable the driver core to match
up drivers with new devices. Once drivers are known objects within the
system, however, a number of other things become possible. Device drivers
can export information and configuration variables that are independent
of any specific device.

## Device Drivers Base

void driver\_init(void)
:   initialize driver model.

**Parameters**

`void`
:   no arguments

**Description**

Call the driver model init functions to initialize their
subsystems. Called early from init/main.c.

struct [device](#c.device "device") \*driver\_find\_device\_by\_name(const struct [device\_driver](#c.device_driver "device_driver") \*drv, const char \*name)
:   device iterator for locating a particular device of a specific name.

**Parameters**

`const struct device_driver *drv`
:   the driver we’re iterating

`const char *name`
:   name of the device to match

struct [device](#c.device "device") \*driver\_find\_device\_by\_of\_node(const struct [device\_driver](#c.device_driver "device_driver") \*drv, const struct device\_node \*np)
:   device iterator for locating a particular device by of\_node pointer.

**Parameters**

`const struct device_driver *drv`
:   the driver we’re iterating

`const struct device_node *np`
:   of\_node pointer to match.

struct [device](#c.device "device") \*driver\_find\_device\_by\_fwnode(struct [device\_driver](#c.device_driver "device_driver") \*drv, const struct fwnode\_handle \*fwnode)
:   device iterator for locating a particular device by fwnode pointer.

**Parameters**

`struct device_driver *drv`
:   the driver we’re iterating

`const struct fwnode_handle *fwnode`
:   fwnode pointer to match.

struct [device](#c.device "device") \*driver\_find\_device\_by\_devt(const struct [device\_driver](#c.device_driver "device_driver") \*drv, dev\_t devt)
:   device iterator for locating a particular device by devt.

**Parameters**

`const struct device_driver *drv`
:   the driver we’re iterating

`dev_t devt`
:   devt pointer to match.

struct [device](#c.device "device") \*driver\_find\_device\_by\_acpi\_dev(const struct [device\_driver](#c.device_driver "device_driver") \*drv, const struct acpi\_device \*adev)
:   device iterator for locating a particular device matching the ACPI\_COMPANION device.

**Parameters**

`const struct device_driver *drv`
:   the driver we’re iterating

`const struct acpi_device *adev`
:   ACPI\_COMPANION device to match.

module\_driver

`module_driver (__driver, __register, __unregister, ...)`

> Helper macro for drivers that don’t do anything special in module init/exit. This eliminates a lot of boilerplate. Each module may only use this macro once, and calling it replaces [`module_init()`](basics.html#c.module_init "module_init") and [`module_exit()`](basics.html#c.module_exit "module_exit").

**Parameters**

`__driver`
:   driver name

`__register`
:   register function for this driver type

`__unregister`
:   unregister function for this driver type

`...`
:   Additional arguments to be passed to \_\_register and \_\_unregister.

**Description**

Use this macro to construct bus specific macros for registering
drivers, and do not use it on its own.

builtin\_driver

`builtin_driver (__driver, __register, ...)`

> Helper macro for drivers that don’t do anything special in init and have no exit. This eliminates some boilerplate. Each driver may only use this macro once, and calling it replaces device\_initcall (or in some cases, the legacy \_\_initcall). This is meant to be a direct parallel of [`module_driver()`](#c.module_driver "module_driver") above but without the \_\_exit stuff that is not used for builtin cases.

**Parameters**

`__driver`
:   driver name

`__register`
:   register function for this driver type

`...`
:   Additional arguments to be passed to \_\_register

**Description**

Use this macro to construct bus specific macros for registering
drivers, and do not use it on its own.

int driver\_set\_override(struct [device](#c.device "device") \*dev, const char \*\*override, const char \*s, size\_t len)
:   Helper to set or clear driver override.

**Parameters**

`struct device *dev`
:   Device to change

`const char **override`
:   Address of string to change (e.g. [`device->driver_override`](#c.device "device"));
    The contents will be freed and hold newly allocated override.

`const char *s`
:   NUL-terminated string, new driver name to force a match, pass empty
    string to clear it (”” or “n”, where the latter is only for sysfs
    interface).

`size_t len`
:   length of **s**

**Description**

Helper to set or clear driver override in a device, intended for the cases
when the driver\_override field is allocated by driver/bus code.

**Return**

0 on success or a negative error code on failure.

int driver\_for\_each\_device(struct [device\_driver](#c.device_driver "device_driver") \*drv, struct [device](#c.device "device") \*start, void \*data, device\_iter\_t fn)
:   Iterator for devices bound to a driver.

**Parameters**

`struct device_driver *drv`
:   Driver we’re iterating.

`struct device *start`
:   Device to begin with

`void *data`
:   Data to pass to the callback.

`device_iter_t fn`
:   Function to call for each device.

**Description**

Iterate over the **drv**’s list of devices calling **fn** for each one.

struct [device](#c.device "device") \*driver\_find\_device(const struct [device\_driver](#c.device_driver "device_driver") \*drv, struct [device](#c.device "device") \*start, const void \*data, device\_match\_t match)
:   device iterator for locating a particular device.

**Parameters**

`const struct device_driver *drv`
:   The device’s driver

`struct device *start`
:   Device to begin with

`const void *data`
:   Data to pass to match function

`device_match_t match`
:   Callback function to check device

**Description**

This is similar to the [`driver_for_each_device()`](#c.driver_for_each_device "driver_for_each_device") function above, but
it returns a reference to a device that is ‘found’ for later use, as
determined by the **match** callback.

The callback should return 0 if the device doesn’t match and non-zero
if it does. If the callback returns non-zero, this function will
return to the caller and not iterate over any more devices.

int driver\_create\_file(const struct [device\_driver](#c.device_driver "device_driver") \*drv, const struct driver\_attribute \*attr)
:   create sysfs file for driver.

**Parameters**

`const struct device_driver *drv`
:   driver.

`const struct driver_attribute *attr`
:   driver attribute descriptor.

void driver\_remove\_file(const struct [device\_driver](#c.device_driver "device_driver") \*drv, const struct driver\_attribute \*attr)
:   remove sysfs file for driver.

**Parameters**

`const struct device_driver *drv`
:   driver.

`const struct driver_attribute *attr`
:   driver attribute descriptor.

int driver\_register(struct [device\_driver](#c.device_driver "device_driver") \*drv)
:   register driver with bus

**Parameters**

`struct device_driver *drv`
:   driver to register

**Description**

We pass off most of the work to the `bus_add_driver()` call,
since most of the things we have to do deal with the bus
structures.

void driver\_unregister(struct [device\_driver](#c.device_driver "device_driver") \*drv)
:   remove driver from system.

**Parameters**

`struct device_driver *drv`
:   driver.

**Description**

Again, we pass off most of the work to the bus-level call.

void device\_link\_wait\_removal(void)
:   Wait for ongoing devlink removal jobs to terminate

**Parameters**

`void`
:   no arguments

struct [device\_link](#c.device_link "device_link") \*device\_link\_add(struct [device](#c.device "device") \*consumer, struct [device](#c.device "device") \*supplier, u32 flags)
:   Create a link between two devices.

**Parameters**

`struct device *consumer`
:   Consumer end of the link.

`struct device *supplier`
:   Supplier end of the link.

`u32 flags`
:   Link flags.

**Return**

On success, a device\_link `struct will` be returned.
On error or invalid flag settings, NULL will be returned.

**Description**

The caller is responsible for the proper synchronization of the link creation
with runtime PM. First, setting the DL\_FLAG\_PM\_RUNTIME flag will cause the
runtime PM framework to take the link into account. Second, if the
DL\_FLAG\_RPM\_ACTIVE flag is set in addition to it, the supplier devices will
be forced into the active meta state and reference-counted upon the creation
of the link. If DL\_FLAG\_PM\_RUNTIME is not set, DL\_FLAG\_RPM\_ACTIVE will be
ignored.

If DL\_FLAG\_STATELESS is set in **flags**, the caller of this function is
expected to release the link returned by it directly with the help of either
[`device_link_del()`](#c.device_link_del "device_link_del") or [`device_link_remove()`](#c.device_link_remove "device_link_remove").

If that flag is not set, however, the caller of this function is handing the
management of the link over to the driver core entirely and its return value
can only be used to check whether or not the link is present. In that case,
the DL\_FLAG\_AUTOREMOVE\_CONSUMER and DL\_FLAG\_AUTOREMOVE\_SUPPLIER device link
flags can be used to indicate to the driver core when the link can be safely
deleted. Namely, setting one of them in **flags** indicates to the driver core
that the link is not going to be used (by the given caller of this function)
after unbinding the consumer or supplier driver, respectively, from its
device, so the link can be deleted at that point. If none of them is set,
the link will be maintained until one of the devices pointed to by it (either
the consumer or the supplier) is unregistered.

Also, if DL\_FLAG\_STATELESS, DL\_FLAG\_AUTOREMOVE\_CONSUMER and
DL\_FLAG\_AUTOREMOVE\_SUPPLIER are not set in **flags** (that is, a persistent
managed device link is being added), the DL\_FLAG\_AUTOPROBE\_CONSUMER flag can
be used to request the driver core to automatically probe for a consumer
driver after successfully binding a driver to the supplier device.

The combination of DL\_FLAG\_STATELESS and one of DL\_FLAG\_AUTOREMOVE\_CONSUMER,
DL\_FLAG\_AUTOREMOVE\_SUPPLIER, or DL\_FLAG\_AUTOPROBE\_CONSUMER set in **flags** at
the same time is invalid and will cause NULL to be returned upfront.
However, if a device link between the given **consumer** and **supplier** pair
exists already when this function is called for them, the existing link will
be returned regardless of its current type and status (the link’s flags may
be modified then). The caller of this function is then expected to treat
the link as though it has just been created, so (in particular) if
DL\_FLAG\_STATELESS was passed in **flags**, the link needs to be released
explicitly when not needed any more (as stated above).

A side effect of the link creation is re-ordering of dpm\_list and the
devices\_kset list by moving the consumer device and all devices depending
on it to the ends of these lists (that does not happen to devices that have
not been registered when this function is called).

The supplier device is required to be registered when this function is called
and NULL will be returned if that is not the case. The consumer device need
not be registered, however.

void device\_link\_del(struct [device\_link](#c.device_link "device_link") \*link)
:   Delete a stateless link between two devices.

**Parameters**

`struct device_link *link`
:   Device link to delete.

**Description**

The caller must ensure proper synchronization of this function with runtime
PM. If the link was added multiple times, it needs to be deleted as often.
Care is required for hotplugged devices: Their links are purged on removal
and calling [`device_link_del()`](#c.device_link_del "device_link_del") is then no longer allowed.

void device\_link\_remove(void \*consumer, struct [device](#c.device "device") \*supplier)
:   Delete a stateless link between two devices.

**Parameters**

`void *consumer`
:   Consumer end of the link.

`struct device *supplier`
:   Supplier end of the link.

**Description**

The caller must ensure proper synchronization of this function with runtime
PM.

const char \*dev\_driver\_string(const struct [device](#c.device "device") \*dev)
:   Return a device’s driver name, if at all possible

**Parameters**

`const struct device *dev`
:   [`struct device`](#c.device "device") to get the name of

**Description**

Will return the device’s driver’s name if it is bound to a device. If
the device is not bound to a driver, it will return the name of the bus
it is attached to. If it is not attached to a bus either, an empty
string will be returned.

int devm\_device\_add\_group(struct [device](#c.device "device") \*dev, const struct attribute\_group \*grp)
:   given a device, create a managed attribute group

**Parameters**

`struct device *dev`
:   The device to create the group for

`const struct attribute_group *grp`
:   The attribute group to create

**Description**

This function creates a group for the first time. It will explicitly
warn and error if any of the attribute files being created already exist.

Returns 0 on success or error code on failure.

int device\_create\_file(struct [device](#c.device "device") \*dev, const struct [device\_attribute](#c.device_attribute "device_attribute") \*attr)
:   create sysfs attribute file for device.

**Parameters**

`struct device *dev`
:   device.

`const struct device_attribute *attr`
:   device attribute descriptor.

void device\_remove\_file(struct [device](#c.device "device") \*dev, const struct [device\_attribute](#c.device_attribute "device_attribute") \*attr)
:   remove sysfs attribute file.

**Parameters**

`struct device *dev`
:   device.

`const struct device_attribute *attr`
:   device attribute descriptor.

bool device\_remove\_file\_self(struct [device](#c.device "device") \*dev, const struct [device\_attribute](#c.device_attribute "device_attribute") \*attr)
:   remove sysfs attribute file from its own method.

**Parameters**

`struct device *dev`
:   device.

`const struct device_attribute *attr`
:   device attribute descriptor.

**Description**

See `kernfs_remove_self()` for details.

int device\_create\_bin\_file(struct [device](#c.device "device") \*dev, const struct bin\_attribute \*attr)
:   create sysfs binary attribute file for device.

**Parameters**

`struct device *dev`
:   device.

`const struct bin_attribute *attr`
:   device binary attribute descriptor.

void device\_remove\_bin\_file(struct [device](#c.device "device") \*dev, const struct bin\_attribute \*attr)
:   remove sysfs binary attribute file

**Parameters**

`struct device *dev`
:   device.

`const struct bin_attribute *attr`
:   device binary attribute descriptor.

void device\_initialize(struct [device](#c.device "device") \*dev)
:   init device structure.

**Parameters**

`struct device *dev`
:   device.

**Description**

This prepares the device for use by other layers by initializing
its fields.
It is the first half of [`device_register()`](#c.device_register "device_register"), if called by
that function, though it can also be called separately, so one
may use **dev**’s fields. In particular, [`get_device()`](#c.get_device "get_device")/[`put_device()`](#c.put_device "put_device")
may be used for reference counting of **dev** after calling this
function.

All fields in **dev** must be initialized by the caller to 0, except
for those explicitly set to some other value. The simplest
approach is to use [`kzalloc()`](../core-api/mm-api.html#c.kzalloc "kzalloc") to allocate the structure containing
**dev**.

**NOTE**

Use [`put_device()`](#c.put_device "put_device") to give up your reference instead of freeing
**dev** directly once you have called this function.

int dev\_set\_name(struct [device](#c.device "device") \*dev, const char \*fmt, ...)
:   set a device name

**Parameters**

`struct device *dev`
:   device

`const char *fmt`
:   format string for the device’s name

`...`
:   variable arguments

int device\_add(struct [device](#c.device "device") \*dev)
:   add device to device hierarchy.

**Parameters**

`struct device *dev`
:   device.

**Description**

This is part 2 of [`device_register()`](#c.device_register "device_register"), though may be called
separately \_iff\_ [`device_initialize()`](#c.device_initialize "device_initialize") has been called separately.

This adds **dev** to the kobject hierarchy via [`kobject_add()`](basics.html#c.kobject_add "kobject_add"), adds it
to the global and sibling lists for the device, then
adds it to the other relevant subsystems of the driver model.

Do not call this routine or [`device_register()`](#c.device_register "device_register") more than once for
any device structure. The driver model core is not designed to work
with devices that get unregistered and then spring back to life.
(Among other things, it’s very hard to guarantee that all references
to the previous incarnation of **dev** have been dropped.) Allocate
and register a fresh new [`struct device`](#c.device "device") instead.

**NOTE**

\_Never\_ directly free **dev** after calling this function, even
if it returned an error! Always use [`put_device()`](#c.put_device "put_device") to give up your
reference instead.

Rule of thumb is: if [`device_add()`](#c.device_add "device_add") succeeds, you should call
[`device_del()`](#c.device_del "device_del") when you want to get rid of it. If [`device_add()`](#c.device_add "device_add") has
*not* succeeded, use *only* [`put_device()`](#c.put_device "put_device") to drop the reference
count.

int device\_register(struct [device](#c.device "device") \*dev)
:   register a device with the system.

**Parameters**

`struct device *dev`
:   pointer to the device structure

**Description**

This happens in two clean steps - initialize the device
and add it to the system. The two steps can be called
separately, but this is the easiest and most common.
I.e. you should only call the two helpers separately if
have a clearly defined need to use and refcount the device
before it is added to the hierarchy.

For more information, see the kerneldoc for [`device_initialize()`](#c.device_initialize "device_initialize")
and [`device_add()`](#c.device_add "device_add").

**NOTE**

\_Never\_ directly free **dev** after calling this function, even
if it returned an error! Always use [`put_device()`](#c.put_device "put_device") to give up the
reference initialized in this function instead.

struct [device](#c.device "device") \*get\_device(struct [device](#c.device "device") \*dev)
:   increment reference count for device.

**Parameters**

`struct device *dev`
:   device.

**Description**

This simply forwards the call to [`kobject_get()`](basics.html#c.kobject_get "kobject_get"), though
we do take care to provide for the case that we get a NULL
pointer passed in.

void put\_device(struct [device](#c.device "device") \*dev)
:   decrement reference count.

**Parameters**

`struct device *dev`
:   device in question.

void device\_del(struct [device](#c.device "device") \*dev)
:   delete device from system.

**Parameters**

`struct device *dev`
:   device.

**Description**

This is the first part of the device unregistration
sequence. This removes the device from the lists we control
from here, has it removed from the other driver model
subsystems it was added to in [`device_add()`](#c.device_add "device_add"), and removes it
from the kobject hierarchy.

**NOTE**

this should be called manually \_iff\_ [`device_add()`](#c.device_add "device_add") was
also called manually.

void device\_unregister(struct [device](#c.device "device") \*dev)
:   unregister device from system.

**Parameters**

`struct device *dev`
:   device going away.

**Description**

We do this in two parts, like we do [`device_register()`](#c.device_register "device_register"). First,
we remove it from all the subsystems with [`device_del()`](#c.device_del "device_del"), then
we decrement the reference count via [`put_device()`](#c.put_device "put_device"). If that
is the final reference count, the device will be cleaned up
via `device_release()` above. Otherwise, the structure will
stick around until the final reference to the device is dropped.

int device\_for\_each\_child(struct [device](#c.device "device") \*parent, void \*data, device\_iter\_t fn)
:   device child iterator.

**Parameters**

`struct device *parent`
:   parent [`struct device`](#c.device "device").

`void *data`
:   data for the callback.

`device_iter_t fn`
:   function to be called for each device.

**Description**

Iterate over **parent**’s child devices, and call **fn** for each,
passing it **data**.

We check the return of **fn** each time. If it returns anything
other than 0, we break out and return that value.

int device\_for\_each\_child\_reverse(struct [device](#c.device "device") \*parent, void \*data, device\_iter\_t fn)
:   device child iterator in reversed order.

**Parameters**

`struct device *parent`
:   parent [`struct device`](#c.device "device").

`void *data`
:   data for the callback.

`device_iter_t fn`
:   function to be called for each device.

**Description**

Iterate over **parent**’s child devices, and call **fn** for each,
passing it **data**.

We check the return of **fn** each time. If it returns anything
other than 0, we break out and return that value.

int device\_for\_each\_child\_reverse\_from(struct [device](#c.device "device") \*parent, struct [device](#c.device "device") \*from, void \*data, device\_iter\_t fn)
:   device child iterator in reversed order.

**Parameters**

`struct device *parent`
:   parent [`struct device`](#c.device "device").

`struct device *from`
:   optional starting point in child list

`void *data`
:   data for the callback.

`device_iter_t fn`
:   function to be called for each device.

**Description**

Iterate over **parent**’s child devices, starting at **from**, and call **fn**
for each, passing it **data**. This helper is identical to
[`device_for_each_child_reverse()`](#c.device_for_each_child_reverse "device_for_each_child_reverse") when **from** is NULL.

**fn** is checked each iteration. If it returns anything other than 0,
iteration stop and that value is returned to the caller of
[`device_for_each_child_reverse_from()`](#c.device_for_each_child_reverse_from "device_for_each_child_reverse_from");

struct [device](#c.device "device") \*device\_find\_child(struct [device](#c.device "device") \*parent, const void \*data, device\_match\_t match)
:   device iterator for locating a particular device.

**Parameters**

`struct device *parent`
:   parent [`struct device`](#c.device "device")

`const void *data`
:   Data to pass to match function

`device_match_t match`
:   Callback function to check device

**Description**

This is similar to the [`device_for_each_child()`](#c.device_for_each_child "device_for_each_child") function above, but it
returns a reference to a device that is ‘found’ for later use, as
determined by the **match** callback.

The callback should return 0 if the device doesn’t match and non-zero
if it does. If the callback returns non-zero and a reference to the
current device can be obtained, this function will return to the caller
and not iterate over any more devices.

**NOTE**

you will need to drop the reference with [`put_device()`](#c.put_device "put_device") after use.

struct [device](#c.device "device") \*\_\_root\_device\_register(const char \*name, struct module \*owner)
:   allocate and register a root device

**Parameters**

`const char *name`
:   root device name

`struct module *owner`
:   owner module of the root device, usually THIS\_MODULE

**Description**

This function allocates a root device and registers it
using [`device_register()`](#c.device_register "device_register"). In order to free the returned
device, use [`root_device_unregister()`](#c.root_device_unregister "root_device_unregister").

Root devices are dummy devices which allow other devices
to be grouped under /sys/devices. Use this function to
allocate a root device and then use it as the parent of
any device which should appear under /sys/devices/{name}

The /sys/devices/{name} directory will also contain a
‘module’ symlink which points to the **owner** directory
in sysfs.

Returns [`struct device`](#c.device "device") pointer on success, or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on error.

**Note**

You probably want to use `root_device_register()`.

void root\_device\_unregister(struct [device](#c.device "device") \*dev)
:   unregister and free a root device

**Parameters**

`struct device *dev`
:   device going away

**Description**

This function unregisters and cleans up a device that was created by
`root_device_register()`.

struct [device](#c.device "device") \*device\_create(const struct [class](#c.device_create "class") \*class, struct [device](#c.device "device") \*parent, dev\_t devt, void \*drvdata, const char \*fmt, ...)
:   creates a device and registers it with sysfs

**Parameters**

`const struct class *class`
:   pointer to the [`struct class`](#c.class "class") that this device should be registered to

`struct device *parent`
:   pointer to the parent [`struct device`](#c.device "device") of this new device, if any

`dev_t devt`
:   the dev\_t for the char device to be added

`void *drvdata`
:   the data to be added to the device for callbacks

`const char *fmt`
:   string for the device’s name

`...`
:   variable arguments

**Description**

This function can be used by char device classes. A [`struct device`](#c.device "device")
will be created in sysfs, registered to the specified class.

A “dev” file will be created, showing the dev\_t for the device, if
the dev\_t is not 0,0.
If a pointer to a parent [`struct device`](#c.device "device") is passed in, the newly created
[`struct device`](#c.device "device") will be a child of that device in sysfs.
The pointer to the [`struct device`](#c.device "device") will be returned from the call.
Any further sysfs files that might be required can be created using this
pointer.

Returns [`struct device`](#c.device "device") pointer on success, or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on error.

struct [device](#c.device "device") \*device\_create\_with\_groups(const struct [class](#c.device_create_with_groups "class") \*class, struct [device](#c.device "device") \*parent, dev\_t devt, void \*drvdata, const struct attribute\_group \*\*groups, const char \*fmt, ...)
:   creates a device and registers it with sysfs

**Parameters**

`const struct class *class`
:   pointer to the [`struct class`](#c.class "class") that this device should be registered to

`struct device *parent`
:   pointer to the parent [`struct device`](#c.device "device") of this new device, if any

`dev_t devt`
:   the dev\_t for the char device to be added

`void *drvdata`
:   the data to be added to the device for callbacks

`const struct attribute_group **groups`
:   NULL-terminated list of attribute groups to be created

`const char *fmt`
:   string for the device’s name

`...`
:   variable arguments

**Description**

This function can be used by char device classes. A [`struct device`](#c.device "device")
will be created in sysfs, registered to the specified class.
Additional attributes specified in the groups parameter will also
be created automatically.

A “dev” file will be created, showing the dev\_t for the device, if
the dev\_t is not 0,0.
If a pointer to a parent [`struct device`](#c.device "device") is passed in, the newly created
[`struct device`](#c.device "device") will be a child of that device in sysfs.
The pointer to the [`struct device`](#c.device "device") will be returned from the call.
Any further sysfs files that might be required can be created using this
pointer.

Returns [`struct device`](#c.device "device") pointer on success, or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on error.

void device\_destroy(const struct [class](#c.device_destroy "class") \*class, dev\_t devt)
:   removes a device that was created with [`device_create()`](#c.device_create "device_create")

**Parameters**

`const struct class *class`
:   pointer to the [`struct class`](#c.class "class") that this device was registered with

`dev_t devt`
:   the dev\_t of the device that was previously registered

**Description**

This call unregisters and cleans up a device that was created with a
call to [`device_create()`](#c.device_create "device_create").

int device\_rename(struct [device](#c.device "device") \*dev, const char \*new\_name)
:   renames a device

**Parameters**

`struct device *dev`
:   the pointer to the [`struct device`](#c.device "device") to be renamed

`const char *new_name`
:   the new name of the device

**Description**

It is the responsibility of the caller to provide mutual
exclusion between two different calls of device\_rename
on the same device to ensure that new\_name is valid and
won’t conflict with other devices.

**Note**

given that some subsystems (networking and infiniband) use this
function, with no immediate plans for this to change, we cannot assume or
require that this function not be called at all.

However, if you’re writing new code, do not call this function. The following
text from Kay Sievers offers some insight:

Renaming devices is racy at many levels, symlinks and other stuff are not
replaced atomically, and you get a “move” uevent, but it’s not easy to
connect the event to the old and new device. Device nodes are not renamed at
all, there isn’t even support for that in the kernel now.

In the meantime, during renaming, your target name might be taken by another
driver, creating conflicts. Or the old name is taken directly after you
renamed it -- then you get events for the same DEVPATH, before you even see
the “move” event. It’s just a mess, and nothing new should ever rely on
kernel device renaming. Besides that, it’s not even implemented now for
other things than (driver-core wise very simple) network devices.

Make up a “real” name in the driver before you register anything, or add
some other attributes for userspace to find the device, or use udev to add
symlinks -- but never rename kernel devices later, it’s a complete mess. We
don’t even want to get into that and try to implement the missing pieces in
the core. We really have other pieces to fix in the driver core mess. :)

int device\_move(struct [device](#c.device "device") \*dev, struct [device](#c.device "device") \*new\_parent, enum [dpm\_order](#c.device_move "dpm_order") dpm\_order)
:   moves a device to a new parent

**Parameters**

`struct device *dev`
:   the pointer to the [`struct device`](#c.device "device") to be moved

`struct device *new_parent`
:   the new parent of the device (can be NULL)

`enum dpm_order dpm_order`
:   how to reorder the dpm\_list

int dev\_err\_probe(const struct [device](#c.device "device") \*dev, int err, const char \*fmt, ...)
:   probe error check and log helper

**Parameters**

`const struct device *dev`
:   the pointer to the [`struct device`](#c.device "device")

`int err`
:   error value to test

`const char *fmt`
:   printf-style format string

`...`
:   arguments as specified in the format string

**Description**

This helper implements common pattern present in probe functions for error
checking: print debug or error message depending if the error value is
-EPROBE\_DEFER and propagate error upwards.
In case of -EPROBE\_DEFER it sets also defer probe reason, which can be
checked later by reading devices\_deferred debugfs attribute.
It replaces the following code sequence:

```
if (err != -EPROBE_DEFER)
        dev_err(dev, ...);
else
        dev_dbg(dev, ...);
return err;
```

with:

```
return dev_err_probe(dev, err, ...);
```

Using this helper in your probe function is totally fine even if **err**
is known to never be -EPROBE\_DEFER.
The benefit compared to a normal `dev_err()` is the standardized format
of the error code, which is emitted symbolically (i.e. you get “EAGAIN”
instead of “-35”), and having the error code returned allows more
compact error paths.

Returns **err**.

int dev\_warn\_probe(const struct [device](#c.device "device") \*dev, int err, const char \*fmt, ...)
:   probe error check and log helper

**Parameters**

`const struct device *dev`
:   the pointer to the [`struct device`](#c.device "device")

`int err`
:   error value to test

`const char *fmt`
:   printf-style format string

`...`
:   arguments as specified in the format string

**Description**

This helper implements common pattern present in probe functions for error
checking: print debug or warning message depending if the error value is
-EPROBE\_DEFER and propagate error upwards.
In case of -EPROBE\_DEFER it sets also defer probe reason, which can be
checked later by reading devices\_deferred debugfs attribute.
It replaces the following code sequence:

```
if (err != -EPROBE_DEFER)
        dev_warn(dev, ...);
else
        dev_dbg(dev, ...);
return err;
```

with:

```
return dev_warn_probe(dev, err, ...);
```

Using this helper in your probe function is totally fine even if **err**
is known to never be -EPROBE\_DEFER.
The benefit compared to a normal `dev_warn()` is the standardized format
of the error code, which is emitted symbolically (i.e. you get “EAGAIN”
instead of “-35”), and having the error code returned allows more
compact error paths.

Returns **err**.

void set\_primary\_fwnode(struct [device](#c.device "device") \*dev, struct fwnode\_handle \*fwnode)
:   Change the primary firmware node of a given device.

**Parameters**

`struct device *dev`
:   Device to handle.

`struct fwnode_handle *fwnode`
:   New primary firmware node of the device.

**Description**

Set the device’s firmware node pointer to **fwnode**, but if a secondary
firmware node of the device is present, preserve it.

Valid fwnode cases are:
:   * primary --> secondary --> -ENODEV
    * primary --> NULL
    * secondary --> -ENODEV
    * NULL

void set\_secondary\_fwnode(struct [device](#c.device "device") \*dev, struct fwnode\_handle \*fwnode)
:   Change the secondary firmware node of a given device.

**Parameters**

`struct device *dev`
:   Device to handle.

`struct fwnode_handle *fwnode`
:   New secondary firmware node of the device.

**Description**

If a primary firmware node of the device is present, set its secondary
pointer to **fwnode**. Otherwise, set the device’s firmware node pointer to
**fwnode**.

void device\_remove\_of\_node(struct [device](#c.device "device") \*dev)
:   Remove an of\_node from a device

**Parameters**

`struct device *dev`
:   device whose device tree node is being removed

int device\_add\_of\_node(struct [device](#c.device "device") \*dev, struct device\_node \*of\_node)
:   Add an of\_node to an existing device

**Parameters**

`struct device *dev`
:   device whose device tree node is being added

`struct device_node *of_node`
:   of\_node to add

**Return**

0 on success or error code on failure.

void device\_set\_of\_node\_from\_dev(struct [device](#c.device "device") \*dev, const struct [device](#c.device "device") \*dev2)
:   reuse device-tree node of another device

**Parameters**

`struct device *dev`
:   device whose device-tree node is being set

`const struct device *dev2`
:   device whose device-tree node is being reused

**Description**

Takes another reference to the new device-tree node after first dropping
any reference held to the old node.

struct [device](#c.device "device") \*get\_dev\_from\_fwnode(struct fwnode\_handle \*fwnode)
:   Obtain a reference count of the [`struct device`](#c.device "device") the `struct fwnode_handle` is associated with.

**Parameters**

`struct fwnode_handle *fwnode`
:   The pointer to the `struct fwnode_handle` to obtain the [`struct device`](#c.device "device")
    reference count of.

**Description**

This function obtains a reference count of the device the device pointer
embedded in the `struct fwnode_handle` points to.

Note that the [`struct device`](#c.device "device") pointer embedded in `struct fwnode_handle` does
*not* have a reference count of the [`struct device`](#c.device "device") itself.

Hence, it is a UAF (and thus a bug) to call this function if the caller can’t
guarantee that the last reference count of the corresponding [`struct device`](#c.device "device") is
not dropped concurrently.

This is possible since `struct fwnode_handle` has its own reference count and
hence can out-live the [`struct device`](#c.device "device") it is associated with.

void register\_syscore(struct [syscore](#c.register_syscore "syscore") \*syscore)
:   Register a set of system core operations.

**Parameters**

`struct syscore *syscore`
:   System core operations to register.

void unregister\_syscore(struct [syscore](#c.unregister_syscore "syscore") \*syscore)
:   Unregister a set of system core operations.

**Parameters**

`struct syscore *syscore`
:   System core operations to unregister.

int syscore\_suspend(void)
:   Execute all the registered system core suspend callbacks.

**Parameters**

`void`
:   no arguments

**Description**

This function is executed with one CPU on-line and disabled interrupts.

void syscore\_resume(void)
:   Execute all the registered system core resume callbacks.

**Parameters**

`void`
:   no arguments

**Description**

This function is executed with one CPU on-line and disabled interrupts.

struct [device](#c.device "device") \*class\_find\_device\_by\_name(const struct [class](#c.class_find_device_by_name "class") \*class, const char \*name)
:   device iterator for locating a particular device of a specific name.

**Parameters**

`const struct class *class`
:   class type

`const char *name`
:   name of the device to match

struct [device](#c.device "device") \*class\_find\_device\_by\_of\_node(const struct [class](#c.class_find_device_by_of_node "class") \*class, const struct device\_node \*np)
:   device iterator for locating a particular device matching the of\_node.

**Parameters**

`const struct class *class`
:   class type

`const struct device_node *np`
:   of\_node of the device to match.

struct [device](#c.device "device") \*class\_find\_device\_by\_fwnode(const struct [class](#c.class_find_device_by_fwnode "class") \*class, const struct fwnode\_handle \*fwnode)
:   device iterator for locating a particular device matching the fwnode.

**Parameters**

`const struct class *class`
:   class type

`const struct fwnode_handle *fwnode`
:   fwnode of the device to match.

struct [device](#c.device "device") \*class\_find\_device\_by\_devt(const struct [class](#c.class_find_device_by_devt "class") \*class, dev\_t devt)
:   device iterator for locating a particular device matching the device type.

**Parameters**

`const struct class *class`
:   class type

`dev_t devt`
:   device type of the device to match.

struct [device](#c.device "device") \*class\_find\_device\_by\_acpi\_dev(const struct [class](#c.class_find_device_by_acpi_dev "class") \*class, const struct acpi\_device \*adev)
:   device iterator for locating a particular device matching the ACPI\_COMPANION device.

**Parameters**

`const struct class *class`
:   class type

`const struct acpi_device *adev`
:   ACPI\_COMPANION device to match.

struct [class](#c.class "class") \*class\_create(const char \*name)
:   create a [`struct class`](#c.class "class") structure

**Parameters**

`const char *name`
:   pointer to a string for the name of this class.

**Description**

This is used to create a [`struct class`](#c.class "class") pointer that can then be used
in calls to [`device_create()`](#c.device_create "device_create").

Returns [`struct class`](#c.class "class") pointer on success, or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on error.

Note, the pointer created here is to be destroyed when finished by
making a call to [`class_destroy()`](#c.class_destroy "class_destroy").

void class\_destroy(const struct [class](#c.class "class") \*cls)
:   destroys a [`struct class`](#c.class "class") structure

**Parameters**

`const struct class *cls`
:   pointer to the [`struct class`](#c.class "class") that is to be destroyed

**Description**

Note, the pointer to be destroyed must have been created with a call
to [`class_create()`](#c.class_create "class_create").

void class\_dev\_iter\_init(struct class\_dev\_iter \*iter, const struct [class](#c.class_dev_iter_init "class") \*class, const struct [device](#c.device "device") \*start, const struct device\_type \*type)
:   initialize class device iterator

**Parameters**

`struct class_dev_iter *iter`
:   class iterator to initialize

`const struct class *class`
:   the class we wanna iterate over

`const struct device *start`
:   the device to start iterating from, if any

`const struct device_type *type`
:   device\_type of the devices to iterate over, NULL for all

**Description**

Initialize class iterator **iter** such that it iterates over devices
of **class**. If **start** is set, the list iteration will start there,
otherwise if it is NULL, the iteration starts at the beginning of
the list.

struct [device](#c.device "device") \*class\_dev\_iter\_next(struct class\_dev\_iter \*iter)
:   iterate to the next device

**Parameters**

`struct class_dev_iter *iter`
:   class iterator to proceed

**Description**

Proceed **iter** to the next device and return it. Returns NULL if
iteration is complete.

The returned device is referenced and won’t be released till
iterator is proceed to the next device or exited. The caller is
free to do whatever it wants to do with the device including
calling back into class code.

void class\_dev\_iter\_exit(struct class\_dev\_iter \*iter)
:   finish iteration

**Parameters**

`struct class_dev_iter *iter`
:   class iterator to finish

**Description**

Finish an iteration. Always call this function after iteration is
complete whether the iteration ran till the end or not.

int class\_for\_each\_device(const struct [class](#c.class_for_each_device "class") \*class, const struct [device](#c.device "device") \*start, void \*data, device\_iter\_t fn)
:   device iterator

**Parameters**

`const struct class *class`
:   the class we’re iterating

`const struct device *start`
:   the device to start with in the list, if any.

`void *data`
:   data for the callback

`device_iter_t fn`
:   function to be called for each device

**Description**

Iterate over **class**’s list of devices, and call **fn** for each,
passing it **data**. If **start** is set, the list iteration will start
there, otherwise if it is NULL, the iteration starts at the
beginning of the list.

We check the return of **fn** each time. If it returns anything
other than 0, we break out and return that value.

**fn** is allowed to do anything including calling back into class
code. There’s no locking restriction.

struct [device](#c.device "device") \*class\_find\_device(const struct [class](#c.class_find_device "class") \*class, const struct [device](#c.device "device") \*start, const void \*data, device\_match\_t match)
:   device iterator for locating a particular device

**Parameters**

`const struct class *class`
:   the class we’re iterating

`const struct device *start`
:   Device to begin with

`const void *data`
:   data for the match function

`device_match_t match`
:   function to check device

**Description**

This is similar to the `class_for_each_dev()` function above, but it
returns a reference to a device that is ‘found’ for later use, as
determined by the **match** callback.

The callback should return 0 if the device doesn’t match and non-zero
if it does. If the callback returns non-zero, this function will
return to the caller and not iterate over any more devices.

Note, you will need to drop the reference with [`put_device()`](#c.put_device "put_device") after use.

**match** is allowed to do anything including calling back into class
code. There’s no locking restriction.

struct class\_compat \*class\_compat\_register(const char \*name)
:   register a compatibility class

**Parameters**

`const char *name`
:   the name of the class

**Description**

Compatibility class are meant as a temporary user-space compatibility
workaround when converting a family of class devices to a bus devices.

void class\_compat\_unregister(struct class\_compat \*cls)
:   unregister a compatibility class

**Parameters**

`struct class_compat *cls`
:   the class to unregister

int class\_compat\_create\_link(struct class\_compat \*cls, struct [device](#c.device "device") \*dev)
:   create a compatibility class device link to a bus device

**Parameters**

`struct class_compat *cls`
:   the compatibility class

`struct device *dev`
:   the target bus device

void class\_compat\_remove\_link(struct class\_compat \*cls, struct [device](#c.device "device") \*dev)
:   remove a compatibility class device link to a bus device

**Parameters**

`struct class_compat *cls`
:   the compatibility class

`struct device *dev`
:   the target bus device

bool class\_is\_registered(const struct [class](#c.class_is_registered "class") \*class)
:   determine if at this moment in time, a class is registered in the driver core or not.

**Parameters**

`const struct class *class`
:   the class to check

**Description**

Returns a boolean to state if the class is registered in the driver core
or not. Note that the value could switch right after this call is made,
so only use this in places where you “know” it is safe to do so (usually
to determine if the specific class has been registered yet or not).

Be careful in using this.

struct faux\_device
:   a “faux” device

**Definition**:

```
struct faux_device {
    struct device dev;
};
```

**Members**

`dev`
:   internal [`struct device`](#c.device "device") of the object

**Description**

A simple faux device that can be created/destroyed. To be used when a
driver only needs to have a device to “hang” something off. This can be
used for downloading firmware or other basic tasks. Use this instead of
a `struct platform_device` if the device has no resources assigned to
it at all.

struct faux\_device\_ops
:   a set of callbacks for a [`struct faux_device`](#c.faux_device "faux_device")

**Definition**:

```
struct faux_device_ops {
    int (*probe)(struct faux_device *faux_dev);
    void (*remove)(struct faux_device *faux_dev);
};
```

**Members**

`probe`
:   called when a faux device is probed by the driver core
    before the device is fully bound to the internal faux bus
    code. If probe succeeds, return 0, otherwise return a
    negative error number to stop the probe sequence from
    succeeding.

`remove`
:   called when a faux device is removed from the system

**Description**

Both **probe** and **remove** are optional, if not needed, set to NULL.

struct [faux\_device](#c.faux_device "faux_device") \*faux\_device\_create\_with\_groups(const char \*name, struct [device](#c.device "device") \*parent, const struct [faux\_device\_ops](#c.faux_device_ops "faux_device_ops") \*faux\_ops, const struct attribute\_group \*\*groups)
:   Create and register with the driver core a faux device and populate the device with an initial set of sysfs attributes.

**Parameters**

`const char *name`
:   The name of the device we are adding, must be unique for
    all faux devices.

`struct device *parent`
:   Pointer to a potential parent [`struct device`](#c.device "device"). If set to
    NULL, the device will be created in the “root” of the faux
    device tree in sysfs.

`const struct faux_device_ops *faux_ops`
:   [`struct faux_device_ops`](#c.faux_device_ops "faux_device_ops") that the new device will call back
    into, can be NULL.

`const struct attribute_group **groups`
:   The set of sysfs attributes that will be created for this
    device when it is registered with the driver core.

**Description**

Create a new faux device and register it in the driver core properly.
If present, callbacks in **faux\_ops** will be called with the device that
for the caller to do something with at the proper time given the
device’s lifecycle.

Note, when this function is called, the functions specified in `struct
faux_ops` can be called before the function returns, so be prepared for
everything to be properly initialized before that point in time. If the
probe callback (if one is present) does NOT succeed, the creation of the
device will fail and NULL will be returned.

**Return**

* NULL if an error happened with creating the device
* pointer to a valid [`struct faux_device`](#c.faux_device "faux_device") that is registered with sysfs

struct [faux\_device](#c.faux_device "faux_device") \*faux\_device\_create(const char \*name, struct [device](#c.device "device") \*parent, const struct [faux\_device\_ops](#c.faux_device_ops "faux_device_ops") \*faux\_ops)
:   create and register with the driver core a faux device

**Parameters**

`const char *name`
:   The name of the device we are adding, must be unique for all
    faux devices.

`struct device *parent`
:   Pointer to a potential parent [`struct device`](#c.device "device"). If set to
    NULL, the device will be created in the “root” of the faux
    device tree in sysfs.

`const struct faux_device_ops *faux_ops`
:   [`struct faux_device_ops`](#c.faux_device_ops "faux_device_ops") that the new device will call back
    into, can be NULL.

**Description**

Create a new faux device and register it in the driver core properly.
If present, callbacks in **faux\_ops** will be called with the device that
for the caller to do something with at the proper time given the
device’s lifecycle.

Note, when this function is called, the functions specified in `struct
faux_ops` can be called before the function returns, so be prepared for
everything to be properly initialized before that point in time.

**Return**

* NULL if an error happened with creating the device
* pointer to a valid [`struct faux_device`](#c.faux_device "faux_device") that is registered with sysfs

void faux\_device\_destroy(struct [faux\_device](#c.faux_device "faux_device") \*faux\_dev)
:   destroy a faux device

**Parameters**

`struct faux_device *faux_dev`
:   faux device to destroy

**Description**

Unregisters and cleans up a device that was created with a call to
[`faux_device_create()`](#c.faux_device_create "faux_device_create")

struct node\_access\_nodes
:   Access class device to hold user visible relationships to other nodes.

**Definition**:

```
struct node_access_nodes {
    struct device           dev;
    struct list_head        list_node;
    unsigned int            access;
#ifdef CONFIG_HMEM_REPORTING;
    struct access_coordinate        coord;
#endif;
};
```

**Members**

`dev`
:   Device for this memory access class

`list_node`
:   List element in the node’s access list

`access`
:   The access class rank

`coord`
:   Heterogeneous memory performance coordinates

struct node\_cache\_info
:   Internal tracking for memory node caches

**Definition**:

```
struct node_cache_info {
    struct device dev;
    struct list_head node;
    struct node_cache_attrs cache_attrs;
};
```

**Members**

`dev`
:   Device represeting the cache level

`node`
:   List element for tracking in the node

`cache_attrs`
:   Attributes for this cache level

void node\_add\_cache(unsigned int nid, struct node\_cache\_attrs \*cache\_attrs)
:   add cache attribute to a memory node

**Parameters**

`unsigned int nid`
:   Node identifier that has new cache attributes

`struct node_cache_attrs *cache_attrs`
:   Attributes for the cache being added

int register\_memory\_node\_under\_compute\_node(unsigned int mem\_nid, unsigned int cpu\_nid, enum access\_coordinate\_class access)
:   link memory node to its compute node for a given access class.

**Parameters**

`unsigned int mem_nid`
:   Memory node number

`unsigned int cpu_nid`
:   Cpu node number

`enum access_coordinate_class access`
:   Access class to register

**Description**

> For use with platforms that may have separate memory and compute nodes.
> This function will export node relationships linking which memory
> initiator nodes can access memory targets at a given ranked access
> class.

int register\_node(int nid)
:   Initialize and register the node device.

**Parameters**

`int nid`
:   Node number to use when creating the device.

**Return**

0 on success, -errno otherwise

void unregister\_node(int nid)
:   unregister a node device

**Parameters**

`int nid`
:   nid of the node going away

**Description**

Unregisters the node device at node id **nid**. All the devices on the
node must be unregistered before calling this function.

int transport\_class\_register(struct transport\_class \*tclass)
:   register an initial transport class

**Parameters**

`struct transport_class *tclass`
:   a pointer to the transport class structure to be initialised

**Description**

The transport class contains an embedded class which is used to
identify it. The caller should initialise this structure with
zeros and then generic class must have been initialised with the
actual transport class unique name. There’s a macro
`DECLARE_TRANSPORT_CLASS()` to do this (declared classes still must
be registered).

Returns 0 on success or error on failure.

void transport\_class\_unregister(struct transport\_class \*tclass)
:   unregister a previously registered class

**Parameters**

`struct transport_class *tclass`
:   The transport class to unregister

**Description**

Must be called prior to deallocating the memory for the transport
class.

void anon\_transport\_class\_register(struct anon\_transport\_class \*atc)
:   register an anonymous class

**Parameters**

`struct anon_transport_class *atc`
:   The anon transport class to register

**Description**

The anonymous transport class contains both a transport class and a
container. The idea of an anonymous class is that it never
actually has any device attributes associated with it (and thus
saves on container storage). So it can only be used for triggering
events. Use prezero and then use `DECLARE_ANON_TRANSPORT_CLASS()` to
initialise the anon transport class storage.

void anon\_transport\_class\_unregister(struct anon\_transport\_class \*atc)
:   unregister an anon class

**Parameters**

`struct anon_transport_class *atc`
:   Pointer to the anon transport class to unregister

**Description**

Must be called prior to deallocating the memory for the anon
transport class.

void transport\_setup\_device(struct [device](#c.device "device") \*dev)
:   declare a new dev for transport class association but don’t make it visible yet.

**Parameters**

`struct device *dev`
:   the generic device representing the entity being added

**Description**

Usually, dev represents some component in the HBA system (either
the HBA itself or a device remote across the HBA bus). This
routine is simply a trigger point to see if any set of transport
classes wishes to associate with the added device. This allocates
storage for the class device and initialises it, but does not yet
add it to the system or add attributes to it (you do this with
transport\_add\_device). If you have no need for a separate setup
and add operations, use transport\_register\_device (see
transport\_class.h).

int transport\_add\_device(struct [device](#c.device "device") \*dev)
:   declare a new dev for transport class association

**Parameters**

`struct device *dev`
:   the generic device representing the entity being added

**Description**

Usually, dev represents some component in the HBA system (either
the HBA itself or a device remote across the HBA bus). This
routine is simply a trigger point used to add the device to the
system and register attributes for it.

void transport\_configure\_device(struct [device](#c.device "device") \*dev)
:   configure an already set up device

**Parameters**

`struct device *dev`
:   generic device representing device to be configured

**Description**

The idea of configure is simply to provide a point within the setup
process to allow the transport class to extract information from a
device after it has been setup. This is used in SCSI because we
have to have a setup device to begin using the HBA, but after we
send the initial inquiry, we use configure to extract the device
parameters. The device need not have been added to be configured.

void transport\_remove\_device(struct [device](#c.device "device") \*dev)
:   remove the visibility of a device

**Parameters**

`struct device *dev`
:   generic device to remove

**Description**

This call removes the visibility of the device (to the user from
sysfs), but does not destroy it. To eliminate a device entirely
you must also call transport\_destroy\_device. If you don’t need to
do remove and destroy as separate operations, use
`transport_unregister_device()` (see transport\_class.h) which will
perform both calls for you.

void transport\_destroy\_device(struct [device](#c.device "device") \*dev)
:   destroy a removed device

**Parameters**

`struct device *dev`
:   device to eliminate from the transport class.

**Description**

This call triggers the elimination of storage associated with the
transport classdev. Note: all it really does is relinquish a
reference to the classdev. The memory will not be freed until the
last reference goes to zero. Note also that the classdev retains a
reference count on dev, so dev too will remain for as long as the
transport class device remains around.

int driver\_deferred\_probe\_check\_state(struct [device](#c.device "device") \*dev)
:   Check deferred probe state

**Parameters**

`struct device *dev`
:   device to check

**Return**

* -ENODEV if initcalls have completed and modules are disabled.
* -ETIMEDOUT if the deferred probe timeout was set and has expired
  and modules are enabled.
* -EPROBE\_DEFER in other cases.

**Description**

Drivers or subsystems can opt-in to calling this function instead of directly
returning -EPROBE\_DEFER.

bool device\_is\_bound(struct [device](#c.device "device") \*dev)
:   Check if device is bound to a driver

**Parameters**

`struct device *dev`
:   device to check

**Description**

Returns true if passed device has already finished probing successfully
against a driver.

This function must be called with the device lock held.

int device\_bind\_driver(struct [device](#c.device "device") \*dev)
:   bind a driver to one device.

**Parameters**

`struct device *dev`
:   device.

**Description**

Allow manual attachment of a driver to a device.
Caller must have already set **dev->driver**.

Note that this does not modify the bus reference count.
Please verify that is accounted for before calling this.
(It is ok to call with no other effort from a driver’s `probe()` method.)

This function must be called with the device lock held.

Callers should prefer to use [`device_driver_attach()`](#c.device_driver_attach "device_driver_attach") instead.

void wait\_for\_device\_probe(void)

**Parameters**

`void`
:   no arguments

**Description**

Wait for device probing to be completed.

int device\_attach(struct [device](#c.device "device") \*dev)
:   try to attach device to a driver.

**Parameters**

`struct device *dev`
:   device.

**Description**

Walk the list of drivers that the bus has and call
`driver_probe_device()` for each pair. If a compatible
pair is found, break out and return.

Returns 1 if the device was bound to a driver;
0 if no matching driver was found;
-ENODEV if the device is not registered.

When called for a USB interface, **dev->parent** lock must be held.

int device\_driver\_attach(const struct [device\_driver](#c.device_driver "device_driver") \*drv, struct [device](#c.device "device") \*dev)
:   attach a specific driver to a specific device

**Parameters**

`const struct device_driver *drv`
:   Driver to attach

`struct device *dev`
:   Device to attach it to

**Description**

Manually attach driver to a device. Will acquire both **dev** lock and
**dev->parent** lock if needed. Returns 0 on success, -ERR on failure.

int driver\_attach(const struct [device\_driver](#c.device_driver "device_driver") \*drv)
:   try to bind driver to devices.

**Parameters**

`const struct device_driver *drv`
:   driver.

**Description**

Walk the list of devices that the bus has on it and try to
match the driver with each one. If `driver_probe_device()`
returns 0 and the **dev->driver** is set, we’ve found a
compatible pair.

void device\_release\_driver(struct [device](#c.device "device") \*dev)
:   manually detach device from driver.

**Parameters**

`struct device *dev`
:   device.

**Description**

Manually detach device from driver.
When called for a USB interface, **dev->parent** lock must be held.

If this function is to be called with **dev->parent** lock held, ensure that
the device’s consumers are unbound in advance or that their locks can be
acquired under the **dev->parent** lock.

struct platform\_device\_info
:   set of parameters for creating a platform device

**Definition**:

```
struct platform_device_info {
    struct device *parent;
    struct fwnode_handle *fwnode;
    bool of_node_reused;
    const char *name;
    int id;
    const struct resource *res;
    unsigned int num_res;
    const void *data;
    size_t size_data;
    u64 dma_mask;
    const struct software_node *swnode;
    const struct property_entry *properties;
};
```

**Members**

`parent`
:   parent device for the new platform device.

`fwnode`
:   firmware node associated with the device.

`of_node_reused`
:   indicates that device tree node associated with the device
    is shared with another device, typically its ancestor. Setting this to
    `true` prevents the device from being matched via the OF match table,
    and stops the device core from automatically binding pinctrl
    configuration to avoid disrupting the other device.

`name`
:   name of the device.

`id`
:   instance ID of the device. Use `PLATFORM_DEVID_NONE` if there is only
    one instance of the device, or `PLATFORM_DEVID_AUTO` to let the
    kernel automatically assign a unique instance ID.

`res`
:   set of resources to attach to the device.

`num_res`
:   number of entries in **res**.

`data`
:   device-specific data for this platform device.

`size_data`
:   size of device-specific data.

`dma_mask`
:   DMA mask for the device.

`swnode`
:   a secondary software node to be attached to the device. The node
    will be automatically registered and its lifetime tied to the platform
    device if it is not registered yet.

`properties`
:   a set of software properties for the device. If provided,
    a managed software node will be automatically created and
    assigned to the device. The properties array must be terminated
    with a sentinel entry. Specifying both **properties** and **swnode** is not
    allowed.

**Description**

This structure is used to hold information needed to create and register
a platform device using [`platform_device_register_full()`](#c.platform_device_register_full "platform_device_register_full").

[`platform_device_register_full()`](#c.platform_device_register_full "platform_device_register_full") makes deep copies of **name**, **res**, **data** and
**properties**, so the caller does not need to keep them after registration.
If the registration is performed during initialization, these can be marked
as \_\_initconst.

struct platform\_device \*platform\_device\_register\_resndata(struct [device](#c.device "device") \*parent, const char \*name, int id, const struct resource \*res, unsigned int num, const void \*data, size\_t size)
:   add a platform-level device with resources and platform-specific data

**Parameters**

`struct device *parent`
:   parent device for the device we’re adding

`const char *name`
:   base name of the device we’re adding

`int id`
:   instance id

`const struct resource *res`
:   set of resources that needs to be allocated for the device

`unsigned int num`
:   number of resources

`const void *data`
:   platform specific data for this platform device

`size_t size`
:   size of platform specific data

**Description**

Returns `struct platform_device` pointer on success, or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on error.

struct platform\_device \*platform\_device\_register\_simple(const char \*name, int id, const struct resource \*res, unsigned int num)
:   add a platform-level device and its resources

**Parameters**

`const char *name`
:   base name of the device we’re adding

`int id`
:   instance id

`const struct resource *res`
:   set of resources that needs to be allocated for the device

`unsigned int num`
:   number of resources

**Description**

This function creates a simple platform device that requires minimal
resource and memory management. Canned release function freeing memory
allocated for the device allows drivers using such devices to be
unloaded without waiting for the last reference to the device to be
dropped.

This interface is primarily intended for use with legacy drivers which
probe hardware directly. Because such drivers create sysfs device nodes
themselves, rather than letting system infrastructure handle such device
enumeration tasks, they don’t fully conform to the Linux driver model.
In particular, when such drivers are built as modules, they can’t be
“hotplugged”.

Returns `struct platform_device` pointer on success, or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on error.

struct platform\_device \*platform\_device\_register\_data(struct [device](#c.device "device") \*parent, const char \*name, int id, const void \*data, size\_t size)
:   add a platform-level device with platform-specific data

**Parameters**

`struct device *parent`
:   parent device for the device we’re adding

`const char *name`
:   base name of the device we’re adding

`int id`
:   instance id

`const void *data`
:   platform specific data for this platform device

`size_t size`
:   size of platform specific data

**Description**

This function creates a simple platform device that requires minimal
resource and memory management. Canned release function freeing memory
allocated for the device allows drivers using such devices to be
unloaded without waiting for the last reference to the device to be
dropped.

Returns `struct platform_device` pointer on success, or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on error.

struct resource \*platform\_get\_resource(struct platform\_device \*dev, unsigned int type, unsigned int num)
:   get a resource for a device

**Parameters**

`struct platform_device *dev`
:   platform device

`unsigned int type`
:   resource type

`unsigned int num`
:   resource index

**Return**

a pointer to the resource or NULL on failure.

void \_\_iomem \*devm\_platform\_get\_and\_ioremap\_resource(struct platform\_device \*pdev, unsigned int index, struct resource \*\*res)
:   call `devm_ioremap_resource()` for a platform device and get resource

**Parameters**

`struct platform_device *pdev`
:   platform device to use both for memory resource lookup as well as
    resource management

`unsigned int index`
:   resource index

`struct resource **res`
:   optional output parameter to store a pointer to the obtained resource.

**Return**

a pointer to the remapped memory or an [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") encoded error code
on failure.

void \_\_iomem \*devm\_platform\_ioremap\_resource(struct platform\_device \*pdev, unsigned int index)
:   call `devm_ioremap_resource()` for a platform device

**Parameters**

`struct platform_device *pdev`
:   platform device to use both for memory resource lookup as well as
    resource management

`unsigned int index`
:   resource index

**Return**

a pointer to the remapped memory or an [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") encoded error code
on failure.

void \_\_iomem \*devm\_platform\_ioremap\_resource\_byname(struct platform\_device \*pdev, const char \*name)
:   call devm\_ioremap\_resource for a platform device, retrieve the resource by name

**Parameters**

`struct platform_device *pdev`
:   platform device to use both for memory resource lookup as well as
    resource management

`const char *name`
:   name of the resource

**Return**

a pointer to the remapped memory or an [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") encoded error code
on failure.

int platform\_get\_irq\_affinity(struct platform\_device \*dev, unsigned int num, const struct cpumask \*\*affinity)
:   get an optional IRQ and its affinity for a device

**Parameters**

`struct platform_device *dev`
:   platform device

`unsigned int num`
:   interrupt number index

`const struct cpumask **affinity`
:   optional cpumask pointer to get the affinity of a per-cpu interrupt

**Description**

Gets an interrupt for a platform device. Device drivers should check the
return value for errors so as to not pass a negative integer value to
the [`request_irq()`](../core-api/genericirq.html#c.request_irq "request_irq") APIs. Optional affinity information is provided in the
affinity pointer if available, and NULL otherwise.

**Return**

non-zero interrupt number on success, negative error number on failure.

int platform\_get\_irq\_optional(struct platform\_device \*dev, unsigned int num)
:   get an optional interrupt for a device

**Parameters**

`struct platform_device *dev`
:   platform device

`unsigned int num`
:   interrupt number index

**Description**

Gets an interrupt for a platform device. Device drivers should check the
return value for errors so as to not pass a negative integer value to
the [`request_irq()`](../core-api/genericirq.html#c.request_irq "request_irq") APIs. This is the same as [`platform_get_irq()`](#c.platform_get_irq "platform_get_irq"), except
that it does not print an error message if an interrupt can not be
obtained.

For example:

```
int irq = platform_get_irq_optional(pdev, 0);
if (irq < 0)
        return irq;
```

**Return**

non-zero interrupt number on success, negative error number on failure.

int platform\_get\_irq(struct platform\_device \*dev, unsigned int num)
:   get an IRQ for a device

**Parameters**

`struct platform_device *dev`
:   platform device

`unsigned int num`
:   IRQ number index

**Description**

Gets an IRQ for a platform device and prints an error message if finding the
IRQ fails. Device drivers should check the return value for errors so as to
not pass a negative integer value to the [`request_irq()`](../core-api/genericirq.html#c.request_irq "request_irq") APIs.

For example:

```
int irq = platform_get_irq(pdev, 0);
if (irq < 0)
        return irq;
```

**Return**

non-zero IRQ number on success, negative error number on failure.

int platform\_irq\_count(struct platform\_device \*dev)
:   Count the number of IRQs a platform device uses

**Parameters**

`struct platform_device *dev`
:   platform device

**Return**

Number of IRQs a platform device uses or EPROBE\_DEFER

int devm\_platform\_get\_irqs\_affinity(struct platform\_device \*dev, struct [irq\_affinity](../core-api/genericirq.html#c.irq_affinity "irq_affinity") \*affd, unsigned int minvec, unsigned int maxvec, int \*\*irqs)
:   devm method to get a set of IRQs for a device using an interrupt affinity descriptor

**Parameters**

`struct platform_device *dev`
:   platform device pointer

`struct irq_affinity *affd`
:   affinity descriptor

`unsigned int minvec`
:   minimum count of interrupt vectors

`unsigned int maxvec`
:   maximum count of interrupt vectors

`int **irqs`
:   pointer holder for IRQ numbers

**Description**

Gets a set of IRQs for a platform device, and updates IRQ afffinty according
to the passed affinity descriptor

**Return**

Number of vectors on success, negative error number on failure.

struct resource \*platform\_get\_resource\_byname(struct platform\_device \*dev, unsigned int type, const char \*name)
:   get a resource for a device by name

**Parameters**

`struct platform_device *dev`
:   platform device

`unsigned int type`
:   resource type

`const char *name`
:   resource name

int platform\_get\_irq\_byname(struct platform\_device \*dev, const char \*name)
:   get an IRQ for a device by name

**Parameters**

`struct platform_device *dev`
:   platform device

`const char *name`
:   IRQ name

**Description**

Get an IRQ like [`platform_get_irq()`](#c.platform_get_irq "platform_get_irq"), but then by name rather then by index.

**Return**

non-zero IRQ number on success, negative error number on failure.

int platform\_get\_irq\_byname\_optional(struct platform\_device \*dev, const char \*name)
:   get an optional IRQ for a device by name

**Parameters**

`struct platform_device *dev`
:   platform device

`const char *name`
:   IRQ name

**Description**

Get an optional IRQ by name like [`platform_get_irq_byname()`](#c.platform_get_irq_byname "platform_get_irq_byname"). Except that it
does not print an error message if an IRQ can not be obtained.

**Return**

non-zero IRQ number on success, negative error number on failure.

int platform\_add\_devices(struct platform\_device \*\*devs, int num)
:   add a numbers of platform devices

**Parameters**

`struct platform_device **devs`
:   array of platform devices to add

`int num`
:   number of platform devices in array

**Return**

0 on success, negative error number on failure.

void platform\_device\_put(struct platform\_device \*pdev)
:   destroy a platform device

**Parameters**

`struct platform_device *pdev`
:   platform device to free

**Description**

Free all memory associated with a platform device. This function must
\_only\_ be externally called in error cases. All other usage is a bug.

struct platform\_device \*platform\_device\_alloc(const char \*name, int id)
:   create a platform device

**Parameters**

`const char *name`
:   base name of the device we’re adding

`int id`
:   instance id

**Description**

Create a platform device object which can have other objects attached
to it, and which will have attached objects freed when it is released.

int platform\_device\_add\_resources(struct platform\_device \*pdev, const struct resource \*res, unsigned int num)
:   add resources to a platform device

**Parameters**

`struct platform_device *pdev`
:   platform device allocated by platform\_device\_alloc to add resources to

`const struct resource *res`
:   set of resources that needs to be allocated for the device

`unsigned int num`
:   number of resources

**Description**

Add a copy of the resources to the platform device. The memory
associated with the resources will be freed when the platform device is
released.

int platform\_device\_add\_data(struct platform\_device \*pdev, const void \*data, size\_t size)
:   add platform-specific data to a platform device

**Parameters**

`struct platform_device *pdev`
:   platform device allocated by platform\_device\_alloc to add resources to

`const void *data`
:   platform specific data for this platform device

`size_t size`
:   size of platform specific data

**Description**

Add a copy of platform specific data to the platform device’s
platform\_data pointer. The memory associated with the platform data
will be freed when the platform device is released.

int platform\_device\_add(struct platform\_device \*pdev)
:   add a platform device to device hierarchy

**Parameters**

`struct platform_device *pdev`
:   platform device we’re adding

**Description**

This is part 2 of [`platform_device_register()`](#c.platform_device_register "platform_device_register"), though may be called
separately \_iff\_ pdev was allocated by [`platform_device_alloc()`](#c.platform_device_alloc "platform_device_alloc").

void platform\_device\_del(struct platform\_device \*pdev)
:   remove a platform-level device

**Parameters**

`struct platform_device *pdev`
:   platform device we’re removing

**Description**

Note that this function will also release all memory- and port-based
resources owned by the device (**dev->resource**). This function must
\_only\_ be externally called in error cases. All other usage is a bug.

int platform\_device\_register(struct platform\_device \*pdev)
:   add a platform-level device

**Parameters**

`struct platform_device *pdev`
:   platform device we’re adding

**NOTE**

\_Never\_ directly free **pdev** after calling this function, even if it
returned an error! Always use [`platform_device_put()`](#c.platform_device_put "platform_device_put") to give up the
reference initialised in this function instead.

void platform\_device\_unregister(struct platform\_device \*pdev)
:   unregister a platform-level device

**Parameters**

`struct platform_device *pdev`
:   platform device we’re unregistering

**Description**

Unregistration is done in 2 steps. First we release all resources
and remove it from the subsystem, then we drop reference count by
calling [`platform_device_put()`](#c.platform_device_put "platform_device_put").

struct platform\_device \*platform\_device\_register\_full(const struct [platform\_device\_info](#c.platform_device_info "platform_device_info") \*pdevinfo)
:   add a platform-level device with resources and platform-specific data

**Parameters**

`const struct platform_device_info *pdevinfo`
:   data used to create device

**Description**

Returns `struct platform_device` pointer on success, or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on error.

int \_\_platform\_driver\_register(struct platform\_driver \*drv, struct module \*owner)
:   register a driver for platform-level devices

**Parameters**

`struct platform_driver *drv`
:   platform driver structure

`struct module *owner`
:   owning module/driver

void platform\_driver\_unregister(struct platform\_driver \*drv)
:   unregister a driver for platform-level devices

**Parameters**

`struct platform_driver *drv`
:   platform driver structure

int \_\_platform\_driver\_probe(struct platform\_driver \*drv, int (\*probe)(struct platform\_device\*), struct [module](#c.__platform_driver_probe "module") \*module)
:   register driver for non-hotpluggable device

**Parameters**

`struct platform_driver *drv`
:   platform driver structure

`int (*probe)(struct platform_device *)`
:   the driver probe routine, probably from an \_\_init section

`struct module *module`
:   module which will be the owner of the driver

**Description**

Use this instead of `platform_driver_register()` when you know the device
is not hotpluggable and has already been registered, and you want to
remove its run-once `probe()` infrastructure from memory after the driver
has bound to the device.

One typical use for this would be with drivers for controllers integrated
into system-on-chip processors, where the controller devices have been
configured as part of board setup.

Note that this is incompatible with deferred probing.

Returns zero if the driver registered and bound to a device, else returns
a negative error code and with the driver not registered.

struct platform\_device \*\_\_platform\_create\_bundle(struct platform\_driver \*driver, int (\*probe)(struct platform\_device\*), struct resource \*res, unsigned int n\_res, const void \*data, size\_t size, struct [module](#c.__platform_create_bundle "module") \*module)
:   register driver and create corresponding device

**Parameters**

`struct platform_driver *driver`
:   platform driver structure

`int (*probe)(struct platform_device *)`
:   the driver probe routine, probably from an \_\_init section

`struct resource *res`
:   set of resources that needs to be allocated for the device

`unsigned int n_res`
:   number of resources

`const void *data`
:   platform specific data for this platform device

`size_t size`
:   size of platform specific data

`struct module *module`
:   module which will be the owner of the driver

**Description**

Use this in legacy-style modules that probe hardware directly and
register a single platform device and corresponding platform driver.

Returns `struct platform_device` pointer on success, or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on error.

int \_\_platform\_register\_drivers(struct platform\_driver \*const \*drivers, unsigned int count, struct module \*owner)
:   register an array of platform drivers

**Parameters**

`struct platform_driver * const *drivers`
:   an array of drivers to register

`unsigned int count`
:   the number of drivers to register

`struct module *owner`
:   module owning the drivers

**Description**

Registers platform drivers specified by an array. On failure to register a
driver, all previously registered drivers will be unregistered. Callers of
this API should use [`platform_unregister_drivers()`](#c.platform_unregister_drivers "platform_unregister_drivers") to unregister drivers in
the reverse order.

**Return**

0 on success or a negative error code on failure.

void platform\_unregister\_drivers(struct platform\_driver \*const \*drivers, unsigned int count)
:   unregister an array of platform drivers

**Parameters**

`struct platform_driver * const *drivers`
:   an array of drivers to unregister

`unsigned int count`
:   the number of drivers to unregister

**Description**

Unregisters platform drivers specified by an array. This is typically used
to complement an earlier call to `platform_register_drivers()`. Drivers are
unregistered in the reverse order in which they were registered.

struct [device](#c.device "device") \*platform\_find\_device\_by\_driver(struct [device](#c.device "device") \*start, const struct [device\_driver](#c.device_driver "device_driver") \*drv)
:   Find a platform device with a given driver.

**Parameters**

`struct device *start`
:   The device to start the search from.

`const struct device_driver *drv`
:   The device driver to look for.

struct [device](#c.device "device") \*bus\_find\_device\_by\_name(const struct [bus\_type](#c.bus_type "bus_type") \*bus, struct [device](#c.device "device") \*start, const char \*name)
:   device iterator for locating a particular device of a specific name.

**Parameters**

`const struct bus_type *bus`
:   bus type

`struct device *start`
:   Device to begin with

`const char *name`
:   name of the device to match

struct [device](#c.device "device") \*bus\_find\_device\_by\_of\_node(const struct [bus\_type](#c.bus_type "bus_type") \*bus, const struct device\_node \*np)
:   device iterator for locating a particular device matching the of\_node.

**Parameters**

`const struct bus_type *bus`
:   bus type

`const struct device_node *np`
:   of\_node of the device to match.

struct [device](#c.device "device") \*bus\_find\_device\_by\_fwnode(const struct [bus\_type](#c.bus_type "bus_type") \*bus, const struct fwnode\_handle \*fwnode)
:   device iterator for locating a particular device matching the fwnode.

**Parameters**

`const struct bus_type *bus`
:   bus type

`const struct fwnode_handle *fwnode`
:   fwnode of the device to match.

struct [device](#c.device "device") \*bus\_find\_device\_by\_devt(const struct [bus\_type](#c.bus_type "bus_type") \*bus, dev\_t devt)
:   device iterator for locating a particular device matching the device type.

**Parameters**

`const struct bus_type *bus`
:   bus type

`dev_t devt`
:   device type of the device to match.

struct [device](#c.device "device") \*bus\_find\_next\_device(const struct [bus\_type](#c.bus_type "bus_type") \*bus, struct [device](#c.device "device") \*cur)
:   Find the next device after a given device in a given bus.

**Parameters**

`const struct bus_type *bus`
:   bus type

`struct device *cur`
:   device to begin the search with.

struct [device](#c.device "device") \*bus\_find\_device\_by\_acpi\_dev(const struct [bus\_type](#c.bus_type "bus_type") \*bus, const struct acpi\_device \*adev)
:   device iterator for locating a particular device matching the ACPI COMPANION device.

**Parameters**

`const struct bus_type *bus`
:   bus type

`const struct acpi_device *adev`
:   ACPI COMPANION device to match.

int bus\_for\_each\_dev(const struct [bus\_type](#c.bus_type "bus_type") \*bus, struct [device](#c.device "device") \*start, void \*data, device\_iter\_t fn)
:   device iterator.

**Parameters**

`const struct bus_type *bus`
:   bus type.

`struct device *start`
:   device to start iterating from.

`void *data`
:   data for the callback.

`device_iter_t fn`
:   function to be called for each device.

**Description**

Iterate over **bus**’s list of devices, and call **fn** for each,
passing it **data**. If **start** is not NULL, we use that device to
begin iterating from.

We check the return of **fn** each time. If it returns anything
other than 0, we break out and return that value.

**NOTE**

The device that returns a non-zero value is not retained
in any way, nor is its refcount incremented. If the caller needs
to retain this data, it should do so, and increment the reference
count in the supplied callback.

struct [device](#c.device "device") \*bus\_find\_device(const struct [bus\_type](#c.bus_type "bus_type") \*bus, struct [device](#c.device "device") \*start, const void \*data, device\_match\_t match)
:   device iterator for locating a particular device.

**Parameters**

`const struct bus_type *bus`
:   bus type

`struct device *start`
:   Device to begin with

`const void *data`
:   Data to pass to match function

`device_match_t match`
:   Callback function to check device

**Description**

This is similar to the [`bus_for_each_dev()`](#c.bus_for_each_dev "bus_for_each_dev") function above, but it
returns a reference to a device that is ‘found’ for later use, as
determined by the **match** callback.

The callback should return 0 if the device doesn’t match and non-zero
if it does. If the callback returns non-zero, this function will
return to the caller and not iterate over any more devices.

int bus\_for\_each\_drv(const struct [bus\_type](#c.bus_type "bus_type") \*bus, struct [device\_driver](#c.device_driver "device_driver") \*start, void \*data, int (\*fn)(struct [device\_driver](#c.device_driver "device_driver")\*, void\*))
:   driver iterator

**Parameters**

`const struct bus_type *bus`
:   bus we’re dealing with.

`struct device_driver *start`
:   driver to start iterating on.

`void *data`
:   data to pass to the callback.

`int (*fn)(struct device_driver *, void *)`
:   function to call for each driver.

**Description**

This is nearly identical to the device iterator above.
We iterate over each driver that belongs to **bus**, and call
**fn** for each. If **fn** returns anything but 0, we break out
and return it. If **start** is not NULL, we use it as the head
of the list.

**NOTE**

we don’t return the driver that returns a non-zero
value, nor do we leave the reference count incremented for that
driver. If the caller needs to know that info, it must set it
in the callback. It must also be sure to increment the refcount
so it doesn’t disappear before returning to the caller.

int bus\_rescan\_devices(const struct [bus\_type](#c.bus_type "bus_type") \*bus)
:   rescan devices on the bus for possible drivers

**Parameters**

`const struct bus_type *bus`
:   the bus to scan.

**Description**

This function will look for devices on the bus with no driver
attached and rescan it against existing drivers to see if it matches
any by calling [`device_attach()`](#c.device_attach "device_attach") for the unbound devices.

int device\_reprobe(struct [device](#c.device "device") \*dev)
:   remove driver for a device and probe for a new driver

**Parameters**

`struct device *dev`
:   the device to reprobe

**Description**

This function detaches the attached driver (if any) for the given
device and restarts the driver probing process. It is intended
to use if probing criteria changed during a devices lifetime and
driver attachment should change accordingly.

int bus\_register(const struct [bus\_type](#c.bus_type "bus_type") \*bus)
:   register a driver-core subsystem

**Parameters**

`const struct bus_type *bus`
:   bus to register

**Description**

Once we have that, we register the bus with the kobject
infrastructure, then register the children subsystems it has:
the devices and drivers that belong to the subsystem.

void bus\_unregister(const struct [bus\_type](#c.bus_type "bus_type") \*bus)
:   remove a bus from the system

**Parameters**

`const struct bus_type *bus`
:   bus.

**Description**

Unregister the child subsystems and the bus itself.
Finally, we call `bus_put()` to release the refcount

int subsys\_system\_register(const struct [bus\_type](#c.bus_type "bus_type") \*subsys, const struct attribute\_group \*\*groups)
:   register a subsystem at /sys/devices/system/

**Parameters**

`const struct bus_type *subsys`
:   system subsystem

`const struct attribute_group **groups`
:   default attributes for the root device

**Description**

All ‘system’ subsystems have a /sys/devices/system/<name> root device
with the name of the subsystem. The root device can carry subsystem-
wide attributes. All registered devices are below this single root
device and are named after the subsystem with a simple enumeration
number appended. The registered devices are not explicitly named;
only ‘id’ in the device needs to be set.

Do not use this interface for anything new, it exists for compatibility
with bad ideas only. New subsystems should use plain subsystems; and
add the subsystem-wide attributes should be added to the subsystem
directory itself and not some create fake root-device placed in
/sys/devices/system/<name>.

int subsys\_virtual\_register(const struct [bus\_type](#c.bus_type "bus_type") \*subsys, const struct attribute\_group \*\*groups)
:   register a subsystem at /sys/devices/virtual/

**Parameters**

`const struct bus_type *subsys`
:   virtual subsystem

`const struct attribute_group **groups`
:   default attributes for the root device

**Description**

All ‘virtual’ subsystems have a /sys/devices/system/<name> root device
with the name of the subsystem. The root device can carry subsystem-wide
attributes. All registered devices are below this single root device.
There’s no restriction on device naming. This is for kernel software
constructs which need sysfs interface.

struct [device\_driver](#c.device_driver "device_driver") \*driver\_find(const char \*name, const struct [bus\_type](#c.bus_type "bus_type") \*bus)
:   locate driver on a bus by its name.

**Parameters**

`const char *name`
:   name of the driver.

`const struct bus_type *bus`
:   bus to scan for the driver.

**Description**

Call [`kset_find_obj()`](basics.html#c.kset_find_obj "kset_find_obj") to iterate over list of drivers on
a bus to find driver by name. Return driver if found.

This routine provides no locking to prevent the driver it returns
from being unregistered or unloaded while the caller is using it.
The caller is responsible for preventing this.

struct [device](#c.device "device") \*bus\_get\_dev\_root(const struct [bus\_type](#c.bus_type "bus_type") \*bus)
:   return a pointer to the “device root” of a bus

**Parameters**

`const struct bus_type *bus`
:   bus to return the device root of.

**Description**

If a bus has a “device root” structure, return it, WITH THE REFERENCE
COUNT INCREMENTED.

Note, when finished with the device, a call to [`put_device()`](#c.put_device "put_device") is required.

If the device root is not present (or bus is not a valid pointer), NULL
will be returned.

## Device Drivers DMA Management

void dmam\_free\_coherent(struct [device](#c.device "device") \*dev, size\_t size, void \*vaddr, dma\_addr\_t dma\_handle)
:   Managed `dma_free_coherent()`

**Parameters**

`struct device *dev`
:   Device to free coherent memory for

`size_t size`
:   Size of allocation

`void *vaddr`
:   Virtual address of the memory to free

`dma_addr_t dma_handle`
:   DMA handle of the memory to free

**Description**

Managed `dma_free_coherent()`.

void \*dmam\_alloc\_attrs(struct [device](#c.device "device") \*dev, size\_t size, dma\_addr\_t \*dma\_handle, gfp\_t gfp, unsigned long attrs)
:   Managed `dma_alloc_attrs()`

**Parameters**

`struct device *dev`
:   Device to allocate non\_coherent memory for

`size_t size`
:   Size of allocation

`dma_addr_t *dma_handle`
:   Out argument for allocated DMA handle

`gfp_t gfp`
:   Allocation flags

`unsigned long attrs`
:   Flags in the DMA\_ATTR\_\* namespace.

**Description**

Managed `dma_alloc_attrs()`. Memory allocated using this function will be
automatically released on driver detach.

**Return**

Pointer to allocated memory on success, NULL on failure.

unsigned int dma\_map\_sg\_attrs(struct [device](#c.device "device") \*dev, struct scatterlist \*sg, int nents, enum dma\_data\_direction dir, unsigned long attrs)
:   Map the given buffer for DMA

**Parameters**

`struct device *dev`
:   The device for which to perform the DMA operation

`struct scatterlist *sg`
:   The sg\_table object describing the buffer

`int nents`
:   Number of entries to map

`enum dma_data_direction dir`
:   DMA direction

`unsigned long attrs`
:   Optional DMA attributes for the map operation

**Description**

Maps a buffer described by a scatterlist passed in the sg argument with
nents segments for the **dir** DMA operation by the **dev** device.

Returns the number of mapped entries (which can be less than nents)
on success. Zero is returned for any error.

`dma_unmap_sg_attrs()` should be used to unmap the buffer with the
original sg and original nents (not the value returned by this funciton).

int dma\_map\_sgtable(struct [device](#c.device "device") \*dev, struct sg\_table \*sgt, enum dma\_data\_direction dir, unsigned long attrs)
:   Map the given buffer for DMA

**Parameters**

`struct device *dev`
:   The device for which to perform the DMA operation

`struct sg_table *sgt`
:   The sg\_table object describing the buffer

`enum dma_data_direction dir`
:   DMA direction

`unsigned long attrs`
:   Optional DMA attributes for the map operation

**Description**

Maps a buffer described by a scatterlist stored in the given sg\_table
object for the **dir** DMA operation by the **dev** device. After success, the
ownership for the buffer is transferred to the DMA domain. One has to
call `dma_sync_sgtable_for_cpu()` or `dma_unmap_sgtable()` to move the
ownership of the buffer back to the CPU domain before touching the
buffer by the CPU.

Returns 0 on success or a negative error code on error. The following
error codes are supported with the given meaning:

> `-EINVAL`
> :   An invalid argument, unaligned access or other error
>     in usage. Will not succeed if retried.
>
> `-ENOMEM`
> :   Insufficient resources (like memory or IOVA space) to
>     complete the mapping. Should succeed if retried later.
>
> `-EIO`
> :   Legacy error code with an unknown meaning. eg. this is
>     returned if a lower level call returned
>     DMA\_MAPPING\_ERROR.
>
> `-EREMOTEIO`
> :   The DMA device cannot access P2PDMA memory specified
>     in the sg\_table. This will not succeed if retried.

bool dma\_need\_unmap(struct [device](#c.device "device") \*dev)
:   does this device need dma\_unmap\_\* operations

**Parameters**

`struct device *dev`
:   device to check

**Description**

If this function returns `false`, drivers can skip calling dma\_unmap\_\* after
finishing an I/O. This function must be called after all mappings that might
need to be unmapped have been performed.

bool dma\_can\_mmap(struct [device](#c.device "device") \*dev)
:   check if a given device supports dma\_mmap\_\*

**Parameters**

`struct device *dev`
:   device to check

**Description**

Returns `true` if **dev** supports `dma_mmap_coherent()` and [`dma_mmap_attrs()`](#c.dma_mmap_attrs "dma_mmap_attrs") to
map DMA allocations to userspace.

int dma\_mmap\_attrs(struct [device](#c.device "device") \*dev, struct vm\_area\_struct \*vma, void \*cpu\_addr, dma\_addr\_t dma\_addr, size\_t size, unsigned long attrs)
:   map a coherent DMA allocation into user space

**Parameters**

`struct device *dev`
:   valid [`struct device`](#c.device "device") pointer, or NULL for ISA and EISA-like devices

`struct vm_area_struct *vma`
:   vm\_area\_struct describing requested user mapping

`void *cpu_addr`
:   kernel CPU-view address returned from dma\_alloc\_attrs

`dma_addr_t dma_addr`
:   device-view address returned from dma\_alloc\_attrs

`size_t size`
:   size of memory originally requested in dma\_alloc\_attrs

`unsigned long attrs`
:   attributes of mapping properties requested in dma\_alloc\_attrs

**Description**

Map a coherent DMA buffer previously allocated by dma\_alloc\_attrs into user
space. The coherent DMA buffer must not be freed by the driver until the
user space mapping has been released.

bool dma\_addressing\_limited(struct [device](#c.device "device") \*dev)
:   return if the device is addressing limited

**Parameters**

`struct device *dev`
:   device to check

**Description**

Return `true` if the devices DMA mask is too small to address all memory in
the system, else `false`. Lack of addressing bits is the prime reason for
bounce buffering, but might not be the only one.

## Device drivers PnP support

int pnp\_register\_protocol(struct pnp\_protocol \*protocol)
:   adds a pnp protocol to the pnp layer

**Parameters**

`struct pnp_protocol *protocol`
:   pointer to the corresponding pnp\_protocol structure

**Description**

> Ex protocols: ISAPNP, PNPBIOS, etc

struct pnp\_dev \*pnp\_request\_card\_device(struct pnp\_card\_link \*clink, const char \*id, struct pnp\_dev \*from)
:   Searches for a PnP device under the specified card

**Parameters**

`struct pnp_card_link *clink`
:   pointer to the card link, cannot be NULL

`const char *id`
:   pointer to a PnP ID structure that explains the rules for finding the device

`struct pnp_dev *from`
:   Starting place to search from. If NULL it will start from the beginning.

void pnp\_release\_card\_device(struct pnp\_dev \*dev)
:   call this when the driver no longer needs the device

**Parameters**

`struct pnp_dev *dev`
:   pointer to the PnP device structure

int pnp\_register\_card\_driver(struct pnp\_card\_driver \*drv)
:   registers a PnP card driver with the PnP Layer

**Parameters**

`struct pnp_card_driver *drv`
:   pointer to the driver to register

void pnp\_unregister\_card\_driver(struct pnp\_card\_driver \*drv)
:   unregisters a PnP card driver from the PnP Layer

**Parameters**

`struct pnp_card_driver *drv`
:   pointer to the driver to unregister

struct pnp\_id \*pnp\_add\_id(struct pnp\_dev \*dev, const char \*id)
:   adds an EISA id to the specified device

**Parameters**

`struct pnp_dev *dev`
:   pointer to the desired device

`const char *id`
:   pointer to an EISA id string

int pnp\_start\_dev(struct pnp\_dev \*dev)
:   low-level start of the PnP device

**Parameters**

`struct pnp_dev *dev`
:   pointer to the desired device

**Description**

assumes that resources have already been allocated

int pnp\_stop\_dev(struct pnp\_dev \*dev)
:   low-level disable of the PnP device

**Parameters**

`struct pnp_dev *dev`
:   pointer to the desired device

**Description**

does not free resources

int pnp\_activate\_dev(struct pnp\_dev \*dev)
:   activates a PnP device for use

**Parameters**

`struct pnp_dev *dev`
:   pointer to the desired device

**Description**

does not validate or set resources so be careful.

int pnp\_disable\_dev(struct pnp\_dev \*dev)
:   disables device

**Parameters**

`struct pnp_dev *dev`
:   pointer to the desired device

**Description**

inform the correct pnp protocol so that resources can be used by other devices

int pnp\_is\_active(struct pnp\_dev \*dev)
:   Determines if a device is active based on its current resources

**Parameters**

`struct pnp_dev *dev`
:   pointer to the desired PnP device

## Userspace IO devices

void uio\_event\_notify(struct [uio\_info](#c.uio_info "uio_info") \*info)
:   trigger an interrupt event

**Parameters**

`struct uio_info *info`
:   UIO device capabilities

int \_\_uio\_register\_device(struct module \*owner, struct [device](#c.device "device") \*parent, struct [uio\_info](#c.uio_info "uio_info") \*info)
:   register a new userspace IO device

**Parameters**

`struct module *owner`
:   module that creates the new device

`struct device *parent`
:   parent device

`struct uio_info *info`
:   UIO device capabilities

**Description**

returns zero on success or a negative error code.

int \_\_devm\_uio\_register\_device(struct module \*owner, struct [device](#c.device "device") \*parent, struct [uio\_info](#c.uio_info "uio_info") \*info)
:   Resource managed [`uio_register_device()`](#c.uio_register_device "uio_register_device")

**Parameters**

`struct module *owner`
:   module that creates the new device

`struct device *parent`
:   parent device

`struct uio_info *info`
:   UIO device capabilities

**Description**

returns zero on success or a negative error code.

void uio\_unregister\_device(struct [uio\_info](#c.uio_info "uio_info") \*info)
:   unregister a industrial IO device

**Parameters**

`struct uio_info *info`
:   UIO device capabilities

struct uio\_mem
:   description of a UIO memory region

**Definition**:

```
struct uio_mem {
    const char              *name;
    phys_addr_t addr;
    dma_addr_t dma_addr;
    unsigned long           offs;
    resource_size_t size;
    int memtype;
    void __iomem            *internal_addr;
    struct device           *dma_device;
    struct uio_map          *map;
};
```

**Members**

`name`
:   name of the memory region for identification

`addr`
:   address of the device’s memory rounded to page
    size (phys\_addr is used since addr can be
    logical, virtual, or physical & phys\_addr\_t
    should always be large enough to handle any of
    the address types)

`dma_addr`
:   DMA handle set by dma\_alloc\_coherent, used with
    UIO\_MEM\_DMA\_COHERENT only (**addr** should be the
    void \* returned from the same dma\_alloc\_coherent call)

`offs`
:   offset of device memory within the page

`size`
:   size of IO (multiple of page size)

`memtype`
:   type of memory addr points to

`internal_addr`
:   ioremap-ped version of addr, for driver internal use

`dma_device`
:   device struct that was passed to dma\_alloc\_coherent,
    used with UIO\_MEM\_DMA\_COHERENT only

`map`
:   for use by the UIO core only.

struct uio\_port
:   description of a UIO port region

**Definition**:

```
struct uio_port {
    const char              *name;
    unsigned long           start;
    unsigned long           size;
    int porttype;
    struct uio_portio       *portio;
};
```

**Members**

`name`
:   name of the port region for identification

`start`
:   start of port region

`size`
:   size of port region

`porttype`
:   type of port (see UIO\_PORT\_\* below)

`portio`
:   for use by the UIO core only.

struct uio\_info
:   UIO device capabilities

**Definition**:

```
struct uio_info {
    struct uio_device       *uio_dev;
    const char              *name;
    const char              *version;
    struct uio_mem          mem[MAX_UIO_MAPS];
    struct uio_port         port[MAX_UIO_PORT_REGIONS];
    long irq;
    unsigned long           irq_flags;
    void *priv;
    irqreturn_t (*handler)(int irq, struct uio_info *dev_info);
    int (*mmap_prepare)(struct uio_info *info, struct vm_area_desc *desc);
    int (*open)(struct uio_info *info, struct inode *inode);
    int (*release)(struct uio_info *info, struct inode *inode);
    int (*irqcontrol)(struct uio_info *info, s32 irq_on);
};
```

**Members**

`uio_dev`
:   the UIO device this info belongs to

`name`
:   device name

`version`
:   device driver version

`mem`
:   list of mappable memory regions, size==0 for end of list

`port`
:   list of port regions, size==0 for end of list

`irq`
:   interrupt number or UIO\_IRQ\_CUSTOM

`irq_flags`
:   flags for [`request_irq()`](../core-api/genericirq.html#c.request_irq "request_irq")

`priv`
:   optional private data

`handler`
:   the device’s irq handler

`mmap_prepare`
:   mmap\_prepare operation for this uio device

`open`
:   open operation for this uio device

`release`
:   release operation for this uio device

`irqcontrol`
:   disable/enable irqs when 0/1 is written to /dev/uioX

uio\_register\_device

`uio_register_device (parent, info)`

> register a new userspace IO device

**Parameters**

`parent`
:   parent device

`info`
:   UIO device capabilities

**Description**

returns zero on success or a negative error code.

devm\_uio\_register\_device

`devm_uio_register_device (parent, info)`

> Resource managed [`uio_register_device()`](#c.uio_register_device "uio_register_device")

**Parameters**

`parent`
:   parent device

`info`
:   UIO device capabilities

**Description**

returns zero on success or a negative error code.
