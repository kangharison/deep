# The irq_domain Interrupt Number Mapping Library

> 출처(원문): https://docs.kernel.org/core-api/irq/irq-domain.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# The irq\_domain Interrupt Number Mapping Library

The current design of the Linux kernel uses a single large number
space where each separate IRQ source is assigned a unique number.
This is simple when there is only one interrupt controller. But in
systems with multiple interrupt controllers, the kernel must ensure
that each one gets assigned non-overlapping allocations of Linux
IRQ numbers.

The number of interrupt controllers registered as unique irqchips
shows a rising tendency. For example, subdrivers of different kinds
such as GPIO controllers avoid reimplementing identical callback
mechanisms as the IRQ core system by modelling their interrupt
handlers as irqchips. I.e. in effect cascading interrupt controllers.

So in the past, IRQ numbers could be chosen so that they match the
hardware IRQ line into the root interrupt controller (i.e. the
component actually firing the interrupt line to the CPU). Nowadays,
this number is just a number and the number has no
relationship to hardware interrupt numbers.

For this reason, we need a mechanism to separate controller-local
interrupt numbers, called hardware IRQs, from Linux IRQ numbers.

The irq\_alloc\_desc\*() and irq\_free\_desc\*() APIs provide allocation of
IRQ numbers, but they don’t provide any support for reverse mapping of
the controller-local IRQ (hwirq) number into the Linux IRQ number
space.

The irq\_domain library adds a mapping between hwirq and IRQ numbers on
top of the irq\_alloc\_desc\*() API. An irq\_domain to manage the mapping
is preferred over interrupt controller drivers open coding their own
reverse mapping scheme.

irq\_domain also implements a translation from an abstract [`struct
irq_fwspec`](#c.irq_fwspec "irq_fwspec") to hwirq numbers (Device Tree, non-DT firmware node, ACPI
GSI, and software node so far), and can be easily extended to support
other IRQ topology data sources. The implementation is performed
without any extra platform support code.

## irq\_domain Usage

[`struct irq_domain`](#c.irq_domain "irq_domain") could be defined as an irq domain controller. That
is, it handles the mapping between hardware and virtual interrupt
numbers for a given interrupt domain. The domain structure is
generally created by the PIC code for a given PIC instance (though a
domain can cover more than one PIC if they have a flat number model).
It is the domain callbacks that are responsible for setting the
irq\_chip on a given irq\_desc after it has been mapped.

The host code and data structures use a fwnode\_handle pointer to
identify the domain. In some cases, and in order to preserve source
code compatibility, this fwnode pointer is “upgraded” to a DT
device\_node. For those firmware infrastructures that do not provide a
unique identifier for an interrupt controller, the irq\_domain code
offers a fwnode allocator.

An interrupt controller driver creates and registers a [`struct irq_domain`](#c.irq_domain "irq_domain")
by calling one of the irq\_domain\_create\_\*() functions (each mapping
method has a different allocator function, more on that later). The
function will return a pointer to the [`struct irq_domain`](#c.irq_domain "irq_domain") on success. The
caller must provide the allocator function with a [`struct irq_domain_ops`](#c.irq_domain_ops "irq_domain_ops")
pointer.

In most cases, the irq\_domain will begin empty without any mappings
between hwirq and IRQ numbers. Mappings are added to the irq\_domain
by calling [`irq_create_mapping()`](#c.irq_create_mapping "irq_create_mapping") which accepts the irq\_domain and a
hwirq number as arguments. If a mapping for the hwirq doesn’t already
exist, [`irq_create_mapping()`](#c.irq_create_mapping "irq_create_mapping") allocates a new Linux irq\_desc, associates
it with the hwirq, and calls the `irq_domain_ops.map()`
callback. In there, the driver can perform any required hardware
setup.

Once a mapping has been established, it can be retrieved or used via a
variety of methods:

* [`irq_resolve_mapping()`](#c.irq_resolve_mapping "irq_resolve_mapping") returns a pointer to the irq\_desc structure
  for a given domain and hwirq number, or NULL if there was no
  mapping.
* [`irq_find_mapping()`](#c.irq_find_mapping "irq_find_mapping") returns a Linux IRQ number for a given domain and
  hwirq number, or 0 if there was no mapping
* [`generic_handle_domain_irq()`](../genericirq.html#c.generic_handle_domain_irq "generic_handle_domain_irq") handles an interrupt described by a
  domain and a hwirq number

Note that irq\_domain lookups must happen in contexts that are
compatible with an RCU read-side critical section.

The [`irq_create_mapping()`](#c.irq_create_mapping "irq_create_mapping") function must be called *at least once*
before any call to [`irq_find_mapping()`](#c.irq_find_mapping "irq_find_mapping"), lest the descriptor will not
be allocated.

If the driver has the Linux IRQ number or the irq\_data pointer, and
needs to know the associated hwirq number (such as in the irq\_chip
callbacks) then it can be directly obtained from
`irq_data.hwirq`.

## Types of irq\_domain Mappings

There are several mechanisms available for reverse mapping from hwirq
to Linux IRQ, and each mechanism uses a different allocation function.
Which reverse map type should be used depends on the use case. Each
of the reverse map types are described below:

### Linear

```
irq_domain_create_linear()
```

The linear reverse map maintains a fixed-size table indexed by the
hwirq number. When a hwirq is mapped, an irq\_desc is allocated for
the hwirq, and the IRQ number is stored in the table.

The Linear map is a good choice when the maximum number of hwirqs is
fixed and a relatively small number (~ < 256). The advantages of this
map are fixed-time lookup for IRQ numbers, and irq\_descs are only
allocated for in-use IRQs. The disadvantage is that the table must be
as large as the largest possible hwirq number.

The majority of drivers should use the Linear map.

### Tree

```
irq_domain_create_tree()
```

The irq\_domain maintains a radix tree map from hwirq numbers to Linux
IRQs. When an hwirq is mapped, an irq\_desc is allocated and the
hwirq is used as the lookup key for the radix tree.

The Tree map is a good choice if the hwirq number can be very large
since it doesn’t need to allocate a table as large as the largest
hwirq number. The disadvantage is that hwirq to IRQ number lookup is
dependent on how many entries are in the table.

Very few drivers should need this mapping.

### No Map

```
irq_domain_create_nomap()
```

The No Map mapping is to be used when the hwirq number is
programmable in the hardware. In this case it is best to program the
Linux IRQ number into the hardware itself so that no mapping is
required. Calling [`irq_create_direct_mapping()`](#c.irq_create_direct_mapping "irq_create_direct_mapping") will allocate a Linux
IRQ number and call the .`map()` callback so that driver can program the
Linux IRQ number into the hardware.

Most drivers cannot use this mapping, and it is now gated on the
CONFIG\_IRQ\_DOMAIN\_NOMAP option. Please refrain from introducing new
users of this API.

### Legacy

```
irq_domain_create_simple()
irq_domain_create_legacy()
```

The Legacy mapping is a special case for drivers that already have a
range of irq\_descs allocated for the hwirqs. It is used when the
driver cannot be immediately converted to use the Linear mapping. For
example, many embedded system board support files use a set of #defines
for IRQ numbers that are passed to [`struct device`](../../driver-api/infrastructure.html#c.device "device") registrations. In that
case the Linux IRQ numbers cannot be dynamically assigned and the Legacy
mapping should be used.

As the name implies, the \*`_legacy()` functions are deprecated and only
exist to ease the support of ancient platforms. No new users should be
added. Same goes for the \*`_simple()` functions when their use results
in the legacy behaviour.

The Legacy map assumes a contiguous range of IRQ numbers has already
been allocated for the controller and that the IRQ number can be
calculated by adding a fixed offset to the hwirq number, and
visa-versa. The disadvantage is that it requires the interrupt
controller to manage IRQ allocations and it requires an irq\_desc to be
allocated for every hwirq, even if it is unused.

The Legacy map should only be used if fixed IRQ mappings must be
supported. For example, ISA controllers would use the Legacy map for
mapping Linux IRQs 0-15 so that existing ISA drivers get the correct IRQ
numbers.

Most users of legacy mappings should use [`irq_domain_create_simple()`](#c.irq_domain_create_simple "irq_domain_create_simple")
which will use a legacy domain only if an IRQ range is supplied by the
system and will otherwise use a linear domain mapping. The semantics of
this call are such that if an IRQ range is specified then descriptors
will be allocated on-the-fly for it, and if no range is specified it
will fall through to [`irq_domain_create_linear()`](#c.irq_domain_create_linear "irq_domain_create_linear") which means *no* IRQ
descriptors will be allocated.

A typical use case for simple domains is where an irqchip provider
is supporting both dynamic and static IRQ assignments.

In order to avoid ending up in a situation where a linear domain is
used and no descriptor gets allocated it is very important to make sure
that the driver using the simple domain call [`irq_create_mapping()`](#c.irq_create_mapping "irq_create_mapping")
before any [`irq_find_mapping()`](#c.irq_find_mapping "irq_find_mapping") since the latter will actually work
for the static IRQ assignment case.

### Hierarchy IRQ Domain

On some architectures, there may be multiple interrupt controllers
involved in delivering an interrupt from the device to the target CPU.
Let’s look at a typical interrupt delivery path on x86 platforms:

```
Device --> IOAPIC -> Interrupt remapping Controller -> Local APIC -> CPU
```

There are three interrupt controllers involved:

1. IOAPIC controller
2. Interrupt remapping controller
3. Local APIC controller

To support such a hardware topology and make software architecture match
hardware architecture, an irq\_domain data structure is built for each
interrupt controller and those irq\_domains are organized into hierarchy.
When building irq\_domain hierarchy, the irq\_domain nearest the device is
child and the irq\_domain nearest the CPU is parent. So a hierarchy structure
as below will be built for the example above:

```
CPU Vector irq_domain (root irq_domain to manage CPU vectors)
        ^
        |
Interrupt Remapping irq_domain (manage irq_remapping entries)
        ^
        |
IOAPIC irq_domain (manage IOAPIC delivery entries/pins)
```

There are four major interfaces to use hierarchy irq\_domain:

1. [`irq_domain_alloc_irqs()`](#c.irq_domain_alloc_irqs "irq_domain_alloc_irqs"): allocate IRQ descriptors and interrupt
   controller related resources to deliver these interrupts.
2. [`irq_domain_free_irqs()`](#c.irq_domain_free_irqs "irq_domain_free_irqs"): free IRQ descriptors and interrupt controller
   related resources associated with these interrupts.
3. [`irq_domain_activate_irq()`](#c.irq_domain_activate_irq "irq_domain_activate_irq"): activate interrupt controller hardware to
   deliver the interrupt.
4. [`irq_domain_deactivate_irq()`](#c.irq_domain_deactivate_irq "irq_domain_deactivate_irq"): deactivate interrupt controller hardware
   to stop delivering the interrupt.

The following is needed to support hierarchy irq\_domain:

1. The `parent` field in [`struct irq_domain`](#c.irq_domain "irq_domain") is used to
   maintain irq\_domain hierarchy information.
2. The `parent_data` field in [`struct irq_data`](../genericirq.html#c.irq_data "irq_data") is used to
   build hierarchy irq\_data to match hierarchy irq\_domains. The
   irq\_data is used to store irq\_domain pointer and hardware irq
   number.
3. The `alloc()`, `free()`, and other callbacks in
   [`struct irq_domain_ops`](#c.irq_domain_ops "irq_domain_ops") to support hierarchy irq\_domain operations.

With the support of hierarchy irq\_domain and hierarchy irq\_data ready,
an irq\_domain structure is built for each interrupt controller, and an
irq\_data structure is allocated for each irq\_domain associated with an
IRQ.

For an interrupt controller driver to support hierarchy irq\_domain, it
needs to:

1. Implement irq\_domain\_ops.`alloc()` and irq\_domain\_ops.`free()`
2. Optionally, implement irq\_domain\_ops.`activate()` and
   irq\_domain\_ops.`deactivate()`.
3. Optionally, implement an irq\_chip to manage the interrupt controller
   hardware.
4. There is no need to implement irq\_domain\_ops.`map()` and
   irq\_domain\_ops.`unmap()`. They are unused with hierarchy irq\_domain.

Note the hierarchy irq\_domain is in no way x86-specific, and is
heavily used to support other architectures, such as ARM, ARM64 etc.

#### Stacked irq\_chip

Now, we could go one step further to support stacked (hierarchy)
irq\_chip. That is, an irq\_chip is associated with each irq\_data along
the hierarchy. A child irq\_chip may implement a required action by
itself or by cooperating with its parent irq\_chip.

With stacked irq\_chip, interrupt controller driver only needs to deal
with the hardware managed by itself and may ask for services from its
parent irq\_chip when needed. So we could achieve a much cleaner
software architecture.

## Debugging

Most of the internals of the IRQ subsystem are exposed in debugfs by
turning CONFIG\_GENERIC\_IRQ\_DEBUGFS on.

## Structures and Public Functions Provided

This chapter contains the autogenerated documentation of the structures
and exported kernel API functions which are used for IRQ domains.

struct irq\_fwspec
:   generic IRQ specifier structure

**Definition**:

```
struct irq_fwspec {
    struct fwnode_handle    *fwnode;
    int param_count;
    u32 param[IRQ_DOMAIN_IRQ_SPEC_PARAMS];
};
```

**Members**

`fwnode`
:   Pointer to a firmware-specific descriptor

`param_count`
:   Number of device-specific parameters

`param`
:   Device-specific parameters

**Description**

This structure, directly modeled after of\_phandle\_args, is used to
pass a device-specific description of an interrupt.

struct irq\_fwspec\_info
:   firmware provided IRQ information structure

**Definition**:

```
struct irq_fwspec_info {
    unsigned long           flags;
    const struct cpumask    *affinity;
};
```

**Members**

`flags`
:   Information validity flags

`affinity`
:   Affinity mask for this interrupt

**Description**

This structure reports firmware-specific information about an
interrupt. The only significant information is the affinity of a
per-CPU interrupt, but this is designed to be extended as required.

struct irq\_domain\_ops
:   Methods for irq\_domain objects

**Definition**:

```
struct irq_domain_ops {
    int (*match)(struct irq_domain *d, struct device_node *node, enum irq_domain_bus_token bus_token);
    int (*select)(struct irq_domain *d, struct irq_fwspec *fwspec, enum irq_domain_bus_token bus_token);
    int (*map)(struct irq_domain *d, unsigned int virq, irq_hw_number_t hw);
    void (*unmap)(struct irq_domain *d, unsigned int virq);
    int (*xlate)(struct irq_domain *d, struct device_node *node, const u32 *intspec, unsigned int intsize, unsigned long *out_hwirq, unsigned int *out_type);
#ifdef CONFIG_IRQ_DOMAIN_HIERARCHY;
    int (*alloc)(struct irq_domain *d, unsigned int virq, unsigned int nr_irqs, void *arg);
    void (*free)(struct irq_domain *d, unsigned int virq, unsigned int nr_irqs);
    int (*activate)(struct irq_domain *d, struct irq_data *irqd, bool reserve);
    void (*deactivate)(struct irq_domain *d, struct irq_data *irq_data);
    int (*translate)(struct irq_domain *d, struct irq_fwspec *fwspec, unsigned long *out_hwirq, unsigned int *out_type);
    int (*get_fwspec_info)(struct irq_fwspec *fwspec, struct irq_fwspec_info *info);
#endif;
#ifdef CONFIG_GENERIC_IRQ_DEBUGFS;
    void (*debug_show)(struct seq_file *m, struct irq_domain *d, struct irq_data *irqd, int ind);
#endif;
};
```

**Members**

`match`
:   Match an interrupt controller device node to a domain, returns
    1 on a match

`select`
:   Match an interrupt controller fw specification. It is more generic
    than **match** as it receives a complete [`struct irq_fwspec`](#c.irq_fwspec "irq_fwspec"). Therefore,
    **select** is preferred if provided. Returns 1 on a match.

`map`
:   Create or update a mapping between a virtual irq number and a hw
    irq number. This is called only once for a given mapping.

`unmap`
:   Dispose of such a mapping

`xlate`
:   Given a device tree node and interrupt specifier, decode
    the hardware irq number and linux irq type value.

`alloc`
:   Allocate **nr\_irqs** interrupts starting from **virq**.

`free`
:   Free **nr\_irqs** interrupts starting from **virq**.

`activate`
:   Activate one interrupt in HW (**irqd**). If **reserve** is set, only
    reserve the vector. If unset, assign the vector (called from
    [`request_irq()`](../genericirq.html#c.request_irq "request_irq")).

`deactivate`
:   Disarm one interrupt (**irqd**).

`translate`
:   Given **fwspec**, decode the hardware irq number (**out\_hwirq**) and
    linux irq type value (**out\_type**). This is a generalised **xlate**
    (over [`struct irq_fwspec`](#c.irq_fwspec "irq_fwspec")) and is preferred if provided.

`get_fwspec_info`
:   Given **fwspec**, report additional firmware-provided information in
    **info**. Optional.

`debug_show`
:   For domains to show specific data for an interrupt in debugfs.

**Description**

Functions below are provided by the driver and called whenever a new mapping
is created or an old mapping is disposed. The driver can then proceed to
whatever internal data structures management is required. It also needs
to setup the irq\_desc when returning from `map()`.

struct irq\_domain
:   Hardware interrupt number translation object

**Definition**:

```
struct irq_domain {
    struct list_head                link;
    const char                      *name;
    const struct irq_domain_ops     *ops;
    void *host_data;
    unsigned int                    flags;
    unsigned int                    mapcount;
    struct mutex                    mutex;
    struct irq_domain               *root;
    struct fwnode_handle            *fwnode;
    enum irq_domain_bus_token       bus_token;
    struct irq_domain_chip_generic  *gc;
    struct device                   *dev;
    struct device                   *pm_dev;
#ifdef CONFIG_IRQ_DOMAIN_HIERARCHY;
    struct irq_domain               *parent;
#endif;
#ifdef CONFIG_GENERIC_MSI_IRQ;
    const struct msi_parent_ops     *msi_parent_ops;
#endif;
    void (*exit)(struct irq_domain *d);
    irq_hw_number_t hwirq_max;
    unsigned int                    revmap_size;
    struct radix_tree_root          revmap_tree;
    struct irq_data *revmap[];
};
```

**Members**

`link`
:   Element in global irq\_domain list.

`name`
:   Name of interrupt domain

`ops`
:   Pointer to irq\_domain methods

`host_data`
:   Private data pointer for use by owner. Not touched by irq\_domain
    core code.

`flags`
:   Per irq\_domain flags

`mapcount`
:   The number of mapped interrupts

`mutex`
:   Domain lock, hierarchical domains use root domain’s lock

`root`
:   Pointer to root domain, or containing structure if non-hierarchical

`fwnode`
:   Pointer to firmware node associated with the irq\_domain. Pretty easy
    to swap it for the of\_node via the irq\_domain\_get\_of\_node accessor

`bus_token`
:   **fwnode**’s device\_node might be used for several irq domains. But
    in connection with **bus\_token**, the pair shall be unique in a
    system.

`gc`
:   Pointer to a list of generic chips. There is a helper function for
    setting up one or more generic chips for interrupt controllers
    drivers using the generic chip library which uses this pointer.

`dev`
:   Pointer to the device which instantiated the irqdomain
    With per device irq domains this is not necessarily the same
    as **pm\_dev**.

`pm_dev`
:   Pointer to a device that can be utilized for power management
    purposes related to the irq domain.

`parent`
:   Pointer to parent irq\_domain to support hierarchy irq\_domains

`msi_parent_ops`
:   Pointer to MSI parent domain methods for per device domain init

`exit`
:   Function called when the domain is destroyed

`hwirq_max`
:   Top limit for the HW irq number. Especially to avoid
    conflicts/failures with reserved HW irqs. Can be ~0.

`revmap_size`
:   Size of the linear map table **revmap**

`revmap_tree`
:   Radix map tree for hwirqs that don’t fit in the linear map

`revmap`
:   Linear table of irq\_data pointers

**Description**

Optional elements:

Revmap data, used internally by the irq domain code:

struct irq\_domain\_info
:   Domain information structure

**Definition**:

```
struct irq_domain_info {
    struct fwnode_handle                    *fwnode;
    unsigned int                            domain_flags;
    unsigned int                            size;
    irq_hw_number_t hwirq_max;
    int direct_max;
    unsigned int                            hwirq_base;
    unsigned int                            virq_base;
    enum irq_domain_bus_token               bus_token;
    const char                              *name_suffix;
    const struct irq_domain_ops             *ops;
    void *host_data;
    struct device                           *dev;
#ifdef CONFIG_IRQ_DOMAIN_HIERARCHY;
    struct irq_domain                       *parent;
#endif;
    struct irq_domain_chip_generic_info     *dgc_info;
    int (*init)(struct irq_domain *d);
    void (*exit)(struct irq_domain *d);
};
```

**Members**

`fwnode`
:   firmware node for the interrupt controller

`domain_flags`
:   Additional flags to add to the domain flags

`size`
:   Size of linear map; 0 for radix mapping only

`hwirq_max`
:   Maximum number of interrupts supported by controller

`direct_max`
:   Maximum value of direct maps;
    Use ~0 for no limit; 0 for no direct mapping

`hwirq_base`
:   The first hardware interrupt number (legacy domains only)

`virq_base`
:   The first Linux interrupt number for legacy domains to
    immediately associate the interrupts after domain creation

`bus_token`
:   Domain bus token

`name_suffix`
:   Optional name suffix to avoid collisions when multiple
    domains are added using same fwnode

`ops`
:   Domain operation callbacks

`host_data`
:   Controller private data pointer

`dev`
:   Device which creates the domain

`parent`
:   Pointer to the parent irq domain used in a hierarchy domain

`dgc_info`
:   Geneneric chip information structure pointer used to
    create generic chips for the domain if not NULL.

`init`
:   Function called when the domain is created.
    Allow to do some additional domain initialisation.

`exit`
:   Function called when the domain is destroyed.
    Allow to do some additional cleanup operation.

struct [irq\_domain](#c.irq_domain "irq_domain") \*irq\_domain\_create\_linear(struct fwnode\_handle \*fwnode, unsigned int size, const struct [irq\_domain\_ops](#c.irq_domain_ops "irq_domain_ops") \*ops, void \*host\_data)
:   Allocate and register a linear revmap irq\_domain.

**Parameters**

`struct fwnode_handle *fwnode`
:   pointer to interrupt controller’s FW node.

`unsigned int size`
:   Number of interrupts in the domain.

`const struct irq_domain_ops *ops`
:   map/unmap domain callbacks

`void *host_data`
:   Controller private data pointer

**Return**

Newly created irq\_domain

unsigned int irq\_create\_mapping(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain, irq\_hw\_number\_t hwirq)
:   Map a hardware interrupt into linux irq space

**Parameters**

`struct irq_domain *domain`
:   domain owning this hardware interrupt or NULL for default domain

`irq_hw_number_t hwirq`
:   hardware irq number in that domain space

**Description**

Only one mapping per hardware interrupt is permitted.

If the sense/trigger is to be specified, `set_irq_type()` should be called
on the number returned from that call.

**Return**

Linux irq number or 0 on error

struct irq\_desc \*irq\_resolve\_mapping(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain, irq\_hw\_number\_t hwirq)
:   Find a linux irq from a hw irq number.

**Parameters**

`struct irq_domain *domain`
:   domain owning this hardware interrupt

`irq_hw_number_t hwirq`
:   hardware irq number in that domain space

**Return**

Interrupt descriptor

unsigned int irq\_find\_mapping(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain, irq\_hw\_number\_t hwirq)
:   Find a linux irq from a hw irq number.

**Parameters**

`struct irq_domain *domain`
:   domain owning this hardware interrupt

`irq_hw_number_t hwirq`
:   hardware irq number in that domain space

**Return**

Linux irq number or 0 if not found

struct [irq\_domain](#c.irq_domain "irq_domain") \*irq\_domain\_create\_hierarchy(struct [irq\_domain](#c.irq_domain "irq_domain") \*parent, unsigned int flags, unsigned int size, struct fwnode\_handle \*fwnode, const struct [irq\_domain\_ops](#c.irq_domain_ops "irq_domain_ops") \*ops, void \*host\_data)
:   Add a irqdomain into the hierarchy

**Parameters**

`struct irq_domain *parent`
:   Parent irq domain to associate with the new domain

`unsigned int flags`
:   Irq domain flags associated to the domain

`unsigned int size`
:   Size of the domain. See below

`struct fwnode_handle *fwnode`
:   Optional fwnode of the interrupt controller

`const struct irq_domain_ops *ops`
:   Pointer to the interrupt domain callbacks

`void *host_data`
:   Controller private data pointer

**Description**

If **size** is 0 a tree domain is created, otherwise a linear domain.

If successful the parent is associated to the new domain and the
domain flags are set.

**Return**

A pointer to IRQ domain, or `NULL` on failure.

int irq\_domain\_alloc\_irqs(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain, unsigned int nr\_irqs, int node, void \*arg)
:   Allocate IRQs from domain

**Parameters**

`struct irq_domain *domain`
:   domain to allocate from

`unsigned int nr_irqs`
:   number of IRQs to allocate

`int node`
:   NUMA node id for memory allocation

`void *arg`
:   domain specific argument

**Description**

See [`__irq_domain_alloc_irqs()`](#c.__irq_domain_alloc_irqs "__irq_domain_alloc_irqs")’ documentation.

struct fwnode\_handle \*\_\_irq\_domain\_alloc\_fwnode(unsigned int type, int id, const char \*name, phys\_addr\_t \*pa, struct fwnode\_handle \*parent)
:   Allocate a fwnode\_handle suitable for identifying an irq domain

**Parameters**

`unsigned int type`
:   Type of irqchip\_fwnode. See linux/irqdomain.h

`int id`
:   Optional user provided id if name != NULL

`const char *name`
:   Optional user provided domain name

`phys_addr_t *pa`
:   Optional user-provided physical address

`struct fwnode_handle *parent`
:   Optional parent fwnode\_handle

**Description**

Allocate a `struct irqchip_fwid`, and return a pointer to the embedded
fwnode\_handle (or NULL on failure).

**Note**

The types IRQCHIP\_FWNODE\_NAMED and IRQCHIP\_FWNODE\_NAMED\_ID are
solely to transport name information to irqdomain creation code. The
node is not stored. For other types the pointer is kept in the irq
domain struct.

void irq\_domain\_free\_fwnode(struct fwnode\_handle \*fwnode)
:   Free a non-OF-backed fwnode\_handle

**Parameters**

`struct fwnode_handle *fwnode`
:   fwnode\_handle to free

**Description**

Free a fwnode\_handle allocated with irq\_domain\_alloc\_fwnode.

struct [irq\_domain](#c.irq_domain "irq_domain") \*irq\_domain\_instantiate(const struct [irq\_domain\_info](#c.irq_domain_info "irq_domain_info") \*info)
:   Instantiate a new irq domain data structure

**Parameters**

`const struct irq_domain_info *info`
:   Domain information pointer pointing to the information for this domain

**Return**

A pointer to the instantiated irq domain or an ERR\_PTR value.

void irq\_domain\_remove(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain)
:   Remove an irq domain.

**Parameters**

`struct irq_domain *domain`
:   domain to remove

**Description**

This routine is used to remove an irq domain. The caller must ensure
that all mappings within the domain have been disposed of prior to
use, depending on the revmap type.

struct [irq\_domain](#c.irq_domain "irq_domain") \*irq\_domain\_create\_simple(struct fwnode\_handle \*fwnode, unsigned int size, unsigned int first\_irq, const struct [irq\_domain\_ops](#c.irq_domain_ops "irq_domain_ops") \*ops, void \*host\_data)
:   Register an irq\_domain and optionally map a range of irqs

**Parameters**

`struct fwnode_handle *fwnode`
:   firmware node for the interrupt controller

`unsigned int size`
:   total number of irqs in mapping

`unsigned int first_irq`
:   first number of irq block assigned to the domain,
    pass zero to assign irqs on-the-fly. If first\_irq is non-zero, then
    pre-map all of the irqs in the domain to virqs starting at first\_irq.

`const struct irq_domain_ops *ops`
:   domain callbacks

`void *host_data`
:   Controller private data pointer

**Description**

Allocates an irq\_domain, and optionally if first\_irq is positive then also
allocate irq\_descs and map all of the hwirqs to virqs starting at first\_irq.

This is intended to implement the expected behaviour for most
interrupt controllers. If device tree is used, then first\_irq will be 0 and
irqs get mapped dynamically on the fly. However, if the controller requires
static virq assignments (non-DT boot) then it will set that up correctly.

struct [irq\_domain](#c.irq_domain "irq_domain") \*irq\_find\_matching\_fwspec(struct [irq\_fwspec](#c.irq_fwspec "irq_fwspec") \*fwspec, enum irq\_domain\_bus\_token bus\_token)
:   Locates a domain for a given fwspec

**Parameters**

`struct irq_fwspec *fwspec`
:   FW specifier for an interrupt

`enum irq_domain_bus_token bus_token`
:   domain-specific data

void irq\_set\_default\_domain(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain)
:   Set a “default” irq domain

**Parameters**

`struct irq_domain *domain`
:   default domain pointer

**Description**

For convenience, it’s possible to set a “default” domain that will be used
whenever NULL is passed to [`irq_create_mapping()`](#c.irq_create_mapping "irq_create_mapping"). It makes life easier for
platforms that want to manipulate a few hard coded interrupt numbers that
aren’t properly represented in the device-tree.

struct [irq\_domain](#c.irq_domain "irq_domain") \*irq\_get\_default\_domain(void)
:   Retrieve the “default” irq domain

**Parameters**

`void`
:   no arguments

**Return**

the default domain, if any.

**Description**

Modern code should never use this. This should only be used on
systems that cannot implement a firmware->fwnode mapping (which
both DT and ACPI provide).

unsigned int irq\_create\_direct\_mapping(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain)
:   Allocate an irq for direct mapping

**Parameters**

`struct irq_domain *domain`
:   domain to allocate the irq for or NULL for default domain

**Description**

This routine is used for irq controllers which can choose the hardware
interrupt numbers they generate. In such a case it’s simplest to use
the linux irq as the hardware interrupt number. It still uses the linear
or radix tree to store the mapping, but the irq controller can optimize
the revmap path by using the hwirq directly.

unsigned int irq\_create\_mapping\_affinity(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain, irq\_hw\_number\_t hwirq, const struct [irq\_affinity\_desc](../genericirq.html#c.irq_affinity_desc "irq_affinity_desc") \*affinity)
:   Map a hardware interrupt into linux irq space

**Parameters**

`struct irq_domain *domain`
:   domain owning this hardware interrupt or NULL for default domain

`irq_hw_number_t hwirq`
:   hardware irq number in that domain space

`const struct irq_affinity_desc *affinity`
:   irq affinity

**Description**

Only one mapping per hardware interrupt is permitted. Returns a linux
irq number.
If the sense/trigger is to be specified, `set_irq_type()` should be called
on the number returned from that call.

void irq\_dispose\_mapping(unsigned int virq)
:   Unmap an interrupt

**Parameters**

`unsigned int virq`
:   linux irq number of the interrupt to unmap

struct irq\_desc \*\_\_irq\_resolve\_mapping(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain, irq\_hw\_number\_t hwirq, unsigned int \*irq)
:   Find a linux irq from a hw irq number.

**Parameters**

`struct irq_domain *domain`
:   domain owning this hardware interrupt

`irq_hw_number_t hwirq`
:   hardware irq number in that domain space

`unsigned int *irq`
:   optional pointer to return the Linux irq if required

**Description**

Returns the interrupt descriptor.

int irq\_domain\_xlate\_onecell(struct [irq\_domain](#c.irq_domain "irq_domain") \*d, struct device\_node \*ctrlr, const u32 \*intspec, unsigned int intsize, unsigned long \*out\_hwirq, unsigned int \*out\_type)
:   Generic xlate for direct one cell bindings

**Parameters**

`struct irq_domain *d`
:   Interrupt domain involved in the translation

`struct device_node *ctrlr`
:   The device tree node for the device whose interrupt is translated

`const u32 *intspec`
:   The interrupt specifier data from the device tree

`unsigned int intsize`
:   The number of entries in **intspec**

`unsigned long *out_hwirq`
:   Pointer to storage for the hardware interrupt number

`unsigned int *out_type`
:   Pointer to storage for the interrupt type

**Description**

Device Tree IRQ specifier translation function which works with one cell
bindings where the cell value maps directly to the hwirq number.

int irq\_domain\_xlate\_twocell(struct [irq\_domain](#c.irq_domain "irq_domain") \*d, struct device\_node \*ctrlr, const u32 \*intspec, unsigned int intsize, irq\_hw\_number\_t \*out\_hwirq, unsigned int \*out\_type)
:   Generic xlate for direct two cell bindings

**Parameters**

`struct irq_domain *d`
:   Interrupt domain involved in the translation

`struct device_node *ctrlr`
:   The device tree node for the device whose interrupt is translated

`const u32 *intspec`
:   The interrupt specifier data from the device tree

`unsigned int intsize`
:   The number of entries in **intspec**

`irq_hw_number_t *out_hwirq`
:   Pointer to storage for the hardware interrupt number

`unsigned int *out_type`
:   Pointer to storage for the interrupt type

**Description**

Device Tree IRQ specifier translation function which works with two cell
bindings where the cell values map directly to the hwirq number
and linux irq flags.

int irq\_domain\_xlate\_twothreecell(struct [irq\_domain](#c.irq_domain "irq_domain") \*d, struct device\_node \*ctrlr, const u32 \*intspec, unsigned int intsize, irq\_hw\_number\_t \*out\_hwirq, unsigned int \*out\_type)
:   Generic xlate for direct two or three cell bindings

**Parameters**

`struct irq_domain *d`
:   Interrupt domain involved in the translation

`struct device_node *ctrlr`
:   The device tree node for the device whose interrupt is translated

`const u32 *intspec`
:   The interrupt specifier data from the device tree

`unsigned int intsize`
:   The number of entries in **intspec**

`irq_hw_number_t *out_hwirq`
:   Pointer to storage for the hardware interrupt number

`unsigned int *out_type`
:   Pointer to storage for the interrupt type

**Description**

Device Tree interrupt specifier translation function for two or three
cell bindings, where the cell values map directly to the hardware
interrupt number and the type specifier.

int irq\_domain\_xlate\_onetwocell(struct [irq\_domain](#c.irq_domain "irq_domain") \*d, struct device\_node \*ctrlr, const u32 \*intspec, unsigned int intsize, unsigned long \*out\_hwirq, unsigned int \*out\_type)
:   Generic xlate for one or two cell bindings

**Parameters**

`struct irq_domain *d`
:   Interrupt domain involved in the translation

`struct device_node *ctrlr`
:   The device tree node for the device whose interrupt is translated

`const u32 *intspec`
:   The interrupt specifier data from the device tree

`unsigned int intsize`
:   The number of entries in **intspec**

`unsigned long *out_hwirq`
:   Pointer to storage for the hardware interrupt number

`unsigned int *out_type`
:   Pointer to storage for the interrupt type

**Description**

Device Tree IRQ specifier translation function which works with either one
or two cell bindings where the cell values map directly to the hwirq number
and linux irq flags.

**Note**

don’t use this function unless your interrupt controller explicitly
supports both one and two cell bindings. For the majority of controllers
the `_onecell()` or `_twocell()` variants above should be used.

int irq\_domain\_translate\_onecell(struct [irq\_domain](#c.irq_domain "irq_domain") \*d, struct [irq\_fwspec](#c.irq_fwspec "irq_fwspec") \*fwspec, unsigned long \*out\_hwirq, unsigned int \*out\_type)
:   Generic translate for direct one cell bindings

**Parameters**

`struct irq_domain *d`
:   Interrupt domain involved in the translation

`struct irq_fwspec *fwspec`
:   The firmware interrupt specifier to translate

`unsigned long *out_hwirq`
:   Pointer to storage for the hardware interrupt number

`unsigned int *out_type`
:   Pointer to storage for the interrupt type

int irq\_domain\_translate\_twocell(struct [irq\_domain](#c.irq_domain "irq_domain") \*d, struct [irq\_fwspec](#c.irq_fwspec "irq_fwspec") \*fwspec, unsigned long \*out\_hwirq, unsigned int \*out\_type)
:   Generic translate for direct two cell bindings

**Parameters**

`struct irq_domain *d`
:   Interrupt domain involved in the translation

`struct irq_fwspec *fwspec`
:   The firmware interrupt specifier to translate

`unsigned long *out_hwirq`
:   Pointer to storage for the hardware interrupt number

`unsigned int *out_type`
:   Pointer to storage for the interrupt type

**Description**

Device Tree IRQ specifier translation function which works with two cell
bindings where the cell values map directly to the hwirq number
and linux irq flags.

int irq\_domain\_translate\_twothreecell(struct [irq\_domain](#c.irq_domain "irq_domain") \*d, struct [irq\_fwspec](#c.irq_fwspec "irq_fwspec") \*fwspec, unsigned long \*out\_hwirq, unsigned int \*out\_type)
:   Generic translate for direct two or three cell bindings

**Parameters**

`struct irq_domain *d`
:   Interrupt domain involved in the translation

`struct irq_fwspec *fwspec`
:   The firmware interrupt specifier to translate

`unsigned long *out_hwirq`
:   Pointer to storage for the hardware interrupt number

`unsigned int *out_type`
:   Pointer to storage for the interrupt type

**Description**

Firmware interrupt specifier translation function for two or three cell
specifications, where the parameter values map directly to the hardware
interrupt number and the type specifier.

void irq\_domain\_reset\_irq\_data(struct [irq\_data](#c.irq_domain_reset_irq_data "irq_data") \*irq\_data)
:   Clear hwirq, chip and chip\_data in **irq\_data**

**Parameters**

`struct irq_data *irq_data`
:   The pointer to irq\_data

int irq\_domain\_disconnect\_hierarchy(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain, unsigned int virq)
:   Mark the first unused level of a hierarchy

**Parameters**

`struct irq_domain *domain`
:   IRQ domain from which the hierarchy is to be disconnected

`unsigned int virq`
:   IRQ number where the hierarchy is to be trimmed

**Description**

Marks the **virq** level belonging to **domain** as disconnected.
Returns -EINVAL if **virq** doesn’t have a valid irq\_data pointing
to **domain**.

Its only use is to be able to trim levels of hierarchy that do not
have any real meaning for this interrupt, and that the driver marks
as such from its .`alloc()` callback.

struct [irq\_data](../genericirq.html#c.irq_data "irq_data") \*irq\_domain\_get\_irq\_data(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain, unsigned int virq)
:   Get irq\_data associated with **virq** and **domain**

**Parameters**

`struct irq_domain *domain`
:   domain to match

`unsigned int virq`
:   IRQ number to get irq\_data

int irq\_domain\_set\_hwirq\_and\_chip(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain, unsigned int virq, irq\_hw\_number\_t hwirq, const struct [irq\_chip](../genericirq.html#c.irq_chip "irq_chip") \*chip, void \*chip\_data)
:   Set hwirq and irqchip of **virq** at **domain**

**Parameters**

`struct irq_domain *domain`
:   Interrupt domain to match

`unsigned int virq`
:   IRQ number

`irq_hw_number_t hwirq`
:   The hwirq number

`const struct irq_chip *chip`
:   The associated interrupt chip

`void *chip_data`
:   The associated chip data

void irq\_domain\_set\_info(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain, unsigned int virq, irq\_hw\_number\_t hwirq, const struct [irq\_chip](../genericirq.html#c.irq_chip "irq_chip") \*chip, void \*chip\_data, irq\_flow\_handler\_t handler, void \*handler\_data, const char \*handler\_name)
:   Set the complete data for a **virq** in **domain**

**Parameters**

`struct irq_domain *domain`
:   Interrupt domain to match

`unsigned int virq`
:   IRQ number

`irq_hw_number_t hwirq`
:   The hardware interrupt number

`const struct irq_chip *chip`
:   The associated interrupt chip

`void *chip_data`
:   The associated interrupt chip data

`irq_flow_handler_t handler`
:   The interrupt flow handler

`void *handler_data`
:   The interrupt flow handler data

`const char *handler_name`
:   The interrupt handler name

void irq\_domain\_free\_irqs\_common(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain, unsigned int virq, unsigned int nr\_irqs)
:   Clear irq\_data and free the parent

**Parameters**

`struct irq_domain *domain`
:   Interrupt domain to match

`unsigned int virq`
:   IRQ number to start with

`unsigned int nr_irqs`
:   The number of irqs to free

void irq\_domain\_free\_irqs\_top(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain, unsigned int virq, unsigned int nr\_irqs)
:   Clear handler and handler data, clear irqdata and free parent

**Parameters**

`struct irq_domain *domain`
:   Interrupt domain to match

`unsigned int virq`
:   IRQ number to start with

`unsigned int nr_irqs`
:   The number of irqs to free

int \_\_irq\_domain\_alloc\_irqs(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain, int irq\_base, unsigned int nr\_irqs, int node, void \*arg, bool realloc, const struct [irq\_affinity\_desc](../genericirq.html#c.irq_affinity_desc "irq_affinity_desc") \*affinity)
:   Allocate IRQs from domain

**Parameters**

`struct irq_domain *domain`
:   domain to allocate from

`int irq_base`
:   allocate specified IRQ number if irq\_base >= 0

`unsigned int nr_irqs`
:   number of IRQs to allocate

`int node`
:   NUMA node id for memory allocation

`void *arg`
:   domain specific argument

`bool realloc`
:   IRQ descriptors have already been allocated if true

`const struct irq_affinity_desc *affinity`
:   Optional irq affinity mask for multiqueue devices

**Description**

Allocate IRQ numbers and initialized all data structures to support
hierarchy IRQ domains.
Parameter **realloc** is mainly to support legacy IRQs.
Returns error code or allocated IRQ number

The whole process to setup an IRQ has been split into two steps.
The first step, [`__irq_domain_alloc_irqs()`](#c.__irq_domain_alloc_irqs "__irq_domain_alloc_irqs"), is to allocate IRQ
descriptor and required hardware resources. The second step,
[`irq_domain_activate_irq()`](#c.irq_domain_activate_irq "irq_domain_activate_irq"), is to program the hardware with preallocated
resources. In this way, it’s easier to rollback when failing to
allocate resources.

int irq\_domain\_push\_irq(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain, int virq, void \*arg)
:   Push a domain in to the top of a hierarchy.

**Parameters**

`struct irq_domain *domain`
:   Domain to push.

`int virq`
:   Irq to push the domain in to.

`void *arg`
:   Passed to the irq\_domain\_ops `alloc()` function.

**Description**

For an already existing irqdomain hierarchy, as might be obtained
via a call to `pci_enable_msix()`, add an additional domain to the
head of the processing chain. Must be called before [`request_irq()`](../genericirq.html#c.request_irq "request_irq")
has been called.

int irq\_domain\_pop\_irq(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain, int virq)
:   Remove a domain from the top of a hierarchy.

**Parameters**

`struct irq_domain *domain`
:   Domain to remove.

`int virq`
:   Irq to remove the domain from.

**Description**

Undo the effects of a call to [`irq_domain_push_irq()`](#c.irq_domain_push_irq "irq_domain_push_irq"). Must be
called either before [`request_irq()`](../genericirq.html#c.request_irq "request_irq") or after [`free_irq()`](../genericirq.html#c.free_irq "free_irq").

void irq\_domain\_free\_irqs(unsigned int virq, unsigned int nr\_irqs)
:   Free IRQ number and associated data structures

**Parameters**

`unsigned int virq`
:   base IRQ number

`unsigned int nr_irqs`
:   number of IRQs to free

int irq\_domain\_alloc\_irqs\_parent(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain, unsigned int irq\_base, unsigned int nr\_irqs, void \*arg)
:   Allocate interrupts from parent domain

**Parameters**

`struct irq_domain *domain`
:   Domain below which interrupts must be allocated

`unsigned int irq_base`
:   Base IRQ number

`unsigned int nr_irqs`
:   Number of IRQs to allocate

`void *arg`
:   Allocation data (arch/domain specific)

void irq\_domain\_free\_irqs\_parent(struct [irq\_domain](#c.irq_domain "irq_domain") \*domain, unsigned int irq\_base, unsigned int nr\_irqs)
:   Free interrupts from parent domain

**Parameters**

`struct irq_domain *domain`
:   Domain below which interrupts must be freed

`unsigned int irq_base`
:   Base IRQ number

`unsigned int nr_irqs`
:   Number of IRQs to free

## Internal Functions Provided

This chapter contains the autogenerated documentation of the internal
functions.

int irq\_domain\_activate\_irq(struct [irq\_data](#c.irq_domain_activate_irq "irq_data") \*irq\_data, bool reserve)
:   Call domain\_ops->activate recursively to activate interrupt

**Parameters**

`struct irq_data *irq_data`
:   Outermost irq\_data associated with interrupt

`bool reserve`
:   If set only reserve an interrupt vector instead of assigning one

**Description**

This is the second step to call domain\_ops->activate to program interrupt
controllers, so the interrupt could actually get delivered.

void irq\_domain\_deactivate\_irq(struct [irq\_data](#c.irq_domain_deactivate_irq "irq_data") \*irq\_data)
:   Call domain\_ops->deactivate recursively to deactivate interrupt

**Parameters**

`struct irq_data *irq_data`
:   outermost irq\_data associated with interrupt

**Description**

It calls domain\_ops->deactivate to program interrupt controllers to disable
interrupt delivery.
