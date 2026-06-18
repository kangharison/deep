# FPGA Region

> 출처(원문): https://docs.kernel.org/driver-api/fpga/fpga-region.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# FPGA Region

## Overview

This document is meant to be a brief overview of the FPGA region API usage. A
more conceptual look at regions can be found in the Device Tree binding
document [[1]](#f1).

For the purposes of this API document, let’s just say that a region associates
an FPGA Manager and a bridge (or bridges) with a reprogrammable region of an
FPGA or the whole FPGA. The API provides a way to register a region and to
program a region.

Currently the only layer above fpga-region.c in the kernel is the Device Tree
support (of-fpga-region.c) described in [[1]](#f1). The DT support layer uses regions
to program the FPGA and then DT to handle enumeration. The common region code
is intended to be used by other schemes that have other ways of accomplishing
enumeration after programming.

An fpga-region can be set up to know the following things:

> * which FPGA manager to use to do the programming
> * which bridges to disable before programming and enable afterwards.

Additional info needed to program the FPGA image is passed in the [`struct
fpga_image_info`](fpga-programming.html#c.fpga_image_info "fpga_image_info") including:

> * pointers to the image as either a scatter-gather buffer, a contiguous
>   buffer, or the name of firmware file
> * flags indicating specifics such as whether the image is for partial
>   reconfiguration.

## How to add a new FPGA region

An example of usage can be seen in the probe function of [[2]](#f2).

[1]
([1](#id1),[2](#id2))

../devicetree/bindings/fpga/fpga-region.txt


[[2](#id3)]

../../drivers/fpga/of-fpga-region.c

## API to add a new FPGA region

* [`struct fpga_region`](#c.fpga_region "fpga_region") - The FPGA region struct
* [`struct fpga_region_info`](#c.fpga_region_info "fpga_region_info") - Parameter structure for [`__fpga_region_register_full()`](#c.__fpga_region_register_full "__fpga_region_register_full")
* [`__fpga_region_register_full()`](#c.__fpga_region_register_full "__fpga_region_register_full") - Create and register an FPGA region using the
  fpga\_region\_info structure to provide the full flexibility of options
* [`__fpga_region_register()`](#c.__fpga_region_register "__fpga_region_register") - Create and register an FPGA region using standard
  arguments
* [`fpga_region_unregister()`](#c.fpga_region_unregister "fpga_region_unregister") - Unregister an FPGA region

Helper macros `fpga_region_register()` and `fpga_region_register_full()`
automatically set the module that registers the FPGA region as the owner.

The FPGA region’s probe function will need to get a reference to the FPGA
Manager it will be using to do the programming. This usually would happen
during the region’s probe function.

* [`fpga_mgr_get()`](#c.fpga_mgr_get "fpga_mgr_get") - Get a reference to an FPGA manager, raise ref count
* [`of_fpga_mgr_get()`](#c.of_fpga_mgr_get "of_fpga_mgr_get") - Get a reference to an FPGA manager, raise ref count,
  given a device node.
* [`fpga_mgr_put()`](#c.fpga_mgr_put "fpga_mgr_put") - Put an FPGA manager

The FPGA region will need to specify which bridges to control while programming
the FPGA. The region driver can build a list of bridges during probe time
([fpga\_region](#c.fpga_region "fpga_region")->bridge\_list) or it can have a function that creates
the list of bridges to program just before programming
([fpga\_region](#c.fpga_region "fpga_region")->get\_bridges). The FPGA bridge framework supplies the
following APIs to handle building or tearing down that list.

* [`fpga_bridge_get_to_list()`](#c.fpga_bridge_get_to_list "fpga_bridge_get_to_list") - Get a ref of an FPGA bridge, add it to a
  list
* [`of_fpga_bridge_get_to_list()`](#c.of_fpga_bridge_get_to_list "of_fpga_bridge_get_to_list") - Get a ref of an FPGA bridge, add it to a
  list, given a device node
* [`fpga_bridges_put()`](#c.fpga_bridges_put "fpga_bridges_put") - Given a list of bridges, put them

struct fpga\_region
:   FPGA Region structure

**Definition**:

```
struct fpga_region {
    struct device dev;
    struct mutex mutex;
    struct list_head bridge_list;
    struct fpga_manager *mgr;
    struct fpga_image_info *info;
    struct fpga_compat_id *compat_id;
    struct module *ops_owner;
    void *priv;
    int (*get_bridges)(struct fpga_region *region);
};
```

**Members**

`dev`
:   FPGA Region device

`mutex`
:   enforces exclusive reference to region

`bridge_list`
:   list of FPGA bridges specified in region

`mgr`
:   FPGA manager

`info`
:   FPGA image info

`compat_id`
:   FPGA region id for compatibility check.

`ops_owner`
:   module containing the get\_bridges function

`priv`
:   private data

`get_bridges`
:   optional function to get bridges to a list

struct fpga\_region\_info
:   collection of parameters an FPGA Region

**Definition**:

```
struct fpga_region_info {
    struct fpga_manager *mgr;
    struct fpga_compat_id *compat_id;
    void *priv;
    int (*get_bridges)(struct fpga_region *region);
};
```

**Members**

`mgr`
:   fpga region manager

`compat_id`
:   FPGA region id for compatibility check.

`priv`
:   fpga region private data

`get_bridges`
:   optional function to get bridges to a list

**Description**

fpga\_region\_info contains parameters for the register\_full function.
These are separated into an info structure because they some are optional
others could be added to in the future. The info structure facilitates
maintaining a stable API.

struct [fpga\_region](#c.fpga_region "fpga_region") \*\_\_fpga\_region\_register\_full(struct [device](../infrastructure.html#c.device "device") \*parent, const struct [fpga\_region\_info](#c.fpga_region_info "fpga_region_info") \*info, struct module \*owner)
:   create and register an FPGA Region device

**Parameters**

`struct device *parent`
:   device parent

`const struct fpga_region_info *info`
:   parameters for FPGA Region

`struct module *owner`
:   module containing the get\_bridges function

**Return**

[`struct fpga_region`](#c.fpga_region "fpga_region") or [`ERR_PTR()`](../../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR")

struct [fpga\_region](#c.fpga_region "fpga_region") \*\_\_fpga\_region\_register(struct [device](../infrastructure.html#c.device "device") \*parent, struct [fpga\_manager](fpga-mgr.html#c.fpga_manager "fpga_manager") \*mgr, int (\*get\_bridges)(struct [fpga\_region](#c.fpga_region "fpga_region")\*), struct module \*owner)
:   create and register an FPGA Region device

**Parameters**

`struct device *parent`
:   device parent

`struct fpga_manager *mgr`
:   manager that programs this region

`int (*get_bridges)(struct fpga_region *)`
:   optional function to get bridges to a list

`struct module *owner`
:   module containing the get\_bridges function

**Description**

This simple version of the register function should be sufficient for most users.
The `fpga_region_register_full()` function is available for users that need to
pass additional, optional parameters.

**Return**

[`struct fpga_region`](#c.fpga_region "fpga_region") or [`ERR_PTR()`](../../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR")

void fpga\_region\_unregister(struct [fpga\_region](#c.fpga_region "fpga_region") \*region)
:   unregister an FPGA region

**Parameters**

`struct fpga_region *region`
:   FPGA region

**Description**

This function is intended for use in an FPGA region driver’s remove function.

struct [fpga\_manager](fpga-mgr.html#c.fpga_manager "fpga_manager") \*fpga\_mgr\_get(struct [device](../infrastructure.html#c.device "device") \*dev)
:   Given a device, get a reference to an fpga mgr.

**Parameters**

`struct device *dev`
:   parent device that fpga mgr was registered with

**Return**

fpga manager `struct or` [`IS_ERR()`](../../core-api/kernel-api.html#c.IS_ERR "IS_ERR") condition containing error code.

struct [fpga\_manager](fpga-mgr.html#c.fpga_manager "fpga_manager") \*of\_fpga\_mgr\_get(struct device\_node \*node)
:   Given a device node, get a reference to an fpga mgr.

**Parameters**

`struct device_node *node`
:   device node

**Return**

fpga manager `struct or` [`IS_ERR()`](../../core-api/kernel-api.html#c.IS_ERR "IS_ERR") condition containing error code.

void fpga\_mgr\_put(struct [fpga\_manager](fpga-mgr.html#c.fpga_manager "fpga_manager") \*mgr)
:   release a reference to an fpga manager

**Parameters**

`struct fpga_manager *mgr`
:   fpga manager structure

int fpga\_bridge\_get\_to\_list(struct [device](../infrastructure.html#c.device "device") \*dev, struct [fpga\_image\_info](fpga-programming.html#c.fpga_image_info "fpga_image_info") \*info, struct list\_head \*bridge\_list)
:   given device, get a bridge, add it to a list

**Parameters**

`struct device *dev`
:   FPGA bridge device

`struct fpga_image_info *info`
:   fpga image specific information

`struct list_head *bridge_list`
:   list of FPGA bridges

**Description**

Get an exclusive reference to the bridge and it to the list.

**Return**

0 for success, error code from `fpga_bridge_get()` otherwise.

int of\_fpga\_bridge\_get\_to\_list(struct device\_node \*np, struct [fpga\_image\_info](fpga-programming.html#c.fpga_image_info "fpga_image_info") \*info, struct list\_head \*bridge\_list)
:   get a bridge, add it to a list

**Parameters**

`struct device_node *np`
:   node pointer of an FPGA bridge

`struct fpga_image_info *info`
:   fpga image specific information

`struct list_head *bridge_list`
:   list of FPGA bridges

**Description**

Get an exclusive reference to the bridge and it to the list.

**Return**

0 for success, error code from `of_fpga_bridge_get()` otherwise.

void fpga\_bridges\_put(struct list\_head \*bridge\_list)
:   put bridges

**Parameters**

`struct list_head *bridge_list`
:   list of FPGA bridges

**Description**

For each bridge in the list, put the bridge and remove it from the list.
If list is empty, do nothing.
