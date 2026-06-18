# Xilinx SD-FEC Driver

> 출처(원문): https://docs.kernel.org/misc-devices/xilinx_sdfec.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Xilinx SD-FEC Driver

## Overview

This driver supports SD-FEC Integrated Block for Zynq Ultrascale+™ RFSoCs.

For a full description of SD-FEC core features, see the [SD-FEC Product Guide (PG256)](https://www.xilinx.com/cgi-bin/docs/ipdoc?c=sd_fec;v=latest;d=pg256-sdfec-integrated-block.pdf)

This driver supports the following features:

> * Retrieval of the Integrated Block configuration and status information
> * Configuration of LDPC codes
> * Configuration of Turbo decoding
> * Monitoring errors

Missing features, known issues, and limitations of the SD-FEC driver are as
follows:

> * Only allows a single open file handler to any instance of the driver at any time
> * Reset of the SD-FEC Integrated Block is not controlled by this driver
> * Does not support shared LDPC code table wraparound

The device tree entry is described in:
[linux-xlnx/Documentation/devicetree/bindings/misc/xlnx,sd-fec.yaml](https://github.com/Xilinx/linux-xlnx/blob/master/Documentation/devicetree/bindings/misc/xlnx%2Csd-fec.yaml)

### Modes of Operation

The driver works with the SD-FEC core in two modes of operation:

> * Run-time configuration
> * Programmable Logic (PL) initialization

#### Run-time Configuration

For Run-time configuration the role of driver is to allow the software application to do the following:

> * Load the configuration parameters for either Turbo decode or LDPC encode or decode
> * Activate the SD-FEC core
> * Monitor the SD-FEC core for errors
> * Retrieve the status and configuration of the SD-FEC core

#### Programmable Logic (PL) Initialization

For PL initialization, supporting logic loads configuration parameters for either
the Turbo decode or LDPC encode or decode. The role of the driver is to allow
the software application to do the following:

> * Activate the SD-FEC core
> * Monitor the SD-FEC core for errors
> * Retrieve the status and configuration of the SD-FEC core

## Driver Structure

The driver provides a platform device where the `probe` and `remove`
operations are provided.

> * probe: Updates configuration register with device-tree entries plus determines the current activate state of the core, for example, is the core bypassed or has the core been started.

The driver defines the following driver file operations to provide user
application interfaces:

> * open: Implements restriction that only a single file descriptor can be open per SD-FEC instance at any time
> * release: Allows another file descriptor to be open, that is after current file descriptor is closed
> * poll: Provides a method to monitor for SD-FEC Error events
> * unlocked\_ioctl: Provides the following ioctl commands that allows the application configure the SD-FEC core:
>
>   > + [`XSDFEC_START_DEV`](#c.XSDFEC_START_DEV "XSDFEC_START_DEV")
>   > + [`XSDFEC_STOP_DEV`](#c.XSDFEC_STOP_DEV "XSDFEC_STOP_DEV")
>   > + [`XSDFEC_GET_STATUS`](#c.XSDFEC_GET_STATUS "XSDFEC_GET_STATUS")
>   > + [`XSDFEC_SET_IRQ`](#c.XSDFEC_SET_IRQ "XSDFEC_SET_IRQ")
>   > + [`XSDFEC_SET_TURBO`](#c.XSDFEC_SET_TURBO "XSDFEC_SET_TURBO")
>   > + [`XSDFEC_ADD_LDPC_CODE_PARAMS`](#c.XSDFEC_ADD_LDPC_CODE_PARAMS "XSDFEC_ADD_LDPC_CODE_PARAMS")
>   > + [`XSDFEC_GET_CONFIG`](#c.XSDFEC_GET_CONFIG "XSDFEC_GET_CONFIG")
>   > + [`XSDFEC_SET_ORDER`](#c.XSDFEC_SET_ORDER "XSDFEC_SET_ORDER")
>   > + [`XSDFEC_SET_BYPASS`](#c.XSDFEC_SET_BYPASS "XSDFEC_SET_BYPASS")
>   > + [`XSDFEC_IS_ACTIVE`](#c.XSDFEC_IS_ACTIVE "XSDFEC_IS_ACTIVE")
>   > + [`XSDFEC_CLEAR_STATS`](#c.XSDFEC_CLEAR_STATS "XSDFEC_CLEAR_STATS")
>   > + [`XSDFEC_SET_DEFAULT_CONFIG`](#c.XSDFEC_SET_DEFAULT_CONFIG "XSDFEC_SET_DEFAULT_CONFIG")

## Driver Usage

### Overview

After opening the driver, the user should find out what operations need to be
performed to configure and activate the SD-FEC core and determine the
configuration of the driver.
The following outlines the flow the user should perform:

> * Determine Configuration
> * Set the order, if not already configured as desired
> * Set Turbo decode, LPDC encode or decode parameters, depending on how the
>   SD-FEC core is configured plus if the SD-FEC has not been configured for PL
>   initialization
> * Enable interrupts, if not already enabled
> * Bypass the SD-FEC core, if required
> * Start the SD-FEC core if not already started
> * Get the SD-FEC core status
> * Monitor for interrupts
> * Stop the SD-FEC core

Note: When monitoring for interrupts if a critical error is detected where a reset is required, the driver will be required to load the default configuration.

### Determine Configuration

Determine the configuration of the SD-FEC core by using the ioctl
[`XSDFEC_GET_CONFIG`](#c.XSDFEC_GET_CONFIG "XSDFEC_GET_CONFIG").

### Set the Order

Setting the order determines how the order of Blocks can change from input to output.

Setting the order is done by using the ioctl [`XSDFEC_SET_ORDER`](#c.XSDFEC_SET_ORDER "XSDFEC_SET_ORDER")

Setting the order can only be done if the following restrictions are met:

> * The `state` member of struct [`xsdfec_status`](#c.xsdfec_status "xsdfec_status") filled by the ioctl [`XSDFEC_GET_STATUS`](#c.XSDFEC_GET_STATUS "XSDFEC_GET_STATUS") indicates the SD-FEC core has not STARTED

### Add LDPC Codes

The following steps indicate how to add LDPC codes to the SD-FEC core:

> * Use the auto-generated parameters to fill the [`struct xsdfec_ldpc_params`](#c.xsdfec_ldpc_params "xsdfec_ldpc_params") for the desired LDPC code.
> * Set the SC, QA, and LA table offsets for the LPDC parameters and the parameters in the structure [`struct xsdfec_ldpc_params`](#c.xsdfec_ldpc_params "xsdfec_ldpc_params")
> * Set the desired Code Id value in the structure [`struct xsdfec_ldpc_params`](#c.xsdfec_ldpc_params "xsdfec_ldpc_params")
> * Add the LPDC Code Parameters using the ioctl [`XSDFEC_ADD_LDPC_CODE_PARAMS`](#c.XSDFEC_ADD_LDPC_CODE_PARAMS "XSDFEC_ADD_LDPC_CODE_PARAMS")
> * For the applied LPDC Code Parameter use the function `xsdfec_calculate_shared_ldpc_table_entry_size()` to calculate the size of shared LPDC code tables. This allows the user to determine the shared table usage so when selecting the table offsets for the next LDPC code parameters unused table areas can be selected.
> * Repeat for each LDPC code parameter.

Adding LDPC codes can only be done if the following restrictions are met:

> * The `code` member of [`struct xsdfec_config`](#c.xsdfec_config "xsdfec_config") filled by the ioctl [`XSDFEC_GET_CONFIG`](#c.XSDFEC_GET_CONFIG "XSDFEC_GET_CONFIG") indicates the SD-FEC core is configured as LDPC
> * The `code_wr_protect` of [`struct xsdfec_config`](#c.xsdfec_config "xsdfec_config") filled by the ioctl [`XSDFEC_GET_CONFIG`](#c.XSDFEC_GET_CONFIG "XSDFEC_GET_CONFIG") indicates that write protection is not enabled
> * The `state` member of struct [`xsdfec_status`](#c.xsdfec_status "xsdfec_status") filled by the ioctl [`XSDFEC_GET_STATUS`](#c.XSDFEC_GET_STATUS "XSDFEC_GET_STATUS") indicates the SD-FEC core has not started

### Set Turbo Decode

Configuring the Turbo decode parameters is done by using the ioctl [`XSDFEC_SET_TURBO`](#c.XSDFEC_SET_TURBO "XSDFEC_SET_TURBO") using auto-generated parameters to fill the [`struct xsdfec_turbo`](#c.xsdfec_turbo "xsdfec_turbo") for the desired Turbo code.

Adding Turbo decode can only be done if the following restrictions are met:

> * The `code` member of [`struct xsdfec_config`](#c.xsdfec_config "xsdfec_config") filled by the ioctl [`XSDFEC_GET_CONFIG`](#c.XSDFEC_GET_CONFIG "XSDFEC_GET_CONFIG") indicates the SD-FEC core is configured as TURBO
> * The `state` member of struct [`xsdfec_status`](#c.xsdfec_status "xsdfec_status") filled by the ioctl [`XSDFEC_GET_STATUS`](#c.XSDFEC_GET_STATUS "XSDFEC_GET_STATUS") indicates the SD-FEC core has not STARTED

### Enable Interrupts

Enabling or disabling interrupts is done by using the ioctl [`XSDFEC_SET_IRQ`](#c.XSDFEC_SET_IRQ "XSDFEC_SET_IRQ"). The members of the parameter passed, [`struct xsdfec_irq`](#c.xsdfec_irq "xsdfec_irq"), to the ioctl are used to set and clear different categories of interrupts. The category of interrupt is controlled as following:

> * `enable_isr` controls the `tlast` interrupts
> * `enable_ecc_isr` controls the ECC interrupts

If the `code` member of [`struct xsdfec_config`](#c.xsdfec_config "xsdfec_config") filled by the ioctl [`XSDFEC_GET_CONFIG`](#c.XSDFEC_GET_CONFIG "XSDFEC_GET_CONFIG") indicates the SD-FEC core is configured as TURBO then the enabling ECC errors is not required.

### Bypass the SD-FEC

Bypassing the SD-FEC is done by using the ioctl [`XSDFEC_SET_BYPASS`](#c.XSDFEC_SET_BYPASS "XSDFEC_SET_BYPASS")

Bypassing the SD-FEC can only be done if the following restrictions are met:

> * The `state` member of [`struct xsdfec_status`](#c.xsdfec_status "xsdfec_status") filled by the ioctl [`XSDFEC_GET_STATUS`](#c.XSDFEC_GET_STATUS "XSDFEC_GET_STATUS") indicates the SD-FEC core has not STARTED

### Start the SD-FEC core

Start the SD-FEC core by using the ioctl [`XSDFEC_START_DEV`](#c.XSDFEC_START_DEV "XSDFEC_START_DEV")

### Get SD-FEC Status

Get the SD-FEC status of the device by using the ioctl [`XSDFEC_GET_STATUS`](#c.XSDFEC_GET_STATUS "XSDFEC_GET_STATUS"), which will fill the [`struct xsdfec_status`](#c.xsdfec_status "xsdfec_status")

### Monitor for Interrupts

> * Use the poll system call to monitor for an interrupt. The poll system call waits for an interrupt to wake it up or times out if no interrupt occurs.
> * On return Poll `revents` will indicate whether stats and/or state have been updated
>   :   + `POLLPRI` indicates a critical error and the user should use [`XSDFEC_GET_STATUS`](#c.XSDFEC_GET_STATUS "XSDFEC_GET_STATUS") and [`XSDFEC_GET_STATS`](#c.XSDFEC_GET_STATS "XSDFEC_GET_STATS") to confirm
>       + `POLLRDNORM` indicates a non-critical error has occurred and the user should use [`XSDFEC_GET_STATS`](#c.XSDFEC_GET_STATS "XSDFEC_GET_STATS") to confirm
> * Get stats by using the ioctl [`XSDFEC_GET_STATS`](#c.XSDFEC_GET_STATS "XSDFEC_GET_STATS")
>   :   + For critical error the `isr_err_count` or `uecc_count` member of [`struct xsdfec_stats`](#c.xsdfec_stats "xsdfec_stats") is non-zero
>       + For non-critical errors the `cecc_count` member of [`struct xsdfec_stats`](#c.xsdfec_stats "xsdfec_stats") is non-zero
> * Get state by using the ioctl [`XSDFEC_GET_STATUS`](#c.XSDFEC_GET_STATUS "XSDFEC_GET_STATUS")
>   :   + For a critical error the `state` of [`xsdfec_status`](#c.xsdfec_status "xsdfec_status") will indicate a Reset Is Required
> * Clear stats by using the ioctl [`XSDFEC_CLEAR_STATS`](#c.XSDFEC_CLEAR_STATS "XSDFEC_CLEAR_STATS")

If a critical error is detected where a reset is required. The application is required to call the ioctl [`XSDFEC_SET_DEFAULT_CONFIG`](#c.XSDFEC_SET_DEFAULT_CONFIG "XSDFEC_SET_DEFAULT_CONFIG"), after the reset and it is not required to call the ioctl [`XSDFEC_STOP_DEV`](#c.XSDFEC_STOP_DEV "XSDFEC_STOP_DEV")

Note: Using poll system call prevents busy looping using [`XSDFEC_GET_STATS`](#c.XSDFEC_GET_STATS "XSDFEC_GET_STATS") and [`XSDFEC_GET_STATUS`](#c.XSDFEC_GET_STATUS "XSDFEC_GET_STATUS")

### Stop the SD-FEC Core

Stop the device by using the ioctl [`XSDFEC_STOP_DEV`](#c.XSDFEC_STOP_DEV "XSDFEC_STOP_DEV")

### Set the Default Configuration

Load default configuration by using the ioctl [`XSDFEC_SET_DEFAULT_CONFIG`](#c.XSDFEC_SET_DEFAULT_CONFIG "XSDFEC_SET_DEFAULT_CONFIG") to restore the driver.

### Limitations

Users should not duplicate SD-FEC device file handlers, for example fork() or `dup()` a process that has a created an SD-FEC file handler.

## Driver IOCTLs

XSDFEC\_START\_DEV

**Description**

ioctl to start SD-FEC core

This fails if the XSDFEC\_SET\_ORDER ioctl has not been previously called

XSDFEC\_STOP\_DEV

**Description**

ioctl to stop the SD-FEC core

XSDFEC\_GET\_STATUS

**Description**

ioctl that returns status of SD-FEC core

XSDFEC\_SET\_IRQ

**Parameters**

**struct** xsdfec\_irq \*
:   Pointer to the [`struct xsdfec_irq`](#c.xsdfec_irq "xsdfec_irq") that contains the interrupt settings
    for the SD-FEC core

**Description**

ioctl to enable or disable irq

XSDFEC\_SET\_TURBO

**Parameters**

**struct** xsdfec\_turbo \*
:   Pointer to the [`struct xsdfec_turbo`](#c.xsdfec_turbo "xsdfec_turbo") that contains the Turbo decode
    settings for the SD-FEC core

**Description**

ioctl that sets the SD-FEC Turbo parameter values

This can only be used when the driver is in the XSDFEC\_STOPPED state

XSDFEC\_ADD\_LDPC\_CODE\_PARAMS

**Parameters**

**struct** xsdfec\_ldpc\_params \*
:   Pointer to the [`struct xsdfec_ldpc_params`](#c.xsdfec_ldpc_params "xsdfec_ldpc_params") that contains the LDPC code
    parameters to be added to the SD-FEC Block

**Description**
ioctl to add an LDPC code to the SD-FEC LDPC codes

This can only be used when:

* Driver is in the XSDFEC\_STOPPED state
* SD-FEC core is configured as LPDC
* SD-FEC Code Write Protection is disabled

XSDFEC\_GET\_CONFIG

**Parameters**

**struct** xsdfec\_config \*
:   Pointer to the [`struct xsdfec_config`](#c.xsdfec_config "xsdfec_config") that contains the current
    configuration settings of the SD-FEC Block

**Description**

ioctl that returns SD-FEC core configuration

XSDFEC\_SET\_ORDER

**Parameters**

**struct** unsigned long \*
:   Pointer to the unsigned long that contains a value from the
    **enum** xsdfec\_order

**Description**

ioctl that sets order, if order of blocks can change from input to output

This can only be used when the driver is in the XSDFEC\_STOPPED state

XSDFEC\_SET\_BYPASS

**Parameters**

**struct** bool \*
:   Pointer to bool that sets the bypass value, where false results in
    normal operation and false results in the SD-FEC performing the
    configured operations (same number of cycles) but output data matches
    the input data

**Description**

ioctl that sets bypass.

This can only be used when the driver is in the XSDFEC\_STOPPED state

XSDFEC\_IS\_ACTIVE

**Parameters**

**struct** bool \*
:   Pointer to bool that returns true if the SD-FEC is processing data

**Description**

ioctl that determines if SD-FEC is processing data

XSDFEC\_CLEAR\_STATS

**Description**

ioctl that clears error stats collected during interrupts

XSDFEC\_GET\_STATS

**Parameters**

**struct** xsdfec\_stats \*
:   Pointer to the [`struct xsdfec_stats`](#c.xsdfec_stats "xsdfec_stats") that will contain the updated stats
    values

**Description**

ioctl that returns SD-FEC core stats

This can only be used when the driver is in the XSDFEC\_STOPPED state

XSDFEC\_SET\_DEFAULT\_CONFIG

**Description**

ioctl that returns SD-FEC core to default config, use after a reset

This can only be used when the driver is in the XSDFEC\_STOPPED state

## Driver Type Definitions

enum xsdfec\_code
:   Code Type.

**Constants**

`XSDFEC_TURBO_CODE`
:   Driver is configured for Turbo mode.

`XSDFEC_LDPC_CODE`
:   Driver is configured for LDPC mode.

**Description**

This `enum is` used to indicate the mode of the driver. The mode is determined
by checking which codes are set in the driver. Note that the mode cannot be
changed by the driver.

enum xsdfec\_order
:   Order

**Constants**

`XSDFEC_MAINTAIN_ORDER`
:   Maintain order execution of blocks.

`XSDFEC_OUT_OF_ORDER`
:   Out-of-order execution of blocks.

**Description**

This `enum is` used to indicate whether the order of blocks can change from
input to output.

enum xsdfec\_turbo\_alg
:   Turbo Algorithm Type.

**Constants**

`XSDFEC_MAX_SCALE`
:   Max Log-Map algorithm with extrinsic scaling. When
    scaling is set to this is equivalent to the Max Log-Map
    algorithm.

`XSDFEC_MAX_STAR`
:   Log-Map algorithm.

`XSDFEC_TURBO_ALG_MAX`
:   Used to indicate out of bound Turbo algorithms.

**Description**

This `enum specifies` which Turbo Decode algorithm is in use.

enum xsdfec\_state
:   State.

**Constants**

`XSDFEC_INIT`
:   Driver is initialized.

`XSDFEC_STARTED`
:   Driver is started.

`XSDFEC_STOPPED`
:   Driver is stopped.

`XSDFEC_NEEDS_RESET`
:   Driver needs to be reset.

`XSDFEC_PL_RECONFIGURE`
:   Programmable Logic needs to be recofigured.

**Description**

This `enum is` used to indicate the state of the driver.

enum xsdfec\_axis\_width
:   AXIS\_WIDTH.DIN Setting for 128-bit width.

**Constants**

`XSDFEC_1x128b`
:   DIN data input stream consists of a 128-bit lane

`XSDFEC_2x128b`
:   DIN data input stream consists of two 128-bit lanes

`XSDFEC_4x128b`
:   DIN data input stream consists of four 128-bit lanes

**Description**

This `enum is` used to indicate the AXIS\_WIDTH.DIN setting for 128-bit width.
The number of lanes of the DIN data input stream depends upon the
AXIS\_WIDTH.DIN parameter.

enum xsdfec\_axis\_word\_include
:   Words Configuration.

**Constants**

`XSDFEC_FIXED_VALUE`
:   Fixed, the DIN\_WORDS AXI4-Stream interface is removed
    from the IP instance and is driven with the specified
    number of words.

`XSDFEC_IN_BLOCK`
:   In Block, configures the IP instance to expect a single
    DIN\_WORDS value per input code block. The DIN\_WORDS
    interface is present.

`XSDFEC_PER_AXI_TRANSACTION`
:   Per Transaction, configures the IP instance to
    expect one DIN\_WORDS value per input transaction on the DIN interface. The
    DIN\_WORDS interface is present.

`XSDFEC_AXIS_WORDS_INCLUDE_MAX`
:   Used to indicate out of bound Words
    Configurations.

**Description**

This `enum is` used to specify the DIN\_WORDS configuration.

struct xsdfec\_turbo
:   User data for Turbo codes.

**Definition**:

```
struct xsdfec_turbo {
    __u32 alg;
    __u8 scale;
};
```

**Members**

`alg`
:   Specifies which Turbo decode algorithm to use

`scale`
:   Specifies the extrinsic scaling to apply when the Max Scale algorithm
    has been selected

**Description**

Turbo code structure to communicate parameters to XSDFEC driver.

struct xsdfec\_ldpc\_params
:   User data for LDPC codes.

**Definition**:

```
struct xsdfec_ldpc_params {
    __u32 n;
    __u32 k;
    __u32 psize;
    __u32 nlayers;
    __u32 nqc;
    __u32 nmqc;
    __u32 nm;
    __u32 norm_type;
    __u32 no_packing;
    __u32 special_qc;
    __u32 no_final_parity;
    __u32 max_schedule;
    __u32 sc_off;
    __u32 la_off;
    __u32 qc_off;
    __u32 *sc_table;
    __u32 *la_table;
    __u32 *qc_table;
    __u16 code_id;
};
```

**Members**

`n`
:   Number of code word bits

`k`
:   Number of information bits

`psize`
:   Size of sub-matrix

`nlayers`
:   Number of layers in code

`nqc`
:   Quasi Cyclic Number

`nmqc`
:   Number of M-sized QC operations in parity check matrix

`nm`
:   Number of M-size vectors in N

`norm_type`
:   Normalization required or not

`no_packing`
:   Determines if multiple QC ops should be performed

`special_qc`
:   Sub-Matrix property for Circulant weight > 0

`no_final_parity`
:   Decide if final parity check needs to be performed

`max_schedule`
:   Experimental code word scheduling limit

`sc_off`
:   SC offset

`la_off`
:   LA offset

`qc_off`
:   QC offset

`sc_table`
:   Pointer to SC Table which must be page aligned

`la_table`
:   Pointer to LA Table which must be page aligned

`qc_table`
:   Pointer to QC Table which must be page aligned

`code_id`
:   LDPC Code

**Description**

This structure describes the LDPC code that is passed to the driver by the
application.

struct xsdfec\_status
:   Status of SD-FEC core.

**Definition**:

```
struct xsdfec_status {
    __u32 state;
    __s8 activity;
};
```

**Members**

`state`
:   State of the SD-FEC core

`activity`
:   Describes if the SD-FEC instance is Active

struct xsdfec\_irq
:   Enabling or Disabling Interrupts.

**Definition**:

```
struct xsdfec_irq {
    __s8 enable_isr;
    __s8 enable_ecc_isr;
};
```

**Members**

`enable_isr`
:   If true enables the ISR

`enable_ecc_isr`
:   If true enables the ECC ISR

struct xsdfec\_config
:   Configuration of SD-FEC core.

**Definition**:

```
struct xsdfec_config {
    __u32 code;
    __u32 order;
    __u32 din_width;
    __u32 din_word_include;
    __u32 dout_width;
    __u32 dout_word_include;
    struct xsdfec_irq irq;
    __s8 bypass;
    __s8 code_wr_protect;
};
```

**Members**

`code`
:   The codes being used by the SD-FEC instance

`order`
:   Order of Operation

`din_width`
:   Width of the DIN AXI4-Stream

`din_word_include`
:   How DIN\_WORDS are inputted

`dout_width`
:   Width of the DOUT AXI4-Stream

`dout_word_include`
:   HOW DOUT\_WORDS are outputted

`irq`
:   Enabling or disabling interrupts

`bypass`
:   Is the core being bypassed

`code_wr_protect`
:   Is write protection of LDPC codes enabled

struct xsdfec\_stats
:   Stats retrived by ioctl XSDFEC\_GET\_STATS. Used to buffer atomic\_t variables from `struct xsdfec_dev`. Counts are accumulated until the user clears them.

**Definition**:

```
struct xsdfec_stats {
    __u32 isr_err_count;
    __u32 cecc_count;
    __u32 uecc_count;
};
```

**Members**

`isr_err_count`
:   Count of ISR errors

`cecc_count`
:   Count of Correctable ECC errors (SBE)

`uecc_count`
:   Count of Uncorrectable ECC errors (MBE)

struct xsdfec\_ldpc\_param\_table\_sizes
:   Used to store sizes of SD-FEC table entries for an individual LPDC code parameter.

**Definition**:

```
struct xsdfec_ldpc_param_table_sizes {
    __u32 sc_size;
    __u32 la_size;
    __u32 qc_size;
};
```

**Members**

`sc_size`
:   Size of SC table used

`la_size`
:   Size of LA table used

`qc_size`
:   Size of QC table used
