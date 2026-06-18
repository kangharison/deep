# FPGA Manager

> 출처(원문): https://docs.kernel.org/driver-api/fpga/fpga-mgr.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# FPGA Manager

## Overview

The FPGA manager core exports a set of functions for programming an FPGA with
an image. The API is manufacturer agnostic. All manufacturer specifics are
hidden away in a low level driver which registers a set of ops with the core.
The FPGA image data itself is very manufacturer specific, but for our purposes
it’s just binary data. The FPGA manager core won’t parse it.

The FPGA image to be programmed can be in a scatter gather list, a single
contiguous buffer, or a firmware file. Because allocating contiguous kernel
memory for the buffer should be avoided, users are encouraged to use a scatter
gather list instead if possible.

The particulars for programming the image are presented in a structure ([`struct
fpga_image_info`](fpga-programming.html#c.fpga_image_info "fpga_image_info")). This `struct contains` parameters such as pointers to the
FPGA image as well as image-specific particulars such as whether the image was
built for full or partial reconfiguration.

## How to support a new FPGA device

To add another FPGA manager, write a driver that implements a set of ops. The
probe function calls `fpga_mgr_register()` or `fpga_mgr_register_full()`,
such as:

```
static const struct fpga_manager_ops socfpga_fpga_ops = {
        .write_init = socfpga_fpga_ops_configure_init,
        .write = socfpga_fpga_ops_configure_write,
        .write_complete = socfpga_fpga_ops_configure_complete,
        .state = socfpga_fpga_ops_state,
};

static int socfpga_fpga_probe(struct platform_device *pdev)
{
        struct device *dev = &pdev->dev;
        struct socfpga_fpga_priv *priv;
        struct fpga_manager *mgr;
        int ret;

        priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
        if (!priv)
                return -ENOMEM;

        /*
         * do ioremaps, get interrupts, etc. and save
         * them in priv
         */

        mgr = fpga_mgr_register(dev, "Altera SOCFPGA FPGA Manager",
                                &socfpga_fpga_ops, priv);
        if (IS_ERR(mgr))
                return PTR_ERR(mgr);

        platform_set_drvdata(pdev, mgr);

        return 0;
}

static int socfpga_fpga_remove(struct platform_device *pdev)
{
        struct fpga_manager *mgr = platform_get_drvdata(pdev);

        fpga_mgr_unregister(mgr);

        return 0;
}
```

Alternatively, the probe function could call one of the resource managed
register functions, `devm_fpga_mgr_register()` or
`devm_fpga_mgr_register_full()`. When these functions are used, the
parameter syntax is the same, but the call to `fpga_mgr_unregister()` should be
removed. In the above example, the `socfpga_fpga_remove()` function would not be
required.

The ops will implement whatever device specific register writes are needed to
do the programming sequence for this particular FPGA. These ops return 0 for
success or negative error codes otherwise.

The programming sequence is::
:   1. .parse\_header (optional, may be called once or multiple times)
    2. .write\_init
    3. .write or .write\_sg (may be called once or multiple times)
    4. .write\_complete

The .parse\_header function will set header\_size and data\_size to
[`struct fpga_image_info`](fpga-programming.html#c.fpga_image_info "fpga_image_info"). Before parse\_header call, header\_size is initialized
with initial\_header\_size. If flag skip\_header of fpga\_manager\_ops is true,
.write function will get image buffer starting at header\_size offset from the
beginning. If data\_size is set, .write function will get data\_size bytes of
the image buffer, otherwise .write will get data up to the end of image buffer.
This will not affect .write\_sg, .write\_sg will still get whole image in
sg\_table form. If FPGA image is already mapped as a single contiguous buffer,
whole buffer will be passed into .parse\_header. If image is in scatter-gather
form, core code will buffer up at least .initial\_header\_size before the first
call of .parse\_header, if it is not enough, .parse\_header should set desired
size into info->header\_size and return -EAGAIN, then it will be called again
with greater part of image buffer on the input.

The .write\_init function will prepare the FPGA to receive the image data. The
buffer passed into .write\_init will be at least info->header\_size bytes long;
if the whole bitstream is not immediately available then the core code will
buffer up at least this much before starting.

The .write function writes a buffer to the FPGA. The buffer may be contain the
whole FPGA image or may be a smaller chunk of an FPGA image. In the latter
case, this function is called multiple times for successive chunks. This interface
is suitable for drivers which use PIO.

The .write\_sg version behaves the same as .write except the input is a sg\_table
scatter list. This interface is suitable for drivers which use DMA.

The .write\_complete function is called after all the image has been written
to put the FPGA into operating mode.

The ops include a .state function which will determine the state the FPGA is in
and return a code of type [`enum fpga_mgr_states`](#c.fpga_mgr_states "fpga_mgr_states"). It doesn’t result in a change
in state.

## API for implementing a new FPGA Manager driver

* `fpga_mgr_states` - Values for [fpga\_manager](#c.fpga_manager "fpga_manager")->state.
* [`struct fpga_manager`](#c.fpga_manager "fpga_manager") - the FPGA manager struct
* [`struct fpga_manager_ops`](#c.fpga_manager_ops "fpga_manager_ops") - Low level FPGA manager driver ops
* [`struct fpga_manager_info`](#c.fpga_manager_info "fpga_manager_info") - Parameter structure for `fpga_mgr_register_full()`
* [`__fpga_mgr_register_full()`](#c.__fpga_mgr_register_full "__fpga_mgr_register_full") - Create and register an FPGA manager using the
  fpga\_mgr\_info structure to provide the full flexibility of options
* [`__fpga_mgr_register()`](#c.__fpga_mgr_register "__fpga_mgr_register") - Create and register an FPGA manager using standard
  arguments
* [`__devm_fpga_mgr_register_full()`](#c.__devm_fpga_mgr_register_full "__devm_fpga_mgr_register_full") - Resource managed version of
  [`__fpga_mgr_register_full()`](#c.__fpga_mgr_register_full "__fpga_mgr_register_full")
* [`__devm_fpga_mgr_register()`](#c.__devm_fpga_mgr_register "__devm_fpga_mgr_register") - Resource managed version of [`__fpga_mgr_register()`](#c.__fpga_mgr_register "__fpga_mgr_register")
* [`fpga_mgr_unregister()`](#c.fpga_mgr_unregister "fpga_mgr_unregister") - Unregister an FPGA manager

Helper macros `fpga_mgr_register_full()`, `fpga_mgr_register()`,
`devm_fpga_mgr_register_full()`, and `devm_fpga_mgr_register()` are available
to ease the registration.

enum fpga\_mgr\_states
:   fpga framework states

**Constants**

`FPGA_MGR_STATE_UNKNOWN`
:   can’t determine state

`FPGA_MGR_STATE_POWER_OFF`
:   FPGA power is off

`FPGA_MGR_STATE_POWER_UP`
:   FPGA reports power is up

`FPGA_MGR_STATE_RESET`
:   FPGA in reset state

`FPGA_MGR_STATE_FIRMWARE_REQ`
:   firmware request in progress

`FPGA_MGR_STATE_FIRMWARE_REQ_ERR`
:   firmware request failed

`FPGA_MGR_STATE_PARSE_HEADER`
:   parse FPGA image header

`FPGA_MGR_STATE_PARSE_HEADER_ERR`
:   Error during PARSE\_HEADER stage

`FPGA_MGR_STATE_WRITE_INIT`
:   preparing FPGA for programming

`FPGA_MGR_STATE_WRITE_INIT_ERR`
:   Error during WRITE\_INIT stage

`FPGA_MGR_STATE_WRITE`
:   writing image to FPGA

`FPGA_MGR_STATE_WRITE_ERR`
:   Error while writing FPGA

`FPGA_MGR_STATE_WRITE_COMPLETE`
:   Doing post programming steps

`FPGA_MGR_STATE_WRITE_COMPLETE_ERR`
:   Error during WRITE\_COMPLETE

`FPGA_MGR_STATE_OPERATING`
:   FPGA is programmed and operating

struct fpga\_manager
:   fpga manager structure

**Definition**:

```
struct fpga_manager {
    const char *name;
    struct device dev;
    struct mutex ref_mutex;
    enum fpga_mgr_states state;
    struct fpga_compat_id *compat_id;
    const struct fpga_manager_ops *mops;
    struct module *mops_owner;
    void *priv;
};
```

**Members**

`name`
:   name of low level fpga manager

`dev`
:   fpga manager device

`ref_mutex`
:   only allows one reference to fpga manager

`state`
:   state of fpga manager

`compat_id`
:   FPGA manager id for compatibility check.

`mops`
:   pointer to `struct of` fpga manager ops

`mops_owner`
:   module containing the mops

`priv`
:   low level driver private date

struct fpga\_manager\_ops
:   ops for low level fpga manager drivers

**Definition**:

```
struct fpga_manager_ops {
    size_t initial_header_size;
    bool skip_header;
    enum fpga_mgr_states (*state)(struct fpga_manager *mgr);
    u64 (*status)(struct fpga_manager *mgr);
    int (*parse_header)(struct fpga_manager *mgr, struct fpga_image_info *info, const char *buf, size_t count);
    int (*write_init)(struct fpga_manager *mgr, struct fpga_image_info *info, const char *buf, size_t count);
    int (*write)(struct fpga_manager *mgr, const char *buf, size_t count);
    int (*write_sg)(struct fpga_manager *mgr, struct sg_table *sgt);
    int (*write_complete)(struct fpga_manager *mgr, struct fpga_image_info *info);
    void (*fpga_remove)(struct fpga_manager *mgr);
    const struct attribute_group **groups;
};
```

**Members**

`initial_header_size`
:   minimum number of bytes that should be passed into
    parse\_header and write\_init.

`skip_header`
:   bool flag to tell fpga-mgr core whether it should skip
    info->header\_size part at the beginning of the image when invoking
    write callback.

`state`
:   returns an `enum value` of the FPGA’s state

`status`
:   returns status of the FPGA, including reconfiguration error code

`parse_header`
:   parse FPGA image header to set info->header\_size and
    info->data\_size. In case the input buffer is not large enough, set
    required size to info->header\_size and return -EAGAIN.

`write_init`
:   prepare the FPGA to receive configuration data

`write`
:   write count bytes of configuration data to the FPGA

`write_sg`
:   write the scatter list of configuration data to the FPGA

`write_complete`
:   set FPGA to operating state after writing is done

`fpga_remove`
:   optional: Set FPGA into a specific state during driver remove

`groups`
:   optional attribute groups.

**Description**

fpga\_manager\_ops are the low level functions implemented by a specific
fpga manager driver. The optional ones are tested for NULL before being
called, so leaving them out is fine.

struct fpga\_manager\_info
:   collection of parameters for an FPGA Manager

**Definition**:

```
struct fpga_manager_info {
    const char *name;
    struct fpga_compat_id *compat_id;
    const struct fpga_manager_ops *mops;
    void *priv;
};
```

**Members**

`name`
:   fpga manager name

`compat_id`
:   FPGA manager id for compatibility check.

`mops`
:   pointer to structure of fpga manager ops

`priv`
:   fpga manager private data

**Description**

fpga\_manager\_info contains parameters for the register\_full function.
These are separated into an info structure because they some are optional
others could be added to in the future. The info structure facilitates
maintaining a stable API.

struct [fpga\_manager](#c.fpga_manager "fpga_manager") \*\_\_fpga\_mgr\_register\_full(struct [device](../infrastructure.html#c.device "device") \*parent, const struct [fpga\_manager\_info](#c.fpga_manager_info "fpga_manager_info") \*info, struct module \*owner)
:   create and register an FPGA Manager device

**Parameters**

`struct device *parent`
:   fpga manager device from pdev

`const struct fpga_manager_info *info`
:   parameters for fpga manager

`struct module *owner`
:   owner module containing the ops

**Description**

The caller of this function is responsible for calling [`fpga_mgr_unregister()`](#c.fpga_mgr_unregister "fpga_mgr_unregister").
Using `devm_fpga_mgr_register_full()` instead is recommended.

**Return**

pointer to [`struct fpga_manager`](#c.fpga_manager "fpga_manager") pointer or [`ERR_PTR()`](../../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR")

struct [fpga\_manager](#c.fpga_manager "fpga_manager") \*\_\_fpga\_mgr\_register(struct [device](../infrastructure.html#c.device "device") \*parent, const char \*name, const struct [fpga\_manager\_ops](#c.fpga_manager_ops "fpga_manager_ops") \*mops, void \*priv, struct module \*owner)
:   create and register an FPGA Manager device

**Parameters**

`struct device *parent`
:   fpga manager device from pdev

`const char *name`
:   fpga manager name

`const struct fpga_manager_ops *mops`
:   pointer to structure of fpga manager ops

`void *priv`
:   fpga manager private data

`struct module *owner`
:   owner module containing the ops

**Description**

The caller of this function is responsible for calling [`fpga_mgr_unregister()`](#c.fpga_mgr_unregister "fpga_mgr_unregister").
Using `devm_fpga_mgr_register()` instead is recommended. This simple
version of the register function should be sufficient for most users. The
`fpga_mgr_register_full()` function is available for users that need to pass
additional, optional parameters.

**Return**

pointer to [`struct fpga_manager`](#c.fpga_manager "fpga_manager") pointer or [`ERR_PTR()`](../../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR")

struct [fpga\_manager](#c.fpga_manager "fpga_manager") \*\_\_devm\_fpga\_mgr\_register\_full(struct [device](../infrastructure.html#c.device "device") \*parent, const struct [fpga\_manager\_info](#c.fpga_manager_info "fpga_manager_info") \*info, struct module \*owner)
:   resource managed variant of `fpga_mgr_register()`

**Parameters**

`struct device *parent`
:   fpga manager device from pdev

`const struct fpga_manager_info *info`
:   parameters for fpga manager

`struct module *owner`
:   owner module containing the ops

**Return**

fpga manager pointer on success, negative error code otherwise.

**Description**

This is the devres variant of `fpga_mgr_register_full()` for which the unregister
function will be called automatically when the managing device is detached.

struct [fpga\_manager](#c.fpga_manager "fpga_manager") \*\_\_devm\_fpga\_mgr\_register(struct [device](../infrastructure.html#c.device "device") \*parent, const char \*name, const struct [fpga\_manager\_ops](#c.fpga_manager_ops "fpga_manager_ops") \*mops, void \*priv, struct module \*owner)
:   resource managed variant of `fpga_mgr_register()`

**Parameters**

`struct device *parent`
:   fpga manager device from pdev

`const char *name`
:   fpga manager name

`const struct fpga_manager_ops *mops`
:   pointer to structure of fpga manager ops

`void *priv`
:   fpga manager private data

`struct module *owner`
:   owner module containing the ops

**Return**

fpga manager pointer on success, negative error code otherwise.

**Description**

This is the devres variant of `fpga_mgr_register()` for which the
unregister function will be called automatically when the managing
device is detached.

void fpga\_mgr\_unregister(struct [fpga\_manager](#c.fpga_manager "fpga_manager") \*mgr)
:   unregister an FPGA manager

**Parameters**

`struct fpga_manager *mgr`
:   fpga manager struct

**Description**

This function is intended for use in an FPGA manager driver’s remove function.
