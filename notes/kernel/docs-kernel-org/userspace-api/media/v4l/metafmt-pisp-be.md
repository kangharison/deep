# 2.13.6.V4L2_META_FMT_RPI_BE_CFG

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/metafmt-pisp-be.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.13.6. V4L2\_META\_FMT\_RPI\_BE\_CFG

## 2.13.6.1. Raspberry Pi PiSP Back End configuration format

The Raspberry Pi PiSP Back End memory-to-memory image signal processor is
configured by userspace by providing a buffer of configuration parameters
to the pispbe-config output video device node using the
[`v4l2_meta_format`](dev-meta.html#c.V4L.v4l2_meta_format "v4l2_meta_format") interface.

The PiSP Back End processes images in tiles, and its configuration requires
specifying two different sets of parameters by populating the members of
[`pisp_be_tiles_config`](#c.V4L.pisp_be_tiles_config "pisp_be_tiles_config") defined in the `pisp_be_config.h` header file.

The [Raspberry Pi PiSP technical specification](https://datasheets.raspberrypi.com/camera/raspberry-pi-image-signal-processor-specification.pdf)
provide detailed description of the ISP back end configuration and programming
model.

### 2.13.6.1.1. Global configuration data

The global configuration data describe how the pixels in a particular image are
to be processed and is therefore shared across all the tiles of the image. So
for example, LSC (Lens Shading Correction) or Denoise parameters would be common
across all tiles from the same frame.

Global configuration data are passed to the ISP by populating the member of
[`pisp_be_config`](#c.V4L.pisp_be_config "pisp_be_config").

### 2.13.6.1.2. Tile parameters

As the ISP processes images in tiles, each set of tiles parameters describe how
a single tile in an image is going to be processed. A single set of tile
parameters consist of 160 bytes of data and to process a batch of tiles several
sets of tiles parameters are required.

Tiles parameters are passed to the ISP by populating the member of
`pisp_tile` and the `num_tiles` fields of [`pisp_be_tiles_config`](#c.V4L.pisp_be_tiles_config "pisp_be_tiles_config").

## 2.13.6.2. Raspberry Pi PiSP Back End uAPI data types

This section describes the data types exposed to userspace by the Raspberry Pi
PiSP Back End. The section is informative only, for a detailed description of
each field refer to the [Raspberry Pi PiSP technical specification](https://datasheets.raspberrypi.com/camera/raspberry-pi-image-signal-processor-specification.pdf).

struct pisp\_be\_global\_config
:   PiSP global enable bitmaps

**Definition**:

```
struct pisp_be_global_config {
    __u32 bayer_enables;
    __u32 rgb_enables;
    __u8 bayer_order;
    __u8 pad[3];
};
```

**Members**

`bayer_enables`
:   Bayer input enable flags

`rgb_enables`
:   RGB output enable flags

`bayer_order`
:   Bayer input format ordering

`pad`
:   Padding bytes

struct pisp\_be\_input\_buffer\_config
:   PiSP Back End input buffer

**Definition**:

```
struct pisp_be_input_buffer_config {
    __u32 addr[3][2];
};
```

**Members**

`addr`
:   Input buffer address

struct pisp\_be\_dpc\_config
:   PiSP Back End DPC config

**Definition**:

```
struct pisp_be_dpc_config {
    __u8 coeff_level;
    __u8 coeff_range;
    __u8 pad;
#define PISP_BE_DPC_FLAG_FOLDBACK 1;
    __u8 flags;
};
```

**Members**

`coeff_level`
:   Coefficient for the darkest neighbouring pixel value

`coeff_range`
:   Coefficient for the range of pixels for this Bayer channel

`pad`
:   Padding byte

`flags`
:   DPC configuration flags

**Description**

Defective Pixel Correction configuration

struct pisp\_be\_geq\_config
:   PiSP Back End GEQ config

**Definition**:

```
struct pisp_be_geq_config {
    __u16 offset;
#define PISP_BE_GEQ_SHARPER (1U << 15);
#define PISP_BE_GEQ_SLOPE ((1 << 10) - 1);
    __u16 slope_sharper;
    __u16 min;
    __u16 max;
};
```

**Members**

`offset`
:   Offset value for threshold calculation

`slope_sharper`
:   Slope/Sharper configuration

`min`
:   Minimum value the threshold may have

`max`
:   Maximum value the threshold may have

**Description**

Green Equalisation configuration

struct pisp\_be\_tdn\_input\_buffer\_config
:   PiSP Back End TDN input buffer

**Definition**:

```
struct pisp_be_tdn_input_buffer_config {
    __u32 addr[2];
};
```

**Members**

`addr`
:   TDN input buffer address

struct pisp\_be\_tdn\_config
:   PiSP Back End TDN config

**Definition**:

```
struct pisp_be_tdn_config {
    __u16 black_level;
    __u16 ratio;
    __u16 noise_constant;
    __u16 noise_slope;
    __u16 threshold;
    __u8 reset;
    __u8 pad;
};
```

**Members**

`black_level`
:   Black level value subtracted from pixels

`ratio`
:   Multiplier for the LTA input frame

`noise_constant`
:   Constant offset value used in noise estimation

`noise_slope`
:   Noise estimation multiplier

`threshold`
:   Threshold for TDN operations

`reset`
:   Disable TDN operations

`pad`
:   Padding byte

**Description**

Temporal Denoise configuration

struct pisp\_be\_tdn\_output\_buffer\_config
:   PiSP Back End TDN output buffer

**Definition**:

```
struct pisp_be_tdn_output_buffer_config {
    __u32 addr[2];
};
```

**Members**

`addr`
:   TDN output buffer address

struct pisp\_be\_sdn\_config
:   PiSP Back End SDN config

**Definition**:

```
struct pisp_be_sdn_config {
    __u16 black_level;
    __u8 leakage;
    __u8 pad;
    __u16 noise_constant;
    __u16 noise_slope;
    __u16 noise_constant2;
    __u16 noise_slope2;
};
```

**Members**

`black_level`
:   Black level subtracted from pixel for noise estimation

`leakage`
:   Proportion of the original undenoised value to mix in
    denoised output

`pad`
:   Padding byte

`noise_constant`
:   Noise constant used for noise estimation

`noise_slope`
:   Noise slope value used for noise estimation

`noise_constant2`
:   Second noise constant used for noise estimation

`noise_slope2`
:   Second slope value used for noise estimation

**Description**

Spatial Denoise configuration

struct pisp\_be\_stitch\_input\_buffer\_config
:   PiSP Back End Stitch input

**Definition**:

```
struct pisp_be_stitch_input_buffer_config {
    __u32 addr[2];
};
```

**Members**

`addr`
:   Stitch input buffer address

struct pisp\_be\_stitch\_config
:   PiSP Back End Stitch config

**Definition**:

```
struct pisp_be_stitch_config {
    __u16 threshold_lo;
    __u8 threshold_diff_power;
    __u8 pad;
    __u16 exposure_ratio;
    __u8 motion_threshold_256;
    __u8 motion_threshold_recip;
};
```

**Members**

`threshold_lo`
:   Low threshold value

`threshold_diff_power`
:   Low and high threshold difference

`pad`
:   Padding bytes

`exposure_ratio`
:   Multiplier to convert long exposure pixels into
    short exposure pixels

`motion_threshold_256`
:   Motion threshold above which short exposure
    pixels are used

`motion_threshold_recip`
:   Reciprocal of motion\_threshold\_256 value

**Description**

Stitch block configuration

struct pisp\_be\_stitch\_output\_buffer\_config
:   PiSP Back End Stitch output

**Definition**:

```
struct pisp_be_stitch_output_buffer_config {
    __u32 addr[2];
};
```

**Members**

`addr`
:   Stitch input buffer address

struct pisp\_be\_cdn\_config
:   PiSP Back End CDN config

**Definition**:

```
struct pisp_be_cdn_config {
    __u16 thresh;
    __u8 iir_strength;
    __u8 g_adjust;
};
```

**Members**

`thresh`
:   Constant for noise estimation

`iir_strength`
:   Relative strength of the IIR part of the filter

`g_adjust`
:   Proportion of the change assigned to the G channel

**Description**

Colour Denoise configuration

struct pisp\_be\_lsc\_config
:   PiSP Back End LSC config

**Definition**:

```
struct pisp_be_lsc_config {
    __u16 grid_step_x;
    __u16 grid_step_y;
#define PISP_BE_LSC_LUT_SIZE    (PISP_BE_LSC_GRID_SIZE + 1);
    __u32 lut_packed[PISP_BE_LSC_LUT_SIZE][PISP_BE_LSC_LUT_SIZE];
};
```

**Members**

`grid_step_x`
:   Reciprocal of cell size width

`grid_step_y`
:   Reciprocal of cell size height

`lut_packed`
:   Jointly-coded RGB gains for each LSC grid

**Description**

Lens Shading Correction configuration

struct pisp\_be\_lsc\_extra
:   PiSP Back End LSC Extra config

**Definition**:

```
struct pisp_be_lsc_extra {
    __u16 offset_x;
    __u16 offset_y;
};
```

**Members**

`offset_x`
:   Horizontal offset into the LSC table of this tile

`offset_y`
:   Vertical offset into the LSC table of this tile

struct pisp\_be\_cac\_config
:   PiSP Back End CAC config

**Definition**:

```
struct pisp_be_cac_config {
    __u16 grid_step_x;
    __u16 grid_step_y;
#define PISP_BE_CAC_LUT_SIZE            (PISP_BE_CAC_GRID_SIZE + 1);
    __s8 lut[PISP_BE_CAC_LUT_SIZE][PISP_BE_CAC_LUT_SIZE][2][2];
};
```

**Members**

`grid_step_x`
:   Reciprocal of cell size width

`grid_step_y`
:   Reciprocal of cell size height

`lut`
:   Pixel shift for the CAC grid

**Description**

Chromatic Aberration Correction config

struct pisp\_be\_cac\_extra
:   PiSP Back End CAC extra config

**Definition**:

```
struct pisp_be_cac_extra {
    __u16 offset_x;
    __u16 offset_y;
};
```

**Members**

`offset_x`
:   Horizontal offset into the CAC table of this tile

`offset_y`
:   Horizontal offset into the CAC table of this tile

struct pisp\_be\_debin\_config
:   PiSP Back End Debin config

**Definition**:

```
struct pisp_be_debin_config {
    __s8 coeffs[PISP_BE_DEBIN_NUM_COEFFS];
    __s8 h_enable;
    __s8 v_enable;
    __s8 pad[2];
};
```

**Members**

`coeffs`
:   Filter coefficients for debinning

`h_enable`
:   Horizontal debinning enable

`v_enable`
:   Vertical debinning enable

`pad`
:   Padding bytes

**Description**

Debinning configuration

struct pisp\_be\_tonemap\_config
:   PiSP Back End Tonemap config

**Definition**:

```
struct pisp_be_tonemap_config {
    __u16 detail_constant;
    __u16 detail_slope;
    __u16 iir_strength;
    __u16 strength;
    __u32 lut[PISP_BE_TONEMAP_LUT_SIZE];
};
```

**Members**

`detail_constant`
:   Constant value for threshold calculation

`detail_slope`
:   Slope value for threshold calculation

`iir_strength`
:   Relative strength of the IIR fiter

`strength`
:   Strength factor

`lut`
:   Look-up table for tonemap curve

**Description**

Tonemapping configuration

struct pisp\_be\_demosaic\_config
:   PiSP Back End Demosaic config

**Definition**:

```
struct pisp_be_demosaic_config {
    __u8 sharper;
    __u8 fc_mode;
    __u8 pad[2];
};
```

**Members**

`sharper`
:   Use other Bayer channels to increase sharpness

`fc_mode`
:   Built-in false colour suppression mode

`pad`
:   Padding bytes

**Description**

Demosaic configuration

struct pisp\_be\_ccm\_config
:   PiSP Back End CCM config

**Definition**:

```
struct pisp_be_ccm_config {
    __s16 coeffs[9];
    __u8 pad[2];
    __s32 offsets[3];
};
```

**Members**

`coeffs`
:   Matrix coefficients

`pad`
:   Padding bytes

`offsets`
:   Offsets triplet

**Description**

Colour Correction Matrix configuration

struct pisp\_be\_sat\_control\_config
:   PiSP Back End SAT config

**Definition**:

```
struct pisp_be_sat_control_config {
    __u8 shift_r;
    __u8 shift_g;
    __u8 shift_b;
    __u8 pad;
};
```

**Members**

`shift_r`
:   Left shift for Red colour channel

`shift_g`
:   Left shift for Green colour channel

`shift_b`
:   Left shift for Blue colour channel

`pad`
:   Padding byte

**Description**

Saturation Control configuration

struct pisp\_be\_false\_colour\_config
:   PiSP Back End False Colour config

**Definition**:

```
struct pisp_be_false_colour_config {
    __u8 distance;
    __u8 pad[3];
};
```

**Members**

`distance`
:   Distance of neighbouring pixels, either 1 or 2

`pad`
:   Padding bytes

**Description**

False Colour configuration

struct pisp\_be\_sharpen\_config
:   PiSP Back End Sharpening config

**Definition**:

```
struct pisp_be_sharpen_config {
    __s8 kernel0[PISP_BE_SHARPEN_SIZE * PISP_BE_SHARPEN_SIZE];
    __s8 pad0[3];
    __s8 kernel1[PISP_BE_SHARPEN_SIZE * PISP_BE_SHARPEN_SIZE];
    __s8 pad1[3];
    __s8 kernel2[PISP_BE_SHARPEN_SIZE * PISP_BE_SHARPEN_SIZE];
    __s8 pad2[3];
    __s8 kernel3[PISP_BE_SHARPEN_SIZE * PISP_BE_SHARPEN_SIZE];
    __s8 pad3[3];
    __s8 kernel4[PISP_BE_SHARPEN_SIZE * PISP_BE_SHARPEN_SIZE];
    __s8 pad4[3];
    __u16 threshold_offset0;
    __u16 threshold_slope0;
    __u16 scale0;
    __u16 pad5;
    __u16 threshold_offset1;
    __u16 threshold_slope1;
    __u16 scale1;
    __u16 pad6;
    __u16 threshold_offset2;
    __u16 threshold_slope2;
    __u16 scale2;
    __u16 pad7;
    __u16 threshold_offset3;
    __u16 threshold_slope3;
    __u16 scale3;
    __u16 pad8;
    __u16 threshold_offset4;
    __u16 threshold_slope4;
    __u16 scale4;
    __u16 pad9;
    __u16 positive_strength;
    __u16 positive_pre_limit;
    __u16 positive_func[PISP_BE_SHARPEN_FUNC_NUM_POINTS];
    __u16 positive_limit;
    __u16 negative_strength;
    __u16 negative_pre_limit;
    __u16 negative_func[PISP_BE_SHARPEN_FUNC_NUM_POINTS];
    __u16 negative_limit;
    __u8 enables;
    __u8 white;
    __u8 black;
    __u8 grey;
};
```

**Members**

`kernel0`
:   Coefficient for filter 0

`pad0`
:   Padding byte

`kernel1`
:   Coefficient for filter 1

`pad1`
:   Padding byte

`kernel2`
:   Coefficient for filter 2

`pad2`
:   Padding byte

`kernel3`
:   Coefficient for filter 3

`pad3`
:   Padding byte

`kernel4`
:   Coefficient for filter 4

`pad4`
:   Padding byte

`threshold_offset0`
:   Offset for filter 0 response calculation

`threshold_slope0`
:   Slope multiplier for the filter 0 response calculation

`scale0`
:   Scale factor for filter 0 response calculation

`pad5`
:   Padding byte

`threshold_offset1`
:   Offset for filter 0 response calculation

`threshold_slope1`
:   Slope multiplier for the filter 0 response calculation

`scale1`
:   Scale factor for filter 0 response calculation

`pad6`
:   Padding byte

`threshold_offset2`
:   Offset for filter 0 response calculation

`threshold_slope2`
:   Slope multiplier for the filter 0 response calculation

`scale2`
:   Scale factor for filter 0 response calculation

`pad7`
:   Padding byte

`threshold_offset3`
:   Offset for filter 0 response calculation

`threshold_slope3`
:   Slope multiplier for the filter 0 response calculation

`scale3`
:   Scale factor for filter 0 response calculation

`pad8`
:   Padding byte

`threshold_offset4`
:   Offset for filter 0 response calculation

`threshold_slope4`
:   Slope multiplier for the filter 0 response calculation

`scale4`
:   Scale factor for filter 0 response calculation

`pad9`
:   Padding byte

`positive_strength`
:   Factor to scale the positive sharpening strength

`positive_pre_limit`
:   Maximum allowed possible positive sharpening value

`positive_func`
:   Gain factor applied to positive sharpening response

`positive_limit`
:   Final gain factor applied to positive sharpening

`negative_strength`
:   Factor to scale the negative sharpening strength

`negative_pre_limit`
:   Maximum allowed possible negative sharpening value

`negative_func`
:   Gain factor applied to negative sharpening response

`negative_limit`
:   Final gain factor applied to negative sharpening

`enables`
:   Filter enable mask

`white`
:   White output pixel filter mask

`black`
:   Black output pixel filter mask

`grey`
:   Grey output pixel filter mask

**Description**

Sharpening configuration

struct pisp\_be\_sh\_fc\_combine\_config
:   PiSP Back End Sharpening and False Colour config

**Definition**:

```
struct pisp_be_sh_fc_combine_config {
    __u8 y_factor;
    __u8 c1_factor;
    __u8 c2_factor;
    __u8 pad;
};
```

**Members**

`y_factor`
:   Control amount of desaturation of pixels being darkened

`c1_factor`
:   Control amount of brightening of a pixel for the Cb
    channel

`c2_factor`
:   Control amount of brightening of a pixel for the Cr
    channel

`pad`
:   Padding byte

**Description**

Sharpening and False Colour configuration

struct pisp\_be\_gamma\_config
:   PiSP Back End Gamma configuration

**Definition**:

```
struct pisp_be_gamma_config {
    __u32 lut[PISP_BE_GAMMA_LUT_SIZE];
};
```

**Members**

`lut`
:   Gamma curve look-up table

struct pisp\_be\_crop\_config
:   PiSP Back End Crop config

**Definition**:

```
struct pisp_be_crop_config {
    __u16 offset_x, offset_y;
    __u16 width, height;
};
```

**Members**

`offset_x`
:   Number of pixels cropped from the left of the tile

`offset_y`
:   Number of pixels cropped from the top of the tile

`width`
:   Width of the cropped tile output

`height`
:   Height of the cropped tile output

**Description**

Crop configuration

struct pisp\_be\_resample\_config
:   PiSP Back End Resampling config

**Definition**:

```
struct pisp_be_resample_config {
    __u16 scale_factor_h, scale_factor_v;
    __s16 coef[PISP_BE_RESAMPLE_FILTER_SIZE];
};
```

**Members**

`scale_factor_h`
:   Horizontal scale factor

`scale_factor_v`
:   Vertical scale factor

`coef`
:   Resample coefficients

**Description**

Resample configuration

struct pisp\_be\_resample\_extra
:   PiSP Back End Resample config

**Definition**:

```
struct pisp_be_resample_extra {
    __u16 scaled_width;
    __u16 scaled_height;
    __s16 initial_phase_h[3];
    __s16 initial_phase_v[3];
};
```

**Members**

`scaled_width`
:   Width in pixels of the scaled output

`scaled_height`
:   Height in pixels of the scaled output

`initial_phase_h`
:   Initial horizontal phase

`initial_phase_v`
:   Initial vertical phase

**Description**

Resample configuration

struct pisp\_be\_downscale\_config
:   PiSP Back End Downscale config

**Definition**:

```
struct pisp_be_downscale_config {
    __u16 scale_factor_h;
    __u16 scale_factor_v;
    __u16 scale_recip_h;
    __u16 scale_recip_v;
};
```

**Members**

`scale_factor_h`
:   Horizontal scale factor

`scale_factor_v`
:   Vertical scale factor

`scale_recip_h`
:   Horizontal reciprocal factor

`scale_recip_v`
:   Vertical reciprocal factor

**Description**

Downscale configuration

struct pisp\_be\_downscale\_extra
:   PiSP Back End Downscale Extra config

**Definition**:

```
struct pisp_be_downscale_extra {
    __u16 scaled_width;
    __u16 scaled_height;
};
```

**Members**

`scaled_width`
:   Scaled image width

`scaled_height`
:   Scaled image height

struct pisp\_be\_hog\_config
:   PiSP Back End HOG config

**Definition**:

```
struct pisp_be_hog_config {
    __u8 compute_signed;
    __u8 channel_mix[3];
    __u32 stride;
};
```

**Members**

`compute_signed`
:   Set 0 for unsigned gradients, 1 for signed

`channel_mix`
:   Channels proportions to use

`stride`
:   Stride in bytes between blocks directly below

**Description**

Histogram of Oriented Gradients configuration

enum pisp\_be\_transform
:   PiSP Back End Transform flags

**Constants**

`PISP_BE_TRANSFORM_NONE`
:   No transform

`PISP_BE_TRANSFORM_HFLIP`
:   Horizontal flip

`PISP_BE_TRANSFORM_VFLIP`
:   Vertical flip

`PISP_BE_TRANSFORM_ROT180`
:   180 degress rotation

struct pisp\_be\_output\_buffer\_config
:   PiSP Back End Output buffer

**Definition**:

```
struct pisp_be_output_buffer_config {
    __u32 addr[3][2];
};
```

**Members**

`addr`
:   Output buffer address

struct pisp\_be\_hog\_buffer\_config
:   PiSP Back End HOG buffer

**Definition**:

```
struct pisp_be_hog_buffer_config {
    __u32 addr[2];
};
```

**Members**

`addr`
:   HOG buffer address

struct pisp\_be\_config
:   RaspberryPi PiSP Back End Processing configuration

**Definition**:

```
struct pisp_be_config {
    struct pisp_be_input_buffer_config input_buffer;
    struct pisp_be_tdn_input_buffer_config tdn_input_buffer;
    struct pisp_be_stitch_input_buffer_config stitch_input_buffer;
    struct pisp_be_tdn_output_buffer_config tdn_output_buffer;
    struct pisp_be_stitch_output_buffer_config stitch_output_buffer;
    struct pisp_be_output_buffer_config output_buffer[PISP_BACK_END_NUM_OUTPUTS];
    struct pisp_be_hog_buffer_config hog_buffer;
    struct pisp_be_global_config global;
    struct pisp_image_format_config input_format;
    struct pisp_decompress_config decompress;
    struct pisp_be_dpc_config dpc;
    struct pisp_be_geq_config geq;
    struct pisp_image_format_config tdn_input_format;
    struct pisp_decompress_config tdn_decompress;
    struct pisp_be_tdn_config tdn;
    struct pisp_compress_config tdn_compress;
    struct pisp_image_format_config tdn_output_format;
    struct pisp_be_sdn_config sdn;
    struct pisp_bla_config blc;
    struct pisp_compress_config stitch_compress;
    struct pisp_image_format_config stitch_output_format;
    struct pisp_image_format_config stitch_input_format;
    struct pisp_decompress_config stitch_decompress;
    struct pisp_be_stitch_config stitch;
    struct pisp_be_lsc_config lsc;
    struct pisp_wbg_config wbg;
    struct pisp_be_cdn_config cdn;
    struct pisp_be_cac_config cac;
    struct pisp_be_debin_config debin;
    struct pisp_be_tonemap_config tonemap;
    struct pisp_be_demosaic_config demosaic;
    struct pisp_be_ccm_config ccm;
    struct pisp_be_sat_control_config sat_control;
    struct pisp_be_ccm_config ycbcr;
    struct pisp_be_sharpen_config sharpen;
    struct pisp_be_false_colour_config false_colour;
    struct pisp_be_sh_fc_combine_config sh_fc_combine;
    struct pisp_be_ccm_config ycbcr_inverse;
    struct pisp_be_gamma_config gamma;
    struct pisp_be_ccm_config csc[PISP_BACK_END_NUM_OUTPUTS];
    struct pisp_be_downscale_config downscale[PISP_BACK_END_NUM_OUTPUTS];
    struct pisp_be_resample_config resample[PISP_BACK_END_NUM_OUTPUTS];
    struct pisp_be_output_format_config output_format[PISP_BACK_END_NUM_OUTPUTS];
    struct pisp_be_hog_config hog;
    struct pisp_be_axi_config axi;
    struct pisp_be_lsc_extra lsc_extra;
    struct pisp_be_cac_extra cac_extra;
    struct pisp_be_downscale_extra downscale_extra[PISP_BACK_END_NUM_OUTPUTS];
    struct pisp_be_resample_extra resample_extra[PISP_BACK_END_NUM_OUTPUTS];
    struct pisp_be_crop_config crop;
    struct pisp_image_format_config hog_format;
    __u32 dirty_flags_bayer;
    __u32 dirty_flags_rgb;
    __u32 dirty_flags_extra;
};
```

**Members**

`input_buffer`
:   Input buffer addresses

`tdn_input_buffer`
:   TDN input buffer addresses

`stitch_input_buffer`
:   Stitch input buffer addresses

`tdn_output_buffer`
:   TDN output buffer addresses

`stitch_output_buffer`
:   Stitch output buffer addresses

`output_buffer`
:   Output buffers addresses

`hog_buffer`
:   HOG buffer addresses

`global`
:   Global PiSP configuration

`input_format`
:   Input image format

`decompress`
:   Decompress configuration

`dpc`
:   Defective Pixel Correction configuration

`geq`
:   Green Equalisation configuration

`tdn_input_format`
:   Temporal Denoise input format

`tdn_decompress`
:   Temporal Denoise decompress configuration

`tdn`
:   Temporal Denoise configuration

`tdn_compress`
:   Temporal Denoise compress configuration

`tdn_output_format`
:   Temporal Denoise output format

`sdn`
:   Spatial Denoise configuration

`blc`
:   Black Level Correction configuration

`stitch_compress`
:   Stitch compress configuration

`stitch_output_format`
:   Stitch output format

`stitch_input_format`
:   Stitch input format

`stitch_decompress`
:   Stitch decompress configuration

`stitch`
:   Stitch configuration

`lsc`
:   Lens Shading Correction configuration

`wbg`
:   White Balance Gain configuration

`cdn`
:   Colour Denoise configuration

`cac`
:   Colour Aberration Correction configuration

`debin`
:   Debinning configuration

`tonemap`
:   Tonemapping configuration

`demosaic`
:   Demosaicing configuration

`ccm`
:   Colour Correction Matrix configuration

`sat_control`
:   Saturation Control configuration

`ycbcr`
:   YCbCr colour correction configuration

`sharpen`
:   Sharpening configuration

`false_colour`
:   False colour correction

`sh_fc_combine`
:   Sharpening and False Colour correction

`ycbcr_inverse`
:   Inverse YCbCr colour correction

`gamma`
:   Gamma curve configuration

`csc`
:   Color Space Conversion configuration

`downscale`
:   Downscale configuration

`resample`
:   Resampling configuration

`output_format`
:   Output format configuration

`hog`
:   HOG configuration

`axi`
:   AXI bus configuration

`lsc_extra`
:   LSC extra info

`cac_extra`
:   CAC extra info

`downscale_extra`
:   Downscaler extra info

`resample_extra`
:   Resample extra info

`crop`
:   Crop configuration

`hog_format`
:   HOG format info

`dirty_flags_bayer`
:   Bayer enable dirty flags
    (`pisp_be_bayer_enable`)

`dirty_flags_rgb`
:   RGB enable dirty flags
    (`pisp_be_rgb_enable`)

`dirty_flags_extra`
:   Extra dirty flags

enum pisp\_tile\_edge
:   PiSP Back End Tile position

**Constants**

`PISP_LEFT_EDGE`
:   Left edge tile

`PISP_RIGHT_EDGE`
:   Right edge tile

`PISP_TOP_EDGE`
:   Top edge tile

`PISP_BOTTOM_EDGE`
:   Bottom edge tile

struct pisp\_tile
:   Raspberry Pi PiSP Back End tile configuration

**Definition**:

```
struct pisp_tile {
    __u8 edge;
    __u8 pad0[3];
    __u32 input_addr_offset;
    __u32 input_addr_offset2;
    __u16 input_offset_x;
    __u16 input_offset_y;
    __u16 input_width;
    __u16 input_height;
    __u32 tdn_input_addr_offset;
    __u32 tdn_output_addr_offset;
    __u32 stitch_input_addr_offset;
    __u32 stitch_output_addr_offset;
    __u32 lsc_grid_offset_x;
    __u32 lsc_grid_offset_y;
    __u32 cac_grid_offset_x;
    __u32 cac_grid_offset_y;
    __u16 crop_x_start[PISP_BACK_END_NUM_OUTPUTS];
    __u16 crop_x_end[PISP_BACK_END_NUM_OUTPUTS];
    __u16 crop_y_start[PISP_BACK_END_NUM_OUTPUTS];
    __u16 crop_y_end[PISP_BACK_END_NUM_OUTPUTS];
    __u16 downscale_phase_x[3 * PISP_BACK_END_NUM_OUTPUTS];
    __u16 downscale_phase_y[3 * PISP_BACK_END_NUM_OUTPUTS];
    __u16 resample_in_width[PISP_BACK_END_NUM_OUTPUTS];
    __u16 resample_in_height[PISP_BACK_END_NUM_OUTPUTS];
    __u16 resample_phase_x[3 * PISP_BACK_END_NUM_OUTPUTS];
    __u16 resample_phase_y[3 * PISP_BACK_END_NUM_OUTPUTS];
    __u16 output_offset_x[PISP_BACK_END_NUM_OUTPUTS];
    __u16 output_offset_y[PISP_BACK_END_NUM_OUTPUTS];
    __u16 output_width[PISP_BACK_END_NUM_OUTPUTS];
    __u16 output_height[PISP_BACK_END_NUM_OUTPUTS];
    __u32 output_addr_offset[PISP_BACK_END_NUM_OUTPUTS];
    __u32 output_addr_offset2[PISP_BACK_END_NUM_OUTPUTS];
    __u32 output_hog_addr_offset;
};
```

**Members**

`edge`
:   Edge tile flag

`pad0`
:   Padding bytes

`input_addr_offset`
:   Top-left pixel offset, in bytes

`input_addr_offset2`
:   Top-left pixel offset, in bytes for the second/
    third image planes

`input_offset_x`
:   Horizontal offset in pixels of this tile in the
    input image

`input_offset_y`
:   Vertical offset in pixels of this tile in the
    input image

`input_width`
:   Width in pixels of this tile

`input_height`
:   Height in pixels of the this tile

`tdn_input_addr_offset`
:   TDN input image offset, in bytes

`tdn_output_addr_offset`
:   TDN output image offset, in bytes

`stitch_input_addr_offset`
:   Stitch input image offset, in bytes

`stitch_output_addr_offset`
:   Stitch output image offset, in bytes

`lsc_grid_offset_x`
:   Horizontal offset in the LSC table for this tile

`lsc_grid_offset_y`
:   Vertical offset in the LSC table for this tile

`cac_grid_offset_x`
:   Horizontal offset in the CAC table for this tile

`cac_grid_offset_y`
:   Horizontal offset in the CAC table for this tile

`crop_x_start`
:   Number of pixels cropped from the left of the
    tile

`crop_x_end`
:   Number of pixels cropped from the right of the
    tile

`crop_y_start`
:   Number of pixels cropped from the top of the
    tile

`crop_y_end`
:   Number of pixels cropped from the bottom of the
    tile

`downscale_phase_x`
:   Initial horizontal phase in pixels

`downscale_phase_y`
:   Initial vertical phase in pixels

`resample_in_width`
:   Width in pixels of the tile entering the
    Resample block

`resample_in_height`
:   Height in pixels of the tile entering the
    Resample block

`resample_phase_x`
:   Initial horizontal phase for the Resample block

`resample_phase_y`
:   Initial vertical phase for the Resample block

`output_offset_x`
:   Horizontal offset in pixels where the tile will
    be written into the output image

`output_offset_y`
:   Vertical offset in pixels where the tile will be
    written into the output image

`output_width`
:   Width in pixels in the output image of this tile

`output_height`
:   Height in pixels in the output image of this tile

`output_addr_offset`
:   Offset in bytes into the output buffer

`output_addr_offset2`
:   Offset in bytes into the output buffer for the
    second and third plane

`output_hog_addr_offset`
:   Offset in bytes into the HOG buffer where
    results of this tile are to be written

**Description**

Tile parameters: each set of tile parameters is a 160-bytes block of data
which contains the tile processing parameters.

struct pisp\_be\_tiles\_config
:   Raspberry Pi PiSP Back End configuration

**Definition**:

```
struct pisp_be_tiles_config {
    struct pisp_be_config config;
    struct pisp_tile tiles[PISP_BACK_END_NUM_TILES];
    __u32 num_tiles;
};
```

**Members**

`config`
:   PiSP Back End configuration

`tiles`
:   Tile descriptors

`num_tiles`
:   Number of tiles
