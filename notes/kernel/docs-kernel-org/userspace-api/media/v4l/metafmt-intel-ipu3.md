# 2.13.5.V4L2_META_FMT_IPU3_PARAMS (‘ip3p’), V4L2_META_FMT_IPU3_3A (‘ip3s’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/metafmt-intel-ipu3.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.13.5. V4L2\_META\_FMT\_IPU3\_PARAMS (‘ip3p’), V4L2\_META\_FMT\_IPU3\_3A (‘ip3s’)

## 2.13.5.1. 3A statistics

The IPU3 ImgU 3A statistics accelerators collect different statistics over
an input Bayer frame. Those statistics are obtained from the “ipu3-imgu [01] 3a
stat” metadata capture video nodes, using the [`v4l2_meta_format`](dev-meta.html#c.V4L.v4l2_meta_format "v4l2_meta_format")
interface. They are formatted as described by the [`ipu3_uapi_stats_3a`](#c.V4L.ipu3_uapi_stats_3a "ipu3_uapi_stats_3a")
structure.

The statistics collected are AWB (Auto-white balance) RGBS (Red, Green, Blue and
Saturation measure) cells, AWB filter response, AF (Auto-focus) filter response,
and AE (Auto-exposure) histogram.

The struct [`ipu3_uapi_4a_config`](#c.V4L.ipu3_uapi_4a_config "ipu3_uapi_4a_config") saves all configurable parameters.

```
struct ipu3_uapi_stats_3a {
        struct ipu3_uapi_awb_raw_buffer awb_raw_buffer;
        struct ipu3_uapi_ae_raw_buffer_aligned ae_raw_buffer[IPU3_UAPI_MAX_STRIPES];
        struct ipu3_uapi_af_raw_buffer af_raw_buffer;
        struct ipu3_uapi_awb_fr_raw_buffer awb_fr_raw_buffer;
        struct ipu3_uapi_4a_config stats_4a_config;
        __u32 ae_join_buffers;
        __u8 padding[28];
        struct ipu3_uapi_stats_3a_bubble_info_per_stripe stats_3a_bubble_per_stripe;
        struct ipu3_uapi_ff_status stats_3a_status;
};
```

## 2.13.5.2. Pipeline parameters

The pipeline parameters are passed to the “ipu3-imgu [01] parameters” metadata
output video nodes, using the [`v4l2_meta_format`](dev-meta.html#c.V4L.v4l2_meta_format "v4l2_meta_format") interface. They are
formatted as described by the [`ipu3_uapi_params`](#c.V4L.ipu3_uapi_params "ipu3_uapi_params") structure.

Both 3A statistics and pipeline parameters described here are closely tied to
the underlying camera sub-system (CSS) APIs. They are usually consumed and
produced by dedicated user space libraries that comprise the important tuning
tools, thus freeing the developers from being bothered with the low level
hardware and algorithm details.

```
struct ipu3_uapi_params {
        /* Flags which of the settings below are to be applied */
        struct ipu3_uapi_flags use;

        /* Accelerator cluster parameters */
        struct ipu3_uapi_acc_param acc_param;

        /* ISP vector address space parameters */
        struct ipu3_uapi_isp_lin_vmem_params lin_vmem_params;
        struct ipu3_uapi_isp_tnr3_vmem_params tnr3_vmem_params;
        struct ipu3_uapi_isp_xnr3_vmem_params xnr3_vmem_params;

        /* ISP data memory (DMEM) parameters */
        struct ipu3_uapi_isp_tnr3_params tnr3_dmem_params;
        struct ipu3_uapi_isp_xnr3_params xnr3_dmem_params;

        /* Optical black level compensation */
        struct ipu3_uapi_obgrid_param obgrid_param;
};
```

## 2.13.5.3. Intel IPU3 ImgU uAPI data types

struct ipu3\_uapi\_grid\_config
:   Grid plane config

**Definition**:

```
struct ipu3_uapi_grid_config {
    __u8 width;
    __u8 height;
    __u16 block_width_log2:3;
    __u16 block_height_log2:3;
    __u16 height_per_slice:8;
    __u16 x_start;
    __u16 y_start;
    __u16 x_end;
    __u16 y_end;
};
```

**Members**

`width`
:   Grid horizontal dimensions, in number of grid blocks(cells).
    For AWB, the range is (16, 80).
    For AF/AE, the range is (16, 32).

`height`
:   Grid vertical dimensions, in number of grid cells.
    For AWB, the range is (16, 60).
    For AF/AE, the range is (16, 24).

`block_width_log2`
:   Log2 of the width of each cell in pixels.
    For AWB, the range is [3, 6].
    For AF/AE, the range is [3, 7].

`block_height_log2`
:   Log2 of the height of each cell in pixels.
    For AWB, the range is [3, 6].
    For AF/AE, the range is [3, 7].

`height_per_slice`
:   The number of blocks in vertical axis per slice.
    Default 2.

`x_start`
:   X value of top left corner of Region of Interest(ROI).

`y_start`
:   Y value of top left corner of ROI

`x_end`
:   X value of bottom right corner of ROI

`y_end`
:   Y value of bottom right corner of ROI

**Description**

Due to the size of total amount of collected data, most statistics
create a grid-based output, and the data is then divided into “slices”.

struct ipu3\_uapi\_awb\_set\_item
:   Memory layout for each cell in AWB

**Definition**:

```
struct ipu3_uapi_awb_set_item {
    __u8 Gr_avg;
    __u8 R_avg;
    __u8 B_avg;
    __u8 Gb_avg;
    __u8 sat_ratio;
    __u8 padding0;
    __u8 padding1;
    __u8 padding2;
};
```

**Members**

`Gr_avg`
:   Green average for red lines in the cell.

`R_avg`
:   Red average in the cell.

`B_avg`
:   Blue average in the cell.

`Gb_avg`
:   Green average for blue lines in the cell.

`sat_ratio`
:   Percentage of pixels over the thresholds specified in
    ipu3\_uapi\_awb\_config\_s, coded from 0 to 255.

`padding0`
:   Unused byte for padding.

`padding1`
:   Unused byte for padding.

`padding2`
:   Unused byte for padding.

struct ipu3\_uapi\_awb\_raw\_buffer
:   AWB raw buffer

**Definition**:

```
struct ipu3_uapi_awb_raw_buffer {
    struct ipu3_uapi_awb_set_item meta_data[IPU3_UAPI_AWB_MAX_BUFFER_SIZE];
};
```

**Members**

`meta_data`
:   buffer to hold auto white balance meta data which is
    the average values for each color channel.

struct ipu3\_uapi\_awb\_config\_s
:   AWB config

**Definition**:

```
struct ipu3_uapi_awb_config_s {
    __u16 rgbs_thr_gr;
    __u16 rgbs_thr_r;
    __u16 rgbs_thr_gb;
    __u16 rgbs_thr_b;
    struct ipu3_uapi_grid_config grid;
};
```

**Members**

`rgbs_thr_gr`
:   gr threshold value.

`rgbs_thr_r`
:   Red threshold value.

`rgbs_thr_gb`
:   gb threshold value.

`rgbs_thr_b`
:   Blue threshold value.

`grid`
:   [`ipu3_uapi_grid_config`](#c.V4L.ipu3_uapi_grid_config "ipu3_uapi_grid_config"), the default grid resolution is 16x16 cells.

**Description**

The threshold is a saturation measure range [0, 8191], 8191 is default.
Values over threshold may be optionally rejected for averaging.

struct ipu3\_uapi\_awb\_config
:   AWB config wrapper

**Definition**:

```
struct ipu3_uapi_awb_config {
    struct ipu3_uapi_awb_config_s config;
};
```

**Members**

`config`
:   config for auto white balance as defined by [`ipu3_uapi_awb_config_s`](#c.V4L.ipu3_uapi_awb_config_s "ipu3_uapi_awb_config_s")

struct ipu3\_uapi\_ae\_raw\_buffer
:   AE global weighted histogram

**Definition**:

```
struct ipu3_uapi_ae_raw_buffer {
    __u32 vals[IPU3_UAPI_AE_BINS * IPU3_UAPI_AE_COLORS];
};
```

**Members**

`vals`
:   Sum of IPU3\_UAPI\_AE\_COLORS in cell

**Description**

Each histogram contains IPU3\_UAPI\_AE\_BINS bins. Each bin has 24 bit unsigned
for counting the number of the pixel.

struct ipu3\_uapi\_ae\_raw\_buffer\_aligned
:   AE raw buffer

**Definition**:

```
struct ipu3_uapi_ae_raw_buffer_aligned {
    struct ipu3_uapi_ae_raw_buffer buff;
};
```

**Members**

`buff`
:   [`ipu3_uapi_ae_raw_buffer`](#c.V4L.ipu3_uapi_ae_raw_buffer "ipu3_uapi_ae_raw_buffer") to hold full frame meta data.

struct ipu3\_uapi\_ae\_grid\_config
:   AE weight grid

**Definition**:

```
struct ipu3_uapi_ae_grid_config {
    __u8 width;
    __u8 height;
    __u8 block_width_log2:4;
    __u8 block_height_log2:4;
    __u8 reserved0:5;
    __u8 ae_en:1;
    __u8 rst_hist_array:1;
    __u8 done_rst_hist_array:1;
    __u16 x_start;
    __u16 y_start;
    __u16 x_end;
    __u16 y_end;
};
```

**Members**

`width`
:   Grid horizontal dimensions. Value: [16, 32], default 16.

`height`
:   Grid vertical dimensions. Value: [16, 24], default 16.

`block_width_log2`
:   Log2 of the width of the grid cell, value: [3, 7].

`block_height_log2`
:   Log2 of the height of the grid cell, value: [3, 7].
    default is 3 (cell size 8x8), 4 cell per grid.

`reserved0`
:   reserved

`ae_en`
:   0: does not write to [`ipu3_uapi_ae_raw_buffer_aligned`](#c.V4L.ipu3_uapi_ae_raw_buffer_aligned "ipu3_uapi_ae_raw_buffer_aligned") array,
    1: write normally.

`rst_hist_array`
:   write 1 to trigger histogram array reset.

`done_rst_hist_array`
:   flag for histogram array reset done.

`x_start`
:   X value of top left corner of ROI, default 0.

`y_start`
:   Y value of top left corner of ROI, default 0.

`x_end`
:   X value of bottom right corner of ROI

`y_end`
:   Y value of bottom right corner of ROI

**Description**

The AE block accumulates 4 global weighted histograms(R, G, B, Y) over
a defined ROI within the frame. The contribution of each pixel into the
histogram, defined by [`ipu3_uapi_ae_weight_elem`](#c.V4L.ipu3_uapi_ae_weight_elem "ipu3_uapi_ae_weight_elem") LUT, is indexed by a grid.

struct ipu3\_uapi\_ae\_weight\_elem
:   AE weights LUT

**Definition**:

```
struct ipu3_uapi_ae_weight_elem {
    __u32 cell0:4;
    __u32 cell1:4;
    __u32 cell2:4;
    __u32 cell3:4;
    __u32 cell4:4;
    __u32 cell5:4;
    __u32 cell6:4;
    __u32 cell7:4;
};
```

**Members**

`cell0`
:   weighted histogram grid value.

`cell1`
:   weighted histogram grid value.

`cell2`
:   weighted histogram grid value.

`cell3`
:   weighted histogram grid value.

`cell4`
:   weighted histogram grid value.

`cell5`
:   weighted histogram grid value.

`cell6`
:   weighted histogram grid value.

`cell7`
:   weighted histogram grid value.

**Description**

Use weighted grid value to give a different contribution factor to each cell.
Precision u4, range [0, 15].

struct ipu3\_uapi\_ae\_ccm
:   AE coefficients for WB and CCM

**Definition**:

```
struct ipu3_uapi_ae_ccm {
    __u16 gain_gr;
    __u16 gain_r;
    __u16 gain_b;
    __u16 gain_gb;
    __s16 mat[16];
};
```

**Members**

`gain_gr`
:   WB gain factor for the gr channels. Default 256.

`gain_r`
:   WB gain factor for the r channel. Default 256.

`gain_b`
:   WB gain factor for the b channel. Default 256.

`gain_gb`
:   WB gain factor for the gb channels. Default 256.

`mat`
:   4x4 matrix that transforms Bayer quad output from WB to RGB+Y.

**Description**

Default:
:   128, 0, 0, 0,
    0, 128, 0, 0,
    0, 0, 128, 0,
    0, 0, 0, 128,

As part of the raw frame pre-process stage, the WB and color conversion need
to be applied to expose the impact of these gain operations.

struct ipu3\_uapi\_ae\_config
:   AE config

**Definition**:

```
struct ipu3_uapi_ae_config {
    struct ipu3_uapi_ae_grid_config grid_cfg;
    struct ipu3_uapi_ae_weight_elem weights[IPU3_UAPI_AE_WEIGHTS];
    struct ipu3_uapi_ae_ccm ae_ccm;
};
```

**Members**

`grid_cfg`
:   config for auto exposure statistics grid. See struct
    [`ipu3_uapi_ae_grid_config`](#c.V4L.ipu3_uapi_ae_grid_config "ipu3_uapi_ae_grid_config"), as Imgu did not support output
    auto exposure statistics, so user can ignore this configuration
    and use the RGB table in auto-whitebalance statistics instead.

`weights`
:   `IPU3_UAPI_AE_WEIGHTS` is based on 32x24 blocks in the grid.
    Each grid cell has a corresponding value in weights LUT called
    grid value, global histogram is updated based on grid value and
    pixel value.

`ae_ccm`
:   Color convert matrix pre-processing block.

**Description**

Calculate AE grid from image resolution, resample ae weights.

struct ipu3\_uapi\_af\_filter\_config
:   AF 2D filter for contrast measurements

**Definition**:

```
struct ipu3_uapi_af_filter_config {
    struct {
        __u8 a1;
        __u8 a2;
        __u8 a3;
        __u8 a4;
    } y1_coeff_0;
    struct {
        __u8 a5;
        __u8 a6;
        __u8 a7;
        __u8 a8;
    } y1_coeff_1;
    struct {
        __u8 a9;
        __u8 a10;
        __u8 a11;
        __u8 a12;
    } y1_coeff_2;
    __u32 y1_sign_vec;
    struct {
        __u8 a1;
        __u8 a2;
        __u8 a3;
        __u8 a4;
    } y2_coeff_0;
    struct {
        __u8 a5;
        __u8 a6;
        __u8 a7;
        __u8 a8;
    } y2_coeff_1;
    struct {
        __u8 a9;
        __u8 a10;
        __u8 a11;
        __u8 a12;
    } y2_coeff_2;
    __u32 y2_sign_vec;
    struct {
        __u8 y_gen_rate_gr;
        __u8 y_gen_rate_r;
        __u8 y_gen_rate_b;
        __u8 y_gen_rate_gb;
    } y_calc;
    struct {
        __u32 reserved0:8;
        __u32 y1_nf:4;
        __u32 reserved1:4;
        __u32 y2_nf:4;
        __u32 reserved2:12;
    } nf;
};
```

**Members**

`y1_coeff_0`
:   filter Y1, structure: 3x11, support both symmetry and
    anti-symmetry type. A12 is center, A1-A11 are neighbours.
    for analyzing low frequency content, used to calculate sum
    of gradients in x direction.

`y1_coeff_0.a1`
:   filter1 coefficients A1, u8, default 0.

`y1_coeff_0.a2`
:   filter1 coefficients A2, u8, default 0.

`y1_coeff_0.a3`
:   filter1 coefficients A3, u8, default 0.

`y1_coeff_0.a4`
:   filter1 coefficients A4, u8, default 0.

`y1_coeff_1`
:   Struct

`y1_coeff_1.a5`
:   filter1 coefficients A5, u8, default 0.

`y1_coeff_1.a6`
:   filter1 coefficients A6, u8, default 0.

`y1_coeff_1.a7`
:   filter1 coefficients A7, u8, default 0.

`y1_coeff_1.a8`
:   filter1 coefficients A8, u8, default 0.

`y1_coeff_2`
:   Struct

`y1_coeff_2.a9`
:   filter1 coefficients A9, u8, default 0.

`y1_coeff_2.a10`
:   filter1 coefficients A10, u8, default 0.

`y1_coeff_2.a11`
:   filter1 coefficients A11, u8, default 0.

`y1_coeff_2.a12`
:   filter1 coefficients A12, u8, default 128.

`y1_sign_vec`
:   Each bit corresponds to one coefficient sign bit,
    0: positive, 1: negative, default 0.

`y2_coeff_0`
:   Y2, same structure as Y1. For analyzing high frequency content.

`y2_coeff_0.a1`
:   filter2 coefficients A1, u8, default 0.

`y2_coeff_0.a2`
:   filter2 coefficients A2, u8, default 0.

`y2_coeff_0.a3`
:   filter2 coefficients A3, u8, default 0.

`y2_coeff_0.a4`
:   filter2 coefficients A4, u8, default 0.

`y2_coeff_1`
:   Struct

`y2_coeff_1.a5`
:   filter2 coefficients A5, u8, default 0.

`y2_coeff_1.a6`
:   filter2 coefficients A6, u8, default 0.

`y2_coeff_1.a7`
:   filter2 coefficients A7, u8, default 0.

`y2_coeff_1.a8`
:   filter2 coefficients A8, u8, default 0.

`y2_coeff_2`
:   Struct

`y2_coeff_2.a9`
:   filter1 coefficients A9, u8, default 0.

`y2_coeff_2.a10`
:   filter1 coefficients A10, u8, default 0.

`y2_coeff_2.a11`
:   filter1 coefficients A11, u8, default 0.

`y2_coeff_2.a12`
:   filter1 coefficients A12, u8, default 128.

`y2_sign_vec`
:   Each bit corresponds to one coefficient sign bit,
    0: positive, 1: negative, default 0.

`y_calc`
:   Pre-processing that converts Bayer quad to RGB+Y values to be
    used for building histogram. Range [0, 32], default 8.

`y_calc.y_gen_rate_gr`
:   Contribution ratio Gr for Y

`y_calc.y_gen_rate_r`
:   Contribution ratio R for Y

`y_calc.y_gen_rate_b`
:   Contribution ratio B for Y

`y_calc.y_gen_rate_gb`
:   Contribution ratio Gb for Y

`nf`
:   The shift right value that should be applied during the Y1/Y2 filter to
    make sure the total memory needed is 2 bytes per grid cell.

`nf.reserved0`
:   reserved

`nf.y1_nf`
:   Normalization factor for the convolution coeffs of y1,
    should be log2 of the sum of the abs values of the filter
    coeffs, default 7 (2^7 = 128).

`nf.reserved1`
:   reserved

`nf.y2_nf`
:   Normalization factor for y2, should be log2 of the sum of the
    abs values of the filter coeffs.

`nf.reserved2`
:   reserved

**Description**

> Rule:
> y\_gen\_rate\_gr + y\_gen\_rate\_r + y\_gen\_rate\_b + y\_gen\_rate\_gb = 32
> A single Y is calculated based on sum of Gr/R/B/Gb based on
> their contribution ratio.

struct ipu3\_uapi\_af\_raw\_buffer
:   AF meta data

**Definition**:

```
struct ipu3_uapi_af_raw_buffer {
    __u8 y_table[IPU3_UAPI_AF_Y_TABLE_MAX_SIZE];
};
```

**Members**

`y_table`
:   Each color component will be convolved separately with filter1
    and filter2 and the result will be summed out and averaged for
    each cell.

struct ipu3\_uapi\_af\_config\_s
:   AF config

**Definition**:

```
struct ipu3_uapi_af_config_s {
    struct ipu3_uapi_af_filter_config filter_config;
    __u8 padding[4];
    struct ipu3_uapi_grid_config grid_cfg;
};
```

**Members**

`filter_config`
:   AF uses Y1 and Y2 filters as configured in
    [`ipu3_uapi_af_filter_config`](#c.V4L.ipu3_uapi_af_filter_config "ipu3_uapi_af_filter_config")

`padding`
:   paddings

`grid_cfg`
:   See [`ipu3_uapi_grid_config`](#c.V4L.ipu3_uapi_grid_config "ipu3_uapi_grid_config"), default resolution 16x16. Use large
    grid size for large image and vice versa.

struct ipu3\_uapi\_awb\_fr\_raw\_buffer
:   AWB filter response meta data

**Definition**:

```
struct ipu3_uapi_awb_fr_raw_buffer {
    __u8 meta_data[IPU3_UAPI_AWB_FR_BAYER_TABLE_MAX_SIZE];
};
```

**Members**

`meta_data`
:   Statistics output on the grid after convolving with 1D filter.

struct ipu3\_uapi\_awb\_fr\_config\_s
:   AWB filter response config

**Definition**:

```
struct ipu3_uapi_awb_fr_config_s {
    struct ipu3_uapi_grid_config grid_cfg;
    __u8 bayer_coeff[6];
    __u16 reserved1;
    __u32 bayer_sign;
    __u8 bayer_nf;
    __u8 reserved2[7];
};
```

**Members**

`grid_cfg`
:   grid config, default 16x16.

`bayer_coeff`
:   1D Filter 1x11 center symmetry/anti-symmetry.
    coefficients defaults { 0, 0, 0, 0, 0, 128 }.
    Applied on whole image for each Bayer channel separately
    by a weighted sum of its 11x1 neighbors.

`reserved1`
:   reserved

`bayer_sign`
:   sign of filter coefficients, default 0.

`bayer_nf`
:   normalization factor for the convolution coeffs, to make sure
    total memory needed is within pre-determined range.
    NF should be the log2 of the sum of the abs values of the
    filter coeffs, range [7, 14], default 7.

`reserved2`
:   reserved

struct ipu3\_uapi\_4a\_config
:   4A config

**Definition**:

```
struct ipu3_uapi_4a_config {
    struct ipu3_uapi_awb_config_s awb_config;
    struct ipu3_uapi_ae_grid_config ae_grd_config;
    __u8 padding[20];
    struct ipu3_uapi_af_config_s af_config;
    struct ipu3_uapi_awb_fr_config_s awb_fr_config;
};
```

**Members**

`awb_config`
:   [`ipu3_uapi_awb_config_s`](#c.V4L.ipu3_uapi_awb_config_s "ipu3_uapi_awb_config_s"), default resolution 16x16

`ae_grd_config`
:   auto exposure statistics [`ipu3_uapi_ae_grid_config`](#c.V4L.ipu3_uapi_ae_grid_config "ipu3_uapi_ae_grid_config")

`padding`
:   paddings

`af_config`
:   auto focus config [`ipu3_uapi_af_config_s`](#c.V4L.ipu3_uapi_af_config_s "ipu3_uapi_af_config_s")

`awb_fr_config`
:   [`ipu3_uapi_awb_fr_config_s`](#c.V4L.ipu3_uapi_awb_fr_config_s "ipu3_uapi_awb_fr_config_s"), default resolution 16x16

struct ipu3\_uapi\_bubble\_info
:   Bubble info for host side debugging

**Definition**:

```
struct ipu3_uapi_bubble_info {
    __u32 num_of_stripes;
    __u8 padding[28];
    __u32 num_sets;
    __u8 padding1[28];
    __u32 size_of_set;
    __u8 padding2[28];
    __u32 bubble_size;
    __u8 padding3[28];
};
```

**Members**

`num_of_stripes`
:   A single frame is divided into several parts called stripes
    due to limitation on line buffer memory.
    The separation between the stripes is vertical. Each such
    stripe is processed as a single frame by the ISP pipe.

`padding`
:   padding bytes.

`num_sets`
:   number of sets.

`padding1`
:   padding bytes.

`size_of_set`
:   set size.

`padding2`
:   padding bytes.

`bubble_size`
:   is the amount of padding in the bubble expressed in “sets”.

`padding3`
:   padding bytes.

struct ipu3\_uapi\_ff\_status
:   Enable bits for each 3A fixed function

**Definition**:

```
struct ipu3_uapi_ff_status {
    __u32 awb_en;
    __u8 padding[28];
    __u32 ae_en;
    __u8 padding1[28];
    __u32 af_en;
    __u8 padding2[28];
    __u32 awb_fr_en;
    __u8 padding3[28];
};
```

**Members**

`awb_en`
:   auto white balance enable

`padding`
:   padding config

`ae_en`
:   auto exposure enable

`padding1`
:   padding config

`af_en`
:   auto focus enable

`padding2`
:   padding config

`awb_fr_en`
:   awb filter response enable bit

`padding3`
:   padding config

struct ipu3\_uapi\_stats\_3a
:   3A statistics

**Definition**:

```
struct ipu3_uapi_stats_3a {
    struct ipu3_uapi_awb_raw_buffer awb_raw_buffer;
    struct ipu3_uapi_ae_raw_buffer_aligned ae_raw_buffer[IPU3_UAPI_MAX_STRIPES];
    struct ipu3_uapi_af_raw_buffer af_raw_buffer;
    struct ipu3_uapi_awb_fr_raw_buffer awb_fr_raw_buffer;
    struct ipu3_uapi_4a_config stats_4a_config;
    __u32 ae_join_buffers;
    __u8 padding[28];
    struct ipu3_uapi_stats_3a_bubble_info_per_stripe stats_3a_bubble_per_stripe;
    struct ipu3_uapi_ff_status stats_3a_status;
};
```

**Members**

`awb_raw_buffer`
:   auto white balance meta data [`ipu3_uapi_awb_raw_buffer`](#c.V4L.ipu3_uapi_awb_raw_buffer "ipu3_uapi_awb_raw_buffer")

`ae_raw_buffer`
:   auto exposure raw data [`ipu3_uapi_ae_raw_buffer_aligned`](#c.V4L.ipu3_uapi_ae_raw_buffer_aligned "ipu3_uapi_ae_raw_buffer_aligned")
    current Imgu does not output the auto exposure statistics
    to ae\_raw\_buffer, the user such as 3A algorithm can use the
    RGB table in [`ipu3_uapi_awb_raw_buffer`](#c.V4L.ipu3_uapi_awb_raw_buffer "ipu3_uapi_awb_raw_buffer") to do auto-exposure.

`af_raw_buffer`
:   [`ipu3_uapi_af_raw_buffer`](#c.V4L.ipu3_uapi_af_raw_buffer "ipu3_uapi_af_raw_buffer") for auto focus meta data

`awb_fr_raw_buffer`
:   value as specified by [`ipu3_uapi_awb_fr_raw_buffer`](#c.V4L.ipu3_uapi_awb_fr_raw_buffer "ipu3_uapi_awb_fr_raw_buffer")

`stats_4a_config`
:   4a statistics config as defined by [`ipu3_uapi_4a_config`](#c.V4L.ipu3_uapi_4a_config "ipu3_uapi_4a_config").

`ae_join_buffers`
:   1 to use ae\_raw\_buffer.

`padding`
:   padding config

`stats_3a_bubble_per_stripe`
:   a `ipu3_uapi_stats_3a_bubble_info_per_stripe`

`stats_3a_status`
:   3a statistics status set in [`ipu3_uapi_ff_status`](#c.V4L.ipu3_uapi_ff_status "ipu3_uapi_ff_status")

struct ipu3\_uapi\_bnr\_static\_config\_wb\_gains\_config
:   White balance gains

**Definition**:

```
struct ipu3_uapi_bnr_static_config_wb_gains_config {
    __u16 gr;
    __u16 r;
    __u16 b;
    __u16 gb;
};
```

**Members**

`gr`
:   white balance gain for Gr channel.

`r`
:   white balance gain for R channel.

`b`
:   white balance gain for B channel.

`gb`
:   white balance gain for Gb channel.

**Description**

For BNR parameters WB gain factor for the three channels [Ggr, Ggb, Gb, Gr].
Their precision is U3.13 and the range is (0, 8) and the actual gain is
Gx + 1, it is typically Gx = 1.

Pout = {Pin \* (1 + Gx)}.

struct ipu3\_uapi\_bnr\_static\_config\_wb\_gains\_thr\_config
:   Threshold config

**Definition**:

```
struct ipu3_uapi_bnr_static_config_wb_gains_thr_config {
    __u8 gr;
    __u8 r;
    __u8 b;
    __u8 gb;
};
```

**Members**

`gr`
:   white balance threshold gain for Gr channel.

`r`
:   white balance threshold gain for R channel.

`b`
:   white balance threshold gain for B channel.

`gb`
:   white balance threshold gain for Gb channel.

**Description**

Defines the threshold that specifies how different a defect pixel can be from
its neighbors.(used by dynamic defect pixel correction sub block)
Precision u4.4 range [0, 8].

struct ipu3\_uapi\_bnr\_static\_config\_thr\_coeffs\_config
:   Noise model coefficients that controls noise threshold

**Definition**:

```
struct ipu3_uapi_bnr_static_config_thr_coeffs_config {
    __u32 cf:13;
    __u32 reserved0:3;
    __u32 cg:5;
    __u32 ci:5;
    __u32 reserved1:1;
    __u32 r_nf:5;
};
```

**Members**

`cf`
:   Free coefficient for threshold calculation, range [0, 8191], default 0.

`reserved0`
:   reserved

`cg`
:   Gain coefficient for threshold calculation, [0, 31], default 8.

`ci`
:   Intensity coefficient for threshold calculation. range [0, 0x1f]
    default 6.
    format: u3.2 (3 most significant bits represent whole number,
    2 least significant bits represent the fractional part
    with each count representing 0.25)
    e.g. 6 in binary format is 00110, that translates to 1.5

`reserved1`
:   reserved

`r_nf`
:   Normalization shift value for r^2 calculation, range [12, 20]
    where r is a radius of pixel [row, col] from centor of sensor.
    default 14.

**Description**

Threshold used to distinguish between noise and details.

struct ipu3\_uapi\_bnr\_static\_config\_thr\_ctrl\_shd\_config
:   Shading config

**Definition**:

```
struct ipu3_uapi_bnr_static_config_thr_ctrl_shd_config {
    __u8 gr;
    __u8 r;
    __u8 b;
    __u8 gb;
};
```

**Members**

`gr`
:   Coefficient defines lens shading gain approximation for gr channel

`r`
:   Coefficient defines lens shading gain approximation for r channel

`b`
:   Coefficient defines lens shading gain approximation for b channel

`gb`
:   Coefficient defines lens shading gain approximation for gb channel

**Description**

Parameters for noise model (NM) adaptation of BNR due to shading correction.
All above have precision of u3.3, default to 0.

struct ipu3\_uapi\_bnr\_static\_config\_opt\_center\_config
:   Optical center config

**Definition**:

```
struct ipu3_uapi_bnr_static_config_opt_center_config {
    __s32 x_reset:13;
    __u32 reserved0:3;
    __s32 y_reset:13;
    __u32 reserved2:3;
};
```

**Members**

`x_reset`
:   Reset value of X (col start - X center). Precision s12.0.

`reserved0`
:   reserved

`y_reset`
:   Reset value of Y (row start - Y center). Precision s12.0.

`reserved2`
:   reserved

**Description**

Distance from corner to optical center for NM adaptation due to shading
correction (should be calculated based on shading tables)

struct ipu3\_uapi\_bnr\_static\_config\_lut\_config
:   BNR square root lookup table

**Definition**:

```
struct ipu3_uapi_bnr_static_config_lut_config {
    __u8 values[IPU3_UAPI_BNR_LUT_SIZE];
};
```

**Members**

`values`
:   pre-calculated values of square root function.

**Description**

LUT implementation of square root operation.

struct ipu3\_uapi\_bnr\_static\_config\_bp\_ctrl\_config
:   Detect bad pixels (bp)

**Definition**:

```
struct ipu3_uapi_bnr_static_config_bp_ctrl_config {
    __u32 bp_thr_gain:5;
    __u32 reserved0:2;
    __u32 defect_mode:1;
    __u32 bp_gain:6;
    __u32 reserved1:18;
    __u32 w0_coeff:4;
    __u32 reserved2:4;
    __u32 w1_coeff:4;
    __u32 reserved3:20;
};
```

**Members**

`bp_thr_gain`
:   Defines the threshold that specifies how different a
    defect pixel can be from its neighbors. Threshold is
    dependent on de-noise threshold calculated by algorithm.
    Range [4, 31], default 4.

`reserved0`
:   reserved

`defect_mode`
:   Mode of addressed defect pixels,
    0 - single defect pixel is expected,
    1 - 2 adjacent defect pixels are expected, default 1.

`bp_gain`
:   Defines how 2nd derivation that passes through a defect pixel
    is different from 2nd derivations that pass through
    neighbor pixels. u4.2, range [0, 256], default 8.

`reserved1`
:   reserved

`w0_coeff`
:   Blending coefficient of defect pixel correction.
    Precision u4, range [0, 8], default 8.

`reserved2`
:   reserved

`w1_coeff`
:   Enable influence of incorrect defect pixel correction to be
    avoided. Precision u4, range [1, 8], default 8.

`reserved3`
:   reserved

struct ipu3\_uapi\_bnr\_static\_config\_dn\_detect\_ctrl\_config
:   Denoising config

**Definition**:

```
struct ipu3_uapi_bnr_static_config_dn_detect_ctrl_config {
    __u32 alpha:4;
    __u32 beta:4;
    __u32 gamma:4;
    __u32 reserved0:4;
    __u32 max_inf:4;
    __u32 reserved1:7;
    __u32 gd_enable:1;
    __u32 bpc_enable:1;
    __u32 bnr_enable:1;
    __u32 ff_enable:1;
    __u32 reserved2:1;
};
```

**Members**

`alpha`
:   Weight of central element of smoothing filter.

`beta`
:   Weight of peripheral elements of smoothing filter, default 4.

`gamma`
:   Weight of diagonal elements of smoothing filter, default 4.

`reserved0`
:   reserved

`max_inf`
:   Maximum increase of peripheral or diagonal element influence
    relative to the pre-defined value range: [0x5, 0xa]

`reserved1`
:   reserved

`gd_enable`
:   Green disparity enable control, 0 - disable, 1 - enable.

`bpc_enable`
:   Bad pixel correction enable control, 0 - disable, 1 - enable.

`bnr_enable`
:   Bayer noise removal enable control, 0 - disable, 1 - enable.

`ff_enable`
:   Fixed function enable, 0 - disable, 1 - enable.

`reserved2`
:   reserved

**Description**

beta and gamma parameter define the strength of the noise removal filter.
:   All above has precision u0.4, range [0, 0xf]
    format: u0.4 (no / zero bits represent whole number,
    4 bits represent the fractional part
    with each count representing 0.0625)
    e.g. 0xf translates to 0.0625x15 = 0.9375

struct ipu3\_uapi\_bnr\_static\_config\_opt\_center\_sqr\_config
:   BNR optical square

**Definition**:

```
struct ipu3_uapi_bnr_static_config_opt_center_sqr_config {
    __u32 x_sqr_reset;
    __u32 y_sqr_reset;
};
```

**Members**

`x_sqr_reset`
:   Reset value of X^2.

`y_sqr_reset`
:   Reset value of Y^2.

**Description**

Please note:

> 1. X and Y ref to
>    [`ipu3_uapi_bnr_static_config_opt_center_config`](#c.V4L.ipu3_uapi_bnr_static_config_opt_center_config "ipu3_uapi_bnr_static_config_opt_center_config")
> 2. Both structs are used in threshold formula to calculate r^2, where r
>    is a radius of pixel [row, col] from centor of sensor.

struct ipu3\_uapi\_bnr\_static\_config
:   BNR static config

**Definition**:

```
struct ipu3_uapi_bnr_static_config {
    struct ipu3_uapi_bnr_static_config_wb_gains_config wb_gains;
    struct ipu3_uapi_bnr_static_config_wb_gains_thr_config wb_gains_thr;
    struct ipu3_uapi_bnr_static_config_thr_coeffs_config thr_coeffs;
    struct ipu3_uapi_bnr_static_config_thr_ctrl_shd_config thr_ctrl_shd;
    struct ipu3_uapi_bnr_static_config_opt_center_config opt_center;
    struct ipu3_uapi_bnr_static_config_lut_config lut;
    struct ipu3_uapi_bnr_static_config_bp_ctrl_config bp_ctrl;
    struct ipu3_uapi_bnr_static_config_dn_detect_ctrl_config dn_detect_ctrl;
    __u32 column_size;
    struct ipu3_uapi_bnr_static_config_opt_center_sqr_config opt_center_sqr;
};
```

**Members**

`wb_gains`
:   white balance gains [`ipu3_uapi_bnr_static_config_wb_gains_config`](#c.V4L.ipu3_uapi_bnr_static_config_wb_gains_config "ipu3_uapi_bnr_static_config_wb_gains_config")

`wb_gains_thr`
:   white balance gains threshold as defined by
    [`ipu3_uapi_bnr_static_config_wb_gains_thr_config`](#c.V4L.ipu3_uapi_bnr_static_config_wb_gains_thr_config "ipu3_uapi_bnr_static_config_wb_gains_thr_config")

`thr_coeffs`
:   coefficients of threshold
    [`ipu3_uapi_bnr_static_config_thr_coeffs_config`](#c.V4L.ipu3_uapi_bnr_static_config_thr_coeffs_config "ipu3_uapi_bnr_static_config_thr_coeffs_config")

`thr_ctrl_shd`
:   control of shading threshold
    [`ipu3_uapi_bnr_static_config_thr_ctrl_shd_config`](#c.V4L.ipu3_uapi_bnr_static_config_thr_ctrl_shd_config "ipu3_uapi_bnr_static_config_thr_ctrl_shd_config")

`opt_center`
:   optical center [`ipu3_uapi_bnr_static_config_opt_center_config`](#c.V4L.ipu3_uapi_bnr_static_config_opt_center_config "ipu3_uapi_bnr_static_config_opt_center_config")

`lut`
:   lookup table [`ipu3_uapi_bnr_static_config_lut_config`](#c.V4L.ipu3_uapi_bnr_static_config_lut_config "ipu3_uapi_bnr_static_config_lut_config")

`bp_ctrl`
:   detect and remove bad pixels as defined in struct
    [`ipu3_uapi_bnr_static_config_bp_ctrl_config`](#c.V4L.ipu3_uapi_bnr_static_config_bp_ctrl_config "ipu3_uapi_bnr_static_config_bp_ctrl_config")

`dn_detect_ctrl`
:   detect and remove noise.
    [`ipu3_uapi_bnr_static_config_dn_detect_ctrl_config`](#c.V4L.ipu3_uapi_bnr_static_config_dn_detect_ctrl_config "ipu3_uapi_bnr_static_config_dn_detect_ctrl_config")

`column_size`
:   The number of pixels in column.

`opt_center_sqr`
:   Reset value of r^2 to optical center, see
    [`ipu3_uapi_bnr_static_config_opt_center_sqr_config`](#c.V4L.ipu3_uapi_bnr_static_config_opt_center_sqr_config "ipu3_uapi_bnr_static_config_opt_center_sqr_config").

**Description**

Above parameters and opt\_center\_sqr are used for white balance and shading.

struct ipu3\_uapi\_bnr\_static\_config\_green\_disparity
:   Correct green disparity

**Definition**:

```
struct ipu3_uapi_bnr_static_config_green_disparity {
    __u32 gd_red:6;
    __u32 reserved0:2;
    __u32 gd_green:6;
    __u32 reserved1:2;
    __u32 gd_blue:6;
    __u32 reserved2:10;
    __u32 gd_black:14;
    __u32 reserved3:2;
    __u32 gd_shading:7;
    __u32 reserved4:1;
    __u32 gd_support:2;
    __u32 reserved5:1;
    __u32 gd_clip:1;
    __u32 gd_central_weight:4;
};
```

**Members**

`gd_red`
:   Shading gain coeff for gr disparity level in bright red region.
    Precision u0.6, default 4(0.0625).

`reserved0`
:   reserved

`gd_green`
:   Shading gain coeff for gr disparity level in bright green
    region. Precision u0.6, default 4(0.0625).

`reserved1`
:   reserved

`gd_blue`
:   Shading gain coeff for gr disparity level in bright blue region.
    Precision u0.6, default 4(0.0625).

`reserved2`
:   reserved

`gd_black`
:   Maximal green disparity level in dark region (stronger disparity
    assumed to be image detail). Precision u14, default 80.

`reserved3`
:   reserved

`gd_shading`
:   Change maximal green disparity level according to square
    distance from image center.

`reserved4`
:   reserved

`gd_support`
:   Lower bound for the number of second green color pixels in
    current pixel neighborhood with less than threshold difference
    from it.

`reserved5`
:   reserved

`gd_clip`
:   Turn green disparity clip on/off, [0, 1], default 1.

`gd_central_weight`
:   Central pixel weight in 9 pixels weighted sum.

**Description**

The shading gain coeff of red, green, blue and black are used to calculate
threshold given a pixel’s color value and its coordinates in the image.

struct ipu3\_uapi\_dm\_config
:   De-mosaic parameters

**Definition**:

```
struct ipu3_uapi_dm_config {
    __u32 dm_en:1;
    __u32 ch_ar_en:1;
    __u32 fcc_en:1;
    __u32 reserved0:13;
    __u32 frame_width:16;
    __u32 gamma_sc:5;
    __u32 reserved1:3;
    __u32 lc_ctrl:5;
    __u32 reserved2:3;
    __u32 cr_param1:5;
    __u32 reserved3:3;
    __u32 cr_param2:5;
    __u32 reserved4:3;
    __u32 coring_param:5;
    __u32 reserved5:27;
};
```

**Members**

`dm_en`
:   de-mosaic enable.

`ch_ar_en`
:   Checker artifacts removal enable flag. Default 0.

`fcc_en`
:   False color correction (FCC) enable flag. Default 0.

`reserved0`
:   reserved

`frame_width`
:   do not care

`gamma_sc`
:   Sharpening coefficient (coefficient of 2-d derivation of
    complementary color in Hamilton-Adams interpolation).
    u5, range [0, 31], default 8.

`reserved1`
:   reserved

`lc_ctrl`
:   Parameter that controls weights of Chroma Homogeneity metric
    in calculation of final homogeneity metric.
    u5, range [0, 31], default 7.

`reserved2`
:   reserved

`cr_param1`
:   First parameter that defines Checker artifact removal
    feature gain. Precision u5, range [0, 31], default 8.

`reserved3`
:   reserved

`cr_param2`
:   Second parameter that defines Checker artifact removal
    feature gain. Precision u5, range [0, 31], default 8.

`reserved4`
:   reserved

`coring_param`
:   Defines power of false color correction operation.
    low for preserving edge colors, high for preserving gray
    edge artifacts.
    Precision u1.4, range [0, 1.9375], default 4 (0.25).

`reserved5`
:   reserved

**Description**

The demosaic fixed function block is responsible to covert Bayer(mosaiced)
images into color images based on demosaicing algorithm.

struct ipu3\_uapi\_ccm\_mat\_config
:   Color correction matrix

**Definition**:

```
struct ipu3_uapi_ccm_mat_config {
    __s16 coeff_m11;
    __s16 coeff_m12;
    __s16 coeff_m13;
    __s16 coeff_o_r;
    __s16 coeff_m21;
    __s16 coeff_m22;
    __s16 coeff_m23;
    __s16 coeff_o_g;
    __s16 coeff_m31;
    __s16 coeff_m32;
    __s16 coeff_m33;
    __s16 coeff_o_b;
};
```

**Members**

`coeff_m11`
:   CCM 3x3 coefficient, range [-65536, 65535]

`coeff_m12`
:   CCM 3x3 coefficient, range [-8192, 8191]

`coeff_m13`
:   CCM 3x3 coefficient, range [-32768, 32767]

`coeff_o_r`
:   Bias 3x1 coefficient, range [-8191, 8181]

`coeff_m21`
:   CCM 3x3 coefficient, range [-32767, 32767]

`coeff_m22`
:   CCM 3x3 coefficient, range [-8192, 8191]

`coeff_m23`
:   CCM 3x3 coefficient, range [-32768, 32767]

`coeff_o_g`
:   Bias 3x1 coefficient, range [-8191, 8181]

`coeff_m31`
:   CCM 3x3 coefficient, range [-32768, 32767]

`coeff_m32`
:   CCM 3x3 coefficient, range [-8192, 8191]

`coeff_m33`
:   CCM 3x3 coefficient, range [-32768, 32767]

`coeff_o_b`
:   Bias 3x1 coefficient, range [-8191, 8181]

**Description**

Transform sensor specific color space to standard sRGB by applying 3x3 matrix
and adding a bias vector O. The transformation is basically a rotation and
translation in the 3-dimensional color spaces. Here are the defaults:

> 9775, -2671, 1087, 0
> -1071, 8303, 815, 0
> -23, -7887, 16103, 0

struct ipu3\_uapi\_gamma\_corr\_ctrl
:   Gamma correction

**Definition**:

```
struct ipu3_uapi_gamma_corr_ctrl {
    __u32 enable:1;
    __u32 reserved:31;
};
```

**Members**

`enable`
:   gamma correction enable.

`reserved`
:   reserved

struct ipu3\_uapi\_gamma\_corr\_lut
:   Per-pixel tone mapping implemented as LUT.

**Definition**:

```
struct ipu3_uapi_gamma_corr_lut {
    __u16 lut[IPU3_UAPI_GAMMA_CORR_LUT_ENTRIES];
};
```

**Members**

`lut`
:   256 tabulated values of the gamma function. LUT[1].. LUT[256]
    format u13.0, range [0, 8191].

**Description**

The tone mapping operation is done by a Piece wise linear graph
that is implemented as a lookup table(LUT). The pixel component input
intensity is the X-axis of the graph which is the table entry.

struct ipu3\_uapi\_gamma\_config
:   Gamma config

**Definition**:

```
struct ipu3_uapi_gamma_config {
    struct ipu3_uapi_gamma_corr_ctrl gc_ctrl;
    struct ipu3_uapi_gamma_corr_lut gc_lut;
};
```

**Members**

`gc_ctrl`
:   control of gamma correction [`ipu3_uapi_gamma_corr_ctrl`](#c.V4L.ipu3_uapi_gamma_corr_ctrl "ipu3_uapi_gamma_corr_ctrl")

`gc_lut`
:   lookup table of gamma correction [`ipu3_uapi_gamma_corr_lut`](#c.V4L.ipu3_uapi_gamma_corr_lut "ipu3_uapi_gamma_corr_lut")

struct ipu3\_uapi\_csc\_mat\_config
:   Color space conversion matrix config

**Definition**:

```
struct ipu3_uapi_csc_mat_config {
    __s16 coeff_c11;
    __s16 coeff_c12;
    __s16 coeff_c13;
    __s16 coeff_b1;
    __s16 coeff_c21;
    __s16 coeff_c22;
    __s16 coeff_c23;
    __s16 coeff_b2;
    __s16 coeff_c31;
    __s16 coeff_c32;
    __s16 coeff_c33;
    __s16 coeff_b3;
};
```

**Members**

`coeff_c11`
:   Conversion matrix value, format s0.14, range [-16384, 16383].

`coeff_c12`
:   Conversion matrix value, format s0.14, range [-8192, 8191].

`coeff_c13`
:   Conversion matrix value, format s0.14, range [-16384, 16383].

`coeff_b1`
:   Bias 3x1 coefficient, s13.0 range [-8192, 8191].

`coeff_c21`
:   Conversion matrix value, format s0.14, range [-16384, 16383].

`coeff_c22`
:   Conversion matrix value, format s0.14, range [-8192, 8191].

`coeff_c23`
:   Conversion matrix value, format s0.14, range [-16384, 16383].

`coeff_b2`
:   Bias 3x1 coefficient, s13.0 range [-8192, 8191].

`coeff_c31`
:   Conversion matrix value, format s0.14, range [-16384, 16383].

`coeff_c32`
:   Conversion matrix value, format s0.14, range [-8192, 8191].

`coeff_c33`
:   Conversion matrix value, format s0.14, range [-16384, 16383].

`coeff_b3`
:   Bias 3x1 coefficient, s13.0 range [-8192, 8191].

**Description**

To transform each pixel from RGB to YUV (Y - brightness/luminance,
UV -chroma) by applying the pixel’s values by a 3x3 matrix and adding an
optional bias 3x1 vector. Here are the default values for the matrix:

> 4898, 9617, 1867, 0,
> -2410, -4732, 7143, 0,
> 10076, -8437, -1638, 0,
>
> (i.e. for real number 0.299, 0.299 \* 2^14 becomes 4898.)

struct ipu3\_uapi\_cds\_params
:   Chroma down-scaling

**Definition**:

```
struct ipu3_uapi_cds_params {
    __u32 ds_c00:2;
    __u32 ds_c01:2;
    __u32 ds_c02:2;
    __u32 ds_c03:2;
    __u32 ds_c10:2;
    __u32 ds_c11:2;
    __u32 ds_c12:2;
    __u32 ds_c13:2;
    __u32 ds_nf:5;
    __u32 reserved0:3;
    __u32 csc_en:1;
    __u32 uv_bin_output:1;
    __u32 reserved1:6;
};
```

**Members**

`ds_c00`
:   range [0, 3]

`ds_c01`
:   range [0, 3]

`ds_c02`
:   range [0, 3]

`ds_c03`
:   range [0, 3]

`ds_c10`
:   range [0, 3]

`ds_c11`
:   range [0, 3]

`ds_c12`
:   range [0, 3]

`ds_c13`
:   range [0, 3]

`ds_nf`
:   Normalization factor for Chroma output downscaling filter,
    range 0,4, default 2.

`reserved0`
:   reserved

`csc_en`
:   Color space conversion enable

`uv_bin_output`
:   0: output YUV 4.2.0, 1: output YUV 4.2.2(default).

`reserved1`
:   reserved

**Description**

In case user does not provide, above 4x2 filter will use following defaults:
:   1, 3, 3, 1,
    1, 3, 3, 1,

struct ipu3\_uapi\_shd\_grid\_config
:   Bayer shading(darkening) correction

**Definition**:

```
struct ipu3_uapi_shd_grid_config {
    __u8 width;
    __u8 height;
    __u8 block_width_log2:3;
    __u8 reserved0:1;
    __u8 block_height_log2:3;
    __u8 reserved1:1;
    __u8 grid_height_per_slice;
    __s16 x_start;
    __s16 y_start;
};
```

**Members**

`width`
:   Grid horizontal dimensions, u8, [8, 128], default 73

`height`
:   Grid vertical dimensions, u8, [8, 128], default 56

`block_width_log2`
:   Log2 of the width of the grid cell in pixel count
    u4, [0, 15], default value 5.

`reserved0`
:   reserved

`block_height_log2`
:   Log2 of the height of the grid cell in pixel count
    u4, [0, 15], default value 6.

`reserved1`
:   reserved

`grid_height_per_slice`
:   SHD\_MAX\_CELLS\_PER\_SET/width.
    (with SHD\_MAX\_CELLS\_PER\_SET = 146).

`x_start`
:   X value of top left corner of sensor relative to ROI
    s13, [-4096, 0], default 0, only negative values.

`y_start`
:   Y value of top left corner of sensor relative to ROI
    s13, [-4096, 0], default 0, only negative values.

struct ipu3\_uapi\_shd\_general\_config
:   Shading general config

**Definition**:

```
struct ipu3_uapi_shd_general_config {
    __u32 init_set_vrt_offst_ul:8;
    __u32 shd_enable:1;
    __u32 gain_factor:2;
    __u32 reserved:21;
};
```

**Members**

`init_set_vrt_offst_ul`
:   set vertical offset,
    y\_start >> block\_height\_log2 % grid\_height\_per\_slice.

`shd_enable`
:   shading enable.

`gain_factor`
:   Gain factor. Shift calculated anti shading value. Precision u2.
    0x0 - gain factor [1, 5], means no shift interpolated value.
    0x1 - gain factor [1, 9], means shift interpolated by 1.
    0x2 - gain factor [1, 17], means shift interpolated by 2.

`reserved`
:   reserved

**Description**

Correction is performed by multiplying a gain factor for each of the 4 Bayer
channels as a function of the pixel location in the sensor.

struct ipu3\_uapi\_shd\_black\_level\_config
:   Black level correction

**Definition**:

```
struct ipu3_uapi_shd_black_level_config {
    __s16 bl_r;
    __s16 bl_gr;
    __s16 bl_gb;
    __s16 bl_b;
};
```

**Members**

`bl_r`
:   Bios values for green red. s11 range [-2048, 2047].

`bl_gr`
:   Bios values for green blue. s11 range [-2048, 2047].

`bl_gb`
:   Bios values for red. s11 range [-2048, 2047].

`bl_b`
:   Bios values for blue. s11 range [-2048, 2047].

struct ipu3\_uapi\_shd\_config\_static
:   Shading config static

**Definition**:

```
struct ipu3_uapi_shd_config_static {
    struct ipu3_uapi_shd_grid_config grid;
    struct ipu3_uapi_shd_general_config general;
    struct ipu3_uapi_shd_black_level_config black_level;
};
```

**Members**

`grid`
:   shading grid config [`ipu3_uapi_shd_grid_config`](#c.V4L.ipu3_uapi_shd_grid_config "ipu3_uapi_shd_grid_config")

`general`
:   shading general config [`ipu3_uapi_shd_general_config`](#c.V4L.ipu3_uapi_shd_general_config "ipu3_uapi_shd_general_config")

`black_level`
:   black level config for shading correction as defined by
    [`ipu3_uapi_shd_black_level_config`](#c.V4L.ipu3_uapi_shd_black_level_config "ipu3_uapi_shd_black_level_config")

struct ipu3\_uapi\_shd\_lut
:   Shading gain factor lookup table.

**Definition**:

```
struct ipu3_uapi_shd_lut {
    struct {
        struct {
            __u16 r;
            __u16 gr;
        } r_and_gr[IPU3_UAPI_SHD_MAX_CELLS_PER_SET];
        __u8 reserved1[24];
        struct {
            __u16 gb;
            __u16 b;
        } gb_and_b[IPU3_UAPI_SHD_MAX_CELLS_PER_SET];
        __u8 reserved2[24];
    } sets[IPU3_UAPI_SHD_MAX_CFG_SETS];
};
```

**Members**

`sets`
:   array

`sets.r_and_gr`
:   Red and GreenR Lookup table.

`sets.r_and_gr.r`
:   Red shading factor.

`sets.r_and_gr.gr`
:   GreenR shading factor.

`sets.reserved1`
:   reserved

`sets.gb_and_b`
:   GreenB and Blue Lookup table.

`sets.gb_and_b.gb`
:   GreenB shading factor.

`sets.gb_and_b.b`
:   Blue shading factor.

`sets.reserved2`
:   reserved

**Description**

Map to shading correction LUT register set.

struct ipu3\_uapi\_shd\_config
:   Shading config

**Definition**:

```
struct ipu3_uapi_shd_config {
    struct ipu3_uapi_shd_config_static shd;
    struct ipu3_uapi_shd_lut shd_lut;
};
```

**Members**

`shd`
:   shading static config, see [`ipu3_uapi_shd_config_static`](#c.V4L.ipu3_uapi_shd_config_static "ipu3_uapi_shd_config_static")

`shd_lut`
:   shading lookup table [`ipu3_uapi_shd_lut`](#c.V4L.ipu3_uapi_shd_lut "ipu3_uapi_shd_lut")

struct ipu3\_uapi\_iefd\_cux2
:   IEFd Config Unit 2 parameters

**Definition**:

```
struct ipu3_uapi_iefd_cux2 {
    __u32 x0:9;
    __u32 x1:9;
    __u32 a01:9;
    __u32 b01:5;
};
```

**Members**

`x0`
:   X0 point of Config Unit, u9.0, default 0.

`x1`
:   X1 point of Config Unit, u9.0, default 0.

`a01`
:   Slope A of Config Unit, s4.4, default 0.

`b01`
:   Slope B, always 0.

**Description**

Calculate weight for blending directed and non-directed denoise elements

**Note**

Each instance of Config Unit needs X coordinate of n points and
slope A factor between points calculated by driver based on calibration
parameters.

All CU inputs are unsigned, they will be converted to signed when written
to register, i.e. a01 will be written to 9 bit register in s4.4 format.
The data precision s4.4 means 4 bits for integer parts and 4 bits for the
fractional part, the first bit indicates positive or negative value.
For userspace software (commonly the imaging library), the computation for
the CU slope values should be based on the slope resolution 1/16 (binary
0.0001 - the minimal interval value), the slope value range is [-256, +255].
This applies to [`ipu3_uapi_iefd_cux6_ed`](#c.V4L.ipu3_uapi_iefd_cux6_ed "ipu3_uapi_iefd_cux6_ed"), [`ipu3_uapi_iefd_cux2_1`](#c.V4L.ipu3_uapi_iefd_cux2_1 "ipu3_uapi_iefd_cux2_1"),
[`ipu3_uapi_iefd_cux2_1`](#c.V4L.ipu3_uapi_iefd_cux2_1 "ipu3_uapi_iefd_cux2_1"), [`ipu3_uapi_iefd_cux4`](#c.V4L.ipu3_uapi_iefd_cux4 "ipu3_uapi_iefd_cux4") and [`ipu3_uapi_iefd_cux6_rad`](#c.V4L.ipu3_uapi_iefd_cux6_rad "ipu3_uapi_iefd_cux6_rad").

struct ipu3\_uapi\_iefd\_cux6\_ed
:   Calculate power of non-directed sharpening element, Config Unit 6 for edge detail (ED).

**Definition**:

```
struct ipu3_uapi_iefd_cux6_ed {
    __u32 x0:9;
    __u32 x1:9;
    __u32 x2:9;
    __u32 reserved0:5;
    __u32 x3:9;
    __u32 x4:9;
    __u32 x5:9;
    __u32 reserved1:5;
    __u32 a01:9;
    __u32 a12:9;
    __u32 a23:9;
    __u32 reserved2:5;
    __u32 a34:9;
    __u32 a45:9;
    __u32 reserved3:14;
    __u32 b01:9;
    __u32 b12:9;
    __u32 b23:9;
    __u32 reserved4:5;
    __u32 b34:9;
    __u32 b45:9;
    __u32 reserved5:14;
};
```

**Members**

`x0`
:   X coordinate of point 0, u9.0, default 0.

`x1`
:   X coordinate of point 1, u9.0, default 0.

`x2`
:   X coordinate of point 2, u9.0, default 0.

`reserved0`
:   reserved

`x3`
:   X coordinate of point 3, u9.0, default 0.

`x4`
:   X coordinate of point 4, u9.0, default 0.

`x5`
:   X coordinate of point 5, u9.0, default 0.

`reserved1`
:   reserved

`a01`
:   slope A points 01, s4.4, default 0.

`a12`
:   slope A points 12, s4.4, default 0.

`a23`
:   slope A points 23, s4.4, default 0.

`reserved2`
:   reserved

`a34`
:   slope A points 34, s4.4, default 0.

`a45`
:   slope A points 45, s4.4, default 0.

`reserved3`
:   reserved

`b01`
:   slope B points 01, s4.4, default 0.

`b12`
:   slope B points 12, s4.4, default 0.

`b23`
:   slope B points 23, s4.4, default 0.

`reserved4`
:   reserved

`b34`
:   slope B points 34, s4.4, default 0.

`b45`
:   slope B points 45, s4.4, default 0.

`reserved5`
:   reserved.

struct ipu3\_uapi\_iefd\_cux2\_1
:   Calculate power of non-directed denoise element apply.

**Definition**:

```
struct ipu3_uapi_iefd_cux2_1 {
    __u32 x0:9;
    __u32 x1:9;
    __u32 a01:9;
    __u32 reserved1:5;
    __u32 b01:8;
    __u32 reserved2:24;
};
```

**Members**

`x0`
:   X0 point of Config Unit, u9.0, default 0.

`x1`
:   X1 point of Config Unit, u9.0, default 0.

`a01`
:   Slope A of Config Unit, s4.4, default 0.

`reserved1`
:   reserved

`b01`
:   offset B0 of Config Unit, u7.0, default 0.

`reserved2`
:   reserved

struct ipu3\_uapi\_iefd\_cux4
:   Calculate power of non-directed sharpening element.

**Definition**:

```
struct ipu3_uapi_iefd_cux4 {
    __u32 x0:9;
    __u32 x1:9;
    __u32 x2:9;
    __u32 reserved0:5;
    __u32 x3:9;
    __u32 a01:9;
    __u32 a12:9;
    __u32 reserved1:5;
    __u32 a23:9;
    __u32 b01:8;
    __u32 b12:8;
    __u32 reserved2:7;
    __u32 b23:8;
    __u32 reserved3:24;
};
```

**Members**

`x0`
:   X0 point of Config Unit, u9.0, default 0.

`x1`
:   X1 point of Config Unit, u9.0, default 0.

`x2`
:   X2 point of Config Unit, u9.0, default 0.

`reserved0`
:   reserved

`x3`
:   X3 point of Config Unit, u9.0, default 0.

`a01`
:   Slope A0 of Config Unit, s4.4, default 0.

`a12`
:   Slope A1 of Config Unit, s4.4, default 0.

`reserved1`
:   reserved

`a23`
:   Slope A2 of Config Unit, s4.4, default 0.

`b01`
:   Offset B0 of Config Unit, s7.0, default 0.

`b12`
:   Offset B1 of Config Unit, s7.0, default 0.

`reserved2`
:   reserved

`b23`
:   Offset B2 of Config Unit, s7.0, default 0.

`reserved3`
:   reserved

struct ipu3\_uapi\_iefd\_cux6\_rad
:   Radial Config Unit (CU)

**Definition**:

```
struct ipu3_uapi_iefd_cux6_rad {
    __u32 x0:8;
    __u32 x1:8;
    __u32 x2:8;
    __u32 x3:8;
    __u32 x4:8;
    __u32 x5:8;
    __u32 reserved1:16;
    __u32 a01:16;
    __u32 a12:16;
    __u32 a23:16;
    __u32 a34:16;
    __u32 a45:16;
    __u32 reserved2:16;
    __u32 b01:10;
    __u32 b12:10;
    __u32 b23:10;
    __u32 reserved4:2;
    __u32 b34:10;
    __u32 b45:10;
    __u32 reserved5:12;
};
```

**Members**

`x0`
:   x0 points of Config Unit radial, u8.0

`x1`
:   x1 points of Config Unit radial, u8.0

`x2`
:   x2 points of Config Unit radial, u8.0

`x3`
:   x3 points of Config Unit radial, u8.0

`x4`
:   x4 points of Config Unit radial, u8.0

`x5`
:   x5 points of Config Unit radial, u8.0

`reserved1`
:   reserved

`a01`
:   Slope A of Config Unit radial, s7.8

`a12`
:   Slope A of Config Unit radial, s7.8

`a23`
:   Slope A of Config Unit radial, s7.8

`a34`
:   Slope A of Config Unit radial, s7.8

`a45`
:   Slope A of Config Unit radial, s7.8

`reserved2`
:   reserved

`b01`
:   Slope B of Config Unit radial, s9.0

`b12`
:   Slope B of Config Unit radial, s9.0

`b23`
:   Slope B of Config Unit radial, s9.0

`reserved4`
:   reserved

`b34`
:   Slope B of Config Unit radial, s9.0

`b45`
:   Slope B of Config Unit radial, s9.0

`reserved5`
:   reserved

struct ipu3\_uapi\_yuvp1\_iefd\_cfg\_units
:   IEFd Config Units parameters

**Definition**:

```
struct ipu3_uapi_yuvp1_iefd_cfg_units {
    struct ipu3_uapi_iefd_cux2 cu_1;
    struct ipu3_uapi_iefd_cux6_ed cu_ed;
    struct ipu3_uapi_iefd_cux2 cu_3;
    struct ipu3_uapi_iefd_cux2_1 cu_5;
    struct ipu3_uapi_iefd_cux4 cu_6;
    struct ipu3_uapi_iefd_cux2 cu_7;
    struct ipu3_uapi_iefd_cux4 cu_unsharp;
    struct ipu3_uapi_iefd_cux6_rad cu_radial;
    struct ipu3_uapi_iefd_cux2 cu_vssnlm;
};
```

**Members**

`cu_1`
:   calculate weight for blending directed and
    non-directed denoise elements. See [`ipu3_uapi_iefd_cux2`](#c.V4L.ipu3_uapi_iefd_cux2 "ipu3_uapi_iefd_cux2")

`cu_ed`
:   calculate power of non-directed sharpening element, see
    [`ipu3_uapi_iefd_cux6_ed`](#c.V4L.ipu3_uapi_iefd_cux6_ed "ipu3_uapi_iefd_cux6_ed")

`cu_3`
:   calculate weight for blending directed and
    non-directed denoise elements. A [`ipu3_uapi_iefd_cux2`](#c.V4L.ipu3_uapi_iefd_cux2 "ipu3_uapi_iefd_cux2")

`cu_5`
:   calculate power of non-directed denoise element apply, use
    [`ipu3_uapi_iefd_cux2_1`](#c.V4L.ipu3_uapi_iefd_cux2_1 "ipu3_uapi_iefd_cux2_1")

`cu_6`
:   calculate power of non-directed sharpening element. See
    [`ipu3_uapi_iefd_cux4`](#c.V4L.ipu3_uapi_iefd_cux4 "ipu3_uapi_iefd_cux4")

`cu_7`
:   calculate weight for blending directed and
    non-directed denoise elements. Use [`ipu3_uapi_iefd_cux2`](#c.V4L.ipu3_uapi_iefd_cux2 "ipu3_uapi_iefd_cux2")

`cu_unsharp`
:   Config Unit of unsharp [`ipu3_uapi_iefd_cux4`](#c.V4L.ipu3_uapi_iefd_cux4 "ipu3_uapi_iefd_cux4")

`cu_radial`
:   Config Unit of radial [`ipu3_uapi_iefd_cux6_rad`](#c.V4L.ipu3_uapi_iefd_cux6_rad "ipu3_uapi_iefd_cux6_rad")

`cu_vssnlm`
:   Config Unit of vssnlm [`ipu3_uapi_iefd_cux2`](#c.V4L.ipu3_uapi_iefd_cux2 "ipu3_uapi_iefd_cux2")

struct ipu3\_uapi\_yuvp1\_iefd\_config\_s
:   IEFd config

**Definition**:

```
struct ipu3_uapi_yuvp1_iefd_config_s {
    __u32 horver_diag_coeff:7;
    __u32 reserved0:1;
    __u32 clamp_stitch:6;
    __u32 reserved1:2;
    __u32 direct_metric_update:5;
    __u32 reserved2:3;
    __u32 ed_horver_diag_coeff:7;
    __u32 reserved3:1;
};
```

**Members**

`horver_diag_coeff`
:   Gradient compensation. Compared with vertical /
    horizontal (0 / 90 degree), coefficient of diagonal (45 /
    135 degree) direction should be corrected by approx.
    1/sqrt(2).

`reserved0`
:   reserved

`clamp_stitch`
:   Slope to stitch between clamped and unclamped edge values

`reserved1`
:   reserved

`direct_metric_update`
:   Update coeff for direction metric

`reserved2`
:   reserved

`ed_horver_diag_coeff`
:   Radial Coefficient that compensates for
    different distance for vertical/horizontal and
    diagonal gradient calculation (approx. 1/sqrt(2))

`reserved3`
:   reserved

struct ipu3\_uapi\_yuvp1\_iefd\_control
:   IEFd control

**Definition**:

```
struct ipu3_uapi_yuvp1_iefd_control {
    __u32 iefd_en:1;
    __u32 denoise_en:1;
    __u32 direct_smooth_en:1;
    __u32 rad_en:1;
    __u32 vssnlm_en:1;
    __u32 reserved:27;
};
```

**Members**

`iefd_en`
:   Enable IEFd

`denoise_en`
:   Enable denoise

`direct_smooth_en`
:   Enable directional smooth

`rad_en`
:   Enable radial update

`vssnlm_en`
:   Enable VSSNLM output filter

`reserved`
:   reserved

struct ipu3\_uapi\_sharp\_cfg
:   Sharpening config

**Definition**:

```
struct ipu3_uapi_sharp_cfg {
    __u32 nega_lmt_txt:13;
    __u32 reserved0:19;
    __u32 posi_lmt_txt:13;
    __u32 reserved1:19;
    __u32 nega_lmt_dir:13;
    __u32 reserved2:19;
    __u32 posi_lmt_dir:13;
    __u32 reserved3:19;
};
```

**Members**

`nega_lmt_txt`
:   Sharpening limit for negative overshoots for texture.

`reserved0`
:   reserved

`posi_lmt_txt`
:   Sharpening limit for positive overshoots for texture.

`reserved1`
:   reserved

`nega_lmt_dir`
:   Sharpening limit for negative overshoots for direction (edge).

`reserved2`
:   reserved

`posi_lmt_dir`
:   Sharpening limit for positive overshoots for direction (edge).

`reserved3`
:   reserved

**Description**

Fixed point type u13.0, range [0, 8191].

struct ipu3\_uapi\_far\_w
:   Sharpening config for far sub-group

**Definition**:

```
struct ipu3_uapi_far_w {
    __u32 dir_shrp:7;
    __u32 reserved0:1;
    __u32 dir_dns:7;
    __u32 reserved1:1;
    __u32 ndir_dns_powr:7;
    __u32 reserved2:9;
};
```

**Members**

`dir_shrp`
:   Weight of wide direct sharpening, u1.6, range [0, 64], default 64.

`reserved0`
:   reserved

`dir_dns`
:   Weight of wide direct denoising, u1.6, range [0, 64], default 0.

`reserved1`
:   reserved

`ndir_dns_powr`
:   Power of non-direct denoising,
    Precision u1.6, range [0, 64], default 64.

`reserved2`
:   reserved

struct ipu3\_uapi\_unsharp\_cfg
:   Unsharp config

**Definition**:

```
struct ipu3_uapi_unsharp_cfg {
    __u32 unsharp_weight:7;
    __u32 reserved0:1;
    __u32 unsharp_amount:9;
    __u32 reserved1:15;
};
```

**Members**

`unsharp_weight`
:   Unsharp mask blending weight.
    u1.6, range [0, 64], default 16.
    0 - disabled, 64 - use only unsharp.

`reserved0`
:   reserved

`unsharp_amount`
:   Unsharp mask amount, u4.5, range [0, 511], default 0.

`reserved1`
:   reserved

struct ipu3\_uapi\_yuvp1\_iefd\_shrp\_cfg
:   IEFd sharpness config

**Definition**:

```
struct ipu3_uapi_yuvp1_iefd_shrp_cfg {
    struct ipu3_uapi_sharp_cfg cfg;
    struct ipu3_uapi_far_w far_w;
    struct ipu3_uapi_unsharp_cfg unshrp_cfg;
};
```

**Members**

`cfg`
:   sharpness config [`ipu3_uapi_sharp_cfg`](#c.V4L.ipu3_uapi_sharp_cfg "ipu3_uapi_sharp_cfg")

`far_w`
:   wide range config, value as specified by [`ipu3_uapi_far_w`](#c.V4L.ipu3_uapi_far_w "ipu3_uapi_far_w"):
    The 5x5 environment is separated into 2 sub-groups, the 3x3 nearest
    neighbors (8 pixels called Near), and the second order neighborhood
    around them (16 pixels called Far).

`unshrp_cfg`
:   unsharpness config. [`ipu3_uapi_unsharp_cfg`](#c.V4L.ipu3_uapi_unsharp_cfg "ipu3_uapi_unsharp_cfg")

struct ipu3\_uapi\_unsharp\_coef0
:   Unsharp mask coefficients

**Definition**:

```
struct ipu3_uapi_unsharp_coef0 {
    __u32 c00:9;
    __u32 c01:9;
    __u32 c02:9;
    __u32 reserved:5;
};
```

**Members**

`c00`
:   Coeff11, s0.8, range [-255, 255], default 1.

`c01`
:   Coeff12, s0.8, range [-255, 255], default 5.

`c02`
:   Coeff13, s0.8, range [-255, 255], default 9.

`reserved`
:   reserved

**Description**

Configurable registers for common sharpening support.

struct ipu3\_uapi\_unsharp\_coef1
:   Unsharp mask coefficients

**Definition**:

```
struct ipu3_uapi_unsharp_coef1 {
    __u32 c11:9;
    __u32 c12:9;
    __u32 c22:9;
    __u32 reserved:5;
};
```

**Members**

`c11`
:   Coeff22, s0.8, range [-255, 255], default 29.

`c12`
:   Coeff23, s0.8, range [-255, 255], default 55.

`c22`
:   Coeff33, s0.8, range [-255, 255], default 96.

`reserved`
:   reserved

struct ipu3\_uapi\_yuvp1\_iefd\_unshrp\_cfg
:   Unsharp mask config

**Definition**:

```
struct ipu3_uapi_yuvp1_iefd_unshrp_cfg {
    struct ipu3_uapi_unsharp_coef0 unsharp_coef0;
    struct ipu3_uapi_unsharp_coef1 unsharp_coef1;
};
```

**Members**

`unsharp_coef0`
:   unsharp coefficient 0 config. See [`ipu3_uapi_unsharp_coef0`](#c.V4L.ipu3_uapi_unsharp_coef0 "ipu3_uapi_unsharp_coef0")

`unsharp_coef1`
:   unsharp coefficient 1 config. See [`ipu3_uapi_unsharp_coef1`](#c.V4L.ipu3_uapi_unsharp_coef1 "ipu3_uapi_unsharp_coef1")

struct ipu3\_uapi\_radial\_reset\_xy
:   Radial coordinate reset

**Definition**:

```
struct ipu3_uapi_radial_reset_xy {
    __s32 x:13;
    __u32 reserved0:3;
    __s32 y:13;
    __u32 reserved1:3;
};
```

**Members**

`x`
:   Radial reset of x coordinate. Precision s12, [-4095, 4095], default 0.

`reserved0`
:   reserved

`y`
:   Radial center y coordinate. Precision s12, [-4095, 4095], default 0.

`reserved1`
:   reserved

struct ipu3\_uapi\_radial\_reset\_x2
:   Radial X^2 reset

**Definition**:

```
struct ipu3_uapi_radial_reset_x2 {
    __u32 x2:24;
    __u32 reserved:8;
};
```

**Members**

`x2`
:   Radial reset of x^2 coordinate. Precision u24, default 0.

`reserved`
:   reserved

struct ipu3\_uapi\_radial\_reset\_y2
:   Radial Y^2 reset

**Definition**:

```
struct ipu3_uapi_radial_reset_y2 {
    __u32 y2:24;
    __u32 reserved:8;
};
```

**Members**

`y2`
:   Radial reset of y^2 coordinate. Precision u24, default 0.

`reserved`
:   reserved

struct ipu3\_uapi\_radial\_cfg
:   Radial config

**Definition**:

```
struct ipu3_uapi_radial_cfg {
    __u32 rad_nf:4;
    __u32 reserved0:4;
    __u32 rad_inv_r2:7;
    __u32 reserved1:17;
};
```

**Members**

`rad_nf`
:   Radial. R^2 normalization factor is scale down by 2^ - (15 + scale)

`reserved0`
:   reserved

`rad_inv_r2`
:   Radial R^-2 normelized to (0.5..1).
    Precision u7, range [0, 127].

`reserved1`
:   reserved

struct ipu3\_uapi\_rad\_far\_w
:   Radial FAR sub-group

**Definition**:

```
struct ipu3_uapi_rad_far_w {
    __u32 rad_dir_far_sharp_w:8;
    __u32 rad_dir_far_dns_w:8;
    __u32 rad_ndir_far_dns_power:8;
    __u32 reserved:8;
};
```

**Members**

`rad_dir_far_sharp_w`
:   Weight of wide direct sharpening, u1.6, range [0, 64],
    default 64.

`rad_dir_far_dns_w`
:   Weight of wide direct denoising, u1.6, range [0, 64],
    default 0.

`rad_ndir_far_dns_power`
:   power of non-direct sharpening, u1.6, range [0, 64],
    default 0.

`reserved`
:   reserved

struct ipu3\_uapi\_cu\_cfg0
:   Radius Config Unit cfg0 register

**Definition**:

```
struct ipu3_uapi_cu_cfg0 {
    __u32 cu6_pow:7;
    __u32 reserved0:1;
    __u32 cu_unsharp_pow:7;
    __u32 reserved1:1;
    __u32 rad_cu6_pow:7;
    __u32 reserved2:1;
    __u32 rad_cu_unsharp_pow:6;
    __u32 reserved3:2;
};
```

**Members**

`cu6_pow`
:   Power of CU6. Power of non-direct sharpening, u3.4.

`reserved0`
:   reserved

`cu_unsharp_pow`
:   Power of unsharp mask, u2.4.

`reserved1`
:   reserved

`rad_cu6_pow`
:   Radial/corner CU6. Directed sharpening power, u3.4.

`reserved2`
:   reserved

`rad_cu_unsharp_pow`
:   Radial power of unsharp mask, u2.4.

`reserved3`
:   reserved

struct ipu3\_uapi\_cu\_cfg1
:   Radius Config Unit cfg1 register

**Definition**:

```
struct ipu3_uapi_cu_cfg1 {
    __u32 rad_cu6_x1:9;
    __u32 reserved0:1;
    __u32 rad_cu_unsharp_x1:9;
    __u32 reserved1:13;
};
```

**Members**

`rad_cu6_x1`
:   X1 point of Config Unit 6, precision u9.0.

`reserved0`
:   reserved

`rad_cu_unsharp_x1`
:   X1 point for Config Unit unsharp for radial/corner point
    precision u9.0.

`reserved1`
:   reserved

struct ipu3\_uapi\_yuvp1\_iefd\_rad\_cfg
:   IEFd parameters changed radially over the picture plane.

**Definition**:

```
struct ipu3_uapi_yuvp1_iefd_rad_cfg {
    struct ipu3_uapi_radial_reset_xy reset_xy;
    struct ipu3_uapi_radial_reset_x2 reset_x2;
    struct ipu3_uapi_radial_reset_y2 reset_y2;
    struct ipu3_uapi_radial_cfg cfg;
    struct ipu3_uapi_rad_far_w rad_far_w;
    struct ipu3_uapi_cu_cfg0 cu_cfg0;
    struct ipu3_uapi_cu_cfg1 cu_cfg1;
};
```

**Members**

`reset_xy`
:   reset xy value in radial calculation. [`ipu3_uapi_radial_reset_xy`](#c.V4L.ipu3_uapi_radial_reset_xy "ipu3_uapi_radial_reset_xy")

`reset_x2`
:   reset x square value in radial calculation. See struct
    [`ipu3_uapi_radial_reset_x2`](#c.V4L.ipu3_uapi_radial_reset_x2 "ipu3_uapi_radial_reset_x2")

`reset_y2`
:   reset y square value in radial calculation. See struct
    [`ipu3_uapi_radial_reset_y2`](#c.V4L.ipu3_uapi_radial_reset_y2 "ipu3_uapi_radial_reset_y2")

`cfg`
:   radial config defined in [`ipu3_uapi_radial_cfg`](#c.V4L.ipu3_uapi_radial_cfg "ipu3_uapi_radial_cfg")

`rad_far_w`
:   weight for wide range radial. [`ipu3_uapi_rad_far_w`](#c.V4L.ipu3_uapi_rad_far_w "ipu3_uapi_rad_far_w")

`cu_cfg0`
:   configuration unit 0. See [`ipu3_uapi_cu_cfg0`](#c.V4L.ipu3_uapi_cu_cfg0 "ipu3_uapi_cu_cfg0")

`cu_cfg1`
:   configuration unit 1. See [`ipu3_uapi_cu_cfg1`](#c.V4L.ipu3_uapi_cu_cfg1 "ipu3_uapi_cu_cfg1")

struct ipu3\_uapi\_vss\_lut\_x
:   Vssnlm LUT x0/x1/x2

**Definition**:

```
struct ipu3_uapi_vss_lut_x {
    __u32 vs_x0:8;
    __u32 vs_x1:8;
    __u32 vs_x2:8;
    __u32 reserved2:8;
};
```

**Members**

`vs_x0`
:   Vssnlm LUT x0, precision u8, range [0, 255], default 16.

`vs_x1`
:   Vssnlm LUT x1, precision u8, range [0, 255], default 32.

`vs_x2`
:   Vssnlm LUT x2, precision u8, range [0, 255], default 64.

`reserved2`
:   reserved

struct ipu3\_uapi\_vss\_lut\_y
:   Vssnlm LUT y0/y1/y2

**Definition**:

```
struct ipu3_uapi_vss_lut_y {
    __u32 vs_y1:4;
    __u32 reserved0:4;
    __u32 vs_y2:4;
    __u32 reserved1:4;
    __u32 vs_y3:4;
    __u32 reserved2:12;
};
```

**Members**

`vs_y1`
:   Vssnlm LUT y1, precision u4, range [0, 8], default 1.

`reserved0`
:   reserved

`vs_y2`
:   Vssnlm LUT y2, precision u4, range [0, 8], default 3.

`reserved1`
:   reserved

`vs_y3`
:   Vssnlm LUT y3, precision u4, range [0, 8], default 8.

`reserved2`
:   reserved

struct ipu3\_uapi\_yuvp1\_iefd\_vssnlm\_cfg
:   IEFd Vssnlm Lookup table

**Definition**:

```
struct ipu3_uapi_yuvp1_iefd_vssnlm_cfg {
    struct ipu3_uapi_vss_lut_x vss_lut_x;
    struct ipu3_uapi_vss_lut_y vss_lut_y;
};
```

**Members**

`vss_lut_x`
:   vss lookup table. See [`ipu3_uapi_vss_lut_x`](#c.V4L.ipu3_uapi_vss_lut_x "ipu3_uapi_vss_lut_x") description

`vss_lut_y`
:   vss lookup table. See [`ipu3_uapi_vss_lut_y`](#c.V4L.ipu3_uapi_vss_lut_y "ipu3_uapi_vss_lut_y") description

struct ipu3\_uapi\_yuvp1\_iefd\_config
:   IEFd config

**Definition**:

```
struct ipu3_uapi_yuvp1_iefd_config {
    struct ipu3_uapi_yuvp1_iefd_cfg_units units;
    struct ipu3_uapi_yuvp1_iefd_config_s config;
    struct ipu3_uapi_yuvp1_iefd_control control;
    struct ipu3_uapi_yuvp1_iefd_shrp_cfg sharp;
    struct ipu3_uapi_yuvp1_iefd_unshrp_cfg unsharp;
    struct ipu3_uapi_yuvp1_iefd_rad_cfg rad;
    struct ipu3_uapi_yuvp1_iefd_vssnlm_cfg vsslnm;
};
```

**Members**

`units`
:   configuration unit setting, [`ipu3_uapi_yuvp1_iefd_cfg_units`](#c.V4L.ipu3_uapi_yuvp1_iefd_cfg_units "ipu3_uapi_yuvp1_iefd_cfg_units")

`config`
:   configuration, as defined by [`ipu3_uapi_yuvp1_iefd_config_s`](#c.V4L.ipu3_uapi_yuvp1_iefd_config_s "ipu3_uapi_yuvp1_iefd_config_s")

`control`
:   control setting, as defined by [`ipu3_uapi_yuvp1_iefd_control`](#c.V4L.ipu3_uapi_yuvp1_iefd_control "ipu3_uapi_yuvp1_iefd_control")

`sharp`
:   sharpness setting, as defined by [`ipu3_uapi_yuvp1_iefd_shrp_cfg`](#c.V4L.ipu3_uapi_yuvp1_iefd_shrp_cfg "ipu3_uapi_yuvp1_iefd_shrp_cfg")

`unsharp`
:   unsharpness setting, as defined by [`ipu3_uapi_yuvp1_iefd_unshrp_cfg`](#c.V4L.ipu3_uapi_yuvp1_iefd_unshrp_cfg "ipu3_uapi_yuvp1_iefd_unshrp_cfg")

`rad`
:   radial setting, as defined by [`ipu3_uapi_yuvp1_iefd_rad_cfg`](#c.V4L.ipu3_uapi_yuvp1_iefd_rad_cfg "ipu3_uapi_yuvp1_iefd_rad_cfg")

`vsslnm`
:   vsslnm setting, as defined by [`ipu3_uapi_yuvp1_iefd_vssnlm_cfg`](#c.V4L.ipu3_uapi_yuvp1_iefd_vssnlm_cfg "ipu3_uapi_yuvp1_iefd_vssnlm_cfg")

struct ipu3\_uapi\_yuvp1\_yds\_config
:   Y Down-Sampling config

**Definition**:

```
struct ipu3_uapi_yuvp1_yds_config {
    __u32 c00:2;
    __u32 c01:2;
    __u32 c02:2;
    __u32 c03:2;
    __u32 c10:2;
    __u32 c11:2;
    __u32 c12:2;
    __u32 c13:2;
    __u32 norm_factor:5;
    __u32 reserved0:4;
    __u32 bin_output:1;
    __u32 reserved1:6;
};
```

**Members**

`c00`
:   range [0, 3], default 0x0

`c01`
:   range [0, 3], default 0x1

`c02`
:   range [0, 3], default 0x1

`c03`
:   range [0, 3], default 0x0

`c10`
:   range [0, 3], default 0x0

`c11`
:   range [0, 3], default 0x1

`c12`
:   range [0, 3], default 0x1

`c13`
:   range [0, 3], default 0x0

`norm_factor`
:   Normalization factor, range [0, 4], default 2
    0 - divide by 1
    1 - divide by 2
    2 - divide by 4
    3 - divide by 8
    4 - divide by 16

`reserved0`
:   reserved

`bin_output`
:   Down sampling on Luma channel in two optional modes
    0 - Bin output 4.2.0 (default), 1 output 4.2.2.

`reserved1`
:   reserved

**Description**

Above are 4x2 filter coefficients for chroma output downscaling.

struct ipu3\_uapi\_yuvp1\_chnr\_enable\_config
:   Chroma noise reduction enable

**Definition**:

```
struct ipu3_uapi_yuvp1_chnr_enable_config {
    __u32 enable:1;
    __u32 yuv_mode:1;
    __u32 reserved0:14;
    __u32 col_size:12;
    __u32 reserved1:4;
};
```

**Members**

`enable`
:   enable/disable chroma noise reduction

`yuv_mode`
:   0 - YUV420, 1 - YUV422

`reserved0`
:   reserved

`col_size`
:   number of columns in the frame, max width is 2560

`reserved1`
:   reserved

struct ipu3\_uapi\_yuvp1\_chnr\_coring\_config
:   Coring thresholds for UV

**Definition**:

```
struct ipu3_uapi_yuvp1_chnr_coring_config {
    __u32 u:13;
    __u32 reserved0:3;
    __u32 v:13;
    __u32 reserved1:3;
};
```

**Members**

`u`
:   U coring level, u0.13, range [0.0, 1.0], default 0.0

`reserved0`
:   reserved

`v`
:   V coring level, u0.13, range [0.0, 1.0], default 0.0

`reserved1`
:   reserved

struct ipu3\_uapi\_yuvp1\_chnr\_sense\_gain\_config
:   Chroma noise reduction gains

**Definition**:

```
struct ipu3_uapi_yuvp1_chnr_sense_gain_config {
    __u32 vy:8;
    __u32 vu:8;
    __u32 vv:8;
    __u32 reserved0:8;
    __u32 hy:8;
    __u32 hu:8;
    __u32 hv:8;
    __u32 reserved1:8;
};
```

**Members**

`vy`
:   Sensitivity of horizontal edge of Y, default 100

`vu`
:   Sensitivity of horizontal edge of U, default 100

`vv`
:   Sensitivity of horizontal edge of V, default 100

`reserved0`
:   reserved

`hy`
:   Sensitivity of vertical edge of Y, default 50

`hu`
:   Sensitivity of vertical edge of U, default 50

`hv`
:   Sensitivity of vertical edge of V, default 50

`reserved1`
:   reserved

**Description**

All sensitivity gain parameters have precision u13.0, range [0, 8191].

struct ipu3\_uapi\_yuvp1\_chnr\_iir\_fir\_config
:   Chroma IIR/FIR filter config

**Definition**:

```
struct ipu3_uapi_yuvp1_chnr_iir_fir_config {
    __u32 fir_0h:6;
    __u32 reserved0:2;
    __u32 fir_1h:6;
    __u32 reserved1:2;
    __u32 fir_2h:6;
    __u32 dalpha_clip_val:9;
    __u32 reserved2:1;
};
```

**Members**

`fir_0h`
:   Value of center tap in horizontal FIR, range [0, 32], default 8.

`reserved0`
:   reserved

`fir_1h`
:   Value of distance 1 in horizontal FIR, range [0, 32], default 12.

`reserved1`
:   reserved

`fir_2h`
:   Value of distance 2 tap in horizontal FIR, range [0, 32], default 0.

`dalpha_clip_val`
:   weight for previous row in IIR, range [1, 256], default 0.

`reserved2`
:   reserved

struct ipu3\_uapi\_yuvp1\_chnr\_config
:   Chroma noise reduction config

**Definition**:

```
struct ipu3_uapi_yuvp1_chnr_config {
    struct ipu3_uapi_yuvp1_chnr_enable_config enable;
    struct ipu3_uapi_yuvp1_chnr_coring_config coring;
    struct ipu3_uapi_yuvp1_chnr_sense_gain_config sense_gain;
    struct ipu3_uapi_yuvp1_chnr_iir_fir_config iir_fir;
};
```

**Members**

`enable`
:   chroma noise reduction enable, see
    [`ipu3_uapi_yuvp1_chnr_enable_config`](#c.V4L.ipu3_uapi_yuvp1_chnr_enable_config "ipu3_uapi_yuvp1_chnr_enable_config")

`coring`
:   coring config for chroma noise reduction, see
    [`ipu3_uapi_yuvp1_chnr_coring_config`](#c.V4L.ipu3_uapi_yuvp1_chnr_coring_config "ipu3_uapi_yuvp1_chnr_coring_config")

`sense_gain`
:   sensitivity config for chroma noise reduction, see
    ipu3\_uapi\_yuvp1\_chnr\_sense\_gain\_config

`iir_fir`
:   iir and fir config for chroma noise reduction, see
    ipu3\_uapi\_yuvp1\_chnr\_iir\_fir\_config

struct ipu3\_uapi\_yuvp1\_y\_ee\_nr\_lpf\_config
:   Luma(Y) edge enhancement low-pass filter coefficients

**Definition**:

```
struct ipu3_uapi_yuvp1_y_ee_nr_lpf_config {
    __u32 a_diag:5;
    __u32 reserved0:3;
    __u32 a_periph:5;
    __u32 reserved1:3;
    __u32 a_cent:5;
    __u32 reserved2:9;
    __u32 enable:1;
};
```

**Members**

`a_diag`
:   Smoothing diagonal coefficient, u5.0.

`reserved0`
:   reserved

`a_periph`
:   Image smoothing perpherial, u5.0.

`reserved1`
:   reserved

`a_cent`
:   Image Smoothing center coefficient, u5.0.

`reserved2`
:   reserved

`enable`
:   0: Y\_EE\_NR disabled, output = input; 1: Y\_EE\_NR enabled.

struct ipu3\_uapi\_yuvp1\_y\_ee\_nr\_sense\_config
:   Luma(Y) edge enhancement noise reduction sensitivity gains

**Definition**:

```
struct ipu3_uapi_yuvp1_y_ee_nr_sense_config {
    __u32 edge_sense_0:13;
    __u32 reserved0:3;
    __u32 delta_edge_sense:13;
    __u32 reserved1:3;
    __u32 corner_sense_0:13;
    __u32 reserved2:3;
    __u32 delta_corner_sense:13;
    __u32 reserved3:3;
};
```

**Members**

`edge_sense_0`
:   Sensitivity of edge in dark area. u13.0, default 8191.

`reserved0`
:   reserved

`delta_edge_sense`
:   Difference in the sensitivity of edges between
    the bright and dark areas. u13.0, default 0.

`reserved1`
:   reserved

`corner_sense_0`
:   Sensitivity of corner in dark area. u13.0, default 0.

`reserved2`
:   reserved

`delta_corner_sense`
:   Difference in the sensitivity of corners between
    the bright and dark areas. u13.0, default 8191.

`reserved3`
:   reserved

struct ipu3\_uapi\_yuvp1\_y\_ee\_nr\_gain\_config
:   Luma(Y) edge enhancement noise reduction gain config

**Definition**:

```
struct ipu3_uapi_yuvp1_y_ee_nr_gain_config {
    __u32 gain_pos_0:5;
    __u32 reserved0:3;
    __u32 delta_gain_posi:5;
    __u32 reserved1:3;
    __u32 gain_neg_0:5;
    __u32 reserved2:3;
    __u32 delta_gain_neg:5;
    __u32 reserved3:3;
};
```

**Members**

`gain_pos_0`
:   Gain for positive edge in dark area. u5.0, [0, 16], default 2.

`reserved0`
:   reserved

`delta_gain_posi`
:   Difference in the gain of edges between the bright and
    dark areas for positive edges. u5.0, [0, 16], default 0.

`reserved1`
:   reserved

`gain_neg_0`
:   Gain for negative edge in dark area. u5.0, [0, 16], default 8.

`reserved2`
:   reserved

`delta_gain_neg`
:   Difference in the gain of edges between the bright and
    dark areas for negative edges. u5.0, [0, 16], default 0.

`reserved3`
:   reserved

struct ipu3\_uapi\_yuvp1\_y\_ee\_nr\_clip\_config
:   Luma(Y) edge enhancement noise reduction clipping config

**Definition**:

```
struct ipu3_uapi_yuvp1_y_ee_nr_clip_config {
    __u32 clip_pos_0:5;
    __u32 reserved0:3;
    __u32 delta_clip_posi:5;
    __u32 reserved1:3;
    __u32 clip_neg_0:5;
    __u32 reserved2:3;
    __u32 delta_clip_neg:5;
    __u32 reserved3:3;
};
```

**Members**

`clip_pos_0`
:   Limit of positive edge in dark area
    u5, value [0, 16], default 8.

`reserved0`
:   reserved

`delta_clip_posi`
:   Difference in the limit of edges between the bright
    and dark areas for positive edges.
    u5, value [0, 16], default 8.

`reserved1`
:   reserved

`clip_neg_0`
:   Limit of negative edge in dark area
    u5, value [0, 16], default 8.

`reserved2`
:   reserved

`delta_clip_neg`
:   Difference in the limit of edges between the bright
    and dark areas for negative edges.
    u5, value [0, 16], default 8.

`reserved3`
:   reserved

struct ipu3\_uapi\_yuvp1\_y\_ee\_nr\_frng\_config
:   Luma(Y) edge enhancement noise reduction fringe config

**Definition**:

```
struct ipu3_uapi_yuvp1_y_ee_nr_frng_config {
    __u32 gain_exp:4;
    __u32 reserved0:28;
    __u32 min_edge:13;
    __u32 reserved1:3;
    __u32 lin_seg_param:4;
    __u32 reserved2:4;
    __u32 t1:1;
    __u32 t2:1;
    __u32 reserved3:6;
};
```

**Members**

`gain_exp`
:   Common exponent of gains, u4, [0, 8], default 2.

`reserved0`
:   reserved

`min_edge`
:   Threshold for edge and smooth stitching, u13.

`reserved1`
:   reserved

`lin_seg_param`
:   Power of LinSeg, u4.

`reserved2`
:   reserved

`t1`
:   Parameter for enabling/disabling the edge enhancement, u1.0, [0, 1],
    default 1.

`t2`
:   Parameter for enabling/disabling the smoothing, u1.0, [0, 1],
    default 1.

`reserved3`
:   reserved

struct ipu3\_uapi\_yuvp1\_y\_ee\_nr\_diag\_config
:   Luma(Y) edge enhancement noise reduction diagonal config

**Definition**:

```
struct ipu3_uapi_yuvp1_y_ee_nr_diag_config {
    __u32 diag_disc_g:4;
    __u32 reserved0:4;
    __u32 hvw_hor:4;
    __u32 dw_hor:4;
    __u32 hvw_diag:4;
    __u32 dw_diag:4;
    __u32 reserved1:8;
};
```

**Members**

`diag_disc_g`
:   Coefficient that prioritize diagonal edge direction on
    horizontal or vertical for final enhancement.
    u4.0, [1, 15], default 1.

`reserved0`
:   reserved

`hvw_hor`
:   Weight of horizontal/vertical edge enhancement for hv edge.
    u2.2, [1, 15], default 4.

`dw_hor`
:   Weight of diagonal edge enhancement for hv edge.
    u2.2, [1, 15], default 1.

`hvw_diag`
:   Weight of horizontal/vertical edge enhancement for diagonal edge.
    u2.2, [1, 15], default 1.

`dw_diag`
:   Weight of diagonal edge enhancement for diagonal edge.
    u2.2, [1, 15], default 4.

`reserved1`
:   reserved

struct ipu3\_uapi\_yuvp1\_y\_ee\_nr\_fc\_coring\_config
:   Luma(Y) edge enhancement noise reduction false color correction (FCC) coring config

**Definition**:

```
struct ipu3_uapi_yuvp1_y_ee_nr_fc_coring_config {
    __u32 pos_0:13;
    __u32 reserved0:3;
    __u32 pos_delta:13;
    __u32 reserved1:3;
    __u32 neg_0:13;
    __u32 reserved2:3;
    __u32 neg_delta:13;
    __u32 reserved3:3;
};
```

**Members**

`pos_0`
:   Gain for positive edge in dark, u13.0, [0, 16], default 0.

`reserved0`
:   reserved

`pos_delta`
:   Gain for positive edge in bright, value: pos\_0 + pos\_delta <=16
    u13.0, default 0.

`reserved1`
:   reserved

`neg_0`
:   Gain for negative edge in dark area, u13.0, range [0, 16], default 0.

`reserved2`
:   reserved

`neg_delta`
:   Gain for negative edge in bright area. neg\_0 + neg\_delta <=16
    u13.0, default 0.

`reserved3`
:   reserved

**Description**

Coring is a simple soft thresholding technique.

struct ipu3\_uapi\_yuvp1\_y\_ee\_nr\_config
:   Edge enhancement and noise reduction

**Definition**:

```
struct ipu3_uapi_yuvp1_y_ee_nr_config {
    struct ipu3_uapi_yuvp1_y_ee_nr_lpf_config lpf;
    struct ipu3_uapi_yuvp1_y_ee_nr_sense_config sense;
    struct ipu3_uapi_yuvp1_y_ee_nr_gain_config gain;
    struct ipu3_uapi_yuvp1_y_ee_nr_clip_config clip;
    struct ipu3_uapi_yuvp1_y_ee_nr_frng_config frng;
    struct ipu3_uapi_yuvp1_y_ee_nr_diag_config diag;
    struct ipu3_uapi_yuvp1_y_ee_nr_fc_coring_config fc_coring;
};
```

**Members**

`lpf`
:   low-pass filter config. See [`ipu3_uapi_yuvp1_y_ee_nr_lpf_config`](#c.V4L.ipu3_uapi_yuvp1_y_ee_nr_lpf_config "ipu3_uapi_yuvp1_y_ee_nr_lpf_config")

`sense`
:   sensitivity config. See [`ipu3_uapi_yuvp1_y_ee_nr_sense_config`](#c.V4L.ipu3_uapi_yuvp1_y_ee_nr_sense_config "ipu3_uapi_yuvp1_y_ee_nr_sense_config")

`gain`
:   gain config as defined in [`ipu3_uapi_yuvp1_y_ee_nr_gain_config`](#c.V4L.ipu3_uapi_yuvp1_y_ee_nr_gain_config "ipu3_uapi_yuvp1_y_ee_nr_gain_config")

`clip`
:   clip config as defined in [`ipu3_uapi_yuvp1_y_ee_nr_clip_config`](#c.V4L.ipu3_uapi_yuvp1_y_ee_nr_clip_config "ipu3_uapi_yuvp1_y_ee_nr_clip_config")

`frng`
:   fringe config as defined in [`ipu3_uapi_yuvp1_y_ee_nr_frng_config`](#c.V4L.ipu3_uapi_yuvp1_y_ee_nr_frng_config "ipu3_uapi_yuvp1_y_ee_nr_frng_config")

`diag`
:   diagonal edge config. See [`ipu3_uapi_yuvp1_y_ee_nr_diag_config`](#c.V4L.ipu3_uapi_yuvp1_y_ee_nr_diag_config "ipu3_uapi_yuvp1_y_ee_nr_diag_config")

`fc_coring`
:   coring config for fringe control. See
    [`ipu3_uapi_yuvp1_y_ee_nr_fc_coring_config`](#c.V4L.ipu3_uapi_yuvp1_y_ee_nr_fc_coring_config "ipu3_uapi_yuvp1_y_ee_nr_fc_coring_config")

struct ipu3\_uapi\_yuvp2\_tcc\_gen\_control\_static\_config
:   Total color correction general control config

**Definition**:

```
struct ipu3_uapi_yuvp2_tcc_gen_control_static_config {
    __u32 en:1;
    __u32 blend_shift:3;
    __u32 gain_according_to_y_only:1;
    __u32 reserved0:11;
    __s32 gamma:5;
    __u32 reserved1:3;
    __s32 delta:5;
    __u32 reserved2:3;
};
```

**Members**

`en`
:   0 - TCC disabled. Output = input 1 - TCC enabled.

`blend_shift`
:   blend shift, Range[3, 4], default NA.

`gain_according_to_y_only`
:   0: Gain is calculated according to YUV,
    1: Gain is calculated according to Y only

`reserved0`
:   reserved

`gamma`
:   Final blending coefficients. Values[-16, 16], default NA.

`reserved1`
:   reserved

`delta`
:   Final blending coefficients. Values[-16, 16], default NA.

`reserved2`
:   reserved

struct ipu3\_uapi\_yuvp2\_tcc\_macc\_elem\_static\_config
:   Total color correction multi-axis color control (MACC) config

**Definition**:

```
struct ipu3_uapi_yuvp2_tcc_macc_elem_static_config {
    __s32 a:12;
    __u32 reserved0:4;
    __s32 b:12;
    __u32 reserved1:4;
    __s32 c:12;
    __u32 reserved2:4;
    __s32 d:12;
    __u32 reserved3:4;
};
```

**Members**

`a`
:   a coefficient for 2x2 MACC conversion matrix.

`reserved0`
:   reserved

`b`
:   b coefficient 2x2 MACC conversion matrix.

`reserved1`
:   reserved

`c`
:   c coefficient for 2x2 MACC conversion matrix.

`reserved2`
:   reserved

`d`
:   d coefficient for 2x2 MACC conversion matrix.

`reserved3`
:   reserved

struct ipu3\_uapi\_yuvp2\_tcc\_macc\_table\_static\_config
:   Total color correction multi-axis color control (MACC) table array

**Definition**:

```
struct ipu3_uapi_yuvp2_tcc_macc_table_static_config {
    struct ipu3_uapi_yuvp2_tcc_macc_elem_static_config entries[IPU3_UAPI_YUVP2_TCC_MACC_TABLE_ELEMENTS];
};
```

**Members**

`entries`
:   config for multi axis color correction, as specified by
    [`ipu3_uapi_yuvp2_tcc_macc_elem_static_config`](#c.V4L.ipu3_uapi_yuvp2_tcc_macc_elem_static_config "ipu3_uapi_yuvp2_tcc_macc_elem_static_config")

struct ipu3\_uapi\_yuvp2\_tcc\_inv\_y\_lut\_static\_config
:   Total color correction inverse y lookup table

**Definition**:

```
struct ipu3_uapi_yuvp2_tcc_inv_y_lut_static_config {
    __u16 entries[IPU3_UAPI_YUVP2_TCC_INV_Y_LUT_ELEMENTS];
};
```

**Members**

`entries`
:   lookup table for inverse y estimation, and use it to estimate the
    ratio between luma and chroma. Chroma by approximate the absolute
    value of the radius on the chroma plane (R = sqrt(u^2+v^2) ) and
    luma by approximate by 1/Y.

struct ipu3\_uapi\_yuvp2\_tcc\_gain\_pcwl\_lut\_static\_config
:   Total color correction lookup table for PCWL

**Definition**:

```
struct ipu3_uapi_yuvp2_tcc_gain_pcwl_lut_static_config {
    __u16 entries[IPU3_UAPI_YUVP2_TCC_GAIN_PCWL_LUT_ELEMENTS];
};
```

**Members**

`entries`
:   lookup table for gain piece wise linear transformation (PCWL)

struct ipu3\_uapi\_yuvp2\_tcc\_r\_sqr\_lut\_static\_config
:   Total color correction lookup table for r square root

**Definition**:

```
struct ipu3_uapi_yuvp2_tcc_r_sqr_lut_static_config {
    __s16 entries[IPU3_UAPI_YUVP2_TCC_R_SQR_LUT_ELEMENTS];
};
```

**Members**

`entries`
:   lookup table for r square root estimation

struct ipu3\_uapi\_yuvp2\_tcc\_static\_config
:   Total color correction static

**Definition**:

```
struct ipu3_uapi_yuvp2_tcc_static_config {
    struct ipu3_uapi_yuvp2_tcc_gen_control_static_config gen_control;
    struct ipu3_uapi_yuvp2_tcc_macc_table_static_config macc_table;
    struct ipu3_uapi_yuvp2_tcc_inv_y_lut_static_config inv_y_lut;
    struct ipu3_uapi_yuvp2_tcc_gain_pcwl_lut_static_config gain_pcwl;
    struct ipu3_uapi_yuvp2_tcc_r_sqr_lut_static_config r_sqr_lut;
};
```

**Members**

`gen_control`
:   general config for Total Color Correction

`macc_table`
:   config for multi axis color correction

`inv_y_lut`
:   lookup table for inverse y estimation

`gain_pcwl`
:   lookup table for gain PCWL

`r_sqr_lut`
:   lookup table for r square root estimation.

struct ipu3\_uapi\_anr\_transform\_config
:   Advanced noise reduction transform

**Definition**:

```
struct ipu3_uapi_anr_transform_config {
    __u32 enable:1;
    __u32 adaptive_treshhold_en:1;
    __u32 reserved1:30;
    __u8 reserved2[44];
    struct ipu3_uapi_anr_alpha alpha[3];
    struct ipu3_uapi_anr_beta beta[3];
    struct ipu3_uapi_anr_plane_color color[3];
    __u16 sqrt_lut[IPU3_UAPI_ANR_LUT_SIZE];
    __s16 xreset:13;
    __u16 reserved3:3;
    __s16 yreset:13;
    __u16 reserved4:3;
    __u32 x_sqr_reset:24;
    __u32 r_normfactor:5;
    __u32 reserved5:3;
    __u32 y_sqr_reset:24;
    __u32 gain_scale:8;
};
```

**Members**

`enable`
:   advanced noise reduction enabled.

`adaptive_treshhold_en`
:   On IPU3, adaptive threshold is always enabled.

`reserved1`
:   reserved

`reserved2`
:   reserved

`alpha`
:   using following defaults:
    13, 13, 13, 13, 0, 0, 0, 0
    11, 11, 11, 11, 0, 0, 0, 0
    14, 14, 14, 14, 0, 0, 0, 0

`beta`
:   use following defaults:
    24, 24, 24, 24
    21, 20, 20, 21
    25, 25, 25, 25

`color`
:   use defaults defined in driver/media/pci/intel/ipu3-tables.c

`sqrt_lut`
:   11 bits per element, values =
    [724 768 810 849 887
    923 958 991 1024 1056
    1116 1145 1173 1201 1086
    1228 1254 1280 1305 1330
    1355 1379 1402 1425 1448]

`xreset`
:   Reset value of X for r^2 calculation Value: col\_start-X\_center
    Constraint: Xreset + FrameWdith=4095 Xreset= -4095, default -1632.

`reserved3`
:   reserved

`yreset`
:   Reset value of Y for r^2 calculation Value: row\_start-Y\_center
    Constraint: Yreset + FrameHeight=4095 Yreset= -4095, default -1224.

`reserved4`
:   reserved

`x_sqr_reset`
:   Reset value of X^2 for r^2 calculation Value = (Xreset)^2

`r_normfactor`
:   Normalization factor for R. Default 14.

`reserved5`
:   reserved

`y_sqr_reset`
:   Reset value of Y^2 for r^2 calculation Value = (Yreset)^2

`gain_scale`
:   Parameter describing shading gain as a function of distance
    from the image center.
    A single value per frame, loaded by the driver. Default 115.

struct ipu3\_uapi\_anr\_stitch\_pyramid
:   ANR stitch pyramid

**Definition**:

```
struct ipu3_uapi_anr_stitch_pyramid {
    __u32 entry0:6;
    __u32 entry1:6;
    __u32 entry2:6;
    __u32 reserved:14;
};
```

**Members**

`entry0`
:   pyramid LUT entry0, range [0x0, 0x3f]

`entry1`
:   pyramid LUT entry1, range [0x0, 0x3f]

`entry2`
:   pyramid LUT entry2, range [0x0, 0x3f]

`reserved`
:   reserved

struct ipu3\_uapi\_anr\_stitch\_config
:   ANR stitch config

**Definition**:

```
struct ipu3_uapi_anr_stitch_config {
    __u32 anr_stitch_en;
    __u8 reserved[44];
    struct ipu3_uapi_anr_stitch_pyramid pyramid[IPU3_UAPI_ANR_PYRAMID_SIZE];
};
```

**Members**

`anr_stitch_en`
:   enable stitch. Enabled with 1.

`reserved`
:   reserved

`pyramid`
:   pyramid table as defined by [`ipu3_uapi_anr_stitch_pyramid`](#c.V4L.ipu3_uapi_anr_stitch_pyramid "ipu3_uapi_anr_stitch_pyramid")
    default values:
    { 1, 3, 5 }, { 7, 7, 5 }, { 3, 1, 3 },
    { 9, 15, 21 }, { 21, 15, 9 }, { 3, 5, 15 },
    { 25, 35, 35 }, { 25, 15, 5 }, { 7, 21, 35 },
    { 49, 49, 35 }, { 21, 7, 7 }, { 21, 35, 49 },
    { 49, 35, 21 }, { 7, 5, 15 }, { 25, 35, 35 },
    { 25, 15, 5 }, { 3, 9, 15 }, { 21, 21, 15 },
    { 9, 3, 1 }, { 3, 5, 7 }, { 7, 5, 3}, { 1 }

struct ipu3\_uapi\_anr\_config
:   ANR config

**Definition**:

```
struct ipu3_uapi_anr_config {
    struct ipu3_uapi_anr_transform_config transform;
    struct ipu3_uapi_anr_stitch_config stitch;
};
```

**Members**

`transform`
:   advanced noise reduction transform config as specified by
    [`ipu3_uapi_anr_transform_config`](#c.V4L.ipu3_uapi_anr_transform_config "ipu3_uapi_anr_transform_config")

`stitch`
:   create 4x4 patch from 4 surrounding 8x8 patches.

struct ipu3\_uapi\_acc\_param
:   Accelerator cluster parameters

**Definition**:

```
struct ipu3_uapi_acc_param {
    struct ipu3_uapi_bnr_static_config bnr;
    struct ipu3_uapi_bnr_static_config_green_disparity green_disparity;
    struct ipu3_uapi_dm_config dm;
    struct ipu3_uapi_ccm_mat_config ccm;
    struct ipu3_uapi_gamma_config gamma;
    struct ipu3_uapi_csc_mat_config csc;
    struct ipu3_uapi_cds_params cds;
    struct ipu3_uapi_shd_config shd;
    struct ipu3_uapi_yuvp1_iefd_config iefd;
    struct ipu3_uapi_yuvp1_yds_config yds_c0;
    struct ipu3_uapi_yuvp1_chnr_config chnr_c0;
    struct ipu3_uapi_yuvp1_y_ee_nr_config y_ee_nr;
    struct ipu3_uapi_yuvp1_yds_config yds;
    struct ipu3_uapi_yuvp1_chnr_config chnr;
    struct ipu3_uapi_yuvp1_yds_config yds2;
    struct ipu3_uapi_yuvp2_tcc_static_config tcc;
    struct ipu3_uapi_anr_config anr;
    struct ipu3_uapi_awb_fr_config_s awb_fr;
    struct ipu3_uapi_ae_config ae;
    struct ipu3_uapi_af_config_s af;
    struct ipu3_uapi_awb_config awb;
};
```

**Members**

`bnr`
:   parameters for bayer noise reduction static config. See
    [`ipu3_uapi_bnr_static_config`](#c.V4L.ipu3_uapi_bnr_static_config "ipu3_uapi_bnr_static_config")

`green_disparity`
:   disparity static config between gr and gb channel.
    See [`ipu3_uapi_bnr_static_config_green_disparity`](#c.V4L.ipu3_uapi_bnr_static_config_green_disparity "ipu3_uapi_bnr_static_config_green_disparity")

`dm`
:   de-mosaic config. See [`ipu3_uapi_dm_config`](#c.V4L.ipu3_uapi_dm_config "ipu3_uapi_dm_config")

`ccm`
:   color correction matrix. See [`ipu3_uapi_ccm_mat_config`](#c.V4L.ipu3_uapi_ccm_mat_config "ipu3_uapi_ccm_mat_config")

`gamma`
:   gamma correction config. See [`ipu3_uapi_gamma_config`](#c.V4L.ipu3_uapi_gamma_config "ipu3_uapi_gamma_config")

`csc`
:   color space conversion matrix. See [`ipu3_uapi_csc_mat_config`](#c.V4L.ipu3_uapi_csc_mat_config "ipu3_uapi_csc_mat_config")

`cds`
:   color down sample config. See [`ipu3_uapi_cds_params`](#c.V4L.ipu3_uapi_cds_params "ipu3_uapi_cds_params")

`shd`
:   lens shading correction config. See [`ipu3_uapi_shd_config`](#c.V4L.ipu3_uapi_shd_config "ipu3_uapi_shd_config")

`iefd`
:   Image enhancement filter and denoise config.
    [`ipu3_uapi_yuvp1_iefd_config`](#c.V4L.ipu3_uapi_yuvp1_iefd_config "ipu3_uapi_yuvp1_iefd_config")

`yds_c0`
:   y down scaler config. [`ipu3_uapi_yuvp1_yds_config`](#c.V4L.ipu3_uapi_yuvp1_yds_config "ipu3_uapi_yuvp1_yds_config")

`chnr_c0`
:   chroma noise reduction config. [`ipu3_uapi_yuvp1_chnr_config`](#c.V4L.ipu3_uapi_yuvp1_chnr_config "ipu3_uapi_yuvp1_chnr_config")

`y_ee_nr`
:   y edge enhancement and noise reduction config.
    [`ipu3_uapi_yuvp1_y_ee_nr_config`](#c.V4L.ipu3_uapi_yuvp1_y_ee_nr_config "ipu3_uapi_yuvp1_y_ee_nr_config")

`yds`
:   y down scaler config. See [`ipu3_uapi_yuvp1_yds_config`](#c.V4L.ipu3_uapi_yuvp1_yds_config "ipu3_uapi_yuvp1_yds_config")

`chnr`
:   chroma noise reduction config. See [`ipu3_uapi_yuvp1_chnr_config`](#c.V4L.ipu3_uapi_yuvp1_chnr_config "ipu3_uapi_yuvp1_chnr_config")

`yds2`
:   y channel down scaler config. See [`ipu3_uapi_yuvp1_yds_config`](#c.V4L.ipu3_uapi_yuvp1_yds_config "ipu3_uapi_yuvp1_yds_config")

`tcc`
:   total color correction config as defined in struct
    [`ipu3_uapi_yuvp2_tcc_static_config`](#c.V4L.ipu3_uapi_yuvp2_tcc_static_config "ipu3_uapi_yuvp2_tcc_static_config")

`anr`
:   advanced noise reduction config.See [`ipu3_uapi_anr_config`](#c.V4L.ipu3_uapi_anr_config "ipu3_uapi_anr_config")

`awb_fr`
:   AWB filter response config. See ipu3\_uapi\_awb\_fr\_config

`ae`
:   auto exposure config As specified by [`ipu3_uapi_ae_config`](#c.V4L.ipu3_uapi_ae_config "ipu3_uapi_ae_config")

`af`
:   auto focus config. As specified by `ipu3_uapi_af_config`

`awb`
:   auto white balance config. As specified by [`ipu3_uapi_awb_config`](#c.V4L.ipu3_uapi_awb_config "ipu3_uapi_awb_config")

**Description**

ACC refers to the HW cluster containing all Fixed Functions (FFs). Each FF
implements a specific algorithm.

struct ipu3\_uapi\_isp\_lin\_vmem\_params
:   Linearization parameters

**Definition**:

```
struct ipu3_uapi_isp_lin_vmem_params {
    __s16 lin_lutlow_gr[IPU3_UAPI_LIN_LUT_SIZE];
    __s16 lin_lutlow_r[IPU3_UAPI_LIN_LUT_SIZE];
    __s16 lin_lutlow_b[IPU3_UAPI_LIN_LUT_SIZE];
    __s16 lin_lutlow_gb[IPU3_UAPI_LIN_LUT_SIZE];
    __s16 lin_lutdif_gr[IPU3_UAPI_LIN_LUT_SIZE];
    __s16 lin_lutdif_r[IPU3_UAPI_LIN_LUT_SIZE];
    __s16 lin_lutdif_b[IPU3_UAPI_LIN_LUT_SIZE];
    __s16 lin_lutdif_gb[IPU3_UAPI_LIN_LUT_SIZE];
};
```

**Members**

`lin_lutlow_gr`
:   linearization look-up table for GR channel interpolation.

`lin_lutlow_r`
:   linearization look-up table for R channel interpolation.

`lin_lutlow_b`
:   linearization look-up table for B channel interpolation.

`lin_lutlow_gb`
:   linearization look-up table for GB channel interpolation.
    lin\_lutlow\_gr / lin\_lutlow\_r / lin\_lutlow\_b /
    lin\_lutlow\_gb <= LIN\_MAX\_VALUE - 1.

`lin_lutdif_gr`
:   lin\_lutlow\_gr[i+1] - lin\_lutlow\_gr[i].

`lin_lutdif_r`
:   lin\_lutlow\_r[i+1] - lin\_lutlow\_r[i].

`lin_lutdif_b`
:   lin\_lutlow\_b[i+1] - lin\_lutlow\_b[i].

`lin_lutdif_gb`
:   lin\_lutlow\_gb[i+1] - lin\_lutlow\_gb[i].

struct ipu3\_uapi\_isp\_tnr3\_vmem\_params
:   Temporal noise reduction vector memory parameters

**Definition**:

```
struct ipu3_uapi_isp_tnr3_vmem_params {
    __u16 slope[IPU3_UAPI_ISP_TNR3_VMEM_LEN];
    __u16 reserved1[IPU3_UAPI_ISP_VEC_ELEMS - IPU3_UAPI_ISP_TNR3_VMEM_LEN];
    __u16 sigma[IPU3_UAPI_ISP_TNR3_VMEM_LEN];
    __u16 reserved2[IPU3_UAPI_ISP_VEC_ELEMS - IPU3_UAPI_ISP_TNR3_VMEM_LEN];
};
```

**Members**

`slope`
:   slope setting in interpolation curve for temporal noise reduction.

`reserved1`
:   reserved

`sigma`
:   knee point setting in interpolation curve for temporal
    noise reduction.

`reserved2`
:   reserved

struct ipu3\_uapi\_isp\_tnr3\_params
:   Temporal noise reduction v3 parameters

**Definition**:

```
struct ipu3_uapi_isp_tnr3_params {
    __u32 knee_y1;
    __u32 knee_y2;
    __u32 maxfb_y;
    __u32 maxfb_u;
    __u32 maxfb_v;
    __u32 round_adj_y;
    __u32 round_adj_u;
    __u32 round_adj_v;
    __u32 ref_buf_select;
};
```

**Members**

`knee_y1`
:   Knee point TNR3 assumes standard deviation of Y,U and
    V at Y1 are TnrY1\_Sigma\_Y, U and V.

`knee_y2`
:   Knee point TNR3 assumes standard deviation of Y,U and
    V at Y2 are TnrY2\_Sigma\_Y, U and V.

`maxfb_y`
:   Max feedback gain for Y

`maxfb_u`
:   Max feedback gain for U

`maxfb_v`
:   Max feedback gain for V

`round_adj_y`
:   rounding Adjust for Y

`round_adj_u`
:   rounding Adjust for U

`round_adj_v`
:   rounding Adjust for V

`ref_buf_select`
:   selection of the reference frame buffer to be used.

struct ipu3\_uapi\_isp\_xnr3\_vmem\_params
:   Extreme noise reduction v3 vector memory parameters

**Definition**:

```
struct ipu3_uapi_isp_xnr3_vmem_params {
    __u16 x[IPU3_UAPI_ISP_VEC_ELEMS];
    __u16 a[IPU3_UAPI_ISP_VEC_ELEMS];
    __u16 b[IPU3_UAPI_ISP_VEC_ELEMS];
    __u16 c[IPU3_UAPI_ISP_VEC_ELEMS];
};
```

**Members**

`x`
:   xnr3 parameters.

`a`
:   xnr3 parameters.

`b`
:   xnr3 parameters.

`c`
:   xnr3 parameters.

struct ipu3\_uapi\_xnr3\_alpha\_params
:   Extreme noise reduction v3 alpha tuning parameters

**Definition**:

```
struct ipu3_uapi_xnr3_alpha_params {
    __u32 y0;
    __u32 u0;
    __u32 v0;
    __u32 ydiff;
    __u32 udiff;
    __u32 vdiff;
};
```

**Members**

`y0`
:   Sigma for Y range similarity in dark area.

`u0`
:   Sigma for U range similarity in dark area.

`v0`
:   Sigma for V range similarity in dark area.

`ydiff`
:   Sigma difference for Y between bright area and dark area.

`udiff`
:   Sigma difference for U between bright area and dark area.

`vdiff`
:   Sigma difference for V between bright area and dark area.

struct ipu3\_uapi\_xnr3\_coring\_params
:   Extreme noise reduction v3 coring parameters

**Definition**:

```
struct ipu3_uapi_xnr3_coring_params {
    __u32 u0;
    __u32 v0;
    __u32 udiff;
    __u32 vdiff;
};
```

**Members**

`u0`
:   Coring Threshold of U channel in dark area.

`v0`
:   Coring Threshold of V channel in dark area.

`udiff`
:   Threshold difference of U channel between bright and dark area.

`vdiff`
:   Threshold difference of V channel between bright and dark area.

struct ipu3\_uapi\_xnr3\_blending\_params
:   Blending factor

**Definition**:

```
struct ipu3_uapi_xnr3_blending_params {
    __u32 strength;
};
```

**Members**

`strength`
:   The factor for blending output with input. This is tuning
    parameterHigher values lead to more aggressive XNR operation.

struct ipu3\_uapi\_isp\_xnr3\_params
:   Extreme noise reduction v3 parameters

**Definition**:

```
struct ipu3_uapi_isp_xnr3_params {
    struct ipu3_uapi_xnr3_alpha_params alpha;
    struct ipu3_uapi_xnr3_coring_params coring;
    struct ipu3_uapi_xnr3_blending_params blending;
};
```

**Members**

`alpha`
:   parameters for xnr3 alpha. See [`ipu3_uapi_xnr3_alpha_params`](#c.V4L.ipu3_uapi_xnr3_alpha_params "ipu3_uapi_xnr3_alpha_params")

`coring`
:   parameters for xnr3 coring. See [`ipu3_uapi_xnr3_coring_params`](#c.V4L.ipu3_uapi_xnr3_coring_params "ipu3_uapi_xnr3_coring_params")

`blending`
:   parameters for xnr3 blending. See [`ipu3_uapi_xnr3_blending_params`](#c.V4L.ipu3_uapi_xnr3_blending_params "ipu3_uapi_xnr3_blending_params")

struct ipu3\_uapi\_obgrid\_param
:   Optical black level compensation parameters

**Definition**:

```
struct ipu3_uapi_obgrid_param {
    __u16 gr;
    __u16 r;
    __u16 b;
    __u16 gb;
};
```

**Members**

`gr`
:   Grid table values for color GR

`r`
:   Grid table values for color R

`b`
:   Grid table values for color B

`gb`
:   Grid table values for color GB

**Description**

Black level is different for red, green, and blue channels. So black level
compensation is different per channel.

struct ipu3\_uapi\_flags
:   bits to indicate which pipeline needs update

**Definition**:

```
struct ipu3_uapi_flags {
    __u32 gdc:1;
    __u32 obgrid:1;
    __u32 reserved1:30;
    __u32 acc_bnr:1;
    __u32 acc_green_disparity:1;
    __u32 acc_dm:1;
    __u32 acc_ccm:1;
    __u32 acc_gamma:1;
    __u32 acc_csc:1;
    __u32 acc_cds:1;
    __u32 acc_shd:1;
    __u32 reserved2:2;
    __u32 acc_iefd:1;
    __u32 acc_yds_c0:1;
    __u32 acc_chnr_c0:1;
    __u32 acc_y_ee_nr:1;
    __u32 acc_yds:1;
    __u32 acc_chnr:1;
    __u32 acc_ytm:1;
    __u32 acc_yds2:1;
    __u32 acc_tcc:1;
    __u32 acc_dpc:1;
    __u32 acc_bds:1;
    __u32 acc_anr:1;
    __u32 acc_awb_fr:1;
    __u32 acc_ae:1;
    __u32 acc_af:1;
    __u32 acc_awb:1;
    __u32 reserved3:4;
    __u32 lin_vmem_params:1;
    __u32 tnr3_vmem_params:1;
    __u32 xnr3_vmem_params:1;
    __u32 tnr3_dmem_params:1;
    __u32 xnr3_dmem_params:1;
    __u32 reserved4:1;
    __u32 obgrid_param:1;
    __u32 reserved5:25;
};
```

**Members**

`gdc`
:   0 = no update, 1 = update.

`obgrid`
:   0 = no update, 1 = update.

`reserved1`
:   Not used.

`acc_bnr`
:   0 = no update, 1 = update.

`acc_green_disparity`
:   0 = no update, 1 = update.

`acc_dm`
:   0 = no update, 1 = update.

`acc_ccm`
:   0 = no update, 1 = update.

`acc_gamma`
:   0 = no update, 1 = update.

`acc_csc`
:   0 = no update, 1 = update.

`acc_cds`
:   0 = no update, 1 = update.

`acc_shd`
:   0 = no update, 1 = update.

`reserved2`
:   Not used.

`acc_iefd`
:   0 = no update, 1 = update.

`acc_yds_c0`
:   0 = no update, 1 = update.

`acc_chnr_c0`
:   0 = no update, 1 = update.

`acc_y_ee_nr`
:   0 = no update, 1 = update.

`acc_yds`
:   0 = no update, 1 = update.

`acc_chnr`
:   0 = no update, 1 = update.

`acc_ytm`
:   0 = no update, 1 = update.

`acc_yds2`
:   0 = no update, 1 = update.

`acc_tcc`
:   0 = no update, 1 = update.

`acc_dpc`
:   0 = no update, 1 = update.

`acc_bds`
:   0 = no update, 1 = update.

`acc_anr`
:   0 = no update, 1 = update.

`acc_awb_fr`
:   0 = no update, 1 = update.

`acc_ae`
:   0 = no update, 1 = update.

`acc_af`
:   0 = no update, 1 = update.

`acc_awb`
:   0 = no update, 1 = update.

`reserved3`
:   Not used.

`lin_vmem_params`
:   0 = no update, 1 = update.

`tnr3_vmem_params`
:   0 = no update, 1 = update.

`xnr3_vmem_params`
:   0 = no update, 1 = update.

`tnr3_dmem_params`
:   0 = no update, 1 = update.

`xnr3_dmem_params`
:   0 = no update, 1 = update.

`reserved4`
:   Not used.

`obgrid_param`
:   0 = no update, 1 = update.

`reserved5`
:   Not used.

struct ipu3\_uapi\_params
:   V4L2\_META\_FMT\_IPU3\_PARAMS

**Definition**:

```
struct ipu3_uapi_params {
    struct ipu3_uapi_flags use;
    struct ipu3_uapi_acc_param acc_param;
    struct ipu3_uapi_isp_lin_vmem_params lin_vmem_params;
    struct ipu3_uapi_isp_tnr3_vmem_params tnr3_vmem_params;
    struct ipu3_uapi_isp_xnr3_vmem_params xnr3_vmem_params;
    struct ipu3_uapi_isp_tnr3_params tnr3_dmem_params;
    struct ipu3_uapi_isp_xnr3_params xnr3_dmem_params;
    struct ipu3_uapi_obgrid_param obgrid_param;
};
```

**Members**

`use`
:   select which parameters to apply, see [`ipu3_uapi_flags`](#c.V4L.ipu3_uapi_flags "ipu3_uapi_flags")

`acc_param`
:   ACC parameters, as specified by [`ipu3_uapi_acc_param`](#c.V4L.ipu3_uapi_acc_param "ipu3_uapi_acc_param")

`lin_vmem_params`
:   linearization VMEM, as specified by
    [`ipu3_uapi_isp_lin_vmem_params`](#c.V4L.ipu3_uapi_isp_lin_vmem_params "ipu3_uapi_isp_lin_vmem_params")

`tnr3_vmem_params`
:   tnr3 VMEM as specified by
    [`ipu3_uapi_isp_tnr3_vmem_params`](#c.V4L.ipu3_uapi_isp_tnr3_vmem_params "ipu3_uapi_isp_tnr3_vmem_params")

`xnr3_vmem_params`
:   xnr3 VMEM as specified by
    [`ipu3_uapi_isp_xnr3_vmem_params`](#c.V4L.ipu3_uapi_isp_xnr3_vmem_params "ipu3_uapi_isp_xnr3_vmem_params")

`tnr3_dmem_params`
:   tnr3 DMEM as specified by [`ipu3_uapi_isp_tnr3_params`](#c.V4L.ipu3_uapi_isp_tnr3_params "ipu3_uapi_isp_tnr3_params")

`xnr3_dmem_params`
:   xnr3 DMEM as specified by [`ipu3_uapi_isp_xnr3_params`](#c.V4L.ipu3_uapi_isp_xnr3_params "ipu3_uapi_isp_xnr3_params")

`obgrid_param`
:   obgrid parameters as specified by
    [`ipu3_uapi_obgrid_param`](#c.V4L.ipu3_uapi_obgrid_param "ipu3_uapi_obgrid_param")

**Description**

The video queue “parameters” is of format V4L2\_META\_FMT\_IPU3\_PARAMS.
This is a “single plane” v4l2\_meta\_format using V4L2\_BUF\_TYPE\_META\_OUTPUT.

[`struct ipu3_uapi_params`](#c.V4L.ipu3_uapi_params "V4L.ipu3_uapi_params") as defined below contains a lot of parameters and
ipu3\_uapi\_flags selects which parameters to apply.
