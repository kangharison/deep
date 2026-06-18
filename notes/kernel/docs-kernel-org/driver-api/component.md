# Component Helper for Aggregate Drivers

> 출처(원문): https://docs.kernel.org/driver-api/component.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Component Helper for Aggregate Drivers

The component helper allows drivers to collect a pile of sub-devices,
including their bound drivers, into an aggregate driver. Various subsystems
already provide functions to get hold of such components, e.g.
`of_clk_get_by_name()`. The component helper can be used when such a
subsystem-specific way to find a device is not available: The component
helper fills the niche of aggregate drivers for specific hardware, where
further standardization into a subsystem would not be practical. The common
example is when a logical device (e.g. a DRM display driver) is spread around
the SoC on various components (scanout engines, blending blocks, transcoders
for various outputs and so on).

The component helper also doesn’t solve runtime dependencies, e.g. for system
suspend and resume operations. See also [device links](device_link.html#device-link).

Components are registered using [`component_add()`](#c.component_add "component_add") and unregistered with
[`component_del()`](#c.component_del "component_del"), usually from the driver’s probe and disconnect functions.

Aggregate drivers first assemble a component match list of what they need
using [`component_match_add()`](#c.component_match_add "component_match_add"). This is then registered as an aggregate driver
using [`component_master_add_with_match()`](#c.component_master_add_with_match "component_master_add_with_match"), and unregistered using
[`component_master_del()`](#c.component_master_del "component_master_del").

## API

struct component\_ops
:   callbacks for component drivers

**Definition**:

```
struct component_ops {
    int (*bind)(struct device *comp, struct device *master, void *master_data);
    void (*unbind)(struct device *comp, struct device *master, void *master_data);
};
```

**Members**

`bind`
:   Called through [`component_bind_all()`](#c.component_bind_all "component_bind_all") when the aggregate driver is
    ready to bind the overall driver.

`unbind`
:   Called through [`component_unbind_all()`](#c.component_unbind_all "component_unbind_all") when the aggregate driver is
    ready to bind the overall driver, or when [`component_bind_all()`](#c.component_bind_all "component_bind_all") fails
    part-ways through and needs to unbind some already bound components.

**Description**

Components are registered with [`component_add()`](#c.component_add "component_add") and unregistered with
[`component_del()`](#c.component_del "component_del").

struct component\_master\_ops
:   callback for the aggregate driver

**Definition**:

```
struct component_master_ops {
    int (*bind)(struct device *master);
    void (*unbind)(struct device *master);
};
```

**Members**

`bind`
:   Called when all components or the aggregate driver, as specified in
    the match list passed to [`component_master_add_with_match()`](#c.component_master_add_with_match "component_master_add_with_match"), are
    ready. Usually there are 3 steps to bind an aggregate driver:

    1. Allocate a structure for the aggregate driver.
    2. Bind all components to the aggregate driver by calling
       [`component_bind_all()`](#c.component_bind_all "component_bind_all") with the aggregate driver structure as opaque
       pointer data.
    3. Register the aggregate driver with the subsystem to publish its
       interfaces.

    Note that the lifetime of the aggregate driver does not align with
    any of the underlying [`struct device`](infrastructure.html#c.device "device") instances. Therefore devm cannot
    be used and all resources acquired or allocated in this callback must
    be explicitly released in the **unbind** callback.

`unbind`
:   Called when either the aggregate driver, using
    [`component_master_del()`](#c.component_master_del "component_master_del"), or one of its components, using
    [`component_del()`](#c.component_del "component_del"), is unregistered.

**Description**

Aggregate drivers are registered with [`component_master_add_with_match()`](#c.component_master_add_with_match "component_master_add_with_match") and
unregistered with [`component_master_del()`](#c.component_master_del "component_master_del").

void component\_match\_add(struct [device](infrastructure.html#c.device "device") \*parent, struct component\_match \*\*matchptr, int (\*compare)(struct [device](infrastructure.html#c.device "device")\*, void\*), void \*compare\_data)
:   add a component match entry

**Parameters**

`struct device *parent`
:   device with the aggregate driver

`struct component_match **matchptr`
:   pointer to the list of component matches

`int (*compare)(struct device *, void *)`
:   compare function to match against all components

`void *compare_data`
:   opaque pointer passed to the **compare** function

**Description**

Adds a new component match to the list stored in **matchptr**, which the **parent**
aggregate driver needs to function. The list of component matches pointed to
by **matchptr** must be initialized to NULL before adding the first match. This
only matches against components added with [`component_add()`](#c.component_add "component_add").

The allocated match list in **matchptr** is automatically released using devm
actions.

See also [`component_match_add_release()`](#c.component_match_add_release "component_match_add_release") and [`component_match_add_typed()`](#c.component_match_add_typed "component_match_add_typed").

int component\_compare\_of(struct [device](infrastructure.html#c.device "device") \*dev, void \*data)
:   A common component compare function for of\_node

**Parameters**

`struct device *dev`
:   component device

`void *data`
:   **compare\_data** from [`component_match_add_release()`](#c.component_match_add_release "component_match_add_release")

**Description**

A common compare function when compare\_data is device of\_node. e.g.
component\_match\_add\_release(masterdev, `match`, component\_release\_of,
component\_compare\_of, component\_dev\_of\_node)

void component\_release\_of(struct [device](infrastructure.html#c.device "device") \*dev, void \*data)
:   A common component release function for of\_node

**Parameters**

`struct device *dev`
:   component device

`void *data`
:   **compare\_data** from [`component_match_add_release()`](#c.component_match_add_release "component_match_add_release")

**Description**

About the example, Please see [`component_compare_of()`](#c.component_compare_of "component_compare_of").

int component\_compare\_dev(struct [device](infrastructure.html#c.device "device") \*dev, void \*data)
:   A common component compare function for dev

**Parameters**

`struct device *dev`
:   component device

`void *data`
:   **compare\_data** from [`component_match_add_release()`](#c.component_match_add_release "component_match_add_release")

**Description**

A common compare function when compare\_data is struce device. e.g.
component\_match\_add(masterdev, `match`, component\_compare\_dev, component\_dev)

int component\_compare\_dev\_name(struct [device](infrastructure.html#c.device "device") \*dev, void \*data)
:   A common component compare function for device name

**Parameters**

`struct device *dev`
:   component device

`void *data`
:   **compare\_data** from [`component_match_add_release()`](#c.component_match_add_release "component_match_add_release")

**Description**

A common compare function when compare\_data is device name string. e.g.
component\_match\_add(masterdev, `match`, component\_compare\_dev\_name,
“component\_dev\_name”)

void component\_match\_add\_release(struct [device](infrastructure.html#c.device "device") \*parent, struct component\_match \*\*matchptr, void (\*release)(struct [device](infrastructure.html#c.device "device")\*, void\*), int (\*compare)(struct [device](infrastructure.html#c.device "device")\*, void\*), void \*compare\_data)
:   add a component match entry with release callback

**Parameters**

`struct device *parent`
:   parent device of the aggregate driver

`struct component_match **matchptr`
:   pointer to the list of component matches

`void (*release)(struct device *, void *)`
:   release function for **compare\_data**

`int (*compare)(struct device *, void *)`
:   compare function to match against all components

`void *compare_data`
:   opaque pointer passed to the **compare** function

**Description**

Adds a new component match to the list stored in **matchptr**, which the
aggregate driver needs to function. The list of component matches pointed to
by **matchptr** must be initialized to NULL before adding the first match. This
only matches against components added with [`component_add()`](#c.component_add "component_add").

The allocated match list in **matchptr** is automatically released using devm
actions, where upon **release** will be called to free any references held by
**compare\_data**, e.g. when **compare\_data** is a `device_node` that must be
released with [`of_node_put()`](../devicetree/kernel-api.html#c.of_node_put "of_node_put").

See also [`component_match_add()`](#c.component_match_add "component_match_add") and [`component_match_add_typed()`](#c.component_match_add_typed "component_match_add_typed").

void component\_match\_add\_typed(struct [device](infrastructure.html#c.device "device") \*parent, struct component\_match \*\*matchptr, int (\*compare\_typed)(struct [device](infrastructure.html#c.device "device")\*, int, void\*), void \*compare\_data)
:   add a component match entry for a typed component

**Parameters**

`struct device *parent`
:   parent device of the aggregate driver

`struct component_match **matchptr`
:   pointer to the list of component matches

`int (*compare_typed)(struct device *, int, void *)`
:   compare function to match against all typed components

`void *compare_data`
:   opaque pointer passed to the **compare** function

**Description**

Adds a new component match to the list stored in **matchptr**, which the
aggregate driver needs to function. The list of component matches pointed to
by **matchptr** must be initialized to NULL before adding the first match. This
only matches against components added with [`component_add_typed()`](#c.component_add_typed "component_add_typed").

The allocated match list in **matchptr** is automatically released using devm
actions.

See also [`component_match_add_release()`](#c.component_match_add_release "component_match_add_release") and [`component_match_add_typed()`](#c.component_match_add_typed "component_match_add_typed").

int component\_master\_add\_with\_match(struct [device](infrastructure.html#c.device "device") \*parent, const struct [component\_master\_ops](#c.component_master_ops "component_master_ops") \*ops, struct component\_match \*match)
:   register an aggregate driver

**Parameters**

`struct device *parent`
:   parent device of the aggregate driver

`const struct component_master_ops *ops`
:   callbacks for the aggregate driver

`struct component_match *match`
:   component match list for the aggregate driver

**Description**

Registers a new aggregate driver consisting of the components added to **match**
by calling one of the [`component_match_add()`](#c.component_match_add "component_match_add") functions. Once all components in
**match** are available, it will be assembled by calling
[`component_master_ops.bind`](#c.component_master_ops "component_master_ops") from **ops**. Must be unregistered by calling
[`component_master_del()`](#c.component_master_del "component_master_del").

void component\_master\_del(struct [device](infrastructure.html#c.device "device") \*parent, const struct [component\_master\_ops](#c.component_master_ops "component_master_ops") \*ops)
:   unregister an aggregate driver

**Parameters**

`struct device *parent`
:   parent device of the aggregate driver

`const struct component_master_ops *ops`
:   callbacks for the aggregate driver

**Description**

Unregisters an aggregate driver registered with
[`component_master_add_with_match()`](#c.component_master_add_with_match "component_master_add_with_match"). If necessary the aggregate driver is first
disassembled by calling [`component_master_ops.unbind`](#c.component_master_ops "component_master_ops") from **ops**.

void component\_unbind\_all(struct [device](infrastructure.html#c.device "device") \*parent, void \*data)
:   unbind all components of an aggregate driver

**Parameters**

`struct device *parent`
:   parent device of the aggregate driver

`void *data`
:   opaque pointer, passed to all components

**Description**

Unbinds all components of the aggregate device by passing **data** to their
[`component_ops.unbind`](#c.component_ops "component_ops") functions. Should be called from
[`component_master_ops.unbind`](#c.component_master_ops "component_master_ops").

int component\_bind\_all(struct [device](infrastructure.html#c.device "device") \*parent, void \*data)
:   bind all components of an aggregate driver

**Parameters**

`struct device *parent`
:   parent device of the aggregate driver

`void *data`
:   opaque pointer, passed to all components

**Description**

Binds all components of the aggregate **dev** by passing **data** to their
[`component_ops.bind`](#c.component_ops "component_ops") functions. Should be called from
[`component_master_ops.bind`](#c.component_master_ops "component_master_ops").

int component\_add\_typed(struct [device](infrastructure.html#c.device "device") \*dev, const struct [component\_ops](#c.component_ops "component_ops") \*ops, int subcomponent)
:   register a component

**Parameters**

`struct device *dev`
:   component device

`const struct component_ops *ops`
:   component callbacks

`int subcomponent`
:   nonzero identifier for subcomponents

**Description**

Register a new component for **dev**. Functions in **ops** will be call when the
aggregate driver is ready to bind the overall driver by calling
[`component_bind_all()`](#c.component_bind_all "component_bind_all"). See also [`struct component_ops`](#c.component_ops "component_ops").

**subcomponent** must be nonzero and is used to differentiate between multiple
components registered on the same device **dev**. These components are match
using [`component_match_add_typed()`](#c.component_match_add_typed "component_match_add_typed").

The component needs to be unregistered at driver unload/disconnect by
calling [`component_del()`](#c.component_del "component_del").

See also [`component_add()`](#c.component_add "component_add").

int component\_add(struct [device](infrastructure.html#c.device "device") \*dev, const struct [component\_ops](#c.component_ops "component_ops") \*ops)
:   register a component

**Parameters**

`struct device *dev`
:   component device

`const struct component_ops *ops`
:   component callbacks

**Description**

Register a new component for **dev**. Functions in **ops** will be called when the
aggregate driver is ready to bind the overall driver by calling
[`component_bind_all()`](#c.component_bind_all "component_bind_all"). See also [`struct component_ops`](#c.component_ops "component_ops").

The component needs to be unregistered at driver unload/disconnect by
calling [`component_del()`](#c.component_del "component_del").

See also [`component_add_typed()`](#c.component_add_typed "component_add_typed") for a variant that allows multiple different
components on the same device.

void component\_del(struct [device](infrastructure.html#c.device "device") \*dev, const struct [component\_ops](#c.component_ops "component_ops") \*ops)
:   unregister a component

**Parameters**

`struct device *dev`
:   component device

`const struct component_ops *ops`
:   component callbacks

**Description**

Unregister a component added with [`component_add()`](#c.component_add "component_add"). If the component is bound
into an aggregate driver, this will force the entire aggregate driver, including
all its components, to be unbound.
