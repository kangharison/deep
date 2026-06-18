# DCN Blocks

> 출처(원문): https://docs.kernel.org/gpu/amdgpu/display/dcn-blocks.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# DCN Blocks

In this section, you will find some extra details about some of the DCN blocks
and the code documentation when it is automatically generated.

## DCHUBBUB

There is only one common DCHUBBUB. It contains the common request and return
blocks for the Data Fabric Interface that are not clock/power gated.

## HUBP

Display Controller Hub (DCHUB) is the gateway between the Scalable Data Port
(SDP) and DCN. This component has multiple features, such as memory
arbitration, rotation, and cursor manipulation.

There is one HUBP allocated per pipe, which fetches data and converts
different pixel formats (i.e. ARGB8888, NV12, etc) into linear, interleaved
and fixed-depth streams of pixel data.

## DPP

The DPP (Display Pipe and Plane) block is the unified display data
processing engine in DCN for processing graphic or video data on per DPP
rectangle base. This rectangle can be a part of SLS (Single Large Surface),
or a layer to be blended with other DPP, or a rectangle associated with a
display tile.

It provides various functions including:
- graphic color keyer
- graphic cursor compositing
- graphic or video image source to destination scaling
- image sharping
- video format conversion from 4:2:0 or 4:2:2 to 4:4:4
- Color Space Conversion
- Host LUT gamma adjustment
- Color Gamut Remap
- brightness and contrast adjustment.

DPP pipe consists of Converter and Cursor (CNVC), Scaler (DSCL), Color
Management (CM), Output Buffer (OBUF) and Digital Bypass (DPB) module
connected in a video/graphics pipeline.

struct cnv\_alpha\_2bit\_lut
:   Set the 8bit alpha values based on the 2 bit alpha

**Definition**:

```
struct cnv_alpha_2bit_lut {
    int lut0;
    int lut1;
    int lut2;
    int lut3;
};
```

**Members**

`lut0`
:   ALPHA\_2BIT\_LUT. ALPHA\_2BIT\_LUT0. Default: 0b00000000

`lut1`
:   ALPHA\_2BIT\_LUT. ALPHA\_2BIT\_LUT1. Default: 0b01010101

`lut2`
:   ALPHA\_2BIT\_LUT. ALPHA\_2BIT\_LUT2. Default: 0b10101010

`lut3`
:   ALPHA\_2BIT\_LUT. ALPHA\_2BIT\_LUT3. Default: 0b11111111

## MPC

Multiple Pipe/Plane Combiner (MPC) is a component in the hardware pipeline
that performs blending of multiple planes, using global and per-pixel alpha.
It also performs post-blending color correction operations according to the
hardware capabilities, such as color transformation matrix and gamma 1D and
3D LUT.

MPC receives output from all DPP pipes and combines them to multiple outputs
supporting “M MPC inputs -> N MPC outputs” flexible composition
architecture. It features:

* Programmable blending structure to allow software controlled blending and
  cascading;
* Programmable window location of each DPP in active region of display;
* Combining multiple DPP pipes in one active region when a single DPP pipe
  cannot process very large surface;
* Combining multiple DPP from different SLS with blending;
* Stereo formats from single DPP in top-bottom or side-by-side modes;
* Stereo formats from 2 DPPs;
* Alpha blending of multiple layers from different DPP pipes;
* Programmable background color;

struct mpcc
:   MPCC connection and blending configuration for a single MPCC instance.

**Definition**:

```
struct mpcc {
    int mpcc_id;
    int dpp_id;
    struct mpcc *mpcc_bot;
    struct mpcc_blnd_cfg blnd_cfg;
    struct mpcc_sm_cfg sm_cfg;
    bool shared_bottom;
};
```

**Members**

`mpcc_id`
:   MPCC physical instance.

`dpp_id`
:   DPP input to this MPCC

`mpcc_bot`
:   Pointer to bottom layer MPCC. NULL when not connected.

`blnd_cfg`
:   The blending configuration for this MPCC.

`sm_cfg`
:   stereo mix setting for this MPCC

`shared_bottom`
:   If MPCC output to both OPP and DWB endpoints, true. Otherwise, false.

**Description**

This `struct is` used as a node in an MPC tree.

struct mpc\_tree
:   MPC tree represents all MPCC connections for a pipe.

**Definition**:

```
struct mpc_tree {
    int opp_id;
    struct mpcc *opp_list;
};
```

**Members**

`opp_id`
:   The OPP instance that owns this MPC tree.

`opp_list`
:   the top MPCC layer of the MPC tree that outputs to OPP endpoint

struct mpc\_funcs
:   funcs

**Definition**:

```
struct mpc_funcs {
    void (*read_mpcc_state)( struct mpc *mpc, int mpcc_inst, struct mpcc_state *s);
    void (*mpc_read_reg_state)( struct mpc *mpc, int mpcc_inst, struct dcn_mpc_reg_state *mpc_reg_state);
    struct mpcc* (*insert_plane)( struct mpc *mpc, struct mpc_tree *tree, struct mpcc_blnd_cfg *blnd_cfg, struct mpcc_sm_cfg *sm_cfg, struct mpcc *insert_above_mpcc, int dpp_id, int mpcc_id);
    void (*remove_mpcc)( struct mpc *mpc, struct mpc_tree *tree, struct mpcc *mpcc);
    void (*mpc_init)(struct mpc *mpc);
    void (*mpc_init_single_inst)( struct mpc *mpc, unsigned int mpcc_id);
    void (*update_blending)( struct mpc *mpc, struct mpcc_blnd_cfg *blnd_cfg, int mpcc_id);
    void (*cursor_lock)( struct mpc *mpc, int opp_id, bool lock);
    struct mpcc* (*insert_plane_to_secondary)( struct mpc *mpc, struct mpc_tree *tree, struct mpcc_blnd_cfg *blnd_cfg, struct mpcc_sm_cfg *sm_cfg, struct mpcc *insert_above_mpcc, int dpp_id, int mpcc_id);
    void (*remove_mpcc_from_secondary)( struct mpc *mpc, struct mpc_tree *tree, struct mpcc *mpcc);
    struct mpcc* (*get_mpcc_for_dpp_from_secondary)( struct mpc_tree *tree, int dpp_id);
    struct mpcc* (*get_mpcc_for_dpp)( struct mpc_tree *tree, int dpp_id);
    void (*wait_for_idle)(struct mpc *mpc, int id);
    void (*assert_mpcc_idle_before_connect)(struct mpc *mpc, int mpcc_id);
    void (*init_mpcc_list_from_hw)( struct mpc *mpc, struct mpc_tree *tree);
    void (*set_denorm)(struct mpc *mpc, int opp_id, enum dc_color_depth output_depth);
    void (*set_denorm_clamp)( struct mpc *mpc, int opp_id, struct mpc_denorm_clamp denorm_clamp);
    void (*set_output_csc)(struct mpc *mpc, int opp_id, const uint16_t *regval, enum mpc_output_csc_mode ocsc_mode);
    void (*set_ocsc_default)(struct mpc *mpc, int opp_id, enum dc_color_space color_space, enum mpc_output_csc_mode ocsc_mode);
    void (*set_output_gamma)( struct mpc *mpc, int mpcc_id, const struct pwl_params *params);
    void (*power_on_mpc_mem_pwr)( struct mpc *mpc, int mpcc_id, bool power_on);
    void (*set_dwb_mux)( struct mpc *mpc, int dwb_id, int mpcc_id);
    void (*disable_dwb_mux)( struct mpc *mpc, int dwb_id);
    bool (*is_dwb_idle)( struct mpc *mpc, int dwb_id);
    void (*set_out_rate_control)( struct mpc *mpc, int opp_id, bool enable, bool rate_2x_mode, struct mpc_dwb_flow_control *flow_control);
    void (*set_gamut_remap)( struct mpc *mpc, int mpcc_id, const struct mpc_grph_gamut_adjustment *adjust);
    bool (*program_1dlut)( struct mpc *mpc, const struct pwl_params *params, uint32_t rmu_idx);
    bool (*program_shaper)( struct mpc *mpc, const struct pwl_params *params, uint32_t rmu_idx);
    uint32_t (*acquire_rmu)(struct mpc *mpc, int mpcc_id, int rmu_idx);
    bool (*program_3dlut)( struct mpc *mpc, const struct tetrahedral_params *params, int rmu_idx);
    int (*release_rmu)(struct mpc *mpc, int mpcc_id);
    unsigned int (*get_mpc_out_mux)( struct mpc *mpc, int opp_id);
    void (*set_bg_color)(struct mpc *mpc, struct tg_color *bg_color, int mpcc_id);
    void (*set_mpc_mem_lp_mode)(struct mpc *mpc);
    void (*set_movable_cm_location)(struct mpc *mpc, enum mpcc_movable_cm_location location, int mpcc_id);
    void (*update_3dlut_fast_load_select)(struct mpc *mpc, int mpcc_id, int hubp_idx);
    void (*get_3dlut_fast_load_status)(struct mpc *mpc, int mpcc_id, uint32_t *done, uint32_t *soft_underflow, uint32_t *hard_underflow);
    void (*populate_lut)(struct mpc *mpc, const enum MCM_LUT_ID id, const union mcm_lut_params *params, const bool lut_bank_a, const int mpcc_id);
    void (*program_lut_read_write_control)(struct mpc *mpc, const enum MCM_LUT_ID id, const bool lut_bank_a, const unsigned int bit_depth, const int mpcc_id);
    void (*program_lut_mode)(struct mpc *mpc, const enum MCM_LUT_ID id, const bool enable, const bool lut_bank_a, const enum dc_cm_lut_size size, const int mpcc_id);
    void (*get_lut_mode)(struct mpc *mpc, const enum MCM_LUT_ID id, const int mpcc_id, bool *enable, bool *lut_bank_a);
    struct {
        void (*fl_3dlut_configure)(struct mpc *mpc, struct mpc_fl_3dlut_config *cfg, int mpcc_id);
        void (*enable_3dlut_fl)(struct mpc *mpc, bool enable, int mpcc_id);
        void (*update_3dlut_fast_load_select)(struct mpc *mpc, int mpcc_id, int hubp_idx);
        void (*program_lut_read_write_control)(struct mpc *mpc, const enum MCM_LUT_ID id, bool lut_bank_a, bool enabled, int mpcc_id);
        void (*program_lut_mode)(struct mpc *mpc, bool enable, bool lut_bank_a, int mpcc_id);
        void (*program_3dlut_size)(struct mpc *mpc, const enum dc_cm_lut_size size, int mpcc_id);
        void (*program_bias_scale)(struct mpc *mpc, uint16_t bias, uint16_t scale, int mpcc_id);
        void (*program_bit_depth)(struct mpc *mpc, uint16_t bit_depth, int mpcc_id);
        bool (*is_config_supported)(uint32_t width);
        void (*power_on_shaper_3dlut)(struct mpc *mpc, uint32_t mpcc_id, bool power_on);
        void (*populate_lut)(struct mpc *mpc, const union mcm_lut_params params, bool lut_bank_a, int mpcc_id);
    } rmcm;
};
```

**Members**

`read_mpcc_state`
:   Read register content from given MPCC physical instance.

    Parameters:

    * [in/out] mpc - MPC context
    * [in] mpcc\_instance - MPC context instance
    * [in] mpcc\_state - MPC context state

    Return:

    void

`mpc_read_reg_state`
:   Read MPC register state for debugging underflow purposes.

    Parameters:

    * [in] mpc - MPC context
    * [out] reg\_state - MPC register state structure

    Return:

    void

`insert_plane`
:   Insert DPP into MPC tree based on specified blending position.
    Only used for planes that are part of blending chain for OPP output

    Parameters:

    * [in/out] mpc - MPC context.
    * [in/out] tree - MPC tree structure that plane will be added to.
    * [in] blnd\_cfg - MPCC blending configuration for the new blending layer.
    * [in] sm\_cfg - MPCC stereo mix configuration for the new blending layer.
      :   stereo mix must disable for the very bottom layer of the tree config.
    * [in] insert\_above\_mpcc - Insert new plane above this MPCC.
      :   If NULL, insert as bottom plane.
    * [in] dpp\_id - DPP instance for the plane to be added.
    * [in] mpcc\_id - The MPCC physical instance to use for blending.

    Return:

    [`struct mpcc`](#c.mpcc "mpcc")\* - MPCC that was added.

`remove_mpcc`
:   Remove a specified MPCC from the MPC tree.

    Parameters:

    * [in/out] mpc - MPC context.
    * [in/out] tree - MPC tree structure that plane will be removed from.
    * [in/out] mpcc - MPCC to be removed from tree.

    Return:

    void

`mpc_init`
:   Reset the MPCC HW status by disconnecting all muxes.

    Parameters:

    * [in/out] mpc - MPC context.

    Return:

    void

`mpc_init_single_inst`
:   Initialize given MPCC physical instance.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] mpcc\_id - The MPCC physical instance to be initialized.

`update_blending`
:   Update the blending configuration for a specified MPCC.

    Parameters:

    * [in/out] mpc - MPC context.
    * [in] blnd\_cfg - MPCC blending configuration.
    * [in] mpcc\_id - The MPCC physical instance.

    Return:

    void

`cursor_lock`
:   Lock cursor updates for the specified OPP. OPP defines the set of
    MPCC that are locked together for cursor.

    Parameters:

    * [in] mpc - MPC context.
    * [in] opp\_id - The OPP to lock cursor updates on
    * [in] lock - lock/unlock the OPP

    Return:

    void

`insert_plane_to_secondary`
:   Add DPP into secondary MPC tree based on specified blending
    position. Only used for planes that are part of blending chain for
    DWB output

    Parameters:

    * [in/out] mpc - MPC context.
    * [in/out] tree - MPC tree structure that plane will be added to.
    * [in] blnd\_cfg - MPCC blending configuration for the new blending layer.
    * [in] sm\_cfg - MPCC stereo mix configuration for the new blending layer.
      :   stereo mix must disable for the very bottom layer of the tree config.
    * [in] insert\_above\_mpcc - Insert new plane above this MPCC. If
      :   NULL, insert as bottom plane.
    * [in] dpp\_id - DPP instance for the plane to be added.
    * [in] mpcc\_id - The MPCC physical instance to use for blending.

    Return:

    [`struct mpcc`](#c.mpcc "mpcc")\* - MPCC that was added.

`remove_mpcc_from_secondary`
:   Remove a specified DPP from the ‘secondary’ MPC tree.

    Parameters:

    * [in/out] mpc - MPC context.
    * [in/out] tree - MPC tree structure that plane will be removed from.
    * [in] mpcc - MPCC to be removed from tree.

    Return:

    void

`get_mpcc_for_dpp_from_secondary`
:   Find, if it exists, a MPCC from a given ‘secondary’ MPC tree that
    is associated with specified plane.

    Parameters:
    - [in/out] tree - MPC tree structure to search for plane.
    - [in] dpp\_id - DPP to be searched.

    Return:

    [`struct mpcc`](#c.mpcc "mpcc")\* - pointer to plane or NULL if no plane found.

`get_mpcc_for_dpp`
:   Find, if it exists, a MPCC from a given MPC tree that
    is associated with specified plane.

    Parameters:
    - [in/out] tree - MPC tree structure to search for plane.
    - [in] dpp\_id - DPP to be searched.

    Return:

    [`struct mpcc`](#c.mpcc "mpcc")\* - pointer to plane or NULL if no plane found.

`wait_for_idle`
:   Wait for a MPCC in MPC context to enter idle state.

    Parameters:
    - [in/out] mpc - MPC Context.
    - [in] id - MPCC to wait for idle state.

    Return:

    void

`assert_mpcc_idle_before_connect`
:   Assert if MPCC in MPC context is in idle state.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] id - MPCC to assert idle state.

    Return:

    void

`init_mpcc_list_from_hw`
:   Iterate through the MPCC array from a given MPC context `struct
    and` configure each MPCC according to its registers’ values.

    Parameters:
    - [in/out] mpc - MPC context to initialize MPCC array.
    - [in/out] tree - MPC tree structure containing MPCC contexts to initialize.

    Return:

    void

`set_denorm`
:   Set corresponding OPP DENORM\_CONTROL register value to specific denorm\_mode
    based on given color depth.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] opp\_id - Corresponding OPP to update register.
    - [in] output\_depth - Arbitrary color depth to set denorm\_mode.

    Return:

    void

`set_denorm_clamp`
:   Set denorm clamp values on corresponding OPP DENORM CONTROL register.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] opp\_id - Corresponding OPP to update register.
    - [in] denorm\_clamp - Arbitrary denorm clamp to be set.

    Return:

    void

`set_output_csc`
:   Set the Output Color Space Conversion matrix
    with given values and mode.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] opp\_id - Corresponding OPP to update register.
    - [in] regval - Values to set in CSC matrix.
    - [in] ocsc\_mode - Mode to set CSC.

    Return:

    void

`set_ocsc_default`
:   Set the Output Color Space Conversion matrix
    to default values according to color space.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] opp\_id - Corresponding OPP to update register.
    - [in] color\_space - OCSC color space.
    - [in] ocsc\_mode - Mode to set CSC.

    Return:

    void

`set_output_gamma`
:   Set Output Gamma with given curve parameters.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] mpcc\_id - Corresponding MPC to update registers.
    - [in] params - Parameters.

    Return:

    void

`power_on_mpc_mem_pwr`
:   Power on/off memory LUT for given MPCC.
    Powering on enables LUT to be updated.
    Powering off allows entering low power mode.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] mpcc\_id - MPCC to power on.
    - [in] power\_on

    Return:

    void

`set_dwb_mux`
:   Set corresponding Display Writeback mux
    MPC register field to given MPCC id.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] dwb\_id - DWB to be set.
    - [in] mpcc\_id - MPCC id to be stored in DWB mux register.

    Return:

    void

`disable_dwb_mux`
:   Reset corresponding Display Writeback mux
    MPC register field.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] dwb\_id - DWB to be set.

    Return:

    void

`is_dwb_idle`
:   Check DWB status on MPC\_DWB0\_MUX\_STATUS register field.
    Return if it is null.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] dwb\_id - DWB to be checked.

    Return:

    bool - wheter DWB is idle or not

`set_out_rate_control`
:   Set display output rate control.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] opp\_id - OPP to be set.
    - [in] enable
    - [in] rate\_2x\_mode
    - [in] flow\_control

    Return:

    void

`set_gamut_remap`
:   Set post-blending CTM for given MPCC.

    Parameters:
    - [in] mpc - MPC context.
    - [in] mpcc\_id - MPCC to set gamut map.
    - [in] adjust

    Return:

    void

`program_1dlut`
:   Set 1 dimensional Lookup Table.

    Parameters:
    - [in/out] mpc - MPC context
    - [in] params - curve parameters for the LUT configuration
    - [in] rmu\_idx

    bool - wheter LUT was set (set with given parameters) or not (params is NULL and LUT is disabled).

`program_shaper`
:   Set shaper.

    Parameters:
    - [in/out] mpc - MPC context
    - [in] params - curve parameters to be set
    - [in] rmu\_idx

    Return:

    bool - wheter shaper was set (set with given parameters) or not (params is NULL and LUT is disabled).

`acquire_rmu`
:   Set given MPCC to be multiplexed to given RMU unit.

    Parameters:
    - [in/out] mpc - MPC context
    - [in] mpcc\_id - MPCC
    - [in] rmu\_idx - Given RMU unit to set MPCC to be multiplexed to.

    Return:

    unit32\_t - rmu\_idx if operation was successful, -1 else.

`program_3dlut`
:   Set 3 dimensional Lookup Table.

    Parameters:
    - [in/out] mpc - MPC context
    - [in] params - tetrahedral parameters for the LUT configuration
    - [in] rmu\_idx

    bool - wheter LUT was set (set with given parameters) or not (params is NULL and LUT is disabled).

`release_rmu`
:   For a given MPCC, release the RMU unit it muliplexes to.

    Parameters:
    - [in/out] mpc - MPC context
    - [in] mpcc\_id - MPCC

    Return:

    int - a valid rmu\_idx representing released RMU unit or -1 if there was no RMU unit to release.

`get_mpc_out_mux`
:   Return MPC out mux.

    Parameters:
    - [in] mpc - MPC context.
    - [in] opp\_id - OPP

    Return:

    unsigned int - Out Mux

`set_bg_color`
:   Find corresponding bottommost MPCC and
    set its bg color.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] bg\_color - background color to be set.
    - [in] mpcc\_id

    Return:

    void

`set_mpc_mem_lp_mode`
:   Set mpc\_mem\_lp\_mode.

    Parameters:
    - [in/out] mpc - MPC context.

    Return:

    void

`set_movable_cm_location`
:   Set Movable CM Location.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] location
    - [in] mpcc\_id

    Return:

    void

`update_3dlut_fast_load_select`
:   Update 3D LUT fast load select.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] mpcc\_id
    - [in] hubp\_idx

    Return:

    void

`get_3dlut_fast_load_status`
:   Get 3D LUT fast load status and reference them with done, soft\_underflow and hard\_underflow pointers.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] mpcc\_id
    - [in/out] done
    - [in/out] soft\_underflow
    - [in/out] hard\_underflow

    Return:

    void

`populate_lut`
:   Populate LUT with given tetrahedral parameters.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] id
    - [in] params
    - [in] lut\_bank\_a
    - [in] mpcc\_id

    Return:

    void

`program_lut_read_write_control`
:   Program LUT RW control.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] id
    - [in] lut\_bank\_a
    - [in] bit\_depth
    - [in] mpcc\_id

    Return:

    void

`program_lut_mode`
:   Program LUT mode.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] id
    - [in] enable
    - [in] lut\_bank\_a
    - [in] size
    - [in] mpcc\_id

    Return:

    void

`get_lut_mode`
:   Obtains enablement and ram bank status.

    Parameters:
    - [in/out] mpc - MPC context.
    - [in] id
    - [in] mpcc\_id
    - [out] enable
    - [out] lut\_bank\_a

    Return:

    void

`rmcm`
:   MPC RMCM new HW sequential programming functions

## OPP

The Output Plane Processor (OPP) block groups have functions that format
pixel streams such that they are suitable for display at the display device.
The key functions contained in the OPP are:

* Adaptive Backlight Modulation (ABM)
* Formatter (FMT) which provide pixel-by-pixel operations for format the
  incoming pixel stream.
* Output Buffer that provide pixel replication, and overlapping.
* Interface between MPC and OPTC.
* Clock and reset generation.
* CRC generation.

struct pwl\_float\_data
:   Fixed point RGB color

**Definition**:

```
struct pwl_float_data {
    struct fixed31_32 r;
    struct fixed31_32 g;
    struct fixed31_32 b;
};
```

**Members**

`r`
:   Component Red.

`g`
:   Component Green.

`b`
:   Component Blue.

## DIO

Display Input Output (DIO), is the display input and output unit in DCN. It
includes output encoders to support different display output, like
DisplayPort, HDMI, DVI interface, and others. It also includes the control
and status channels for these interfaces.

bool can\_use\_dio\_link\_hwss(const struct dc\_link \*link, const struct link\_resource \*link\_res)
:   Check if the link\_hwss is accessible

**Parameters**

`const struct dc_link *link`
:   Reference a link `struct containing` one or more sinks and the
    connective status.

`const struct link_resource *link_res`
:   Mappable hardware resource used to enable a link.

**Return**

Return true if the link encoder is accessible from link.

const struct link\_hwss \*get\_dio\_link\_hwss(void)
:   Return link\_hwss reference

**Parameters**

`void`
:   no arguments

**Description**

This function behaves like a get function to return the link\_hwss populated
in the link\_hwss\_dio.c file.

**Return**

Return the reference to the filled `struct of` link\_hwss.
