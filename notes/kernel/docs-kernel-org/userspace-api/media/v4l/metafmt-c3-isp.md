# 2.13.2.V4L2_META_FMT_C3ISP_STATS (‘C3ST’), V4L2_META_FMT_C3ISP_PARAMS (‘C3PM’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/metafmt-c3-isp.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.13.2. V4L2\_META\_FMT\_C3ISP\_STATS (‘C3ST’), V4L2\_META\_FMT\_C3ISP\_PARAMS (‘C3PM’)

## 2.13.2.1. 3A Statistics

The C3 ISP can collect different statistics over an input Bayer frame.
Those statistics are obtained from the “c3-isp-stats” metadata capture video nodes,
using the [`v4l2_meta_format`](dev-meta.html#c.V4L.v4l2_meta_format "v4l2_meta_format") interface.
They are formatted as described by the [`c3_isp_stats_info`](#c.V4L.c3_isp_stats_info "c3_isp_stats_info") structure.

The statistics collected are Auto-white balance,
Auto-exposure and Auto-focus information.

## 2.13.2.2. Configuration Parameters

The configuration parameters are passed to the c3-isp-params metadata output video node,
using the [`v4l2_meta_format`](dev-meta.html#c.V4L.v4l2_meta_format "v4l2_meta_format") interface. Rather than a single `struct containing`
sub-structs for each configurable area of the ISP, parameters for the C3-ISP
are defined as distinct structs or “blocks” which may be added to the data
member of [`c3_isp_params_cfg`](#c.V4L.c3_isp_params_cfg "c3_isp_params_cfg"). Userspace is responsible for
populating the data member with the blocks that need to be configured by the driver, but
need not populate it with **all** the blocks, or indeed with any at all if there
are no configuration changes to make. Populated blocks **must** be consecutive
in the buffer. To assist both userspace and the driver in identifying the
blocks each block-specific `struct embeds`
[`c3_isp_params_block_header`](#c.V4L.c3_isp_params_block_header "c3_isp_params_block_header") as its first member and userspace
must populate the type member with a value from
[`c3_isp_params_block_type`](#c.V4L.c3_isp_params_block_type "c3_isp_params_block_type"). Once the blocks have been populated
into the data buffer, the combined size of all populated blocks shall be set in
the data\_size member of [`c3_isp_params_cfg`](#c.V4L.c3_isp_params_cfg "c3_isp_params_cfg"). For example:

```
struct c3_isp_params_cfg *params =
        (struct c3_isp_params_cfg *)buffer;

params->version = C3_ISP_PARAM_BUFFER_V0;
params->data_size = 0;

void *data = (void *)params->data;

struct c3_isp_params_awb_gains *gains =
        (struct c3_isp_params_awb_gains *)data;

gains->header.type = C3_ISP_PARAMS_BLOCK_AWB_GAINS;
gains->header.flags = C3_ISP_PARAMS_BLOCK_FL_ENABLE;
gains->header.size = sizeof(struct c3_isp_params_awb_gains);

gains->gr_gain = 256;
gains->r_gain = 256;
gains->b_gain = 256;
gains->gb_gain = 256;

data += sizeof(struct c3_isp__params_awb_gains);
params->data_size += sizeof(struct c3_isp_params_awb_gains);

struct c3_isp_params_awb_config *awb_cfg =
        (struct c3_isp_params_awb_config *)data;

awb_cfg->header.type = C3_ISP_PARAMS_BLOCK_AWB_CONFIG;
awb_cfg->header.flags = C3_ISP_PARAMS_BLOCK_FL_ENABLE;
awb_cfg->header.size = sizeof(struct c3_isp_params_awb_config);

awb_cfg->tap_point = C3_ISP_AWB_STATS_TAP_BEFORE_WB;
awb_cfg->satur = 1;
awb_cfg->horiz_zones_num = 32;
awb_cfg->vert_zones_num = 24;

params->data_size += sizeof(struct c3_isp_params_awb_config);
```

## 2.13.2.3. Amlogic C3 ISP uAPI data types

struct c3\_isp\_awb\_zone\_stats
:   AWB statistics of a zone

**Definition**:

```
struct c3_isp_awb_zone_stats {
    __u16 rg;
    __u16 bg;
    __u32 pixel_sum;
};
```

**Members**

`rg`
:   the ratio of R / G in a zone

`bg`
:   the ratio of B / G in a zone

`pixel_sum`
:   the total number of pixels used in a zone

**Description**

AWB zone stats is aligned with 8 bytes

struct c3\_isp\_awb\_stats
:   Auto white balance statistics information.

**Definition**:

```
struct c3_isp_awb_stats {
    struct c3_isp_awb_zone_stats stats[C3_ISP_AWB_MAX_ZONES];
};
```

**Members**

`stats`
:   array of auto white balance statistics

**Description**

AWB statistical information of all zones.

struct c3\_isp\_ae\_zone\_stats
:   AE statistics of a zone

**Definition**:

```
struct c3_isp_ae_zone_stats {
    __u16 hist0;
    __u16 hist1;
    __u16 hist3;
    __u16 hist4;
};
```

**Members**

`hist0`
:   the global normalized pixel count for bin 0

`hist1`
:   the global normalized pixel count for bin 1

`hist3`
:   the global normalized pixel count for bin 3

`hist4`
:   the global normalized pixel count for bin 4

**Description**

AE zone stats is aligned with 8 bytes.
This is a 5-bin histogram and the total sum is normalized to 0xffff.
So hist2 = 0xffff - (hist0 + hist1 + hist3 + hist4)

struct c3\_isp\_ae\_stats
:   Exposure statistics information

**Definition**:

```
struct c3_isp_ae_stats {
    struct c3_isp_ae_zone_stats stats[C3_ISP_AE_MAX_ZONES];
    __u32 reserved[2];
    __u32 hist[1024];
};
```

**Members**

`stats`
:   array of auto exposure block statistics

`reserved`
:   undefined buffer space

`hist`
:   a 1024-bin histogram for the entire image

**Description**

AE statistical information consists of all blocks information and a 1024-bin
histogram.

struct c3\_isp\_af\_zone\_stats
:   AF statistics of a zone

**Definition**:

```
struct c3_isp_af_zone_stats {
    __u16 i2_mat;
    __u16 i4_mat;
    __u16 e4_mat;
    __u16 e4_exp : 5;
    __u16 i2_exp : 5;
    __u16 i4_exp : 6;
};
```

**Members**

`i2_mat`
:   the mantissa of zonal squared image pixel sum

`i4_mat`
:   the mantissa of zonal quartic image pixel sum

`e4_mat`
:   the mantissa of zonal multi-directional quartic edge sum

`e4_exp`
:   the exponent of zonal multi-directional quartic edge sum

`i2_exp`
:   the exponent of zonal squared image pixel sum

`i4_exp`
:   the exponent of zonal quartic image pixel sum

**Description**

AF zone stats is aligned with 8 bytes.
The zonal accumulated contrast metrics are stored in floating point format
with 16 bits mantissa and 5 or 6 bits exponent. Apart from contrast metrics
we accumulate squared image and quartic image data over the zone.

struct c3\_isp\_af\_stats
:   Auto Focus statistics information

**Definition**:

```
struct c3_isp_af_stats {
    struct c3_isp_af_zone_stats stats[C3_ISP_AF_MAX_ZONES];
    __u32 reserved[2];
};
```

**Members**

`stats`
:   array of auto focus block statistics

`reserved`
:   undefined buffer space

**Description**

AF statistical information of each zone

struct c3\_isp\_stats\_info
:   V4L2\_META\_FMT\_C3ISP\_STATS

**Definition**:

```
struct c3_isp_stats_info {
    struct c3_isp_awb_stats awb;
    struct c3_isp_ae_stats ae;
    struct c3_isp_af_stats af;
};
```

**Members**

`awb`
:   auto white balance stats

`ae`
:   auto exposure stats

`af`
:   auto focus stats

**Description**

Contains ISP statistics

enum c3\_isp\_params\_buffer\_version
:   C3 ISP parameters block versioning

**Constants**

`C3_ISP_PARAMS_BUFFER_V0`
:   First version of C3 ISP parameters block

enum c3\_isp\_params\_block\_type
:   Enumeration of C3 ISP parameter blocks

**Constants**

`C3_ISP_PARAMS_BLOCK_AWB_GAINS`
:   White balance gains

`C3_ISP_PARAMS_BLOCK_AWB_CONFIG`
:   AWB statistic format configuration for all
    blocks that control how stats are generated

`C3_ISP_PARAMS_BLOCK_AE_CONFIG`
:   AE statistic format configuration for all
    blocks that control how stats are generated

`C3_ISP_PARAMS_BLOCK_AF_CONFIG`
:   AF statistic format configuration for all
    blocks that control how stats are generated

`C3_ISP_PARAMS_BLOCK_PST_GAMMA`
:   post gamma parameters

`C3_ISP_PARAMS_BLOCK_CCM`
:   Color correction matrix parameters

`C3_ISP_PARAMS_BLOCK_CSC`
:   Color space conversion parameters

`C3_ISP_PARAMS_BLOCK_BLC`
:   Black level correction parameters

`C3_ISP_PARAMS_BLOCK_SENTINEL`
:   First non-valid block index

**Description**

Each block configures a specific processing block of the C3 ISP.
The block type allows the driver to correctly interpret the parameters block
data.

c3\_isp\_params\_block\_header

`c3_isp_params_block_header`

> > C3 ISP parameter block header
>
> **Description**
>
> This structure represents the common part of all the ISP configuration
> blocks and is identical to [`v4l2_isp_params_block_header`](v4l2-isp.html#c.v4l2_isp_params_block_header "v4l2_isp_params_block_header").
>
> The type field is one of the values enumerated by
> [`c3_isp_params_block_type`](#c.V4L.c3_isp_params_block_type "c3_isp_params_block_type") and specifies how the data should be
> interpreted by the driver.
>
> The flags field is a bitmask of per-block flags C3\_ISP\_PARAMS\_FL\_\*.

struct c3\_isp\_params\_awb\_gains
:   Gains for auto-white balance

**Definition**:

```
struct c3_isp_params_awb_gains {
    struct c3_isp_params_block_header header;
    __u16 gr_gain;
    __u16 r_gain;
    __u16 b_gain;
    __u16 gb_gain;
};
```

**Members**

`header`
:   The C3 ISP parameters block header

`gr_gain`
:   Multiplier for Gr channel (Q4.8 format)

`r_gain`
:   Multiplier for R channel (Q4.8 format)

`b_gain`
:   Multiplier for B channel (Q4.8 format)

`gb_gain`
:   Multiplier for Gb channel (Q4.8 format)

**Description**

This `struct allows` users to configure the gains for white balance.
There are four gain settings corresponding to each colour channel in
the bayer domain. All of the gains are stored in Q4.8 format.

header.type should be set to C3\_ISP\_PARAMS\_BLOCK\_AWB\_GAINS
from [`c3_isp_params_block_type`](#c.V4L.c3_isp_params_block_type "c3_isp_params_block_type")

enum c3\_isp\_params\_awb\_tap\_points
:   Tap points for the AWB statistics

**Constants**

`C3_ISP_AWB_STATS_TAP_OFE`
:   immediately after the optical frontend block

`C3_ISP_AWB_STATS_TAP_GE`
:   immediately after the green equal block

`C3_ISP_AWB_STATS_TAP_BEFORE_WB`
:   immediately before the white balance block

`C3_ISP_AWB_STATS_TAP_AFTER_WB`
:   immediately after the white balance block

struct c3\_isp\_params\_awb\_config
:   Stats settings for auto-white balance

**Definition**:

```
struct c3_isp_params_awb_config {
    struct c3_isp_params_block_header header;
    __u8 tap_point;
    __u8 satur_vald;
    __u8 horiz_zones_num;
    __u8 vert_zones_num;
    __u16 rg_min;
    __u16 rg_max;
    __u16 bg_min;
    __u16 bg_max;
    __u16 rg_low;
    __u16 rg_high;
    __u16 bg_low;
    __u16 bg_high;
    __u8 zone_weight[C3_ISP_AWB_MAX_ZONES];
    __u16 horiz_coord[C3_ISP_AWB_MAX_PT_NUM];
    __u16 vert_coord[C3_ISP_AWB_MAX_PT_NUM];
};
```

**Members**

`header`
:   the C3 ISP parameters block header

`tap_point`
:   the tap point from `enum c3_isp_params_awb_tap_point`

`satur_vald`
:   AWB statistic over saturation control
    value: 0: disable, 1: enable

`horiz_zones_num`
:   active number of hotizontal zones [0..32]

`vert_zones_num`
:   active number of vertical zones [0..24]

`rg_min`
:   minimum R/G ratio (Q4.8 format)

`rg_max`
:   maximum R/G ratio (Q4.8 format)

`bg_min`
:   minimum B/G ratio (Q4.8 format)

`bg_max`
:   maximum B/G ratio (Q4.8 format)

`rg_low`
:   R/G ratio trim low (Q4.8 format)

`rg_high`
:   R/G ratio trim hight (Q4.8 format)

`bg_low`
:   B/G ratio trim low (Q4.8 format)

`bg_high`
:   B/G ratio trim high (Q4.8 format)

`zone_weight`
:   array of weights for AWB statistics zones [0..15]

`horiz_coord`
:   the horizontal coordinate of points on the diagonal [0..2888]

`vert_coord`
:   the vertical coordinate of points on the diagonal [0..2240]

**Description**

This `struct allows` the configuration of the statistics generated for auto
white balance.

header.type should be set to C3\_ISP\_PARAMS\_BLOCK\_AWB\_CONFIG
from [`c3_isp_params_block_type`](#c.V4L.c3_isp_params_block_type "c3_isp_params_block_type")

enum c3\_isp\_params\_ae\_tap\_points
:   Tap points for the AE statistics

**Constants**

`C3_ISP_AE_STATS_TAP_GE`
:   immediately after the green equal block

`C3_ISP_AE_STATS_TAP_MLS`
:   immediately after the mesh lens shading block

struct c3\_isp\_params\_ae\_config
:   Stats settings for auto-exposure

**Definition**:

```
struct c3_isp_params_ae_config {
    struct c3_isp_params_block_header header;
    __u8 tap_point;
    __u8 horiz_zones_num;
    __u8 vert_zones_num;
    __u8 zone_weight[C3_ISP_AE_MAX_ZONES];
    __u16 horiz_coord[C3_ISP_AE_MAX_PT_NUM];
    __u16 vert_coord[C3_ISP_AE_MAX_PT_NUM];
    __u16 reserved[3];
};
```

**Members**

`header`
:   the C3 ISP parameters block header

`tap_point`
:   the tap point from `enum c3_isp_params_ae_tap_point`

`horiz_zones_num`
:   active number of horizontal zones [0..17]

`vert_zones_num`
:   active number of vertical zones [0..15]

`zone_weight`
:   array of weights for AE statistics zones [0..15]

`horiz_coord`
:   the horizontal coordinate of points on the diagonal [0..2888]

`vert_coord`
:   the vertical coordinate of points on the diagonal [0..2240]

`reserved`
:   applications must zero this array

**Description**

This `struct allows` the configuration of the statistics generated for
auto exposure.

header.type should be set to C3\_ISP\_PARAMS\_BLOCK\_AE\_CONFIG
from [`c3_isp_params_block_type`](#c.V4L.c3_isp_params_block_type "c3_isp_params_block_type")

enum c3\_isp\_params\_af\_tap\_points
:   Tap points for the AF statistics

**Constants**

`C3_ISP_AF_STATS_TAP_SNR`
:   immediately after the spatial noise reduce block

`C3_ISP_AF_STATS_TAP_DMS`
:   immediately after the demosaic block

struct c3\_isp\_params\_af\_config
:   Stats settings for auto-focus

**Definition**:

```
struct c3_isp_params_af_config {
    struct c3_isp_params_block_header header;
    __u8 tap_point;
    __u8 horiz_zones_num;
    __u8 vert_zones_num;
    __u8 reserved[5];
    __u16 horiz_coord[C3_ISP_AF_MAX_PT_NUM];
    __u16 vert_coord[C3_ISP_AF_MAX_PT_NUM];
};
```

**Members**

`header`
:   the C3 ISP parameters block header

`tap_point`
:   the tap point from `enum c3_isp_params_af_tap_point`

`horiz_zones_num`
:   active number of hotizontal zones [0..17]

`vert_zones_num`
:   active number of vertical zones [0..15]

`reserved`
:   applications must zero this array

`horiz_coord`
:   the horizontal coordinate of points on the diagonal [0..2888]

`vert_coord`
:   the vertical coordinate of points on the diagonal [0..2240]

**Description**

This `struct allows` the configuration of the statistics generated for
auto focus.

header.type should be set to C3\_ISP\_PARAMS\_BLOCK\_AF\_CONFIG
from [`c3_isp_params_block_type`](#c.V4L.c3_isp_params_block_type "c3_isp_params_block_type")

struct c3\_isp\_params\_pst\_gamma
:   Post gamma configuration

**Definition**:

```
struct c3_isp_params_pst_gamma {
    struct c3_isp_params_block_header header;
    __u16 lut[129];
    __u16 reserved[3];
};
```

**Members**

`header`
:   the C3 ISP parameters block header

`lut`
:   lookup table for P-Stitch gamma [0..1023]

`reserved`
:   applications must zero this array

**Description**

This `struct allows` the configuration of the look up table for
post gamma. The gamma curve consists of 129 points, so need to
set lut[129].

header.type should be set to C3\_ISP\_PARAMS\_BLOCK\_PST\_GAMMA
from [`c3_isp_params_block_type`](#c.V4L.c3_isp_params_block_type "c3_isp_params_block_type")

struct c3\_isp\_params\_ccm
:   ISP CCM configuration

**Definition**:

```
struct c3_isp_params_ccm {
    struct c3_isp_params_block_header header;
    __s16 matrix[3][3];
    __u16 reserved[3];
};
```

**Members**

`header`
:   the C3 ISP parameters block header

`matrix`
:   a 3 x 3 matrix used for color correction,
    the value of matrix[x][y] is orig\_value x 256. [-4096..4095]

`reserved`
:   applications must zero this array

**Description**

This `struct allows` the configuration of the matrix for
color correction. The matrix consists of 3 x 3 points,
so need to set matrix[3][3].

header.type should be set to C3\_ISP\_PARAMS\_BLOCK\_CCM
from [`c3_isp_params_block_type`](#c.V4L.c3_isp_params_block_type "c3_isp_params_block_type")

struct c3\_isp\_params\_csc
:   ISP Color Space Conversion configuration

**Definition**:

```
struct c3_isp_params_csc {
    struct c3_isp_params_block_header header;
    __s16 matrix[3][3];
    __u16 reserved[3];
};
```

**Members**

`header`
:   the C3 ISP parameters block header

`matrix`
:   a 3x3 matrix used for the color space conversion,
    the value of matrix[x][y] is orig\_value x 256. [-4096..4095]

`reserved`
:   applications must zero this array

**Description**

This `struct allows` the configuration of the matrix for color space
conversion. The matrix consists of 3 x 3 points, so need to set matrix[3][3].

header.type should be set to C3\_ISP\_PARAMS\_BLOCK\_CSC
from [`c3_isp_params_block_type`](#c.V4L.c3_isp_params_block_type "c3_isp_params_block_type")

struct c3\_isp\_params\_blc
:   ISP Black Level Correction configuration

**Definition**:

```
struct c3_isp_params_blc {
    struct c3_isp_params_block_header header;
    __u16 gr_ofst;
    __u16 r_ofst;
    __u16 b_ofst;
    __u16 gb_ofst;
};
```

**Members**

`header`
:   the C3 ISP parameters block header

`gr_ofst`
:   Gr blc offset (Q4.12 format)

`r_ofst`
:   R blc offset (Q4.12 format)

`b_ofst`
:   B blc offset (Q4.12 format)

`gb_ofst`
:   Gb blc offset(Q4.12 format)

**Description**

This `struct allows` the configuration of the block level offset for each
color channel.

header.type should be set to C3\_ISP\_PARAMS\_BLOCK\_BLC
from [`c3_isp_params_block_type`](#c.V4L.c3_isp_params_block_type "c3_isp_params_block_type")

C3\_ISP\_PARAMS\_MAX\_SIZE

`C3_ISP_PARAMS_MAX_SIZE`

> > Maximum size of all C3 ISP Parameters
>
> **Description**
>
> Though the parameters for the C3 ISP are passed as optional blocks, the
> driver still needs to know the absolute maximum size so that it can allocate
> a buffer sized appropriately to accommodate userspace attempting to set all
> possible parameters in a single frame.

struct c3\_isp\_params\_cfg
:   C3 ISP configuration parameters

**Definition**:

```
struct c3_isp_params_cfg {
    __u32 version;
    __u32 data_size;
    __u8 data[C3_ISP_PARAMS_MAX_SIZE];
};
```

**Members**

`version`
:   The C3 ISP parameters buffer version

`data_size`
:   The C3 ISP configuration data effective size, excluding this
    header

`data`
:   The C3 ISP configuration blocks data

**Description**

This is the driver-specific implementation of
[`v4l2_isp_params_buffer`](v4l2-isp.html#c.v4l2_isp_params_buffer "v4l2_isp_params_buffer").

Currently only C3\_ISP\_PARAM\_BUFFER\_V0 is supported.

The expected memory layout of the parameters buffer is:

```
+-------------------- struct c3_isp_params_cfg ---- ------------------+
| version = C3_ISP_PARAM_BUFFER_V0;                                   |
| data_size = sizeof(struct c3_isp_params_awb_gains) +                |
|              sizeof(struct c3_isp_params_awb_config);       |
| +------------------------- data  ---------------------------------+ |
| | +------------ struct c3_isp_params_awb_gains) ------------------+ |
| | | +---------  struct c3_isp_params_block_header header -----+ | | |
| | | | type = C3_ISP_PARAMS_BLOCK_AWB_GAINS;                   | | | |
| | | | flags = C3_ISP_PARAMS_BLOCK_FL_NONE;                    | | | |
| | | | size = sizeof(struct c3_isp_params_awb_gains);          | | | |
| | | +---------------------------------------------------------+ | | |
| | | gr_gain = ...;                                              | | |
| | | r_gain = ...;                                               | | |
| | | b_gain = ...;                                               | | |
| | | gb_gain = ...;                                              | | |
| | +------------------ struct c3_isp_params_awb_config ----------+ | |
| | | +---------- struct c3_isp_param_block_header header ------+ | | |
| | | | type = C3_ISP_PARAMS_BLOCK_AWB_CONFIG;                  | | | |
| | | | flags = C3_ISP_PARAMS_BLOCK_FL_NONE;                    | | | |
| | | | size = sizeof(struct c3_isp_params_awb_config)          | | | |
| | | +---------------------------------------------------------+ | | |
| | | tap_point = ...;                                            | | |
| | | satur_vald = ...;                                           | | |
| | | horiz_zones_num = ...;                                      | | |
| | | vert_zones_num = ...;                                       | | |
| | +-------------------------------------------------------------+ | |
| +-----------------------------------------------------------------+ |
+---------------------------------------------------------------------+
```
