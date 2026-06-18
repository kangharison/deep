# FPGA Bridge

> 출처(원문): https://docs.kernel.org/driver-api/fpga/fpga-bridge.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# FPGA Bridge

## API to implement a new FPGA bridge

* [`struct fpga_bridge`](#c.fpga_bridge "fpga_bridge") - The FPGA Bridge structure
* [`struct fpga_bridge_ops`](#c.fpga_bridge_ops "fpga_bridge_ops") - Low level Bridge driver ops
* [`__fpga_bridge_register()`](#c.__fpga_bridge_register "__fpga_bridge_register") - Create and register a bridge
* [`fpga_bridge_unregister()`](#c.fpga_bridge_unregister "fpga_bridge_unregister") - Unregister a bridge

The helper macro `fpga_bridge_register()` automatically sets
the module that registers the FPGA bridge as the owner.

struct fpga\_bridge
:   FPGA bridge structure

**Definition**:

```
struct fpga_bridge {
    const char *name;
    struct device dev;
    struct mutex mutex;
    const struct fpga_bridge_ops *br_ops;
    struct module *br_ops_owner;
    struct fpga_image_info *info;
    struct list_head node;
    void *priv;
};
```

**Members**

`name`
:   name of low level FPGA bridge

`dev`
:   FPGA bridge device

`mutex`
:   enforces exclusive reference to bridge

`br_ops`
:   pointer to `struct of` FPGA bridge ops

`br_ops_owner`
:   module containing the br\_ops

`info`
:   fpga image specific information

`node`
:   FPGA bridge list node

`priv`
:   low level driver private date

struct fpga\_bridge\_ops
:   ops for low level FPGA bridge drivers

**Definition**:

```
struct fpga_bridge_ops {
    int (*enable_show)(struct fpga_bridge *bridge);
    int (*enable_set)(struct fpga_bridge *bridge, bool enable);
    void (*fpga_bridge_remove)(struct fpga_bridge *bridge);
    const struct attribute_group **groups;
};
```

**Members**

`enable_show`
:   returns the FPGA bridge’s status

`enable_set`
:   set an FPGA bridge as enabled or disabled

`fpga_bridge_remove`
:   set FPGA into a specific state during driver remove

`groups`
:   optional attribute groups.

struct [fpga\_bridge](#c.fpga_bridge "fpga_bridge") \*\_\_fpga\_bridge\_register(struct [device](../infrastructure.html#c.device "device") \*parent, const char \*name, const struct [fpga\_bridge\_ops](#c.fpga_bridge_ops "fpga_bridge_ops") \*br\_ops, void \*priv, struct module \*owner)
:   create and register an FPGA Bridge device

**Parameters**

`struct device *parent`
:   FPGA bridge device from pdev

`const char *name`
:   FPGA bridge name

`const struct fpga_bridge_ops *br_ops`
:   pointer to structure of fpga bridge ops

`void *priv`
:   FPGA bridge private data

`struct module *owner`
:   owner module containing the br\_ops

**Return**

[`struct fpga_bridge`](#c.fpga_bridge "fpga_bridge") pointer or [`ERR_PTR()`](../../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR")

void fpga\_bridge\_unregister(struct [fpga\_bridge](#c.fpga_bridge "fpga_bridge") \*bridge)
:   unregister an FPGA bridge

**Parameters**

`struct fpga_bridge *bridge`
:   FPGA bridge struct

**Description**

This function is intended for use in an FPGA bridge driver’s remove function.
