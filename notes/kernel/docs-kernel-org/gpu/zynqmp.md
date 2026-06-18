# Xilinx ZynqMP Ultrascale+ DisplayPort Subsystem

> 출처(원문): https://docs.kernel.org/gpu/zynqmp.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Xilinx ZynqMP Ultrascale+ DisplayPort Subsystem

This subsystem handles DisplayPort video and audio output on the ZynqMP. It
supports in-memory framebuffers with the DisplayPort DMA controller
(xilinx-dpdma), as well as “live” video and audio from the programmable logic
(PL). This subsystem can perform several transformations, including color space
conversion, alpha blending, and audio mixing, although not all features are
currently supported.

## debugfs

To support debugging and compliance testing, several test modes can be enabled
though debugfs. The following files in /sys/kernel/debug/dri/X/DP-1/test/
control the DisplayPort test modes:

active:
:   Writing a 1 to this file will activate test mode, and writing a 0 will
    deactivate test mode. Writing a 1 or 0 when the test mode is already
    active/inactive will re-activate/re-deactivate test mode. When test
    mode is inactive, changes made to other files will have no (immediate)
    effect, although the settings will be saved for when test mode is
    activated. When test mode is active, changes made to other files will
    apply immediately.

custom:
:   Custom test pattern value

downspread:
:   Enable/disable clock downspreading (spread-spectrum clocking) by
    writing 1/0

enhanced:
:   Enable/disable enhanced framing

ignore\_aux\_errors:
:   Ignore AUX errors when set to 1. Writes to this file take effect
    immediately (regardless of whether test mode is active) and affect all
    AUX transfers.

ignore\_hpd:
:   Ignore hotplug events (such as cable removals or monitor link
    retraining requests) when set to 1. Writes to this file take effect
    immediately (regardless of whether test mode is active).

laneX\_preemphasis:
:   Preemphasis from 0 (lowest) to 2 (highest) for lane X

laneX\_swing:
:   Voltage swing from 0 (lowest) to 3 (highest) for lane X

lanes:
:   Number of lanes to use (1, 2, or 4)

pattern:
:   Test pattern. May be one of:

    > video
    > :   Use regular video input
    >
    > symbol-error
    > :   Symbol error measurement pattern
    >
    > prbs7
    > :   Output of the PRBS7 (x^7 + x^6 + 1) polynomial
    >
    > 80bit-custom
    > :   A custom 80-bit pattern
    >
    > cp2520
    > :   HBR2 compliance eye pattern
    >
    > tps1
    > :   Link training symbol pattern TPS1 (/D10.2/)
    >
    > tps2
    > :   Link training symbol pattern TPS2
    >
    > tps3
    > :   Link training symbol pattern TPS3 (for HBR2)

rate:
:   Rate in hertz. One of

    > * 5400000000 (HBR2)
    > * 2700000000 (HBR)
    > * 1620000000 (RBR)

You can dump the displayport test settings with the following command:

```
for prop in /sys/kernel/debug/dri/1/DP-1/test/*; do
        printf '%-17s ' ${prop##*/}
        if [ ${prop##*/} = custom ]; then
                hexdump -C $prop | head -1
        else
                cat $prop
        fi
done
```

The output could look something like:

```
active            1
custom            00000000  00 00 00 00 00 00 00 00  00 00                    |..........|
downspread        0
enhanced          1
ignore_aux_errors 1
ignore_hpd        1
lane0_preemphasis 0
lane0_swing       3
lane1_preemphasis 0
lane1_swing       3
lanes             2
pattern           prbs7
rate              1620000000
```

The recommended test procedure is to connect the board to a monitor,
configure test mode, activate test mode, and then disconnect the cable
and connect it to your test equipment of choice. For example, one
sequence of commands could be:

```
echo 1 > /sys/kernel/debug/dri/1/DP-1/test/enhanced
echo tps1 > /sys/kernel/debug/dri/1/DP-1/test/pattern
echo 1620000000 > /sys/kernel/debug/dri/1/DP-1/test/rate
echo 1 > /sys/kernel/debug/dri/1/DP-1/test/ignore_aux_errors
echo 1 > /sys/kernel/debug/dri/1/DP-1/test/ignore_hpd
echo 1 > /sys/kernel/debug/dri/1/DP-1/test/active
```

at which point the cable could be disconnected from the monitor.

## Internals

enum zynqmp\_dpsub\_layer\_id
:   Layer identifier

**Constants**

`ZYNQMP_DPSUB_LAYER_VID`
:   Video layer

`ZYNQMP_DPSUB_LAYER_GFX`
:   Graphics layer

struct zynqmp\_dpsub
:   ZynqMP DisplayPort Subsystem

**Definition**:

```
struct zynqmp_dpsub {
    struct device *dev;
    struct clk *apb_clk;
    struct clk *vid_clk;
    bool vid_clk_from_ps;
    struct clk *aud_clk;
    bool aud_clk_from_ps;
    unsigned int connected_ports;
    bool dma_enabled;
    struct zynqmp_dpsub_drm *drm;
    struct drm_bridge *bridge;
    struct zynqmp_disp *disp;
    struct zynqmp_disp_layer *layers[ZYNQMP_DPSUB_NUM_LAYERS];
    struct zynqmp_dp *dp;
    unsigned int dma_align;
    struct zynqmp_dpsub_audio *audio;
};
```

**Members**

`dev`
:   The physical device

`apb_clk`
:   The APB clock

`vid_clk`
:   Video clock

`vid_clk_from_ps`
:   True of the video clock comes from PS, false from PL

`aud_clk`
:   Audio clock

`aud_clk_from_ps`
:   True of the audio clock comes from PS, false from PL

`connected_ports`
:   Bitmask of connected ports in the device tree

`dma_enabled`
:   True if the DMA interface is enabled, false if the DPSUB is
    driven by the live input

`drm`
:   The DRM/KMS device data

`bridge`
:   The DP encoder bridge

`disp`
:   The display controller

`layers`
:   Video and graphics layers

`dp`
:   The DisplayPort controller

`dma_align`
:   DMA alignment constraint (must be a power of 2)

`audio`
:   DP audio data

struct zynqmp\_dpsub\_drm
:   ZynqMP DisplayPort Subsystem DRM/KMS data

**Definition**:

```
struct zynqmp_dpsub_drm {
    struct zynqmp_dpsub *dpsub;
    struct drm_device dev;
    struct drm_plane planes[ZYNQMP_DPSUB_NUM_LAYERS];
    struct drm_crtc crtc;
    struct drm_encoder encoder;
};
```

**Members**

`dpsub`
:   Backpointer to the DisplayPort subsystem

`dev`
:   The DRM/KMS device

`planes`
:   The DRM planes

`crtc`
:   The DRM CRTC

`encoder`
:   The dummy DRM encoder

enum zynqmp\_dpsub\_layer\_mode
:   Layer mode

**Constants**

`ZYNQMP_DPSUB_LAYER_NONLIVE`
:   non-live (memory) mode

`ZYNQMP_DPSUB_LAYER_LIVE`
:   live (stream) mode

struct zynqmp\_disp\_format
:   Display subsystem format information

**Definition**:

```
struct zynqmp_disp_format {
    u32 drm_fmt;
    u32 bus_fmt;
    u32 buf_fmt;
    bool swap;
    const u32 *sf;
};
```

**Members**

`drm_fmt`
:   DRM format (4CC)

`bus_fmt`
:   Media bus format

`buf_fmt`
:   AV buffer format

`swap`
:   Flag to swap R & B for RGB formats, and U & V for YUV formats

`sf`
:   Scaling factors for color components

struct zynqmp\_disp\_layer\_dma
:   DMA channel for one data plane of a layer

**Definition**:

```
struct zynqmp_disp_layer_dma {
    struct dma_chan *chan;
    struct dma_interleaved_template xt;
    struct data_chunk sgl;
};
```

**Members**

`chan`
:   DMA channel

`xt`
:   Interleaved DMA descriptor template

`sgl`
:   Data chunk for dma\_interleaved\_template

struct zynqmp\_disp\_layer\_info
:   Static layer information

**Definition**:

```
struct zynqmp_disp_layer_info {
    const struct zynqmp_disp_format *formats;
    unsigned int num_formats;
    unsigned int num_channels;
};
```

**Members**

`formats`
:   Array of supported formats

`num_formats`
:   Number of formats in **formats** array

`num_channels`
:   Number of DMA channels

struct zynqmp\_disp\_layer
:   Display layer

**Definition**:

```
struct zynqmp_disp_layer {
    enum zynqmp_dpsub_layer_id id;
    struct zynqmp_disp *disp;
    const struct zynqmp_disp_layer_info *info;
    struct zynqmp_disp_layer_dma dmas[ZYNQMP_DISP_MAX_NUM_SUB_PLANES];
    const struct zynqmp_disp_format *disp_fmt;
    const struct drm_format_info *drm_fmt;
    enum zynqmp_dpsub_layer_mode mode;
};
```

**Members**

`id`
:   Layer ID

`disp`
:   Back pointer to [`struct zynqmp_disp`](#c.zynqmp_disp "zynqmp_disp")

`info`
:   Static layer information

`dmas`
:   DMA channels

`disp_fmt`
:   Current format information

`drm_fmt`
:   Current DRM format information

`mode`
:   Current operation mode

struct zynqmp\_disp
:   Display controller

**Definition**:

```
struct zynqmp_disp {
    struct device *dev;
    struct zynqmp_dpsub *dpsub;
    void __iomem *blend;
    void __iomem *avbuf;
    struct zynqmp_disp_layer layers[ZYNQMP_DPSUB_NUM_LAYERS];
};
```

**Members**

`dev`
:   Device structure

`dpsub`
:   Display subsystem

`blend`
:   Register I/O base address for the blender

`avbuf`
:   Register I/O base address for the audio/video buffer manager

`layers`
:   Layers (planes)

void zynqmp\_disp\_avbuf\_set\_format(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp, struct [zynqmp\_disp\_layer](#c.zynqmp_disp_layer "zynqmp_disp_layer") \*layer, const struct [zynqmp\_disp\_format](#c.zynqmp_disp_format "zynqmp_disp_format") \*fmt)
:   Set the input format for a layer

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

`struct zynqmp_disp_layer *layer`
:   The layer

`const struct zynqmp_disp_format *fmt`
:   The format information

**Description**

Set the video buffer manager format for **layer** to **fmt**.

void zynqmp\_disp\_avbuf\_set\_clocks\_sources(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp, bool video\_from\_ps, bool audio\_from\_ps, bool timings\_internal)
:   Set the clocks sources

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

`bool video_from_ps`
:   True if the video clock originates from the PS

`bool audio_from_ps`
:   True if the audio clock originates from the PS

`bool timings_internal`
:   True if video timings are generated internally

**Description**

Set the source for the video and audio clocks, as well as for the video
timings. Clocks can originate from the PS or PL, and timings can be
generated internally or externally.

void zynqmp\_disp\_avbuf\_enable\_channels(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp)
:   Enable buffer channels

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

**Description**

Enable all (video and audio) buffer channels.

void zynqmp\_disp\_avbuf\_disable\_channels(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp)
:   Disable buffer channels

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

**Description**

Disable all (video and audio) buffer channels.

void zynqmp\_disp\_avbuf\_enable\_audio(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp)
:   Enable audio

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

**Description**

Enable all audio buffers with a non-live (memory) source.

void zynqmp\_disp\_avbuf\_disable\_audio(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp)
:   Disable audio

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

**Description**

Disable all audio buffers.

void zynqmp\_disp\_avbuf\_enable\_video(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp, struct [zynqmp\_disp\_layer](#c.zynqmp_disp_layer "zynqmp_disp_layer") \*layer)
:   Enable a video layer

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

`struct zynqmp_disp_layer *layer`
:   The layer

**Description**

Enable the video/graphics buffer for **layer**.

void zynqmp\_disp\_avbuf\_disable\_video(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp, struct [zynqmp\_disp\_layer](#c.zynqmp_disp_layer "zynqmp_disp_layer") \*layer)
:   Disable a video layer

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

`struct zynqmp_disp_layer *layer`
:   The layer

**Description**

Disable the video/graphics buffer for **layer**.

void zynqmp\_disp\_avbuf\_enable(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp)
:   Enable the video pipe

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

**Description**

De-assert the video pipe reset.

void zynqmp\_disp\_avbuf\_disable(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp)
:   Disable the video pipe

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

**Description**

Assert the video pipe reset.

void zynqmp\_disp\_blend\_set\_output\_format(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp, enum zynqmp\_dpsub\_format format)
:   Set the output format of the blender

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

`enum zynqmp_dpsub_format format`
:   Output format

**Description**

Set the output format of the blender to **format**.

void zynqmp\_disp\_blend\_set\_bg\_color(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp, u32 rcr, u32 gy, u32 bcb)
:   Set the background color

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

`u32 rcr`
:   Red/Cr color component

`u32 gy`
:   Green/Y color component

`u32 bcb`
:   Blue/Cb color component

**Description**

Set the background color to (**rcr**, **gy**, **bcb**), corresponding to the R, G and
B or Cr, Y and Cb components respectively depending on the selected output
format.

void zynqmp\_disp\_blend\_set\_global\_alpha(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp, bool enable, u32 alpha)
:   Configure global alpha blending

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

`bool enable`
:   True to enable global alpha blending

`u32 alpha`
:   Global alpha value (ignored if **enabled** is false)

void zynqmp\_disp\_blend\_layer\_set\_csc(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp, struct [zynqmp\_disp\_layer](#c.zynqmp_disp_layer "zynqmp_disp_layer") \*layer, const u16 \*coeffs, const u32 \*offsets)
:   Configure colorspace conversion for layer

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

`struct zynqmp_disp_layer *layer`
:   The layer

`const u16 *coeffs`
:   Colorspace conversion matrix

`const u32 *offsets`
:   Colorspace conversion offsets

**Description**

Configure the input colorspace conversion matrix and offsets for the **layer**.
Columns of the matrix are automatically swapped based on the input format to
handle RGB and YCrCb components permutations.

void zynqmp\_disp\_blend\_layer\_enable(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp, struct [zynqmp\_disp\_layer](#c.zynqmp_disp_layer "zynqmp_disp_layer") \*layer)
:   Enable a layer

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

`struct zynqmp_disp_layer *layer`
:   The layer

void zynqmp\_disp\_blend\_layer\_disable(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp, struct [zynqmp\_disp\_layer](#c.zynqmp_disp_layer "zynqmp_disp_layer") \*layer)
:   Disable a layer

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

`struct zynqmp_disp_layer *layer`
:   The layer

const struct [zynqmp\_disp\_format](#c.zynqmp_disp_format "zynqmp_disp_format") \*zynqmp\_disp\_layer\_find\_format(struct [zynqmp\_disp\_layer](#c.zynqmp_disp_layer "zynqmp_disp_layer") \*layer, u32 drm\_fmt)
:   Find format information for a DRM format

**Parameters**

`struct zynqmp_disp_layer *layer`
:   The layer

`u32 drm_fmt`
:   DRM format to search

**Description**

Search display subsystem format information corresponding to the given DRM
format **drm\_fmt** for the **layer**, and return a pointer to the format
descriptor.

**Return**

A pointer to the format descriptor if found, NULL otherwise

const struct [zynqmp\_disp\_format](#c.zynqmp_disp_format "zynqmp_disp_format") \*zynqmp\_disp\_layer\_find\_live\_format(struct [zynqmp\_disp\_layer](#c.zynqmp_disp_layer "zynqmp_disp_layer") \*layer, u32 media\_bus\_format)
:   Find format information for given media bus format

**Parameters**

`struct zynqmp_disp_layer *layer`
:   The layer

`u32 media_bus_format`
:   Media bus format to search

**Description**

Search display subsystem format information corresponding to the given media
bus format **media\_bus\_format** for the **layer**, and return a pointer to the
format descriptor.

**Return**

A pointer to the format descriptor if found, NULL otherwise

u32 \*zynqmp\_disp\_layer\_drm\_formats(struct [zynqmp\_disp\_layer](#c.zynqmp_disp_layer "zynqmp_disp_layer") \*layer, unsigned int \*num\_formats)
:   Return the DRM formats supported by the layer

**Parameters**

`struct zynqmp_disp_layer *layer`
:   The layer

`unsigned int *num_formats`
:   Pointer to the returned number of formats

**NOTE**

This function doesn’t make sense for live video layers and will
always return an empty list in such cases. [`zynqmp_disp_live_layer_formats()`](#c.zynqmp_disp_live_layer_formats "zynqmp_disp_live_layer_formats")
should be used to query a list of media bus formats supported by the live
video input layer.

**Return**

A newly allocated u32 array that stores all the DRM formats
supported by the layer. The number of formats in the array is returned
through the num\_formats argument.

u32 \*zynqmp\_disp\_live\_layer\_formats(struct [zynqmp\_disp\_layer](#c.zynqmp_disp_layer "zynqmp_disp_layer") \*layer, unsigned int \*num\_formats)
:   Return the media bus formats supported by the live video layer

**Parameters**

`struct zynqmp_disp_layer *layer`
:   The layer

`unsigned int *num_formats`
:   Pointer to the returned number of formats

**NOTE**

This function should be used only for live video input layers.

**Return**

A newly allocated u32 array of media bus formats supported by the
layer. The number of formats in the array is returned through the
**num\_formats** argument.

void zynqmp\_disp\_layer\_enable(struct [zynqmp\_disp\_layer](#c.zynqmp_disp_layer "zynqmp_disp_layer") \*layer)
:   Enable a layer

**Parameters**

`struct zynqmp_disp_layer *layer`
:   The layer

**Description**

Enable the **layer** in the audio/video buffer manager and the blender. DMA
channels are started separately by [`zynqmp_disp_layer_update()`](#c.zynqmp_disp_layer_update "zynqmp_disp_layer_update").

void zynqmp\_disp\_layer\_disable(struct [zynqmp\_disp\_layer](#c.zynqmp_disp_layer "zynqmp_disp_layer") \*layer)
:   Disable the layer

**Parameters**

`struct zynqmp_disp_layer *layer`
:   The layer

**Description**

Disable the layer by stopping its DMA channels and disabling it in the
audio/video buffer manager and the blender.

void zynqmp\_disp\_layer\_set\_format(struct [zynqmp\_disp\_layer](#c.zynqmp_disp_layer "zynqmp_disp_layer") \*layer, const struct [drm\_format\_info](drm-kms.html#c.drm_format_info "drm_format_info") \*info)
:   Set the layer format

**Parameters**

`struct zynqmp_disp_layer *layer`
:   The layer

`const struct drm_format_info *info`
:   The format info

**NOTE**

Use [`zynqmp_disp_layer_set_live_format()`](#c.zynqmp_disp_layer_set_live_format "zynqmp_disp_layer_set_live_format") to set media bus format for
live video layers.

Set the format for **layer** to **info**. The layer must be disabled.

void zynqmp\_disp\_layer\_set\_live\_format(struct [zynqmp\_disp\_layer](#c.zynqmp_disp_layer "zynqmp_disp_layer") \*layer, u32 media\_bus\_format)
:   Set the live video layer format

**Parameters**

`struct zynqmp_disp_layer *layer`
:   The layer

`u32 media_bus_format`
:   Media bus format to set

**NOTE**

This function should not be used to set format for non-live video
layer. Use [`zynqmp_disp_layer_set_format()`](#c.zynqmp_disp_layer_set_format "zynqmp_disp_layer_set_format") instead.

Set the display format for the live **layer**. The layer must be disabled.

int zynqmp\_disp\_layer\_update(struct [zynqmp\_disp\_layer](#c.zynqmp_disp_layer "zynqmp_disp_layer") \*layer, struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*state)
:   Update the layer framebuffer

**Parameters**

`struct zynqmp_disp_layer *layer`
:   The layer

`struct drm_plane_state *state`
:   The plane state

**Description**

Update the framebuffer for the layer by issuing a new DMA engine transaction
for the new framebuffer.

**Return**

0 on success, or the DMA descriptor failure error otherwise

void zynqmp\_disp\_layer\_release\_dma(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp, struct [zynqmp\_disp\_layer](#c.zynqmp_disp_layer "zynqmp_disp_layer") \*layer)
:   Release DMA channels for a layer

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

`struct zynqmp_disp_layer *layer`
:   The layer

**Description**

Release the DMA channels associated with **layer**.

void zynqmp\_disp\_destroy\_layers(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp)
:   Destroy all layers

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

int zynqmp\_disp\_layer\_request\_dma(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp, struct [zynqmp\_disp\_layer](#c.zynqmp_disp_layer "zynqmp_disp_layer") \*layer)
:   Request DMA channels for a layer

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

`struct zynqmp_disp_layer *layer`
:   The layer

**Description**

Request all DMA engine channels needed by **layer**.

**Return**

0 on success, or the DMA channel request error otherwise

int zynqmp\_disp\_create\_layers(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp)
:   Create and initialize all layers

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

**Return**

0 on success, or the DMA channel request error otherwise

void zynqmp\_disp\_enable(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp)
:   Enable the display controller

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

void zynqmp\_disp\_disable(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp)
:   Disable the display controller

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

int zynqmp\_disp\_setup\_clock(struct [zynqmp\_disp](#c.zynqmp_disp "zynqmp_disp") \*disp, unsigned long mode\_clock)
:   Configure the display controller pixel clock rate

**Parameters**

`struct zynqmp_disp *disp`
:   Display controller

`unsigned long mode_clock`
:   The pixel clock rate, in Hz

**Return**

0 on success, or a negative error clock otherwise

struct zynqmp\_dp\_link\_config
:   Common link config between source and sink

**Definition**:

```
struct zynqmp_dp_link_config {
    int max_rate;
    u8 max_lanes;
};
```

**Members**

`max_rate`
:   maximum link rate

`max_lanes`
:   maximum number of lanes

struct zynqmp\_dp\_mode
:   Configured mode of DisplayPort

**Definition**:

```
struct zynqmp_dp_mode {
    const char *fmt;
    int pclock;
    u8 bw_code;
    u8 lane_cnt;
};
```

**Members**

`fmt`
:   format identifier string

`pclock`
:   pixel clock frequency of current mode

`bw_code`
:   code for bandwidth(link rate)

`lane_cnt`
:   number of lanes

struct zynqmp\_dp\_config
:   Configuration of DisplayPort from DTS

**Definition**:

```
struct zynqmp_dp_config {
    u8 misc0;
    u8 misc1;
    u8 bpp;
};
```

**Members**

`misc0`
:   misc0 configuration (per DP v1.2 spec)

`misc1`
:   misc1 configuration (per DP v1.2 spec)

`bpp`
:   bits per pixel

enum test\_pattern
:   Test patterns for test testing

**Constants**

`TEST_VIDEO`
:   Use regular video input

`TEST_TPS1`
:   Link training symbol pattern TPS1 (/D10.2/)

`TEST_TPS2`
:   Link training symbol pattern TPS2

`TEST_TPS3`
:   Link training symbol pattern TPS3 (for HBR2)

`TEST_SYMBOL_ERROR`
:   Symbol error measurement pattern

`TEST_PRBS7`
:   Output of the PRBS7 (x^7 + x^6 + 1) polynomial

`TEST_80BIT_CUSTOM`
:   A custom 80-bit pattern

`TEST_CP2520`
:   HBR2 compliance eye pattern

struct zynqmp\_dp\_test
:   Configuration for test mode

**Definition**:

```
struct zynqmp_dp_test {
    enum test_pattern pattern;
    bool enhanced, downspread, active;
    u8 custom[10];
    u8 train_set[ZYNQMP_DP_MAX_LANES];
    u8 bw_code;
    u8 link_cnt;
};
```

**Members**

`pattern`
:   The test pattern

`enhanced`
:   Use enhanced framing

`downspread`
:   Use SSC

`active`
:   Whether test mode is active

`custom`
:   Custom pattern for `TEST_80BIT_CUSTOM`

`train_set`
:   Voltage/preemphasis settings

`bw_code`
:   Bandwidth code for the link

`link_cnt`
:   Number of lanes

struct zynqmp\_dp\_train\_set\_priv
:   Private data for train\_set debugfs files

**Definition**:

```
struct zynqmp_dp_train_set_priv {
    struct zynqmp_dp *dp;
    int lane;
};
```

**Members**

`dp`
:   DisplayPort IP core structure

`lane`
:   The lane for this file

struct zynqmp\_dp
:   Xilinx DisplayPort core

**Definition**:

```
struct zynqmp_dp {
    struct drm_dp_aux aux;
    struct drm_bridge bridge;
    struct work_struct hpd_work;
    struct work_struct hpd_irq_work;
    struct completion aux_done;
    struct mutex lock;
    struct drm_bridge *next_bridge;
    struct device *dev;
    struct zynqmp_dpsub *dpsub;
    void __iomem *iomem;
    struct reset_control *reset;
    struct phy *phy[ZYNQMP_DP_MAX_LANES];
    enum drm_connector_status status;
    int irq;
    bool enabled;
    bool ignore_aux_errors;
    bool ignore_hpd;
    struct zynqmp_dp_train_set_priv debugfs_train_set[ZYNQMP_DP_MAX_LANES];
    struct zynqmp_dp_mode mode;
    struct zynqmp_dp_link_config link_config;
    struct zynqmp_dp_test test;
    struct zynqmp_dp_config config;
    u8 dpcd[DP_RECEIVER_CAP_SIZE];
    u8 train_set[ZYNQMP_DP_MAX_LANES];
    u8 num_lanes;
};
```

**Members**

`aux`
:   aux channel

`bridge`
:   DRM bridge for the DP encoder

`hpd_work`
:   hot plug detection worker

`hpd_irq_work`
:   hot plug detection IRQ worker

`aux_done`
:   Completed when we get an AUX reply or timeout

`lock`
:   Mutex protecting this `struct and` register access (but not AUX)

`next_bridge`
:   The downstream bridge

`dev`
:   device structure

`dpsub`
:   Display subsystem

`iomem`
:   device I/O memory for register access

`reset`
:   reset controller

`phy`
:   PHY handles for DP lanes

`status`
:   connection status

`irq`
:   irq

`enabled`
:   flag to indicate if the device is enabled

`ignore_aux_errors`
:   If set, AUX errors are suppressed

`ignore_hpd`
:   If set, HPD events and IRQs are ignored

`debugfs_train_set`
:   Debugfs private data for **train\_set**

`mode`
:   current mode between IP core and sink device

`link_config`
:   common link configuration between IP core and sink device

`test`
:   Configuration for test mode

`config`
:   IP core configuration from DTS

`dpcd`
:   DP configuration data from currently connected sink device

`train_set`
:   set of training data

`num_lanes`
:   number of enabled phy lanes

**Description**

**lock** covers the link configuration in this `struct and` the device’s
registers. It does not cover **aux** or **ignore\_aux\_errors**. It is not strictly
required for any of the members which are only modified at probe/remove time
(e.g. **dev**).

int zynqmp\_dp\_phy\_init(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp)
:   Initialize the phy

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

**Description**

Initialize the phy.

**Return**

0 if the phy instances are initialized correctly, or the error code
returned from the callee functions.

void zynqmp\_dp\_phy\_exit(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp)
:   Exit the phy

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

**Description**

Exit the phy.

int zynqmp\_dp\_phy\_probe(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp)
:   Probe the PHYs

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

**Description**

Probe PHYs for all lanes. Less PHYs may be available than the number of
lanes, which is not considered an error as long as at least one PHY is
found. The caller can check dp->num\_lanes to check how many PHYs were found.

**Return**

* 0 - Success
* `-ENXIO`
  :   + No PHY found
* `-EPROBE_DEFER`
  :   + Probe deferral requested
* Other negative value - PHY retrieval failure

int zynqmp\_dp\_phy\_ready(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp)
:   Check if PHY is ready

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

**Description**

Check if PHY is ready. If PHY is not ready, wait 1ms to check for 100 times.
This amount of delay was suggested by IP designer.

**Return**

0 if PHY is ready, or -ENODEV if PHY is not ready.

int zynqmp\_dp\_max\_rate(int link\_rate, u8 lane\_num, u8 bpp)
:   Calculate and return available max pixel clock

**Parameters**

`int link_rate`
:   link rate (Kilo-bytes / sec)

`u8 lane_num`
:   number of lanes

`u8 bpp`
:   bits per pixel

**Return**

max pixel clock (KHz) supported by current link config.

int zynqmp\_dp\_mode\_configure(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp, int pclock, u8 current\_bw)
:   Configure the link values

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

`int pclock`
:   pixel clock for requested display mode

`u8 current_bw`
:   current link rate

**Description**

Find the link configuration values, rate and lane count for requested pixel
clock **pclock**. The **pclock** is stored in the mode to be used in other
functions later. The returned rate is downshifted from the current rate
**current\_bw**.

**Return**

Current link rate code, or -EINVAL.

void zynqmp\_dp\_adjust\_train(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp, u8 link\_status[DP\_LINK\_STATUS\_SIZE])
:   Adjust train values

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

`u8 link_status[DP_LINK_STATUS_SIZE]`
:   link status from sink which contains requested training values

int zynqmp\_dp\_update\_vs\_emph(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp, u8 \*train\_set)
:   Update the training values

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

`u8 *train_set`
:   A set of training values

**Description**

Update the training values based on the request from sink. The mapped values
are predefined, and values(vs, pe, pc) are from the device manual.

**Return**

0 if vs and emph are updated successfully, or the error code returned
by [`drm_dp_dpcd_write()`](drm-kms-helpers.html#c.drm_dp_dpcd_write "drm_dp_dpcd_write").

int zynqmp\_dp\_link\_train\_cr(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp)
:   Train clock recovery

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

**Return**

0 if clock recovery train is done successfully, or corresponding
error code.

int zynqmp\_dp\_link\_train\_ce(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp)
:   Train channel equalization

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

**Return**

0 if channel equalization train is done successfully, or
corresponding error code.

int zynqmp\_dp\_setup(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp, u8 bw\_code, u8 lane\_cnt, bool enhanced, bool downspread)
:   Set up major link parameters

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

`u8 bw_code`
:   The link bandwidth as a multiple of 270 MHz

`u8 lane_cnt`
:   The number of lanes to use

`bool enhanced`
:   Use enhanced framing

`bool downspread`
:   Enable spread-spectrum clocking

**Return**

0 on success, or -errno on failure

int zynqmp\_dp\_train(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp)
:   Train the link

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

**Return**

0 if all trains are done successfully, or corresponding error code.

void zynqmp\_dp\_train\_loop(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp)
:   Downshift the link rate during training

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

**Description**

Train the link by downshifting the link rate if training is not successful.

int zynqmp\_dp\_aux\_cmd\_submit(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp, u32 cmd, u16 addr, u8 \*buf, u8 bytes, u8 \*reply)
:   Submit aux command

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

`u32 cmd`
:   aux command

`u16 addr`
:   aux address

`u8 *buf`
:   buffer for command data

`u8 bytes`
:   number of bytes for **buf**

`u8 *reply`
:   reply code to be returned

**Description**

Submit an aux command. All aux related commands, native or i2c aux
read/write, are submitted through this function. The function is mapped to
the transfer function of [`struct drm_dp_aux`](drm-kms-helpers.html#c.drm_dp_aux "drm_dp_aux"). This function involves in
multiple register reads/writes, thus synchronization is needed, and it is
done by drm\_dp\_helper using **hw\_mutex**. The calling thread goes into sleep
if there’s no immediate reply to the command submission. The reply code is
returned at **reply** if **reply** != NULL.

**Return**

0 if the command is submitted properly, or corresponding error code:
-EBUSY when there is any request already being processed
-ETIMEDOUT when receiving reply is timed out
-EIO when received bytes are less than requested

int zynqmp\_dp\_aux\_init(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp)
:   Initialize and register the DP AUX

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

**Description**

Program the AUX clock divider and filter and register the DP AUX adapter.

**Return**

0 on success, error value otherwise

void zynqmp\_dp\_aux\_cleanup(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp)
:   Cleanup the DP AUX

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

**Description**

Unregister the DP AUX adapter.

void zynqmp\_dp\_update\_misc(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp)
:   Write the misc registers

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

**Description**

The misc register values are stored in the structure, and this
function applies the values into the registers.

int zynqmp\_dp\_set\_format(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp, const struct [drm\_display\_info](drm-kms.html#c.drm_display_info "drm_display_info") \*info, enum zynqmp\_dpsub\_format format, unsigned int bpc)
:   Set the input format

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

`const struct drm_display_info *info`
:   Display info

`enum zynqmp_dpsub_format format`
:   input format

`unsigned int bpc`
:   bits per component

**Description**

Update misc register values based on input **format** and **bpc**.

**Return**

0 on success, or -EINVAL.

void zynqmp\_dp\_encoder\_mode\_set\_transfer\_unit(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp, const struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*mode)
:   Set the transfer unit values

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

`const struct drm_display_mode *mode`
:   requested display mode

**Description**

Set the transfer unit, and calculate all transfer unit size related values.
Calculation is based on DP and IP core specification.

void zynqmp\_dp\_encoder\_mode\_set\_stream(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp, const struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*mode)
:   Configure the main stream

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

`const struct drm_display_mode *mode`
:   requested display mode

**Description**

Configure the main stream based on the requested mode **mode**. Calculation is
based on IP core specification.

struct [zynqmp\_disp\_layer](#c.zynqmp_disp_layer "zynqmp_disp_layer") \*zynqmp\_dp\_disp\_connected\_live\_layer(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp)
:   Return the first connected live layer

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

**Return**

The first connected live display layer or NULL if none of the live
layers are connected.

int zynqmp\_dp\_set\_test\_pattern(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp, enum [test\_pattern](#c.test_pattern "test_pattern") pattern, u8 \*const custom)
:   Configure the link for a test pattern

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

`enum test_pattern pattern`
:   The test pattern to configure

`u8 *const custom`
:   The custom pattern to use if **pattern** is `TEST_80BIT_CUSTOM`

**Return**

0 on success, or negative errno on (DPCD) failure

void zynqmp\_dp\_enable\_vblank(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp)
:   Enable vblank

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

**Description**

Enable vblank interrupt

void zynqmp\_dp\_disable\_vblank(struct [zynqmp\_dp](#c.zynqmp_dp "zynqmp_dp") \*dp)
:   Disable vblank

**Parameters**

`struct zynqmp_dp *dp`
:   DisplayPort IP core structure

**Description**

Disable vblank interrupt

void zynqmp\_dpsub\_drm\_handle\_vblank(struct [zynqmp\_dpsub](#c.zynqmp_dpsub "zynqmp_dpsub") \*dpsub)
:   Handle the vblank event

**Parameters**

`struct zynqmp_dpsub *dpsub`
:   DisplayPort subsystem

**Description**

This function handles the vblank interrupt, and sends an event to
CRTC object. This will be called by the DP vblank interrupt handler.
