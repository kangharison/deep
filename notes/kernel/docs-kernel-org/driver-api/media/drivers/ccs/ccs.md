# 10.1.14.MIPI CCS camera sensor driver

> 출처(원문): https://docs.kernel.org/driver-api/media/drivers/ccs/ccs.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 10.1.14. MIPI CCS camera sensor driver

The MIPI CCS camera sensor driver is a generic driver for [MIPI CCS](https://www.mipi.org/specifications/camera-command-set) compliant
camera sensors.

Also see [the CCS driver UAPI documentation](../../../../userspace-api/media/drivers/ccs.html#media-ccs-uapi).

## 10.1.14.1. CCS static data

The MIPI CCS driver supports CCS static data for all compliant devices,
including not just those compliant with CCS 1.1 but also CCS 1.0 and SMIA(++).
For CCS the file names are formed as

> ccs/ccs-sensor-vvvv-mmmm-rrrr.fw (sensor) and
> ccs/ccs-module-vvvv-mmmm-rrrr.fw (module).

For SMIA++ compliant devices the corresponding file names are

> ccs/smiapp-sensor-vv-mmmm-rr.fw (sensor) and
> ccs/smiapp-module-vv-mmmm-rrrr.fw (module).

For SMIA (non-++) compliant devices the static data file name is

> ccs/smia-sensor-vv-mmmm-rr.fw (sensor).

vvvv or vv denotes MIPI and SMIA manufacturer IDs respectively, mmmm model ID
and rrrr or rr revision number.

### 10.1.14.1.1. CCS tools

[CCS tools](https://github.com/MIPI-Alliance/ccs-tools/) is a set of
tools for working with CCS static data files. CCS tools includes a
definition of the human-readable CCS static data YAML format and includes a
program to convert it to a binary.

## 10.1.14.2. Register definition generator

The ccs-regs.asc file contains MIPI CCS register definitions that are used
to produce C source code files for definitions that can be better used by
programs written in C language. As there are many dependencies between the
produced files, please do not modify them manually as it’s error-prone and
in vain, but instead change the script producing them.

### 10.1.14.2.1. Usage

Conventionally the script is called this way to update the CCS driver
definitions:

```
$ Documentation/driver-api/media/drivers/ccs/mk-ccs-regs -k \
        -e drivers/media/i2c/ccs/ccs-regs.h \
        -L drivers/media/i2c/ccs/ccs-limits.h \
        -l drivers/media/i2c/ccs/ccs-limits.c \
        -c Documentation/driver-api/media/drivers/ccs/ccs-regs.asc
```

# 10.1.15. CCS PLL calculator

The CCS PLL calculator is used to compute the PLL configuration, given sensor’s
capabilities as well as board configuration and user specified configuration. As
the configuration space that encompasses all these configurations is vast, the
PLL calculator isn’t entirely trivial. Yet it is relatively simple to use for a
driver.

The PLL model implemented by the PLL calculator corresponds to MIPI CCS 1.1.

struct ccs\_pll\_branch\_fr
:   CCS PLL configuration (front)

**Definition**:

```
struct ccs_pll_branch_fr {
    u16 pre_pll_clk_div;
    u16 pll_multiplier;
    u32 pll_ip_clk_freq_hz;
    u32 pll_op_clk_freq_hz;
};
```

**Members**

`pre_pll_clk_div`
:   Pre-PLL clock divisor

`pll_multiplier`
:   PLL multiplier

`pll_ip_clk_freq_hz`
:   PLL input clock frequency

`pll_op_clk_freq_hz`
:   PLL output clock frequency

**Description**

A single branch front-end of the CCS PLL tree.

struct ccs\_pll\_branch\_bk
:   CCS PLL configuration (back)

**Definition**:

```
struct ccs_pll_branch_bk {
    u16 sys_clk_div;
    u16 pix_clk_div;
    u32 sys_clk_freq_hz;
    u32 pix_clk_freq_hz;
};
```

**Members**

`sys_clk_div`
:   System clock divider

`pix_clk_div`
:   Pixel clock divider

`sys_clk_freq_hz`
:   System clock frequency

`pix_clk_freq_hz`
:   Pixel clock frequency

**Description**

A single branch back-end of the CCS PLL tree.

struct ccs\_pll
:   Full CCS PLL configuration

**Definition**:

```
struct ccs_pll {
    u8 bus_type;
    u8 op_lanes;
    u8 vt_lanes;
    struct {
        u8 lanes;
    } csi2;
    u8 binning_horizontal;
    u8 binning_vertical;
    u8 scale_m;
    u8 scale_n;
    u8 bits_per_pixel;
    u8 op_bits_per_lane;
    u16 flags;
    u32 link_freq;
    u32 ext_clk_freq_hz;
    struct ccs_pll_branch_fr vt_fr;
    struct ccs_pll_branch_bk vt_bk;
    struct ccs_pll_branch_fr op_fr;
    struct ccs_pll_branch_bk op_bk;
    u32 pixel_rate_csi;
    u32 pixel_rate_pixel_array;
};
```

**Members**

`bus_type`
:   Type of the data bus, CCS\_PLL\_BUS\_TYPE\_\* (input)

`op_lanes`
:   Number of operational lanes (input)

`vt_lanes`
:   Number of video timing lanes (input)

`csi2`
:   CSI-2 related parameters

`csi2.lanes`
:   The number of the CSI-2 data lanes (input)

`binning_horizontal`
:   Horizontal binning factor (input)

`binning_vertical`
:   Vertical binning factor (input)

`scale_m`
:   Downscaling factor, M component, [16, max] (input)

`scale_n`
:   Downscaling factor, N component, typically 16 (input)

`bits_per_pixel`
:   Bits per pixel on the output data bus (input)

`op_bits_per_lane`
:   Number of bits per OP lane (input)

`flags`
:   CCS\_PLL\_FLAG\_\* (input)

`link_freq`
:   Chosen link frequency (input)

`ext_clk_freq_hz`
:   External clock frequency, i.e. the sensor’s input clock
    (input)

`vt_fr`
:   Video timing front-end configuration (output)

`vt_bk`
:   Video timing back-end configuration (output)

`op_fr`
:   Operational timing front-end configuration (output)

`op_bk`
:   Operational timing back-end configuration (output)

`pixel_rate_csi`
:   Pixel rate on the output data bus (output)

`pixel_rate_pixel_array`
:   Nominal pixel rate in the sensor’s pixel array
    (output)

**Description**

All information required to calculate CCS PLL configuration.

struct ccs\_pll\_branch\_limits\_fr
:   CCS PLL front-end limits

**Definition**:

```
struct ccs_pll_branch_limits_fr {
    u16 min_pre_pll_clk_div;
    u16 max_pre_pll_clk_div;
    u32 min_pll_ip_clk_freq_hz;
    u32 max_pll_ip_clk_freq_hz;
    u16 min_pll_multiplier;
    u16 max_pll_multiplier;
    u32 min_pll_op_clk_freq_hz;
    u32 max_pll_op_clk_freq_hz;
};
```

**Members**

`min_pre_pll_clk_div`
:   Minimum pre-PLL clock divider

`max_pre_pll_clk_div`
:   Maximum pre-PLL clock divider

`min_pll_ip_clk_freq_hz`
:   Minimum PLL input clock frequency

`max_pll_ip_clk_freq_hz`
:   Maximum PLL input clock frequency

`min_pll_multiplier`
:   Minimum PLL multiplier

`max_pll_multiplier`
:   Maximum PLL multiplier

`min_pll_op_clk_freq_hz`
:   Minimum PLL output clock frequency

`max_pll_op_clk_freq_hz`
:   Maximum PLL output clock frequency

struct ccs\_pll\_branch\_limits\_bk
:   CCS PLL back-end limits

**Definition**:

```
struct ccs_pll_branch_limits_bk {
    u16 min_sys_clk_div;
    u16 max_sys_clk_div;
    u32 min_sys_clk_freq_hz;
    u32 max_sys_clk_freq_hz;
    u16 min_pix_clk_div;
    u16 max_pix_clk_div;
    u32 min_pix_clk_freq_hz;
    u32 max_pix_clk_freq_hz;
};
```

**Members**

`min_sys_clk_div`
:   Minimum system clock divider

`max_sys_clk_div`
:   Maximum system clock divider

`min_sys_clk_freq_hz`
:   Minimum system clock frequency

`max_sys_clk_freq_hz`
:   Maximum system clock frequency

`min_pix_clk_div`
:   Minimum pixel clock divider

`max_pix_clk_div`
:   Maximum pixel clock divider

`min_pix_clk_freq_hz`
:   Minimum pixel clock frequency

`max_pix_clk_freq_hz`
:   Maximum pixel clock frequency

struct ccs\_pll\_limits
:   CCS PLL limits

**Definition**:

```
struct ccs_pll_limits {
    u32 min_ext_clk_freq_hz;
    u32 max_ext_clk_freq_hz;
    struct ccs_pll_branch_limits_fr vt_fr;
    struct ccs_pll_branch_limits_bk vt_bk;
    struct ccs_pll_branch_limits_fr op_fr;
    struct ccs_pll_branch_limits_bk op_bk;
    u32 min_line_length_pck_bin;
    u32 min_line_length_pck;
};
```

**Members**

`min_ext_clk_freq_hz`
:   Minimum external clock frequency

`max_ext_clk_freq_hz`
:   Maximum external clock frequency

`vt_fr`
:   Video timing front-end limits

`vt_bk`
:   Video timing back-end limits

`op_fr`
:   Operational timing front-end limits

`op_bk`
:   Operational timing back-end limits

`min_line_length_pck_bin`
:   Minimum line length in pixels, with binning

`min_line_length_pck`
:   Minimum line length in pixels without binning

int ccs\_pll\_calculate(struct [device](../../../infrastructure.html#c.device "device") \*dev, const struct [ccs\_pll\_limits](#c.ccs_pll_limits "ccs_pll_limits") \*limits, struct [ccs\_pll](#c.ccs_pll "ccs_pll") \*pll)
:   Calculate CCS PLL configuration based on input parameters

**Parameters**

`struct device *dev`
:   Device pointer, used for printing messages

`const struct ccs_pll_limits *limits`
:   Limits specific to the sensor

`struct ccs_pll *pll`
:   Given PLL configuration

**Description**

Calculate the CCS PLL configuration based on the limits as well as given
device specific, system specific or user configured input data.

**Copyright** © 2020 Intel Corporation
