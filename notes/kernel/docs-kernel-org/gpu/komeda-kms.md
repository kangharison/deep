# drm/komeda Arm display driver

> 출처(원문): https://docs.kernel.org/gpu/komeda-kms.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# drm/komeda Arm display driver

The drm/komeda driver supports the Arm display processor D71 and later products,
this document gives a brief overview of driver design: how it works and why
design it like that.

## Overview of D71 like display IPs

From D71, Arm display IP begins to adopt a flexible and modularized
architecture. A display pipeline is made up of multiple individual and
functional pipeline stages called components, and every component has some
specific capabilities that can give the flowed pipeline pixel data a
particular processing.

Typical D71 components:

### Layer

Layer is the first pipeline stage, which prepares the pixel data for the next
stage. It fetches the pixel from memory, decodes it if it’s AFBC, rotates the
source image, unpacks or converts YUV pixels to the device internal RGB pixels,
then adjusts the color\_space of pixels if needed.

### Scaler

As its name suggests, scaler takes responsibility for scaling, and D71 also
supports image enhancements by scaler.
The usage of scaler is very flexible and can be connected to layer output
for layer scaling, or connected to compositor and scale the whole display
frame and then feed the output data into wb\_layer which will then write it
into memory.

### Compositor (compiz)

Compositor blends multiple layers or pixel data flows into one single display
frame. its output frame can be fed into post image processor for showing it on
the monitor or fed into wb\_layer and written to memory at the same time.
user can also insert a scaler between compositor and wb\_layer to down scale
the display frame first and then write to memory.

### Writeback Layer (wb\_layer)

Writeback layer does the opposite things of Layer, which connects to compiz
and writes the composition result to memory.

### Post image processor (improc)

Post image processor adjusts frame data like gamma and color space to fit the
requirements of the monitor.

### Timing controller (timing\_ctrlr)

Final stage of display pipeline, Timing controller is not for the pixel
handling, but only for controlling the display timing.

### Merger

D71 scaler mostly only has the half horizontal input/output capabilities
compared with Layer, like if Layer supports 4K input size, the scaler only can
support 2K input/output in the same time. To achieve the ful frame scaling, D71
introduces Layer Split, which splits the whole image to two half parts and feeds
them to two Layers A and B, and does the scaling independently. After scaling
the result need to be fed to merger to merge two part images together, and then
output merged result to compiz.

### Splitter

Similar to Layer Split, but Splitter is used for writeback, which splits the
compiz result to two parts and then feed them to two scalers.

## Possible D71 Pipeline usage

Benefitting from the modularized architecture, D71 pipelines can be easily
adjusted to fit different usages. And D71 has two pipelines, which support two
types of working mode:

* Dual display mode
  Two pipelines work independently and separately to drive two display outputs.
* Single display mode
  Two pipelines work together to drive only one display output.

  On this mode, pipeline\_B doesn’t work independently, but outputs its
  composition result into pipeline\_A, and its pixel timing also derived from
  pipeline\_A.timing\_ctrlr. The pipeline\_B works just like a “slave” of
  pipeline\_A(master)

### Single pipeline data flow

![Single pipeline digraph](../_images/DOT-66002ea43d67b166bf5079b6119bec5b363127ba.svg)

Single pipeline data flow

### Dual pipeline with Slave enabled

![Slave pipeline digraph](../_images/DOT-d59a989ca63363954b8d9c9e03d8cff5effff7e7.svg)

Slave pipeline enabled data flow

### Sub-pipelines for input and output

A complete display pipeline can be easily divided into three sub-pipelines
according to the in/out usage.

#### Layer(input) pipeline

![Layer data digraph](../_images/DOT-18a2db30442be8a3a7d319b58bde80b777ddc6d3.svg)

Layer (input) data flow


![Layer Split digraph](../_images/DOT-fe8cb898f5142ca0241f8f00c5a38ba8acccf617.svg)

Layer Split pipeline

#### Writeback(output) pipeline

![writeback digraph](../_images/DOT-1c48a6e05371ecefe4b6c024d2bea979a24bc08a.svg)

Writeback(output) data flow


![split writeback digraph](../_images/DOT-f14473fbd2d906d47164f55a2529d4ac78b1bc74.svg)

Writeback(output) Split data flow

#### Display output pipeline

![display digraph](../_images/DOT-d0edc63bffe8e7dca5c56d8d28a47b97ffa80d27.svg)

display output data flow

In the following section we’ll see these three sub-pipelines will be handled
by KMS-plane/wb\_conn/crtc respectively.

## Komeda Resource abstraction

### struct komeda\_pipeline/component

To fully utilize and easily access/configure the HW, the driver side also uses
a similar architecture: Pipeline/Component to describe the HW features and
capabilities, and a specific component includes two parts:

* Data flow controlling.
* Specific component capabilities and features.

So the driver defines a common header [`struct komeda_component`](#c.komeda_component "komeda_component") to describe the
data flow control and all specific components are a subclass of this base
structure.

struct komeda\_component

**Definition**:

```
struct komeda_component {
    struct drm_private_obj obj;
    struct komeda_pipeline *pipeline;
    char name[32];
    u32 __iomem *reg;
    u32 id;
    u32 hw_id;
    u8 max_active_inputs;
    u8 max_active_outputs;
    u32 supported_inputs;
    u32 supported_outputs;
    const struct komeda_component_funcs *funcs;
};
```

**Members**

`obj`
:   treat component as private obj

`pipeline`
:   the komeda pipeline this component belongs to

`name`
:   component name

`reg`
:   component register base,
    which is initialized by chip and used by chip only

`id`
:   component id

`hw_id`
:   component hw id,
    which is initialized by chip and used by chip only

`max_active_inputs`
:   **max\_active\_outputs**:

    maximum number of inputs/outputs that can be active at the same time
    Note:
    the number isn’t the bit number of **supported\_inputs** or
    **supported\_outputs**, but may be less than it, since component may not
    support enabling all **supported\_inputs**/outputs at the same time.

`max_active_outputs`
:   maximum number of outputs

`supported_inputs`
:   **supported\_outputs**:

    bitmask of BIT(component->id) for the supported inputs/outputs,
    describes the possibilities of how a component is linked into a
    pipeline.

`supported_outputs`
:   bitmask of supported output componenet ids

`funcs`
:   chip functions to access HW

**Description**

[`struct komeda_component`](#c.komeda_component "komeda_component") describe the data flow capabilities for how to link a
component into the display pipeline.
all specified components are subclass of this structure.

struct komeda\_component\_output

**Definition**:

```
struct komeda_component_output {
    struct komeda_component *component;
    u8 output_port;
};
```

**Members**

`component`
:   indicate which component the data comes from

`output_port`
:   the output port of the [`komeda_component_output.component`](#c.komeda_component_output "komeda_component_output")

**Description**

a component has multiple outputs, if want to know where the data
comes from, only know the component is not enough, we still need to know
its output port

struct komeda\_component\_state

**Definition**:

```
struct komeda_component_state {
    struct drm_private_state obj;
    struct komeda_component *component;
    union {
        struct drm_crtc *crtc;
        struct drm_plane *plane;
        struct drm_connector *wb_conn;
        void *binding_user;
    };
    u16 active_inputs;
    u16 changed_active_inputs;
    u16 affected_inputs;
    struct komeda_component_output inputs[KOMEDA_COMPONENT_N_INPUTS];
};
```

**Members**

`obj`
:   tracking component\_state by drm\_atomic\_state

`component`
:   backpointer to the component

`{unnamed_union}`
:   anonymous

`crtc`
:   backpointer for user crtc

`plane`
:   backpointer for user plane

`wb_conn`
:   backpointer for user wb\_connector

`binding_user`
:   currently bound user, the user can be **crtc**, **plane** or **wb\_conn**,
    which is valid decided by **component** and **inputs**

    * Layer: its user always is plane.
    * compiz/improc/timing\_ctrlr: the user is crtc.
    * wb\_layer: wb\_conn;
    * scaler: plane when input is layer, wb\_conn if input is compiz.

`active_inputs`
:   active\_inputs is bitmask of **inputs** index

    * active\_inputs = changed\_active\_inputs | unchanged\_active\_inputs
    * affected\_inputs = old->active\_inputs | new->active\_inputs;
    * disabling\_inputs = affected\_inputs ^ active\_inputs;
    * changed\_inputs = disabling\_inputs | changed\_active\_inputs;

    NOTE:
    changed\_inputs doesn’t include all active\_input but only
    **changed\_active\_inputs**, and this bitmask can be used in chip
    level for dirty update.

`changed_active_inputs`
:   bitmask of the changed **active\_inputs**

`affected_inputs`
:   bitmask for affected **inputs**

`inputs`
:   the specific inputs[i] only valid on BIT(i) has been set in
    **active\_inputs**, if not the inputs[i] is undefined.

**Description**

component\_state is the data flow configuration of the component, and it’s
the superclass of all specific component\_state like **komeda\_layer\_state**,
**komeda\_scaler\_state**

struct komeda\_pipeline

**Definition**:

```
struct komeda_pipeline {
    struct drm_private_obj obj;
    struct komeda_dev *mdev;
    struct clk *pxlclk;
    int id;
    u32 avail_comps;
    u32 standalone_disabled_comps;
    int n_layers;
    struct komeda_layer *layers[KOMEDA_PIPELINE_MAX_LAYERS];
    int n_scalers;
    struct komeda_scaler *scalers[KOMEDA_PIPELINE_MAX_SCALERS];
    struct komeda_compiz *compiz;
    struct komeda_splitter *splitter;
    struct komeda_merger *merger;
    struct komeda_layer  *wb_layer;
    struct komeda_improc *improc;
    struct komeda_timing_ctrlr *ctrlr;
    const struct komeda_pipeline_funcs *funcs;
    struct device_node *of_node;
    struct device_node *of_output_port;
    struct device_node *of_output_links[2];
    bool dual_link;
};
```

**Members**

`obj`
:   link pipeline as private obj of drm\_atomic\_state

`mdev`
:   the parent komeda\_dev

`pxlclk`
:   pixel clock

`id`
:   pipeline id

`avail_comps`
:   available components mask of pipeline

`standalone_disabled_comps`
:   When disable the pipeline, some components can not be disabled
    together with others, but need a sparated and standalone disable.
    The standalone\_disabled\_comps are the components which need to be
    disabled standalone, and this concept also introduce concept of
    two phase.
    phase 1: for disabling the common components.
    phase 2: for disabling the standalong\_disabled\_comps.

`n_layers`
:   the number of layer on **layers**

`layers`
:   the pipeline layers

`n_scalers`
:   the number of scaler on **scalers**

`scalers`
:   the pipeline scalers

`compiz`
:   compositor

`splitter`
:   for split the compiz output to two half data flows

`merger`
:   merger

`wb_layer`
:   writeback layer

`improc`
:   post image processor

`ctrlr`
:   timing controller

`funcs`
:   chip private pipeline functions

`of_node`
:   pipeline dt node

`of_output_port`
:   pipeline output port

`of_output_links`
:   output connector device nodes

`dual_link`
:   true if of\_output\_links[0] and [1] are both valid

**Description**

Represent a complete display pipeline and hold all functional components.

struct komeda\_pipeline\_state

**Definition**:

```
struct komeda_pipeline_state {
    struct drm_private_state obj;
    struct komeda_pipeline *pipe;
    struct drm_crtc *crtc;
    u32 active_comps;
};
```

**Members**

`obj`
:   tracking pipeline\_state by drm\_atomic\_state

`pipe`
:   backpointer to the pipeline

`crtc`
:   currently bound crtc

`active_comps`
:   bitmask - BIT(component->id) of active components

**NOTE**

Unlike the pipeline, pipeline\_state doesn’t gather any component\_state
into it. It because all component will be managed by drm\_atomic\_state.

## Resource discovery and initialization

Pipeline and component are used to describe how to handle the pixel data. We
still need a @[`struct komeda_dev`](#c.komeda_dev "komeda_dev") to describe the whole view of the device, and
the control-abilites of device.

We have &komeda\_dev, &komeda\_pipeline, &komeda\_component. Now fill devices with
pipelines. Since komeda is not for D71 only but also intended for later products,
of course we’d better share as much as possible between different products. To
achieve this, split the komeda device into two layers: CORE and CHIP.

* CORE: for common features and capabilities handling.
* CHIP: for register programming and HW specific feature (limitation) handling.

CORE can access CHIP by three chip function structures:

* [`struct komeda_dev_funcs`](#c.komeda_dev_funcs "komeda_dev_funcs")
* `struct komeda_pipeline_funcs`
* `struct komeda_component_funcs`

struct komeda\_dev\_funcs

**Definition**:

```
struct komeda_dev_funcs {
    void (*init_format_table)(struct komeda_dev *mdev);
    int (*enum_resources)(struct komeda_dev *mdev);
    void (*cleanup)(struct komeda_dev *mdev);
    int (*connect_iommu)(struct komeda_dev *mdev);
    int (*disconnect_iommu)(struct komeda_dev *mdev);
    irqreturn_t (*irq_handler)(struct komeda_dev *mdev, struct komeda_events *events);
    int (*enable_irq)(struct komeda_dev *mdev);
    int (*disable_irq)(struct komeda_dev *mdev);
    void (*on_off_vblank)(struct komeda_dev *mdev, int master_pipe, bool on);
    void (*dump_register)(struct komeda_dev *mdev, struct seq_file *seq);
    int (*change_opmode)(struct komeda_dev *mdev, int new_mode);
    void (*flush)(struct komeda_dev *mdev, int master_pipe, u32 active_pipes);
};
```

**Members**

`init_format_table`
:   initialize [`komeda_dev->format_table`](#c.komeda_dev "komeda_dev"), this function should be called
    before the `enum_resource`

`enum_resources`
:   for CHIP to report or add pipeline and component resources to CORE

`cleanup`
:   call to chip to cleanup komeda\_dev->chip data

`connect_iommu`
:   Optional, connect to external iommu

`disconnect_iommu`
:   Optional, disconnect to external iommu

`irq_handler`
:   for CORE to get the HW event from the CHIP when interrupt happened.

`enable_irq`
:   enable irq

`disable_irq`
:   disable irq

`on_off_vblank`
:   notify HW to on/off vblank

`dump_register`
:   Optional, dump registers to seq\_file

`change_opmode`
:   Notify HW to switch to a new display operation mode.

`flush`
:   Notify the HW to flush or kickoff the update

**Description**

Supplied by chip level and returned by the chip entry function xxx\_identify,

struct komeda\_dev

**Definition**:

```
struct komeda_dev {
    struct device *dev;
    u32 __iomem   *reg_base;
    struct komeda_chip_info chip;
    struct komeda_format_caps_table fmt_tbl;
    struct clk *aclk;
    int irq;
    struct mutex lock;
    u32 dpmode;
    int n_pipelines;
    struct komeda_pipeline *pipelines[KOMEDA_MAX_PIPELINES];
    const struct komeda_dev_funcs *funcs;
    void *chip_data;
    struct iommu_domain *iommu;
    struct dentry *debugfs_root;
    u16 err_verbosity;
#define KOMEDA_DEV_PRINT_ERR_EVENTS BIT(0);
#define KOMEDA_DEV_PRINT_WARN_EVENTS BIT(1);
#define KOMEDA_DEV_PRINT_INFO_EVENTS BIT(2);
#define KOMEDA_DEV_PRINT_DUMP_STATE_ON_EVENT BIT(8);
#define KOMEDA_DEV_PRINT_DISABLE_RATELIMIT BIT(12);
};
```

**Members**

`dev`
:   the base device structure

`reg_base`
:   the base address of komeda io space

`chip`
:   the basic chip information

`fmt_tbl`
:   initialized by [`komeda_dev_funcs->init_format_table`](#c.komeda_dev_funcs "komeda_dev_funcs")

`aclk`
:   HW main engine clk

`irq`
:   irq number

`lock`
:   used to protect dpmode

`dpmode`
:   current display mode

`n_pipelines`
:   the number of pipe in **pipelines**

`pipelines`
:   the komeda pipelines

`funcs`
:   chip funcs to access to HW

`chip_data`
:   chip data will be added by [`komeda_dev_funcs.enum_resources()`](#c.komeda_dev_funcs "komeda_dev_funcs") and
    destroyed by [`komeda_dev_funcs.cleanup()`](#c.komeda_dev_funcs "komeda_dev_funcs")

`iommu`
:   iommu domain

`debugfs_root`
:   root directory of komeda debugfs

`err_verbosity`
:   bitmask for how much extra info to print on error

    See KOMEDA\_DEV\_\* macros for details. Low byte contains the debug
    level categories, the high byte contains extra debug options.

**Description**

Pipeline and component are used to describe how to handle the pixel data.
komeda\_device is for describing the whole view of the device, and the
control-abilites of device.

## Format handling

struct komeda\_format\_caps

**Definition**:

```
struct komeda_format_caps {
    u32 hw_id;
    u32 fourcc;
    u32 supported_layer_types;
    u32 supported_rots;
    u32 supported_afbc_layouts;
    u64 supported_afbc_features;
};
```

**Members**

`hw_id`
:   hw format id, hw specific value.

`fourcc`
:   drm fourcc format.

`supported_layer_types`
:   indicate which layer supports this format

`supported_rots`
:   allowed rotations for this format

`supported_afbc_layouts`
:   supported afbc layerout

`supported_afbc_features`
:   supported afbc features

**Description**

komeda\_format\_caps is for describing ARM display specific features and
limitations for a specific format, and format\_caps will be linked into
`komeda_framebuffer` like a extension of [`drm_format_info`](drm-kms.html#c.drm_format_info "drm_format_info").

**NOTE**

one fourcc may has two different format\_caps items for fourcc and
fourcc+modifier

struct komeda\_format\_caps\_table
:   format\_caps mananger

**Definition**:

```
struct komeda_format_caps_table {
    u32 n_formats;
    const struct komeda_format_caps *format_caps;
    bool (*format_mod_supported)(const struct komeda_format_caps *caps, u32 layer_type, u64 modifier, u32 rot);
};
```

**Members**

`n_formats`
:   the size of format\_caps list.

`format_caps`
:   format\_caps list.

`format_mod_supported`
:   Optional. Some HW may have special requirements or
    limitations which can not be described by format\_caps, this func supply HW
    the ability to do the further HW specific check.

struct komeda\_fb
:   Entending drm\_framebuffer with komeda attribute

**Definition**:

```
struct komeda_fb {
    struct drm_framebuffer base;
    const struct komeda_format_caps *format_caps;
    bool is_va;
    u32 aligned_w;
    u32 aligned_h;
    u32 afbc_size;
    u32 offset_payload;
};
```

**Members**

`base`
:   [`drm_framebuffer`](drm-kms.html#c.drm_framebuffer "drm_framebuffer")

`format_caps`
:   extends drm\_format\_info for komeda specific information

`is_va`
:   if smmu is enabled, it will be true

`aligned_w`
:   aligned frame buffer width

`aligned_h`
:   aligned frame buffer height

`afbc_size`
:   minimum size of afbc

`offset_payload`
:   start of afbc body buffer

## Attach komeda\_dev to DRM-KMS

Komeda abstracts resources by pipeline/component, but DRM-KMS uses
crtc/plane/connector. One KMS-obj cannot represent only one single component,
since the requirements of a single KMS object cannot simply be achieved by a
single component, usually that needs multiple components to fit the requirement.
Like set mode, gamma, ctm for KMS all target on CRTC-obj, but komeda needs
compiz, improc and timing\_ctrlr to work together to fit these requirements.
And a KMS-Plane may require multiple komeda resources: layer/scaler/compiz.

So, one KMS-Obj represents a sub-pipeline of komeda resources.

* Plane: [Layer(input) pipeline](#layer-input-pipeline)
* Wb\_connector: [Writeback(output) pipeline](#writeback-output-pipeline)
* Crtc: [Display output pipeline](#display-output-pipeline)

So, for komeda, we treat KMS crtc/plane/connector as users of pipeline and
component, and at any one time a pipeline/component only can be used by one
user. And pipeline/component will be treated as private object of DRM-KMS; the
state will be managed by drm\_atomic\_state as well.

### How to map plane to Layer(input) pipeline

Komeda has multiple Layer input pipelines, see:
- [Single pipeline data flow](#single-pipeline-data-flow)
- [Dual pipeline with Slave enabled](#dual-pipeline-with-slave-enabled)

The easiest way is binding a plane to a fixed Layer pipeline, but consider the
komeda capabilities:

* Layer Split, See [Layer(input) pipeline](#layer-input-pipeline)

  Layer\_Split is quite complicated feature, which splits a big image into two
  parts and handles it by two layers and two scalers individually. But it
  imports an edge problem or effect in the middle of the image after the split.
  To avoid such a problem, it needs a complicated Split calculation and some
  special configurations to the layer and scaler. We’d better hide such HW
  related complexity to user mode.
* Slave pipeline, See [Dual pipeline with Slave enabled](#dual-pipeline-with-slave-enabled)

  Since the compiz component doesn’t output alpha value, the slave pipeline
  only can be used for bottom layers composition. The komeda driver wants to
  hide this limitation to the user. The way to do this is to pick a suitable
  Layer according to plane\_state->zpos.

So for komeda, the KMS-plane doesn’t represent a fixed komeda layer pipeline,
but multiple Layers with same capabilities. Komeda will select one or more
Layers to fit the requirement of one KMS-plane.

### Make component/pipeline to be drm\_private\_obj

Add [`drm_private_obj`](drm-kms.html#c.drm_private_obj "drm_private_obj") to [`komeda_component`](#c.komeda_component "komeda_component"), [`komeda_pipeline`](#c.komeda_pipeline "komeda_pipeline")

```
struct komeda_component {
    struct drm_private_obj obj;
    ...
}

struct komeda_pipeline {
    struct drm_private_obj obj;
    ...
}
```

### Tracking component\_state/pipeline\_state by drm\_atomic\_state

Add [`drm_private_state`](drm-kms.html#c.drm_private_state "drm_private_state") and user to [`komeda_component_state`](#c.komeda_component_state "komeda_component_state"),
[`komeda_pipeline_state`](#c.komeda_pipeline_state "komeda_pipeline_state")

```
struct komeda_component_state {
    struct drm_private_state obj;
    void *binding_user;
    ...
}

struct komeda_pipeline_state {
    struct drm_private_state obj;
    struct drm_crtc *crtc;
    ...
}
```

### komeda component validation

Komeda has multiple types of components, but the process of validation are
similar, usually including the following steps:

```
int komeda_xxxx_validate(struct komeda_component_xxx xxx_comp,
            struct komeda_component_output *input_dflow,
            struct drm_plane/crtc/connector *user,
            struct drm_plane/crtc/connector_state, *user_state)
{
     setup 1: check if component is needed, like the scaler is optional depending
              on the user_state; if unneeded, just return, and the caller will
              put the data flow into next stage.
     Setup 2: check user_state with component features and capabilities to see
              if requirements can be met; if not, return fail.
     Setup 3: get component_state from drm_atomic_state, and try set to set
              user to component; fail if component has been assigned to another
              user already.
     Setup 3: configure the component_state, like set its input component,
              convert user_state to component specific state.
     Setup 4: adjust the input_dflow and prepare it for the next stage.
}
```

### komeda\_kms Abstraction

struct komeda\_plane
:   komeda instance of drm\_plane

**Definition**:

```
struct komeda_plane {
    struct drm_plane base;
    struct komeda_layer *layer;
};
```

**Members**

`base`
:   [`drm_plane`](drm-kms.html#c.drm_plane "drm_plane")

`layer`
:   represents available layer input pipelines for this plane.

    NOTE:
    the layer is not for a specific Layer, but indicate a group of
    Layers with same capabilities.

struct komeda\_plane\_state

**Definition**:

```
struct komeda_plane_state {
    struct drm_plane_state base;
    struct list_head zlist_node;
    u8 layer_split : 1;
};
```

**Members**

`base`
:   [`drm_plane_state`](drm-kms.html#c.drm_plane_state "drm_plane_state")

`zlist_node`
:   zorder list node

`layer_split`
:   on/off layer\_split

**Description**

The plane\_state can be split into two data flow (left/right) and handled
by two layers [`komeda_plane.layer`](#c.komeda_plane "komeda_plane") and [`komeda_plane.layer`](#c.komeda_plane "komeda_plane").right

struct komeda\_wb\_connector

**Definition**:

```
struct komeda_wb_connector {
    struct drm_writeback_connector base;
    struct komeda_layer *wb_layer;
};
```

**Members**

`base`
:   [`drm_writeback_connector`](drm-kms.html#c.drm_writeback_connector "drm_writeback_connector")

`wb_layer`
:   represents associated writeback pipeline of komeda

struct komeda\_crtc

**Definition**:

```
struct komeda_crtc {
    struct drm_crtc base;
    struct komeda_pipeline *master;
    struct komeda_pipeline *slave;
    u32 slave_planes;
    struct komeda_wb_connector *wb_conn;
    struct completion *disable_done;
    struct drm_encoder encoder;
};
```

**Members**

`base`
:   [`drm_crtc`](drm-kms.html#c.drm_crtc "drm_crtc")

`master`
:   only master has display output

`slave`
:   optional

    Doesn’t have its own display output, the handled data flow will
    merge into the master.

`slave_planes`
:   komeda slave planes mask

`wb_conn`
:   komeda write back connector

`disable_done`
:   this flip\_done is for tracing the disable

`encoder`
:   encoder at the end of the pipeline

struct komeda\_crtc\_state

**Definition**:

```
struct komeda_crtc_state {
    struct drm_crtc_state base;
    u32 affected_pipes;
    u32 active_pipes;
    u64 clock_ratio;
    u32 max_slave_zorder;
};
```

**Members**

`base`
:   [`drm_crtc_state`](drm-kms.html#c.drm_crtc_state "drm_crtc_state")

`affected_pipes`
:   the affected pipelines in once display instance

`active_pipes`
:   the active pipelines in once display instance

`clock_ratio`
:   ratio of (aclk << 32)/pxlclk

`max_slave_zorder`
:   the maximum of slave zorder

### komde\_kms Functions

int komeda\_crtc\_atomic\_check(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   build display output data flow

**Parameters**

`struct drm_crtc *crtc`
:   DRM crtc

`struct drm_atomic_state *state`
:   the crtc state object

**Description**

crtc\_atomic\_check is the final check stage, so beside build a display data
pipeline according to the crtc\_state, but still needs to release or disable
the unclaimed pipeline resources.

**Return**

Zero for success or -errno

int komeda\_plane\_atomic\_check(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   build input data flow

**Parameters**

`struct drm_plane *plane`
:   DRM plane

`struct drm_atomic_state *state`
:   the plane state object

**Return**

Zero for success or -errno

## Build komeda to be a Linux module driver

Now we have two level devices:

* komeda\_dev: describes the real display hardware.
* komeda\_kms\_dev: attaches or connects komeda\_dev to DRM-KMS.

All komeda operations are supplied or operated by komeda\_dev or komeda\_kms\_dev,
the module driver is only a simple wrapper to pass the Linux command
(probe/remove/pm) into komeda\_dev or komeda\_kms\_dev.
