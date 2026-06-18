# 2.13.9.V4L2_META_FMT_RK_ISP1_PARAMS (‘rk1p’), V4L2_META_FMT_RK_ISP1_STAT_3A (‘rk1s’), V4L2_META_FMT_RK_ISP1_EXT_PARAMS (‘rk1e’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/metafmt-rkisp1.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.13.9. V4L2\_META\_FMT\_RK\_ISP1\_PARAMS (‘rk1p’), V4L2\_META\_FMT\_RK\_ISP1\_STAT\_3A (‘rk1s’), V4L2\_META\_FMT\_RK\_ISP1\_EXT\_PARAMS (‘rk1e’)

## 2.13.9.1. Configuration parameters

The configuration of the RkISP1 ISP is performed by userspace by providing
parameters for the ISP to the driver using the [`v4l2_meta_format`](dev-meta.html#c.V4L.v4l2_meta_format "v4l2_meta_format")
interface.

There are two methods that allow to configure the ISP, the fixed parameters
configuration format and the extensible parameters configuration
format.

### 2.13.9.1.1. Fixed parameters configuration format

When using the fixed configuration format, parameters are passed to the
[rkisp1\_params](../../../admin-guide/media/rkisp1.html#rkisp1-params) metadata output video node, using
the V4L2\_META\_FMT\_RK\_ISP1\_PARAMS meta format.

The buffer contains a single instance of the C structure
[`rkisp1_params_cfg`](#c.V4L.rkisp1_params_cfg "rkisp1_params_cfg") defined in `rkisp1-config.h`. So the structure can
be obtained from the buffer by:

```
struct rkisp1_params_cfg *params = (struct rkisp1_params_cfg*) buffer;
```

This method supports a subset of the ISP features only, new applications should
use the extensible parameters method.

### 2.13.9.1.2. Extensible parameters configuration format

When using the extensible configuration format, parameters are passed to the
[rkisp1\_params](../../../admin-guide/media/rkisp1.html#rkisp1-params) metadata output video node, using
the V4L2\_META\_FMT\_RK\_ISP1\_EXT\_PARAMS meta format.

The buffer contains a single instance of the C structure
[`rkisp1_ext_params_cfg`](#c.V4L.rkisp1_ext_params_cfg "rkisp1_ext_params_cfg") defined in `rkisp1-config.h`. The
[`rkisp1_ext_params_cfg`](#c.V4L.rkisp1_ext_params_cfg "rkisp1_ext_params_cfg") structure is designed to allow userspace to
populate the data buffer with only the configuration data for the ISP blocks it
intends to configure. The extensible parameters format design allows developers
to define new block types to support new configuration parameters, and defines a
versioning scheme so that it can be extended and versioned without breaking
compatibility with existing applications.

For these reasons, this configuration method is preferred over the fixed
parameters format alternative.

## 2.13.9.2. 3A and histogram statistics

The ISP1 device collects different statistics over an input Bayer frame.
Those statistics are obtained from the [rkisp1\_stats](../../../admin-guide/media/rkisp1.html#rkisp1-stats)
metadata capture video node,
using the [`v4l2_meta_format`](dev-meta.html#c.V4L.v4l2_meta_format "v4l2_meta_format") interface. The buffer contains a single
instance of the C structure [`rkisp1_stat_buffer`](#c.V4L.rkisp1_stat_buffer "rkisp1_stat_buffer") defined in
`rkisp1-config.h`. So the structure can be obtained from the buffer by:

```
struct rkisp1_stat_buffer *stats = (struct rkisp1_stat_buffer*) buffer;
```

The statistics collected are Exposure, AWB (Auto-white balance), Histogram and
AF (Auto-focus). See [`rkisp1_stat_buffer`](#c.V4L.rkisp1_stat_buffer "rkisp1_stat_buffer") for details of the statistics.

The 3A statistics and configuration parameters described here are usually
consumed and produced by dedicated user space libraries that comprise the
important tuning tools using software control loop.

### 2.13.9.2.1. rkisp1 uAPI data types

enum rkisp1\_cif\_isp\_version
:   ISP variants

**Constants**

`RKISP1_V10`
:   Used at least in RK3288 and RK3399.

`RKISP1_V11`
:   Declared in the original vendor code, but not used. Same number
    of entries in grids and histogram as v10.

`RKISP1_V12`
:   Used at least in RK3326 and PX30.

`RKISP1_V13`
:   Used at least in RK1808. Same number of entries in grids and
    histogram as v12.

`RKISP1_V_IMX8MP`
:   Used in at least i.MX8MP. Same number of entries in grids
    and histogram as v10.

enum rkisp1\_cif\_isp\_exp\_ctrl\_autostop
:   stop modes

**Constants**

`RKISP1_CIF_ISP_EXP_CTRL_AUTOSTOP_0`
:   continuous measurement

`RKISP1_CIF_ISP_EXP_CTRL_AUTOSTOP_1`
:   stop measuring after a complete frame

enum rkisp1\_cif\_isp\_exp\_meas\_mode
:   Exposure measure mode

**Constants**

`RKISP1_CIF_ISP_EXP_MEASURING_MODE_0`
:   Y = 16 + 0.25R + 0.5G + 0.1094B

`RKISP1_CIF_ISP_EXP_MEASURING_MODE_1`
:   Y = (R + G + B) x (85/256)

struct rkisp1\_cif\_isp\_window
:   measurement window.

**Definition**:

```
struct rkisp1_cif_isp_window {
    __u16 h_offs;
    __u16 v_offs;
    __u16 h_size;
    __u16 v_size;
};
```

**Members**

`h_offs`
:   the horizontal offset of the window from the left of the frame in pixels.

`v_offs`
:   the vertical offset of the window from the top of the frame in pixels.

`h_size`
:   the horizontal size of the window in pixels

`v_size`
:   the vertical size of the window in pixels.

**Description**

Measurements are calculated per window inside the frame.
This `struct represents` a window for a measurement.

struct rkisp1\_cif\_isp\_bls\_fixed\_val
:   BLS fixed subtraction values

**Definition**:

```
struct rkisp1_cif_isp_bls_fixed_val {
    __s16 r;
    __s16 gr;
    __s16 gb;
    __s16 b;
};
```

**Members**

`r`
:   Fixed (signed!) subtraction value for Bayer pattern R

`gr`
:   Fixed (signed!) subtraction value for Bayer pattern Gr

`gb`
:   Fixed (signed!) subtraction value for Bayer pattern Gb

`b`
:   Fixed (signed!) subtraction value for Bayer pattern B

**Description**

The values will be subtracted from the sensor
values. Therefore a negative value means addition instead of subtraction!

struct rkisp1\_cif\_isp\_bls\_config
:   Configuration used by black level subtraction

**Definition**:

```
struct rkisp1_cif_isp_bls_config {
    __u8 enable_auto;
    __u8 en_windows;
    struct rkisp1_cif_isp_window bls_window1;
    struct rkisp1_cif_isp_window bls_window2;
    __u8 bls_samples;
    struct rkisp1_cif_isp_bls_fixed_val fixed_val;
};
```

**Members**

`enable_auto`
:   Automatic mode activated means that the measured values
    are subtracted. Otherwise the fixed subtraction
    values will be subtracted.

`en_windows`
:   enabled window

`bls_window1`
:   Measurement window 1 size

`bls_window2`
:   Measurement window 2 size

`bls_samples`
:   Set amount of measured pixels for each Bayer position
    (A, B,C and D) to 2^bls\_samples.

`fixed_val`
:   Fixed subtraction values

struct rkisp1\_cif\_isp\_dpcc\_methods\_config
:   DPCC methods set configuration

**Definition**:

```
struct rkisp1_cif_isp_dpcc_methods_config {
    __u32 method;
    __u32 line_thresh;
    __u32 line_mad_fac;
    __u32 pg_fac;
    __u32 rnd_thresh;
    __u32 rg_fac;
};
```

**Members**

`method`
:   Method enable bits (RKISP1\_CIF\_ISP\_DPCC\_METHODS\_SET\_\*)

`line_thresh`
:   Line threshold (RKISP1\_CIF\_ISP\_DPCC\_LINE\_THRESH\_\*)

`line_mad_fac`
:   Line Mean Absolute Difference factor (RKISP1\_CIF\_ISP\_DPCC\_LINE\_MAD\_FAC\_\*)

`pg_fac`
:   Peak gradient factor (RKISP1\_CIF\_ISP\_DPCC\_PG\_FAC\_\*)

`rnd_thresh`
:   Rank Neighbor Difference threshold (RKISP1\_CIF\_ISP\_DPCC\_RND\_THRESH\_\*)

`rg_fac`
:   Rank gradient factor (RKISP1\_CIF\_ISP\_DPCC\_RG\_FAC\_\*)

**Description**

This structure stores the configuration of one set of methods for the DPCC
algorithm. Multiple methods can be selected in each set (independently for
the Green and Red/Blue components) through the **method** field, the result is
the logical AND of all enabled methods. The remaining fields set thresholds
and factors for each method.

struct rkisp1\_cif\_isp\_dpcc\_config
:   Configuration used by DPCC

**Definition**:

```
struct rkisp1_cif_isp_dpcc_config {
    __u32 mode;
    __u32 output_mode;
    __u32 set_use;
    struct rkisp1_cif_isp_dpcc_methods_config methods[RKISP1_CIF_ISP_DPCC_METHODS_MAX];
    __u32 ro_limits;
    __u32 rnd_offs;
};
```

**Members**

`mode`
:   DPCC mode (RKISP1\_CIF\_ISP\_DPCC\_MODE\_\*)

`output_mode`
:   Interpolation output mode (RKISP1\_CIF\_ISP\_DPCC\_OUTPUT\_MODE\_\*)

`set_use`
:   Methods sets selection (RKISP1\_CIF\_ISP\_DPCC\_SET\_USE\_\*)

`methods`
:   Methods sets configuration

`ro_limits`
:   Rank order limits (RKISP1\_CIF\_ISP\_DPCC\_RO\_LIMITS\_\*)

`rnd_offs`
:   Differential rank offsets for rank neighbor difference (RKISP1\_CIF\_ISP\_DPCC\_RND\_OFFS\_\*)

**Description**

Configuration used by Defect Pixel Cluster Correction. Three sets of methods
can be configured and selected through the **set\_use** field. The result is the
logical OR of all enabled sets.

struct rkisp1\_cif\_isp\_gamma\_corr\_curve
:   gamma curve point definition y-axis (output).

**Definition**:

```
struct rkisp1_cif_isp_gamma_corr_curve {
    __u16 gamma_y[RKISP1_CIF_ISP_DEGAMMA_CURVE_SIZE];
};
```

**Members**

`gamma_y`
:   the values for the y-axis of gamma curve points. Each value is 12 bit.

**Description**

The reset values define a linear curve which has the same effect as bypass. Reset values are:
gamma\_y[0] = 0x0000, gamma\_y[1] = 0x0100, ... gamma\_y[15] = 0x0f00, gamma\_y[16] = 0xfff

struct rkisp1\_cif\_isp\_gamma\_curve\_x\_axis\_pnts
:   De-Gamma Curve definition x increments (sampling points). gamma\_dx0 is for the lower samples (1-8), gamma\_dx1 is for the higher samples (9-16). The reset values for both fields is 0x44444444. This means that each sample is 4 units away from the previous one on the x-axis.

**Definition**:

```
struct rkisp1_cif_isp_gamma_curve_x_axis_pnts {
    __u32 gamma_dx0;
    __u32 gamma_dx1;
};
```

**Members**

`gamma_dx0`
:   gamma curve sample points definitions. Bits 0:2 for sample 1. Bit 3 unused.
    Bits 4:6 for sample 2. bit 7 unused ... Bits 28:30 for sample 8. Bit 31 unused

`gamma_dx1`
:   gamma curve sample points definitions. Bits 0:2 for sample 9. Bit 3 unused.
    Bits 4:6 for sample 10. bit 7 unused ... Bits 28:30 for sample 16. Bit 31 unused

struct rkisp1\_cif\_isp\_sdg\_config
:   Configuration used by sensor degamma

**Definition**:

```
struct rkisp1_cif_isp_sdg_config {
    struct rkisp1_cif_isp_gamma_corr_curve curve_r;
    struct rkisp1_cif_isp_gamma_corr_curve curve_g;
    struct rkisp1_cif_isp_gamma_corr_curve curve_b;
    struct rkisp1_cif_isp_gamma_curve_x_axis_pnts xa_pnts;
};
```

**Members**

`curve_r`
:   gamma curve point definition axis for red

`curve_g`
:   gamma curve point definition axis for green

`curve_b`
:   gamma curve point definition axis for blue

`xa_pnts`
:   x axis increments

struct rkisp1\_cif\_isp\_lsc\_config
:   Configuration used by Lens shading correction

**Definition**:

```
struct rkisp1_cif_isp_lsc_config {
    __u16 r_data_tbl[RKISP1_CIF_ISP_LSC_SAMPLES_MAX][RKISP1_CIF_ISP_LSC_SAMPLES_MAX];
    __u16 gr_data_tbl[RKISP1_CIF_ISP_LSC_SAMPLES_MAX][RKISP1_CIF_ISP_LSC_SAMPLES_MAX];
    __u16 gb_data_tbl[RKISP1_CIF_ISP_LSC_SAMPLES_MAX][RKISP1_CIF_ISP_LSC_SAMPLES_MAX];
    __u16 b_data_tbl[RKISP1_CIF_ISP_LSC_SAMPLES_MAX][RKISP1_CIF_ISP_LSC_SAMPLES_MAX];
    __u16 x_grad_tbl[RKISP1_CIF_ISP_LSC_SECTORS_TBL_SIZE];
    __u16 y_grad_tbl[RKISP1_CIF_ISP_LSC_SECTORS_TBL_SIZE];
    __u16 x_size_tbl[RKISP1_CIF_ISP_LSC_SECTORS_TBL_SIZE];
    __u16 y_size_tbl[RKISP1_CIF_ISP_LSC_SECTORS_TBL_SIZE];
    __u16 config_width;
    __u16 config_height;
};
```

**Members**

`r_data_tbl`
:   sample table red

`gr_data_tbl`
:   sample table green (red)

`gb_data_tbl`
:   sample table green (blue)

`b_data_tbl`
:   sample table blue

`x_grad_tbl`
:   gradient table x

`y_grad_tbl`
:   gradient table y

`x_size_tbl`
:   size table x

`y_size_tbl`
:   size table y

`config_width`
:   not used at the moment

`config_height`
:   not used at the moment

struct rkisp1\_cif\_isp\_ie\_config
:   Configuration used by image effects

**Definition**:

```
struct rkisp1_cif_isp_ie_config {
    __u16 effect;
    __u16 color_sel;
    __u16 eff_mat_1;
    __u16 eff_mat_2;
    __u16 eff_mat_3;
    __u16 eff_mat_4;
    __u16 eff_mat_5;
    __u16 eff_tint;
};
```

**Members**

`effect`
:   values from ‘`enum v4l2_colorfx`’. Possible values are: V4L2\_COLORFX\_SEPIA,
    V4L2\_COLORFX\_SET\_CBCR, V4L2\_COLORFX\_AQUA, V4L2\_COLORFX\_EMBOSS,
    V4L2\_COLORFX\_SKETCH, V4L2\_COLORFX\_BW, V4L2\_COLORFX\_NEGATIVE

`color_sel`
:   bits 0:2 - colors bitmask (001 - blue, 010 - green, 100 - red).
    bits 8:15 - Threshold value of the RGB colors for the color selection effect.

`eff_mat_1`
:   3x3 Matrix Coefficients for Emboss Effect 1

`eff_mat_2`
:   3x3 Matrix Coefficients for Emboss Effect 2

`eff_mat_3`
:   3x3 Matrix Coefficients for Emboss 3/Sketch 1

`eff_mat_4`
:   3x3 Matrix Coefficients for Sketch Effect 2

`eff_mat_5`
:   3x3 Matrix Coefficients for Sketch Effect 3

`eff_tint`
:   Chrominance increment values of tint (used for sepia effect)

struct rkisp1\_cif\_isp\_cproc\_config
:   Configuration used by Color Processing

**Definition**:

```
struct rkisp1_cif_isp_cproc_config {
    __u8 c_out_range;
    __u8 y_in_range;
    __u8 y_out_range;
    __u8 contrast;
    __u8 brightness;
    __u8 sat;
    __u8 hue;
};
```

**Members**

`c_out_range`
:   Chrominance pixel clipping range at output.
    (0 for limit, 1 for full)

`y_in_range`
:   Luminance pixel clipping range at output.

`y_out_range`
:   Luminance pixel clipping range at output.

`contrast`
:   00~ff, 0.0~1.992

`brightness`
:   80~7F, -128~+127

`sat`
:   saturation, 00~FF, 0.0~1.992

`hue`
:   80~7F, -90~+87.188

struct rkisp1\_cif\_isp\_awb\_meas\_config
:   Configuration for the AWB statistics

**Definition**:

```
struct rkisp1_cif_isp_awb_meas_config {
    struct rkisp1_cif_isp_window awb_wnd;
    __u32 awb_mode;
    __u8 max_y;
    __u8 min_y;
    __u8 max_csum;
    __u8 min_c;
    __u8 frames;
    __u8 awb_ref_cr;
    __u8 awb_ref_cb;
    __u8 enable_ymax_cmp;
};
```

**Members**

`awb_wnd`
:   white balance measurement window (in pixels)

`awb_mode`
:   the awb meas mode. From `enum rkisp1_cif_isp_awb_mode_type`.

`max_y`
:   only pixels values < max\_y contribute to awb measurement, set to 0
    to disable this feature

`min_y`
:   only pixels values > min\_y contribute to awb measurement

`max_csum`
:   Chrominance sum maximum value, only consider pixels with Cb+Cr,
    smaller than threshold for awb measurements

`min_c`
:   Chrominance minimum value, only consider pixels with Cb/Cr
    each greater than threshold value for awb measurements

`frames`
:   number of frames - 1 used for mean value calculation
    (ucFrames=0 means 1 Frame)

`awb_ref_cr`
:   reference Cr value for AWB regulation, target for AWB

`awb_ref_cb`
:   reference Cb value for AWB regulation, target for AWB

`enable_ymax_cmp`
:   enable Y\_MAX compare (Not valid in RGB measurement mode.)

struct rkisp1\_cif\_isp\_awb\_gain\_config
:   Configuration used by auto white balance gain

**Definition**:

```
struct rkisp1_cif_isp_awb_gain_config {
    __u16 gain_red;
    __u16 gain_green_r;
    __u16 gain_blue;
    __u16 gain_green_b;
};
```

**Members**

`gain_red`
:   gain value for red component.

`gain_green_r`
:   gain value for green component in red line.

`gain_blue`
:   gain value for blue component.

`gain_green_b`
:   gain value for green component in blue line.

**Description**

All fields in this `struct are` 10 bit, where:
0x100h = 1, unsigned integer value, range 0 to 4 with 8 bit fractional part.

out\_data\_x = ( AWB\_GAIN\_X \* in\_data + 128) >> 8

struct rkisp1\_cif\_isp\_flt\_config
:   Configuration used by ISP filtering

**Definition**:

```
struct rkisp1_cif_isp_flt_config {
    __u32 mode;
    __u8 grn_stage1;
    __u8 chr_h_mode;
    __u8 chr_v_mode;
    __u32 thresh_bl0;
    __u32 thresh_bl1;
    __u32 thresh_sh0;
    __u32 thresh_sh1;
    __u32 lum_weight;
    __u32 fac_sh1;
    __u32 fac_sh0;
    __u32 fac_mid;
    __u32 fac_bl0;
    __u32 fac_bl1;
};
```

**Members**

`mode`
:   ISP\_FILT\_MODE register fields (from `enum rkisp1_cif_isp_flt_mode`)

`grn_stage1`
:   Green filter stage 1 select (range 0x0...0x8)

`chr_h_mode`
:   Chroma filter horizontal mode

`chr_v_mode`
:   Chroma filter vertical mode

`thresh_bl0`
:   If thresh\_bl1 < sum\_grad < thresh\_bl0 then fac\_bl0 is selected (blurring th)

`thresh_bl1`
:   If sum\_grad < thresh\_bl1 then fac\_bl1 is selected (blurring th)

`thresh_sh0`
:   If thresh\_sh0 < sum\_grad < thresh\_sh1 then thresh\_sh0 is selected (sharpening th)

`thresh_sh1`
:   If thresh\_sh1 < sum\_grad then thresh\_sh1 is selected (sharpening th)

`lum_weight`
:   Parameters for luminance weight function.

`fac_sh1`
:   filter factor for sharp1 level

`fac_sh0`
:   filter factor for sharp0 level

`fac_mid`
:   filter factor for mid level and for static filter mode

`fac_bl0`
:   filter factor for blur 0 level

`fac_bl1`
:   filter factor for blur 1 level (max blur)

**Description**

All 4 threshold fields (thresh\_\*) are 10 bits.
All 6 factor fields (fac\_\*) are 6 bits.

struct rkisp1\_cif\_isp\_bdm\_config
:   Configuration used by Bayer DeMosaic

**Definition**:

```
struct rkisp1_cif_isp_bdm_config {
    __u8 demosaic_th;
};
```

**Members**

`demosaic_th`
:   threshold for bayer demosaicing texture detection

struct rkisp1\_cif\_isp\_ctk\_config
:   Configuration used by Cross Talk correction

**Definition**:

```
struct rkisp1_cif_isp_ctk_config {
    __u16 coeff[3][3];
    __u16 ct_offset[3];
};
```

**Members**

`coeff`
:   color correction matrix. Values are 11-bit signed fixed-point numbers with 4 bit integer
    and 7 bit fractional part, ranging from -8 (0x400) to +7.992 (0x3FF). 0 is
    represented by 0x000 and a coefficient value of 1 as 0x080.

`ct_offset`
:   Red, Green, Blue offsets for the crosstalk correction matrix

struct rkisp1\_cif\_isp\_goc\_config
:   Configuration used by Gamma Out correction

**Definition**:

```
struct rkisp1_cif_isp_goc_config {
    __u32 mode;
    __u16 gamma_y[RKISP1_CIF_ISP_GAMMA_OUT_MAX_SAMPLES];
};
```

**Members**

`mode`
:   goc mode (from `enum rkisp1_cif_isp_goc_mode`)

`gamma_y`
:   gamma out curve y-axis for all color components

**Description**

The number of entries of **gamma\_y** depends on the hardware revision
as is reported by the hw\_revision field of the `struct media_device_info`
that is returned by ioctl MEDIA\_IOC\_DEVICE\_INFO.

V10 has RKISP1\_CIF\_ISP\_GAMMA\_OUT\_MAX\_SAMPLES\_V10 entries, V12 has
RKISP1\_CIF\_ISP\_GAMMA\_OUT\_MAX\_SAMPLES\_V12 entries.
RKISP1\_CIF\_ISP\_GAMMA\_OUT\_MAX\_SAMPLES is equal to the maximum of the two.

struct rkisp1\_cif\_isp\_hst\_config
:   Configuration for Histogram statistics

**Definition**:

```
struct rkisp1_cif_isp_hst_config {
    __u32 mode;
    __u8 histogram_predivider;
    struct rkisp1_cif_isp_window meas_window;
    __u8 hist_weight[RKISP1_CIF_ISP_HISTOGRAM_WEIGHT_GRIDS_SIZE];
};
```

**Members**

`mode`
:   histogram mode (from `enum rkisp1_cif_isp_histogram_mode`)

`histogram_predivider`
:   process every stepsize pixel, all other pixels are
    skipped

`meas_window`
:   coordinates of the measure window

`hist_weight`
:   weighting factor for sub-windows

**Description**

The number of entries of **hist\_weight** depends on the hardware revision
as is reported by the hw\_revision field of the `struct media_device_info`
that is returned by ioctl MEDIA\_IOC\_DEVICE\_INFO.

V10 has RKISP1\_CIF\_ISP\_HISTOGRAM\_WEIGHT\_GRIDS\_SIZE\_V10 entries, V12 has
RKISP1\_CIF\_ISP\_HISTOGRAM\_WEIGHT\_GRIDS\_SIZE\_V12 entries.
RKISP1\_CIF\_ISP\_HISTOGRAM\_WEIGHT\_GRIDS\_SIZE is equal to the maximum of the
two.

struct rkisp1\_cif\_isp\_aec\_config
:   Configuration for Auto Exposure statistics

**Definition**:

```
struct rkisp1_cif_isp_aec_config {
    __u32 mode;
    __u32 autostop;
    struct rkisp1_cif_isp_window meas_window;
};
```

**Members**

`mode`
:   Exposure measure mode (from [`enum rkisp1_cif_isp_exp_meas_mode`](#c.V4L.rkisp1_cif_isp_exp_meas_mode "V4L.rkisp1_cif_isp_exp_meas_mode"))

`autostop`
:   stop mode (from [`enum rkisp1_cif_isp_exp_ctrl_autostop`](#c.V4L.rkisp1_cif_isp_exp_ctrl_autostop "V4L.rkisp1_cif_isp_exp_ctrl_autostop"))

`meas_window`
:   coordinates of the measure window

struct rkisp1\_cif\_isp\_afc\_config
:   Configuration for the Auto Focus statistics

**Definition**:

```
struct rkisp1_cif_isp_afc_config {
    __u8 num_afm_win;
    struct rkisp1_cif_isp_window afm_win[RKISP1_CIF_ISP_AFM_MAX_WINDOWS];
    __u32 thres;
    __u32 var_shift;
};
```

**Members**

`num_afm_win`
:   max RKISP1\_CIF\_ISP\_AFM\_MAX\_WINDOWS

`afm_win`
:   coordinates of the meas window

`thres`
:   threshold used for minimizing the influence of noise

`var_shift`
:   the number of bits for the shift operation at the end of the
    calculation chain.

enum rkisp1\_cif\_isp\_dpf\_gain\_usage
:   dpf gain usage

**Constants**

`RKISP1_CIF_ISP_DPF_GAIN_USAGE_DISABLED`
:   don’t use any gains in preprocessing stage

`RKISP1_CIF_ISP_DPF_GAIN_USAGE_NF_GAINS`
:   use only the noise function gains from
    registers DPF\_NF\_GAIN\_R, ...

`RKISP1_CIF_ISP_DPF_GAIN_USAGE_LSC_GAINS`
:   use only the gains from LSC module

`RKISP1_CIF_ISP_DPF_GAIN_USAGE_NF_LSC_GAINS`
:   use the noise function gains and the
    gains from LSC module

`RKISP1_CIF_ISP_DPF_GAIN_USAGE_AWB_GAINS`
:   use only the gains from AWB module

`RKISP1_CIF_ISP_DPF_GAIN_USAGE_AWB_LSC_GAINS`
:   use the gains from AWB and LSC module

`RKISP1_CIF_ISP_DPF_GAIN_USAGE_MAX`
:   upper border (only for an internal evaluation)

enum rkisp1\_cif\_isp\_dpf\_rb\_filtersize
:   Red and blue filter sizes

**Constants**

`RKISP1_CIF_ISP_DPF_RB_FILTERSIZE_13x9`
:   red and blue filter kernel size 13x9
    (means 7x5 active pixel)

`RKISP1_CIF_ISP_DPF_RB_FILTERSIZE_9x9`
:   red and blue filter kernel size 9x9
    (means 5x5 active pixel)

enum rkisp1\_cif\_isp\_dpf\_nll\_scale\_mode
:   dpf noise level scale mode

**Constants**

`RKISP1_CIF_ISP_NLL_SCALE_LINEAR`
:   use a linear scaling

`RKISP1_CIF_ISP_NLL_SCALE_LOGARITHMIC`
:   use a logarithmic scaling

struct rkisp1\_cif\_isp\_dpf\_nll
:   Noise level lookup

**Definition**:

```
struct rkisp1_cif_isp_dpf_nll {
    __u16 coeff[RKISP1_CIF_ISP_DPF_MAX_NLF_COEFFS];
    __u32 scale_mode;
};
```

**Members**

`coeff`
:   Noise level Lookup coefficient

`scale_mode`
:   dpf noise level scale mode (from [`enum rkisp1_cif_isp_dpf_nll_scale_mode`](#c.V4L.rkisp1_cif_isp_dpf_nll_scale_mode "V4L.rkisp1_cif_isp_dpf_nll_scale_mode"))

struct rkisp1\_cif\_isp\_dpf\_rb\_flt
:   Red blue filter config

**Definition**:

```
struct rkisp1_cif_isp_dpf_rb_flt {
    __u32 fltsize;
    __u8 spatial_coeff[RKISP1_CIF_ISP_DPF_MAX_SPATIAL_COEFFS];
    __u8 r_enable;
    __u8 b_enable;
};
```

**Members**

`fltsize`
:   The filter size for the red and blue pixels
    (from [`enum rkisp1_cif_isp_dpf_rb_filtersize`](#c.V4L.rkisp1_cif_isp_dpf_rb_filtersize "V4L.rkisp1_cif_isp_dpf_rb_filtersize"))

`spatial_coeff`
:   Spatial weights

`r_enable`
:   enable filter processing for red pixels

`b_enable`
:   enable filter processing for blue pixels

struct rkisp1\_cif\_isp\_dpf\_g\_flt
:   Green filter Configuration

**Definition**:

```
struct rkisp1_cif_isp_dpf_g_flt {
    __u8 spatial_coeff[RKISP1_CIF_ISP_DPF_MAX_SPATIAL_COEFFS];
    __u8 gr_enable;
    __u8 gb_enable;
};
```

**Members**

`spatial_coeff`
:   Spatial weights

`gr_enable`
:   enable filter processing for green pixels in green/red lines

`gb_enable`
:   enable filter processing for green pixels in green/blue lines

struct rkisp1\_cif\_isp\_dpf\_gain
:   Noise function Configuration

**Definition**:

```
struct rkisp1_cif_isp_dpf_gain {
    __u32 mode;
    __u16 nf_r_gain;
    __u16 nf_b_gain;
    __u16 nf_gr_gain;
    __u16 nf_gb_gain;
};
```

**Members**

`mode`
:   dpf gain usage (from [`enum rkisp1_cif_isp_dpf_gain_usage`](#c.V4L.rkisp1_cif_isp_dpf_gain_usage "V4L.rkisp1_cif_isp_dpf_gain_usage"))

`nf_r_gain`
:   Noise function Gain that replaces the AWB gain for red pixels

`nf_b_gain`
:   Noise function Gain that replaces the AWB gain for blue pixels

`nf_gr_gain`
:   Noise function Gain that replaces the AWB gain
    for green pixels in a red line

`nf_gb_gain`
:   Noise function Gain that replaces the AWB gain
    for green pixels in a blue line

struct rkisp1\_cif\_isp\_dpf\_config
:   Configuration used by De-noising pre-filter

**Definition**:

```
struct rkisp1_cif_isp_dpf_config {
    struct rkisp1_cif_isp_dpf_gain gain;
    struct rkisp1_cif_isp_dpf_g_flt g_flt;
    struct rkisp1_cif_isp_dpf_rb_flt rb_flt;
    struct rkisp1_cif_isp_dpf_nll nll;
};
```

**Members**

`gain`
:   noise function gain

`g_flt`
:   green filter config

`rb_flt`
:   red blue filter config

`nll`
:   noise level lookup

struct rkisp1\_cif\_isp\_dpf\_strength\_config
:   strength of the filter

**Definition**:

```
struct rkisp1_cif_isp_dpf_strength_config {
    __u8 r;
    __u8 g;
    __u8 b;
};
```

**Members**

`r`
:   filter strength of the RED filter

`g`
:   filter strength of the GREEN filter

`b`
:   filter strength of the BLUE filter

struct rkisp1\_cif\_isp\_isp\_other\_cfg
:   Parameters for some blocks in rockchip isp1

**Definition**:

```
struct rkisp1_cif_isp_isp_other_cfg {
    struct rkisp1_cif_isp_dpcc_config dpcc_config;
    struct rkisp1_cif_isp_bls_config bls_config;
    struct rkisp1_cif_isp_sdg_config sdg_config;
    struct rkisp1_cif_isp_lsc_config lsc_config;
    struct rkisp1_cif_isp_awb_gain_config awb_gain_config;
    struct rkisp1_cif_isp_flt_config flt_config;
    struct rkisp1_cif_isp_bdm_config bdm_config;
    struct rkisp1_cif_isp_ctk_config ctk_config;
    struct rkisp1_cif_isp_goc_config goc_config;
    struct rkisp1_cif_isp_dpf_config dpf_config;
    struct rkisp1_cif_isp_dpf_strength_config dpf_strength_config;
    struct rkisp1_cif_isp_cproc_config cproc_config;
    struct rkisp1_cif_isp_ie_config ie_config;
};
```

**Members**

`dpcc_config`
:   Defect Pixel Cluster Correction config

`bls_config`
:   black level subtraction config

`sdg_config`
:   sensor degamma config

`lsc_config`
:   Lens Shade config

`awb_gain_config`
:   Auto White balance gain config

`flt_config`
:   filter config

`bdm_config`
:   demosaic config

`ctk_config`
:   cross talk config

`goc_config`
:   gamma out config

`dpf_config`
:   De-noising pre-filter config

`dpf_strength_config`
:   dpf strength config

`cproc_config`
:   color process config

`ie_config`
:   image effects config

struct rkisp1\_cif\_isp\_isp\_meas\_cfg
:   Rockchip ISP1 Measure Parameters

**Definition**:

```
struct rkisp1_cif_isp_isp_meas_cfg {
    struct rkisp1_cif_isp_awb_meas_config awb_meas_config;
    struct rkisp1_cif_isp_hst_config hst_config;
    struct rkisp1_cif_isp_aec_config aec_config;
    struct rkisp1_cif_isp_afc_config afc_config;
};
```

**Members**

`awb_meas_config`
:   auto white balance config

`hst_config`
:   histogram config

`aec_config`
:   auto exposure config

`afc_config`
:   auto focus config

struct rkisp1\_params\_cfg
:   Rockchip ISP1 Input Parameters Meta Data

**Definition**:

```
struct rkisp1_params_cfg {
    __u32 module_en_update;
    __u32 module_ens;
    __u32 module_cfg_update;
    struct rkisp1_cif_isp_isp_meas_cfg meas;
    struct rkisp1_cif_isp_isp_other_cfg others;
};
```

**Members**

`module_en_update`
:   mask the enable bits of which module should be updated

`module_ens`
:   mask the enable value of each module, only update the module
    which correspond bit was set in module\_en\_update

`module_cfg_update`
:   mask the config bits of which module should be updated

`meas`
:   measurement config

`others`
:   other config

struct rkisp1\_cif\_isp\_compand\_bls\_config
:   Rockchip ISP1 Companding parameters (BLS)

**Definition**:

```
struct rkisp1_cif_isp_compand_bls_config {
    __u32 r;
    __u32 gr;
    __u32 gb;
    __u32 b;
};
```

**Members**

`r`
:   Fixed subtraction value for Bayer pattern R

`gr`
:   Fixed subtraction value for Bayer pattern Gr

`gb`
:   Fixed subtraction value for Bayer pattern Gb

`b`
:   Fixed subtraction value for Bayer pattern B

**Description**

The values will be subtracted from the sensor values. Note that unlike the
dedicated BLS block, the BLS values in the compander are 20-bit unsigned.

struct rkisp1\_cif\_isp\_compand\_curve\_config
:   Rockchip ISP1 Companding parameters (expand and compression curves)

**Definition**:

```
struct rkisp1_cif_isp_compand_curve_config {
    __u8 px[RKISP1_CIF_ISP_COMPAND_NUM_POINTS];
    __u32 x[RKISP1_CIF_ISP_COMPAND_NUM_POINTS];
    __u32 y[RKISP1_CIF_ISP_COMPAND_NUM_POINTS];
};
```

**Members**

`px`
:   Compand curve x-values. Each value stores the distance from the
    previous x-value, expressed as log2 of the distance on 5 bits.

`x`
:   Compand curve x-values. The functionality of these parameters are
    unknown due to do a lack of hardware documentation, but these are left
    here for future compatibility purposes.

`y`
:   Compand curve y-values

struct rkisp1\_cif\_isp\_wdr\_tone\_curve
:   Tone mapping curve definition for WDR.

**Definition**:

```
struct rkisp1_cif_isp_wdr_tone_curve {
    __u32 dY[RKISP1_CIF_ISP_WDR_CURVE_NUM_DY_REGS];
    __u16 ym[RKISP1_CIF_ISP_WDR_CURVE_NUM_COEFF];
};
```

**Members**

`dY`
:   the dYn increments for horizontal (input) axis of the tone curve.
    each 3-bit dY value represents an increment of 2\*\*(value+3).
    dY[0] bits 0:2 is increment dY1, bit 3 unused
    dY[0] bits 4:6 is increment dY2, bit 7 unused
    ...
    dY[0] bits 28:30 is increment dY8, bit 31 unused
    ... and so on till dY[3] bits 28:30 is increment dY32, bit 31 unused.

`ym`
:   the Ym values for the vertical (output) axis of the tone curve.
    each value is 13 bit.

struct rkisp1\_cif\_isp\_wdr\_iref\_config
:   Illumination reference config for WDR.

**Definition**:

```
struct rkisp1_cif_isp_wdr_iref_config {
    __u8 rgb_factor;
    __u8 use_y9_8;
    __u8 use_rgb7_8;
    __u8 disable_transient;
};
```

**Members**

`rgb_factor`
:   defines how much influence the RGBmax approach has in
    comparison to Y (valid values are 0..8).

`use_y9_8`
:   use Y\*9/8 for maximum value calculation along with the
    default of R, G, B for noise reduction.

`use_rgb7_8`
:   decrease RGBMax by 7/8 for noise reduction.

`disable_transient`
:   disable transient calculation between Y and RGBY\_max.

**Description**

Use illumination reference value as described below, instead of only the
luminance (Y) value for tone mapping and gain calculations:
IRef = (rgb\_factor \* RGBMax\_tr + (8 - rgb\_factor) \* Y)/8

struct rkisp1\_cif\_isp\_wdr\_config
:   Configuration for wide dynamic range.

**Definition**:

```
struct rkisp1_cif_isp_wdr_config {
    struct rkisp1_cif_isp_wdr_tone_curve tone_curve;
    struct rkisp1_cif_isp_wdr_iref_config iref_config;
    __u16 rgb_offset;
    __u16 luma_offset;
    __u16 dmin_thresh;
    __u8 dmin_strength;
    __u8 use_rgb_colorspace;
    __u8 bypass_chroma_mapping;
    __u8 use_iref;
};
```

**Members**

`tone_curve`
:   tone mapping curve.

`iref_config`
:   illumination reference configuration. (when use\_iref is true)

`rgb_offset`
:   RGB offset value for RGB operation mode. (12 bits)

`luma_offset`
:   luminance offset value for RGB operation mode. (12 bits)

`dmin_thresh`
:   lower threshold for deltaMin value. (12 bits)

`dmin_strength`
:   strength factor for deltaMin. (valid range is 0x00..0x10)

`use_rgb_colorspace`
:   use RGB instead of luminance/chrominance colorspace.

`bypass_chroma_mapping`
:   disable chrominance mapping (only valid if
    use\_rgb\_colorspace = 0)

`use_iref`
:   use illumination reference instead of Y for tone mapping
    and gain calculations.

struct rkisp1\_cif\_isp\_awb\_meas
:   AWB measured values

**Definition**:

```
struct rkisp1_cif_isp_awb_meas {
    __u32 cnt;
    __u8 mean_y_or_g;
    __u8 mean_cb_or_b;
    __u8 mean_cr_or_r;
};
```

**Members**

`cnt`
:   White pixel count, number of “white pixels” found during last
    measurement

`mean_y_or_g`
:   Mean value of Y within window and frames,
    Green if RGB is selected.

`mean_cb_or_b`
:   Mean value of Cb within window and frames,
    Blue if RGB is selected.

`mean_cr_or_r`
:   Mean value of Cr within window and frames,
    Red if RGB is selected.

struct rkisp1\_cif\_isp\_awb\_stat
:   statistics automatic white balance data

**Definition**:

```
struct rkisp1_cif_isp_awb_stat {
    struct rkisp1_cif_isp_awb_meas awb_mean[RKISP1_CIF_ISP_AWB_MAX_GRID];
};
```

**Members**

`awb_mean`
:   Mean measured data

struct rkisp1\_cif\_isp\_bls\_meas\_val
:   BLS measured values

**Definition**:

```
struct rkisp1_cif_isp_bls_meas_val {
    __u16 meas_r;
    __u16 meas_gr;
    __u16 meas_gb;
    __u16 meas_b;
};
```

**Members**

`meas_r`
:   Mean measured value for Bayer pattern R

`meas_gr`
:   Mean measured value for Bayer pattern Gr

`meas_gb`
:   Mean measured value for Bayer pattern Gb

`meas_b`
:   Mean measured value for Bayer pattern B

struct rkisp1\_cif\_isp\_ae\_stat
:   statistics auto exposure data

**Definition**:

```
struct rkisp1_cif_isp_ae_stat {
    __u8 exp_mean[RKISP1_CIF_ISP_AE_MEAN_MAX];
    struct rkisp1_cif_isp_bls_meas_val bls_val;
};
```

**Members**

`exp_mean`
:   Mean luminance value of block xx

`bls_val`
:   BLS measured values

**Description**

The number of entries of **exp\_mean** depends on the hardware revision
as is reported by the hw\_revision field of the `struct media_device_info`
that is returned by ioctl MEDIA\_IOC\_DEVICE\_INFO.

V10 has RKISP1\_CIF\_ISP\_AE\_MEAN\_MAX\_V10 entries, V12 has
RKISP1\_CIF\_ISP\_AE\_MEAN\_MAX\_V12 entries. RKISP1\_CIF\_ISP\_AE\_MEAN\_MAX is equal
to the maximum of the two.

Image is divided into 5x5 blocks on V10 and 9x9 blocks on V12.

struct rkisp1\_cif\_isp\_af\_meas\_val
:   AF measured values

**Definition**:

```
struct rkisp1_cif_isp_af_meas_val {
    __u32 sum;
    __u32 lum;
};
```

**Members**

`sum`
:   sharpness value

`lum`
:   luminance value

struct rkisp1\_cif\_isp\_af\_stat
:   statistics auto focus data

**Definition**:

```
struct rkisp1_cif_isp_af_stat {
    struct rkisp1_cif_isp_af_meas_val window[RKISP1_CIF_ISP_AFM_MAX_WINDOWS];
};
```

**Members**

`window`
:   AF measured value of window x

**Description**

The module measures the sharpness in 3 windows of selectable size via
register settings(ISP\_AFM\_\*\_A/B/C)

struct rkisp1\_cif\_isp\_hist\_stat
:   statistics histogram data

**Definition**:

```
struct rkisp1_cif_isp_hist_stat {
    __u32 hist_bins[RKISP1_CIF_ISP_HIST_BIN_N_MAX];
};
```

**Members**

`hist_bins`
:   measured bin counters. Each bin is a 20 bits unsigned fixed point
    type. Bits 0-4 are the fractional part and bits 5-19 are the
    integer part.

**Description**

The window of the measurements area is divided to 5x5 sub-windows for
V10 and to 9x9 sub-windows for V12. The histogram is then computed for each
sub-window independently and the final result is a weighted average of the
histogram measurements on all sub-windows. The window of the measurements
area and the weight of each sub-window are configurable using
struct **rkisp1\_cif\_isp\_hst\_config**.

The histogram contains 16 bins in V10 and 32 bins in V12.

The number of entries of **hist\_bins** depends on the hardware revision
as is reported by the hw\_revision field of the `struct media_device_info`
that is returned by ioctl MEDIA\_IOC\_DEVICE\_INFO.

V10 has RKISP1\_CIF\_ISP\_HIST\_BIN\_N\_MAX\_V10 entries, V12 has
RKISP1\_CIF\_ISP\_HIST\_BIN\_N\_MAX\_V12 entries. RKISP1\_CIF\_ISP\_HIST\_BIN\_N\_MAX is
equal to the maximum of the two.

struct rkisp1\_cif\_isp\_stat
:   Rockchip ISP1 Statistics Data

**Definition**:

```
struct rkisp1_cif_isp_stat {
    struct rkisp1_cif_isp_awb_stat awb;
    struct rkisp1_cif_isp_ae_stat ae;
    struct rkisp1_cif_isp_af_stat af;
    struct rkisp1_cif_isp_hist_stat hist;
};
```

**Members**

`awb`
:   statistics data for automatic white balance

`ae`
:   statistics data for auto exposure

`af`
:   statistics data for auto focus

`hist`
:   statistics histogram data

struct rkisp1\_stat\_buffer
:   Rockchip ISP1 Statistics Meta Data

**Definition**:

```
struct rkisp1_stat_buffer {
    __u32 meas_type;
    __u32 frame_id;
    struct rkisp1_cif_isp_stat params;
};
```

**Members**

`meas_type`
:   measurement types (RKISP1\_CIF\_ISP\_STAT\_\* definitions)

`frame_id`
:   frame ID for sync

`params`
:   statistics data

enum rkisp1\_ext\_params\_block\_type
:   RkISP1 extensible params block type

**Constants**

`RKISP1_EXT_PARAMS_BLOCK_TYPE_BLS`
:   Black level subtraction

`RKISP1_EXT_PARAMS_BLOCK_TYPE_DPCC`
:   Defect pixel cluster correction

`RKISP1_EXT_PARAMS_BLOCK_TYPE_SDG`
:   Sensor de-gamma

`RKISP1_EXT_PARAMS_BLOCK_TYPE_AWB_GAIN`
:   Auto white balance gains

`RKISP1_EXT_PARAMS_BLOCK_TYPE_FLT`
:   ISP filtering

`RKISP1_EXT_PARAMS_BLOCK_TYPE_BDM`
:   Bayer de-mosaic

`RKISP1_EXT_PARAMS_BLOCK_TYPE_CTK`
:   Cross-talk correction

`RKISP1_EXT_PARAMS_BLOCK_TYPE_GOC`
:   Gamma out correction

`RKISP1_EXT_PARAMS_BLOCK_TYPE_DPF`
:   De-noise pre-filter

`RKISP1_EXT_PARAMS_BLOCK_TYPE_DPF_STRENGTH`
:   De-noise pre-filter strength

`RKISP1_EXT_PARAMS_BLOCK_TYPE_CPROC`
:   Color processing

`RKISP1_EXT_PARAMS_BLOCK_TYPE_IE`
:   Image effects

`RKISP1_EXT_PARAMS_BLOCK_TYPE_LSC`
:   Lens shading correction

`RKISP1_EXT_PARAMS_BLOCK_TYPE_AWB_MEAS`
:   Auto white balance statistics

`RKISP1_EXT_PARAMS_BLOCK_TYPE_HST_MEAS`
:   Histogram statistics

`RKISP1_EXT_PARAMS_BLOCK_TYPE_AEC_MEAS`
:   Auto exposure statistics

`RKISP1_EXT_PARAMS_BLOCK_TYPE_AFC_MEAS`
:   Auto-focus statistics

`RKISP1_EXT_PARAMS_BLOCK_TYPE_COMPAND_BLS`
:   BLS in the compand block

`RKISP1_EXT_PARAMS_BLOCK_TYPE_COMPAND_EXPAND`
:   Companding expand curve

`RKISP1_EXT_PARAMS_BLOCK_TYPE_COMPAND_COMPRESS`
:   Companding compress curve

`RKISP1_EXT_PARAMS_BLOCK_TYPE_WDR`
:   Wide dynamic range

rkisp1\_ext\_params\_block\_header

`rkisp1_ext_params_block_header`

> > RkISP1 extensible parameters block header
>
> **Description**
>
> This structure represents the common part of all the ISP configuration
> blocks and is identical to [`v4l2_isp_params_block_header`](v4l2-isp.html#c.v4l2_isp_params_block_header "v4l2_isp_params_block_header").
>
> The type field is one of the values enumerated by
> [`rkisp1_ext_params_block_type`](#c.V4L.rkisp1_ext_params_block_type "rkisp1_ext_params_block_type") and specifies how the data should be
> interpreted by the driver.
>
> The flags field is a bitmask of per-block flags RKISP1\_EXT\_PARAMS\_FL\_\*.

struct rkisp1\_ext\_params\_bls\_config
:   RkISP1 extensible params BLS config

**Definition**:

```
struct rkisp1_ext_params_bls_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_bls_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   Black Level Subtraction configuration, see
    [`rkisp1_cif_isp_bls_config`](#c.V4L.rkisp1_cif_isp_bls_config "rkisp1_cif_isp_bls_config")

**Description**

RkISP1 extensible parameters Black Level Subtraction configuration block.
Identified by `RKISP1_EXT_PARAMS_BLOCK_TYPE_BLS`.

struct rkisp1\_ext\_params\_dpcc\_config
:   RkISP1 extensible params DPCC config

**Definition**:

```
struct rkisp1_ext_params_dpcc_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_dpcc_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   Defective Pixel Cluster Correction configuration, see
    [`rkisp1_cif_isp_dpcc_config`](#c.V4L.rkisp1_cif_isp_dpcc_config "rkisp1_cif_isp_dpcc_config")

**Description**

RkISP1 extensible parameters Defective Pixel Cluster Correction configuration
block. Identified by `RKISP1_EXT_PARAMS_BLOCK_TYPE_DPCC`.

struct rkisp1\_ext\_params\_sdg\_config
:   RkISP1 extensible params SDG config

**Definition**:

```
struct rkisp1_ext_params_sdg_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_sdg_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   Sensor Degamma configuration, see
    [`rkisp1_cif_isp_sdg_config`](#c.V4L.rkisp1_cif_isp_sdg_config "rkisp1_cif_isp_sdg_config")

**Description**

RkISP1 extensible parameters Sensor Degamma configuration block. Identified
by `RKISP1_EXT_PARAMS_BLOCK_TYPE_SDG`.

struct rkisp1\_ext\_params\_lsc\_config
:   RkISP1 extensible params LSC config

**Definition**:

```
struct rkisp1_ext_params_lsc_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_lsc_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   Lens Shading Correction configuration, see
    [`rkisp1_cif_isp_lsc_config`](#c.V4L.rkisp1_cif_isp_lsc_config "rkisp1_cif_isp_lsc_config")

**Description**

RkISP1 extensible parameters Lens Shading Correction configuration block.
Identified by `RKISP1_EXT_PARAMS_BLOCK_TYPE_LSC`.

struct rkisp1\_ext\_params\_awb\_gain\_config
:   RkISP1 extensible params AWB gain config

**Definition**:

```
struct rkisp1_ext_params_awb_gain_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_awb_gain_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   Auto-White Balance Gains configuration, see
    [`rkisp1_cif_isp_awb_gain_config`](#c.V4L.rkisp1_cif_isp_awb_gain_config "rkisp1_cif_isp_awb_gain_config")

**Description**

RkISP1 extensible parameters Auto-White Balance Gains configuration block.
Identified by `RKISP1_EXT_PARAMS_BLOCK_TYPE_AWB_GAIN`.

struct rkisp1\_ext\_params\_flt\_config
:   RkISP1 extensible params FLT config

**Definition**:

```
struct rkisp1_ext_params_flt_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_flt_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   Filter configuration, see [`rkisp1_cif_isp_flt_config`](#c.V4L.rkisp1_cif_isp_flt_config "rkisp1_cif_isp_flt_config")

**Description**

RkISP1 extensible parameters Filter configuration block. Identified by
`RKISP1_EXT_PARAMS_BLOCK_TYPE_FLT`.

struct rkisp1\_ext\_params\_bdm\_config
:   RkISP1 extensible params BDM config

**Definition**:

```
struct rkisp1_ext_params_bdm_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_bdm_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   Demosaicing configuration, see [`rkisp1_cif_isp_bdm_config`](#c.V4L.rkisp1_cif_isp_bdm_config "rkisp1_cif_isp_bdm_config")

**Description**

RkISP1 extensible parameters Demosaicing configuration block. Identified by
`RKISP1_EXT_PARAMS_BLOCK_TYPE_BDM`.

struct rkisp1\_ext\_params\_ctk\_config
:   RkISP1 extensible params CTK config

**Definition**:

```
struct rkisp1_ext_params_ctk_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_ctk_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   Cross-Talk configuration, see [`rkisp1_cif_isp_ctk_config`](#c.V4L.rkisp1_cif_isp_ctk_config "rkisp1_cif_isp_ctk_config")

**Description**

RkISP1 extensible parameters Cross-Talk configuration block. Identified by
`RKISP1_EXT_PARAMS_BLOCK_TYPE_CTK`.

struct rkisp1\_ext\_params\_goc\_config
:   RkISP1 extensible params GOC config

**Definition**:

```
struct rkisp1_ext_params_goc_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_goc_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   Gamma-Out configuration, see [`rkisp1_cif_isp_goc_config`](#c.V4L.rkisp1_cif_isp_goc_config "rkisp1_cif_isp_goc_config")

**Description**

RkISP1 extensible parameters Gamma-Out configuration block. Identified by
`RKISP1_EXT_PARAMS_BLOCK_TYPE_GOC`.

struct rkisp1\_ext\_params\_dpf\_config
:   RkISP1 extensible params DPF config

**Definition**:

```
struct rkisp1_ext_params_dpf_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_dpf_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   De-noise Pre-Filter configuration, see
    [`rkisp1_cif_isp_dpf_config`](#c.V4L.rkisp1_cif_isp_dpf_config "rkisp1_cif_isp_dpf_config")

**Description**

RkISP1 extensible parameters De-noise Pre-Filter configuration block.
Identified by `RKISP1_EXT_PARAMS_BLOCK_TYPE_DPF`.

struct rkisp1\_ext\_params\_dpf\_strength\_config
:   RkISP1 extensible params DPF strength config

**Definition**:

```
struct rkisp1_ext_params_dpf_strength_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_dpf_strength_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   De-noise Pre-Filter strength configuration, see
    [`rkisp1_cif_isp_dpf_strength_config`](#c.V4L.rkisp1_cif_isp_dpf_strength_config "rkisp1_cif_isp_dpf_strength_config")

**Description**

RkISP1 extensible parameters De-noise Pre-Filter strength configuration
block. Identified by `RKISP1_EXT_PARAMS_BLOCK_TYPE_DPF_STRENGTH`.

struct rkisp1\_ext\_params\_cproc\_config
:   RkISP1 extensible params CPROC config

**Definition**:

```
struct rkisp1_ext_params_cproc_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_cproc_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   Color processing configuration, see
    [`rkisp1_cif_isp_cproc_config`](#c.V4L.rkisp1_cif_isp_cproc_config "rkisp1_cif_isp_cproc_config")

**Description**

RkISP1 extensible parameters Color Processing configuration block.
Identified by `RKISP1_EXT_PARAMS_BLOCK_TYPE_CPROC`.

struct rkisp1\_ext\_params\_ie\_config
:   RkISP1 extensible params IE config

**Definition**:

```
struct rkisp1_ext_params_ie_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_ie_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   Image Effect configuration, see [`rkisp1_cif_isp_ie_config`](#c.V4L.rkisp1_cif_isp_ie_config "rkisp1_cif_isp_ie_config")

**Description**

RkISP1 extensible parameters Image Effect configuration block. Identified by
`RKISP1_EXT_PARAMS_BLOCK_TYPE_IE`.

struct rkisp1\_ext\_params\_awb\_meas\_config
:   RkISP1 extensible params AWB Meas config

**Definition**:

```
struct rkisp1_ext_params_awb_meas_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_awb_meas_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   Auto-White Balance measure configuration, see
    [`rkisp1_cif_isp_awb_meas_config`](#c.V4L.rkisp1_cif_isp_awb_meas_config "rkisp1_cif_isp_awb_meas_config")

**Description**

RkISP1 extensible parameters Auto-White Balance Measurement configuration
block. Identified by `RKISP1_EXT_PARAMS_BLOCK_TYPE_AWB_MEAS`.

struct rkisp1\_ext\_params\_hst\_config
:   RkISP1 extensible params Histogram config

**Definition**:

```
struct rkisp1_ext_params_hst_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_hst_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   Histogram statistics configuration, see
    [`rkisp1_cif_isp_hst_config`](#c.V4L.rkisp1_cif_isp_hst_config "rkisp1_cif_isp_hst_config")

**Description**

RkISP1 extensible parameters Histogram statistics configuration block.
Identified by `RKISP1_EXT_PARAMS_BLOCK_TYPE_HST_MEAS`.

struct rkisp1\_ext\_params\_aec\_config
:   RkISP1 extensible params AEC config

**Definition**:

```
struct rkisp1_ext_params_aec_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_aec_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   Auto-Exposure statistics configuration, see
    [`rkisp1_cif_isp_aec_config`](#c.V4L.rkisp1_cif_isp_aec_config "rkisp1_cif_isp_aec_config")

**Description**

RkISP1 extensible parameters Auto-Exposure statistics configuration block.
Identified by `RKISP1_EXT_PARAMS_BLOCK_TYPE_AEC_MEAS`.

struct rkisp1\_ext\_params\_afc\_config
:   RkISP1 extensible params AFC config

**Definition**:

```
struct rkisp1_ext_params_afc_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_afc_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   Auto-Focus statistics configuration, see
    [`rkisp1_cif_isp_afc_config`](#c.V4L.rkisp1_cif_isp_afc_config "rkisp1_cif_isp_afc_config")

**Description**

RkISP1 extensible parameters Auto-Focus statistics configuration block.
Identified by `RKISP1_EXT_PARAMS_BLOCK_TYPE_AFC_MEAS`.

struct rkisp1\_ext\_params\_compand\_bls\_config
:   RkISP1 extensible params Compand BLS config

**Definition**:

```
struct rkisp1_ext_params_compand_bls_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_compand_bls_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   Companding BLS configuration, see
    [`rkisp1_cif_isp_compand_bls_config`](#c.V4L.rkisp1_cif_isp_compand_bls_config "rkisp1_cif_isp_compand_bls_config")

**Description**

RkISP1 extensible parameters Companding configuration block (black level
subtraction). Identified by `RKISP1_EXT_PARAMS_BLOCK_TYPE_COMPAND_BLS`.

struct rkisp1\_ext\_params\_compand\_curve\_config
:   RkISP1 extensible params Compand curve config

**Definition**:

```
struct rkisp1_ext_params_compand_curve_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_compand_curve_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   Companding curve configuration, see
    [`rkisp1_cif_isp_compand_curve_config`](#c.V4L.rkisp1_cif_isp_compand_curve_config "rkisp1_cif_isp_compand_curve_config")

**Description**

RkISP1 extensible parameters Companding configuration block (expand and
compression curves). Identified by
`RKISP1_EXT_PARAMS_BLOCK_TYPE_COMPAND_EXPAND` or
`RKISP1_EXT_PARAMS_BLOCK_TYPE_COMPAND_COMPRESS`.

struct rkisp1\_ext\_params\_wdr\_config
:   RkISP1 extensible params Wide dynamic range config

**Definition**:

```
struct rkisp1_ext_params_wdr_config {
    struct rkisp1_ext_params_block_header header;
    struct rkisp1_cif_isp_wdr_config config;
};
```

**Members**

`header`
:   The RkISP1 extensible parameters header, see
    [`rkisp1_ext_params_block_header`](#c.V4L.rkisp1_ext_params_block_header "rkisp1_ext_params_block_header")

`config`
:   WDR configuration, see
    [`rkisp1_cif_isp_wdr_config`](#c.V4L.rkisp1_cif_isp_wdr_config "rkisp1_cif_isp_wdr_config")

**Description**

RkISP1 extensible parameters WDR block.
Identified by `RKISP1_EXT_PARAMS_BLOCK_TYPE_WDR`

enum rksip1\_ext\_param\_buffer\_version
:   RkISP1 extensible parameters version

**Constants**

`RKISP1_EXT_PARAM_BUFFER_V1`
:   First version of RkISP1 extensible parameters

struct rkisp1\_ext\_params\_cfg
:   RkISP1 extensible parameters configuration

**Definition**:

```
struct rkisp1_ext_params_cfg {
    __u32 version;
    __u32 data_size;
    __u8 data[RKISP1_EXT_PARAMS_MAX_SIZE];
};
```

**Members**

`version`
:   The RkISP1 extensible parameters buffer version, see
    [`rksip1_ext_param_buffer_version`](#c.V4L.rksip1_ext_param_buffer_version "rksip1_ext_param_buffer_version")

`data_size`
:   The RkISP1 configuration data effective size, excluding this
    header

`data`
:   The RkISP1 extensible configuration data blocks

**Description**

This is the driver-specific implementation of
[`v4l2_isp_params_buffer`](v4l2-isp.html#c.v4l2_isp_params_buffer "v4l2_isp_params_buffer").

Currently the single RKISP1\_EXT\_PARAM\_BUFFER\_V1 version is supported.
When a new format version will be added, a mechanism for userspace to query
the supported format versions will be implemented in the form of a read-only
V4L2 control. If such control is not available, userspace should assume only
RKISP1\_EXT\_PARAM\_BUFFER\_V1 is supported by the driver.

The read-only V4L2 control `RKISP1_CID_SUPPORTED_PARAMS_BLOCKS` can be used
to query the blocks supported by the device. It contains a bitmask where each
bit represents the availability of the corresponding entry from the
[`rkisp1_ext_params_block_type`](#c.V4L.rkisp1_ext_params_block_type "rkisp1_ext_params_block_type") enum. The current and default values
of the control represents the blocks supported by the device instance, while
the maximum value represents the blocks supported by the kernel driver,
independently of the device instance.

The expected memory layout of the parameters buffer is:

```
+-------------------- struct rkisp1_ext_params_cfg -------------------+
| version = RKISP1_EXT_PARAM_BUFFER_V1;                               |
| data_size = sizeof(struct rkisp1_ext_params_bls_config)             |
|           + sizeof(struct rkisp1_ext_params_dpcc_config);           |
| +------------------------- data  ---------------------------------+ |
| | +------------- struct rkisp1_ext_params_bls_config -----------+ | |
| | | +-------- struct rkisp1_ext_params_block_header  ---------+ | | |
| | | | type = RKISP1_EXT_PARAMS_BLOCK_TYPE_BLS;                | | | |
| | | | flags = RKISP1_EXT_PARAMS_FL_BLOCK_ENABLE;              | | | |
| | | | size = sizeof(struct rkisp1_ext_params_bls_config);     | | | |
| | | +---------------------------------------------------------+ | | |
| | | +---------- struct rkisp1_cif_isp_bls_config -------------+ | | |
| | | | enable_auto = 0;                                        | | | |
| | | | fixed_val.r = 256;                                      | | | |
| | | | fixed_val.gr = 256;                                     | | | |
| | | | fixed_val.gb = 256;                                     | | | |
| | | | fixed_val.b = 256;                                      | | | |
| | | +---------------------------------------------------------+ | | |
| | +------------ struct rkisp1_ext_params_dpcc_config -----------+ | |
| | | +-------- struct rkisp1_ext_params_block_header  ---------+ | | |
| | | | type = RKISP1_EXT_PARAMS_BLOCK_TYPE_DPCC;               | | | |
| | | | flags = RKISP1_EXT_PARAMS_FL_BLOCK_ENABLE;              | | | |
| | | | size = sizeof(struct rkisp1_ext_params_dpcc_config);    | | | |
| | | +---------------------------------------------------------+ | | |
| | | +---------- struct rkisp1_cif_isp_dpcc_config ------------+ | | |
| | | | mode = RKISP1_CIF_ISP_DPCC_MODE_STAGE1_ENABLE;          | | | |
| | | | output_mode =                                           | | | |
| | | |   RKISP1_CIF_ISP_DPCC_OUTPUT_MODE_STAGE1_INCL_G_CENTER; | | | |
| | | | set_use = ... ;                                         | | | |
| | | | ...  = ... ;                                            | | | |
| | | +---------------------------------------------------------+ | | |
| | +-------------------------------------------------------------+ | |
| +-----------------------------------------------------------------+ |
+---------------------------------------------------------------------+
```
