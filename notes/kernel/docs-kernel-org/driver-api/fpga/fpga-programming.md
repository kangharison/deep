# In-kernel API for FPGA Programming

> 출처(원문): https://docs.kernel.org/driver-api/fpga/fpga-programming.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# In-kernel API for FPGA Programming

## Overview

The in-kernel API for FPGA programming is a combination of APIs from
FPGA manager, bridge, and regions. The actual function used to
trigger FPGA programming is [`fpga_region_program_fpga()`](#c.fpga_region_program_fpga "fpga_region_program_fpga").

[`fpga_region_program_fpga()`](#c.fpga_region_program_fpga "fpga_region_program_fpga") uses functionality supplied by
the FPGA manager and bridges. It will:

> * lock the region’s mutex
> * lock the mutex of the region’s FPGA manager
> * build a list of FPGA bridges if a method has been specified to do so
> * disable the bridges
> * program the FPGA using info passed in [fpga\_region](fpga-region.html#c.fpga_region "fpga_region")->info.
> * re-enable the bridges
> * release the locks

The [`struct fpga_image_info`](#c.fpga_image_info "fpga_image_info") specifies what FPGA image to program. It is
allocated/freed by [`fpga_image_info_alloc()`](#c.fpga_image_info_alloc "fpga_image_info_alloc") and freed with
[`fpga_image_info_free()`](#c.fpga_image_info_free "fpga_image_info_free")

## How to program an FPGA using a region

When the FPGA region driver probed, it was given a pointer to an FPGA manager
driver so it knows which manager to use. The region also either has a list of
bridges to control during programming or it has a pointer to a function that
will generate that list. Here’s some sample code of what to do next:

```
#include <linux/fpga/fpga-mgr.h>
#include <linux/fpga/fpga-region.h>

struct fpga_image_info *info;
int ret;

/*
 * First, alloc the struct with information about the FPGA image to
 * program.
 */
info = fpga_image_info_alloc(dev);
if (!info)
        return -ENOMEM;

/* Set flags as needed, such as: */
info->flags = FPGA_MGR_PARTIAL_RECONFIG;

/*
 * Indicate where the FPGA image is. This is pseudo-code; you're
 * going to use one of these three.
 */
if (image is in a scatter gather table) {

        info->sgt = [your scatter gather table]

} else if (image is in a buffer) {

        info->buf = [your image buffer]
        info->count = [image buffer size]

} else if (image is in a firmware file) {

        info->firmware_name = devm_kstrdup(dev, firmware_name,
                                           GFP_KERNEL);

}

/* Add info to region and do the programming */
region->info = info;
ret = fpga_region_program_fpga(region);

/* Deallocate the image info if you're done with it */
region->info = NULL;
fpga_image_info_free(info);

if (ret)
        return ret;

/* Now enumerate whatever hardware has appeared in the FPGA. */
```

## API for programming an FPGA

* [`fpga_region_program_fpga()`](#c.fpga_region_program_fpga "fpga_region_program_fpga") - Program an FPGA
* [`fpga_image_info()`](#c.fpga_image_info "fpga_image_info") - Specifies what FPGA image to program
* [`fpga_image_info_alloc()`](#c.fpga_image_info_alloc "fpga_image_info_alloc") - Allocate an FPGA image info struct
* [`fpga_image_info_free()`](#c.fpga_image_info_free "fpga_image_info_free") - Free an FPGA image info struct

int fpga\_region\_program\_fpga(struct [fpga\_region](fpga-region.html#c.fpga_region "fpga_region") \*region)
:   program FPGA

**Parameters**

`struct fpga_region *region`
:   FPGA region

**Description**

Program an FPGA using fpga image info (region->info).
If the region has a get\_bridges function, the exclusive reference for the
bridges will be held if programming succeeds. This is intended to prevent
reprogramming the region until the caller considers it safe to do so.
The caller will need to call [`fpga_bridges_put()`](fpga-region.html#c.fpga_bridges_put "fpga_bridges_put") before attempting to
reprogram the region.

**Return**

0 for success or negative error code.

FPGA Manager flags

Flags used in the [`fpga_image_info->flags`](#c.fpga_image_info "fpga_image_info") field

`FPGA_MGR_PARTIAL_RECONFIG`: do partial reconfiguration if supported

`FPGA_MGR_EXTERNAL_CONFIG`: FPGA has been configured prior to Linux booting

`FPGA_MGR_ENCRYPTED_BITSTREAM`: indicates bitstream is encrypted

`FPGA_MGR_BITSTREAM_LSB_FIRST`: SPI bitstream bit order is LSB first

`FPGA_MGR_COMPRESSED_BITSTREAM`: FPGA bitstream is compressed

struct fpga\_image\_info
:   information specific to an FPGA image

**Definition**:

```
struct fpga_image_info {
    u32 flags;
    u32 enable_timeout_us;
    u32 disable_timeout_us;
    u32 config_complete_timeout_us;
    char *firmware_name;
    struct sg_table *sgt;
    const char *buf;
    size_t count;
    size_t header_size;
    size_t data_size;
    int region_id;
    struct device *dev;
#ifdef CONFIG_OF;
    struct device_node *overlay;
#endif;
};
```

**Members**

`flags`
:   boolean flags as defined above

`enable_timeout_us`
:   maximum time to enable traffic through bridge (uSec)

`disable_timeout_us`
:   maximum time to disable traffic through bridge (uSec)

`config_complete_timeout_us`
:   maximum time for FPGA to switch to operating
    status in the write\_complete op.

`firmware_name`
:   name of FPGA image firmware file

`sgt`
:   scatter/gather table containing FPGA image

`buf`
:   contiguous buffer containing FPGA image

`count`
:   size of buf

`header_size`
:   size of image header.

`data_size`
:   size of image data to be sent to the device. If not specified,
    whole image will be used. Header may be skipped in either case.

`region_id`
:   id of target region

`dev`
:   device that owns this

`overlay`
:   Device Tree overlay

struct [fpga\_image\_info](#c.fpga_image_info "fpga_image_info") \*fpga\_image\_info\_alloc(struct [device](../infrastructure.html#c.device "device") \*dev)
:   Allocate an FPGA image info struct

**Parameters**

`struct device *dev`
:   owning device

**Return**

[`struct fpga_image_info`](#c.fpga_image_info "fpga_image_info") or NULL

void fpga\_image\_info\_free(struct [fpga\_image\_info](#c.fpga_image_info "fpga_image_info") \*info)
:   Free an FPGA image info struct

**Parameters**

`struct fpga_image_info *info`
:   FPGA image info `struct to` free
