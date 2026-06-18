# AMDgpu Display Manager

> 출처(원문): https://docs.kernel.org/gpu/amdgpu/display/display-manager.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [AMDgpu Display Manager](#id1)

The AMDgpu display manager, **amdgpu\_dm** (or even simpler,
**dm**) sits between DRM and DC. It acts as a liaison, converting DRM
requests into DC requests, and DC responses into DRM responses.

The root control structure is [`struct amdgpu_display_manager`](#c.amdgpu_display_manager "amdgpu_display_manager").

struct dm\_compressor\_info
:   Buffer info used by frame buffer compression

**Definition**:

```
struct dm_compressor_info {
    void *cpu_addr;
    struct amdgpu_bo *bo_ptr;
    uint64_t gpu_addr;
};
```

**Members**

`cpu_addr`
:   MMIO cpu addr

`bo_ptr`
:   Pointer to the buffer object

`gpu_addr`
:   MMIO gpu addr

struct dm\_boot\_time\_crc\_info
:   Buffer info used by boot time CRC

**Definition**:

```
struct dm_boot_time_crc_info {
    void *cpu_addr;
    struct amdgpu_bo *bo_ptr;
    uint64_t gpu_addr;
    uint32_t size;
};
```

**Members**

`cpu_addr`
:   MMIO cpu addr

`bo_ptr`
:   Pointer to the buffer object

`gpu_addr`
:   MMIO gpu addr

`size`
:   Size of the buffer

struct dmub\_hpd\_work
:   Handle time consuming work in low priority outbox IRQ

**Definition**:

```
struct dmub_hpd_work {
    struct work_struct handle_hpd_work;
    struct dmub_notification *dmub_notify;
    struct amdgpu_device *adev;
};
```

**Members**

`handle_hpd_work`
:   Work to be executed in a separate thread to handle hpd\_low\_irq

`dmub_notify`
:   notification for callback function

`adev`
:   amdgpu\_device pointer

struct vblank\_control\_work
:   Work data for vblank control

**Definition**:

```
struct vblank_control_work {
    struct work_struct work;
    struct amdgpu_display_manager *dm;
    struct amdgpu_crtc *acrtc;
    struct dc_stream_state *stream;
    bool enable;
};
```

**Members**

`work`
:   Kernel work data for the work event

`dm`
:   amdgpu display manager device

`acrtc`
:   amdgpu CRTC instance for which the event has occurred

`stream`
:   DC stream for which the event has occurred

`enable`
:   true if enabling vblank

struct idle\_workqueue
:   Work data for periodic action in idle

**Definition**:

```
struct idle_workqueue {
    struct work_struct work;
    struct amdgpu_display_manager *dm;
    bool enable;
    bool running;
};
```

**Members**

`work`
:   Kernel work data for the work event

`dm`
:   amdgpu display manager device

`enable`
:   true if idle worker is enabled

`running`
:   true if idle worker is running

struct vupdate\_offload\_work
:   Work data for offloading task from vupdate handler

**Definition**:

```
struct vupdate_offload_work {
    struct work_struct work;
    struct amdgpu_device *adev;
    struct dc_stream_state *stream;
    struct dc_crtc_timing_adjust *adjust;
};
```

**Members**

`work`
:   Kernel work data for the work event

`adev`
:   amdgpu\_device back pointer

`stream`
:   DC stream associated with the crtc

`adjust`
:   DC CRTC timing adjust to be applied to the crtc

struct amdgpu\_dm\_luminance\_data
:   Custom luminance data

**Definition**:

```
struct amdgpu_dm_luminance_data {
    u8 luminance;
    u8 input_signal;
};
```

**Members**

`luminance`
:   Luminance in percent

`input_signal`
:   Input signal in range 0-255

struct amdgpu\_dm\_backlight\_caps
:   Information about backlight

**Definition**:

```
struct amdgpu_dm_backlight_caps {
    union dpcd_sink_ext_caps *ext_caps;
    u32 aux_min_input_signal;
    u32 aux_max_input_signal;
    int min_input_signal;
    int max_input_signal;
    bool caps_valid;
    bool aux_support;
    u32 brightness_mask;
    u8 ac_level;
    u8 dc_level;
    u8 data_points;
    struct amdgpu_dm_luminance_data luminance_data[MAX_LUMINANCE_DATA_POINTS];
};
```

**Members**

`ext_caps`
:   Keep the data `struct with` all the information about the
    display support for HDR.

`aux_min_input_signal`
:   Min brightness value supported by the display

`aux_max_input_signal`
:   Max brightness value supported by the display
    in nits.

`min_input_signal`
:   minimum possible input in range 0-255.

`max_input_signal`
:   maximum possible input in range 0-255.

`caps_valid`
:   true if these values are from the ACPI interface.

`aux_support`
:   Describes if the display supports AUX backlight.

`brightness_mask`
:   After deriving brightness, OR it with this mask.
    Workaround for panels with issues with certain brightness values.

`ac_level`
:   the default brightness if booted on AC

`dc_level`
:   the default brightness if booted on DC

`data_points`
:   the number of custom luminance data points

`luminance_data`
:   custom luminance data

**Description**

Describe the backlight support for ACPI or eDP AUX.

struct dal\_allocation
:   Tracks mapped FB memory for SMU communication

**Definition**:

```
struct dal_allocation {
    struct list_head list;
    struct amdgpu_bo *bo;
    void *cpu_ptr;
    u64 gpu_addr;
};
```

**Members**

`list`
:   list of dal allocations

`bo`
:   GPU buffer object

`cpu_ptr`
:   CPU virtual address of the GPU buffer object

`gpu_addr`
:   GPU virtual address of the GPU buffer object

struct hpd\_rx\_irq\_offload\_work\_queue
:   Work queue to handle hpd\_rx\_irq offload work

**Definition**:

```
struct hpd_rx_irq_offload_work_queue {
    struct workqueue_struct *wq;
    spinlock_t offload_lock;
    bool is_handling_link_loss;
    bool is_handling_mst_msg_rdy_event;
    struct amdgpu_dm_connector *aconnector;
};
```

**Members**

`wq`
:   workqueue structure to queue offload work.

`offload_lock`
:   To protect fields of offload work queue.

`is_handling_link_loss`
:   Used to prevent inserting link loss event when
    we’re handling link loss

`is_handling_mst_msg_rdy_event`
:   Used to prevent inserting mst message
    ready event when we’re already handling mst message ready event

`aconnector`
:   The aconnector that this work queue is attached to

struct hpd\_rx\_irq\_offload\_work
:   hpd\_rx\_irq offload work structure

**Definition**:

```
struct hpd_rx_irq_offload_work {
    struct work_struct work;
    union hpd_irq_data data;
    struct hpd_rx_irq_offload_work_queue *offload_wq;
    struct amdgpu_device *adev;
};
```

**Members**

`work`
:   offload work

`data`
:   reference irq data which is used while handling offload work

`offload_wq`
:   offload work queue that this work is queued to

`adev`
:   amdgpu\_device pointer

struct amdgpu\_display\_manager
:   Central amdgpu display manager device

**Definition**:

```
struct amdgpu_display_manager {
    struct dc *dc;
    struct dmub_srv *dmub_srv;
    struct dmub_notification *dmub_notify;
    dmub_notify_interrupt_callback_t dmub_callback[AMDGPU_DMUB_NOTIFICATION_MAX];
    bool dmub_thread_offload[AMDGPU_DMUB_NOTIFICATION_MAX];
    struct dmub_srv_fb_info *dmub_fb_info;
    const struct firmware *dmub_fw;
    struct amdgpu_bo *dmub_bo;
    u64 dmub_bo_gpu_addr;
    void *dmub_bo_cpu_addr;
    uint32_t dmcub_fw_version;
    uint32_t fw_inst_size;
    struct cgs_device *cgs_device;
    struct amdgpu_device *adev;
    struct drm_device *ddev;
    u16 display_indexes_num;
    struct drm_private_obj atomic_obj;
    struct mutex dc_lock;
    struct mutex audio_lock;
    struct drm_audio_component *audio_component;
    bool audio_registered;
    struct list_head irq_handler_list_low_tab[DAL_IRQ_SOURCES_NUMBER];
    struct list_head irq_handler_list_high_tab[DAL_IRQ_SOURCES_NUMBER];
    struct common_irq_params pflip_params[DC_IRQ_SOURCE_PFLIP_LAST - DC_IRQ_SOURCE_PFLIP_FIRST + 1];
    struct common_irq_params vblank_params[DC_IRQ_SOURCE_VBLANK6 - DC_IRQ_SOURCE_VBLANK1 + 1];
    struct common_irq_params vline0_params[DC_IRQ_SOURCE_DC6_VLINE0 - DC_IRQ_SOURCE_DC1_VLINE0 + 1];
    struct common_irq_params vupdate_params[DC_IRQ_SOURCE_VUPDATE6 - DC_IRQ_SOURCE_VUPDATE1 + 1];
    struct common_irq_params dmub_trace_params[1];
    struct common_irq_params dmub_outbox_params[1];
    spinlock_t irq_handler_list_table_lock;
    struct backlight_device *backlight_dev[AMDGPU_DM_MAX_NUM_EDP];
    const struct dc_link *backlight_link[AMDGPU_DM_MAX_NUM_EDP];
    uint8_t num_of_edps;
    struct amdgpu_dm_backlight_caps backlight_caps[AMDGPU_DM_MAX_NUM_EDP];
    struct mod_freesync *freesync_module;
    struct hdcp_workqueue *hdcp_workqueue;
    struct workqueue_struct *vblank_control_workqueue;
    struct idle_workqueue *idle_workqueue;
    struct drm_atomic_state *cached_state;
    struct dc_state *cached_dc_state;
    struct dm_compressor_info compressor;
    const struct firmware *fw_dmcu;
    uint32_t dmcu_fw_version;
    const struct gpu_info_soc_bounding_box_v1_0 *soc_bounding_box;
    uint32_t active_vblank_irq_count;
#if defined(CONFIG_DRM_AMD_SECURE_DISPLAY);
    struct secure_display_context secure_display_ctx;
#endif;
    struct hpd_rx_irq_offload_work_queue *hpd_rx_offload_wq;
    struct amdgpu_encoder mst_encoders[AMDGPU_DM_MAX_CRTC];
    bool force_timing_sync;
    bool disable_hpd_irq;
    bool dmcub_trace_event_en;
    struct list_head da_list;
    struct completion dmub_aux_transfer_done;
    struct workqueue_struct *delayed_hpd_wq;
    u32 brightness[AMDGPU_DM_MAX_NUM_EDP];
    u32 actual_brightness[AMDGPU_DM_MAX_NUM_EDP];
    bool aux_hpd_discon_quirk;
    bool edp0_on_dp1_quirk;
    struct mutex dpia_aux_lock;
    void *bb_from_dmub;
    struct amdgpu_i2c_adapter *oem_i2c;
    struct fused_io_sync {
        struct completion replied;
        char reply_data[0x40];
    } fused_io[8];
    struct dm_boot_time_crc_info boot_time_crc_info;
};
```

**Members**

`dc`
:   Display Core control structure

`dmub_srv`
:   DMUB service, used for controlling the DMUB on hardware
    that supports it. The pointer to the dmub\_srv will be
    NULL on hardware that does not support it.

`dmub_notify`
:   Notification from DMUB.

`dmub_callback`
:   Callback functions to handle notification from DMUB.

`dmub_thread_offload`
:   Flag to indicate if callback is offload.

`dmub_fb_info`
:   Framebuffer regions for the DMUB.

`dmub_fw`
:   DMUB firmware, required on hardware that has DMUB support.

`dmub_bo`
:   Buffer object for the DMUB.

`dmub_bo_gpu_addr`
:   GPU virtual address for the DMUB buffer object.

`dmub_bo_cpu_addr`
:   CPU address for the DMUB buffer object.

`dmcub_fw_version`
:   DMCUB firmware version.

`fw_inst_size`
:   Size of the firmware instruction buffer.

`cgs_device`
:   The Common Graphics Services device. It provides an interface for
    accessing registers.

`adev`
:   AMDGPU base driver structure

`ddev`
:   DRM base driver structure

`display_indexes_num`
:   Max number of display streams supported

`atomic_obj`
:   In combination with `dm_atomic_state` it helps manage
    global atomic state that doesn’t map cleanly into existing
    drm resources, like `dc_context`.

`dc_lock`
:   Guards access to DC functions that can issue register write
    sequences.

`audio_lock`
:   Guards access to audio instance changes.

`audio_component`
:   Used to notify ELD changes to sound driver.

`audio_registered`
:   True if the audio component has been registered
    successfully, false otherwise.

`irq_handler_list_low_tab`
:   Low priority IRQ handler table.

    It is a n\*m table consisting of n IRQ sources, and m handlers per IRQ
    source. Low priority IRQ handlers are deferred to a workqueue to be
    processed. Hence, they can sleep.

    Note that handlers are called in the same order as they were
    registered (FIFO).

`irq_handler_list_high_tab`
:   High priority IRQ handler table.

    It is a n\*m table, same as `irq_handler_list_low_tab`. However,
    handlers in this table are not deferred and are called immediately.

`pflip_params`
:   Page flip IRQ parameters, passed to registered handlers when
    triggered.

`vblank_params`
:   Vertical blanking IRQ parameters, passed to registered handlers when
    triggered.

`vline0_params`
:   OTG vertical interrupt0 IRQ parameters, passed to registered
    handlers when triggered.

`vupdate_params`
:   Vertical update IRQ parameters, passed to registered handlers when
    triggered.

`dmub_trace_params`
:   DMUB trace event IRQ parameters, passed to registered handlers when
    triggered.

`dmub_outbox_params`
:   DMUB Outbox parameters

`irq_handler_list_table_lock`
:   Synchronizes access to IRQ tables

`backlight_dev`
:   Backlight control device

`backlight_link`
:   Link on which to control backlight

`num_of_edps`
:   number of backlight eDPs

`backlight_caps`
:   Capabilities of the backlight device

`freesync_module`
:   Module handling freesync calculations

`hdcp_workqueue`
:   AMDGPU content protection queue

`vblank_control_workqueue`
:   Deferred work for vblank control events.

`idle_workqueue`
:   Periodic work for idle events.

`cached_state`
:   Caches device atomic state for suspend/resume

`cached_dc_state`
:   Cached state of content streams

`compressor`
:   Frame buffer compression buffer. See [`struct dm_compressor_info`](#c.dm_compressor_info "dm_compressor_info")

`fw_dmcu`
:   Reference to DMCU firmware

`dmcu_fw_version`
:   Version of the DMCU firmware

`soc_bounding_box`
:   gpu\_info FW provided soc bounding box `struct or` 0 if not
    available in FW

`active_vblank_irq_count`
:   number of currently active vblank irqs

`secure_display_ctx`
:   Store secure display relevant info. e.g. the ROI information
    , the work\_struct to command dmub, etc.

`hpd_rx_offload_wq`
:   Work queue to offload works of hpd\_rx\_irq

`mst_encoders`
:   fake encoders used for DP MST.

`force_timing_sync`
:   set via debugfs. When set, indicates that all connected
    displays will be forced to synchronize.

`disable_hpd_irq`
:   disables all HPD and HPD RX interrupt handling in the
    driver when true

`dmcub_trace_event_en`
:   enable dmcub trace events

`da_list`
:   DAL fb memory allocation list, for communication with SMU.

`dmub_aux_transfer_done`
:   `struct completion` used to indicate when DMUB
    transfers are done

`delayed_hpd_wq`
:   work queue used to delay DMUB HPD work

`brightness`
:   cached backlight values.

`actual_brightness`
:   last successfully applied backlight values.

`aux_hpd_discon_quirk`
:   quirk for hpd discon while aux is on-going.
    occurred on certain intel platform

`edp0_on_dp1_quirk`
:   quirk for platforms that put edp0 on DP1.

`dpia_aux_lock`
:   Guards access to DPIA AUX

`bb_from_dmub`
:   Bounding box data read from dmub during early initialization for DCN4+
    Data is stored as a byte array that should be casted to the appropriate bb struct

`oem_i2c`
:   OEM i2c bus

`fused_io`
:   dmub fused io interface

struct amdgpu\_hdmi\_vsdb\_info
:   Keep track of the VSDB info

**Definition**:

```
struct amdgpu_hdmi_vsdb_info {
    unsigned int amd_vsdb_version;
    bool freesync_supported;
    unsigned int min_refresh_rate_hz;
    unsigned int max_refresh_rate_hz;
    unsigned int freesync_mccs_vcp_code;
    bool replay_mode;
};
```

**Members**

`amd_vsdb_version`
:   Vendor Specific Data Block Version, should be
    used to determine which Vendor Specific InfoFrame (VSIF) to send.

`freesync_supported`
:   FreeSync Supported.

`min_refresh_rate_hz`
:   FreeSync Minimum Refresh Rate in Hz.

`max_refresh_rate_hz`
:   FreeSync Maximum Refresh Rate in Hz

`freesync_mccs_vcp_code`
:   MCCS VCP code for freesync state

`replay_mode`
:   Replay supported

**Description**

AMDGPU supports FreeSync over HDMI by using the VSDB section, and this
`struct is` useful to keep track of the display-specific information about
FreeSync.

## [Lifecycle](#id2)

DM (and consequently DC) is registered in the amdgpu base driver as a IP
block. When CONFIG\_DRM\_AMD\_DC is enabled, the DM device IP block is added to
the base driver’s device list to be initialized and torn down accordingly.

The functions to do so are provided as hooks in [`struct amd_ip_funcs`](../driver-core.html#c.amd_ip_funcs "amd_ip_funcs").

int dm\_hw\_init(struct amdgpu\_ip\_block \*ip\_block)
:   Initialize DC device

**Parameters**

`struct amdgpu_ip_block *ip_block`
:   Pointer to the amdgpu\_ip\_block for this hw instance.

**Description**

Initialize the [`struct amdgpu_display_manager`](#c.amdgpu_display_manager "amdgpu_display_manager") device. This involves calling
the initializers of each DM component, then populating the `struct with` them.

Although the function implies hardware initialization, both hardware and
software are initialized here. Splitting them out to their relevant init
hooks is a future TODO item.

Some notable things that are initialized here:

* Display Core, both software and hardware
* DC modules that we need (freesync and color management)
* DRM software states
* Interrupt sources and handlers
* Vblank support
* Debug FS entries, if enabled

int dm\_hw\_fini(struct amdgpu\_ip\_block \*ip\_block)
:   Teardown DC device

**Parameters**

`struct amdgpu_ip_block *ip_block`
:   Pointer to the amdgpu\_ip\_block for this hw instance.

**Description**

Teardown components within [`struct amdgpu_display_manager`](#c.amdgpu_display_manager "amdgpu_display_manager") that require
cleanup. This involves cleaning up the DRM device, DC, and any modules that
were loaded. Also flush IRQ workqueues and disable them.

## [Interrupts](#id3)

DM provides another layer of IRQ management on top of what the base driver
already provides. This is something that could be cleaned up, and is a
future TODO item.

The base driver provides IRQ source registration with DRM, handler
registration into the base driver’s IRQ table, and a handler callback
[`amdgpu_irq_handler()`](../driver-core.html#c.amdgpu_irq_handler "amdgpu_irq_handler"), with which DRM calls on interrupts. This generic
handler looks up the IRQ table, and calls the respective
`amdgpu_irq_src_funcs.process` hookups.

What DM provides on top are two IRQ tables specifically for top-half and
bottom-half IRQ handling, with the bottom-half implementing workqueues:

* [`amdgpu_display_manager.irq_handler_list_high_tab`](#c.amdgpu_display_manager "amdgpu_display_manager")
* [`amdgpu_display_manager.irq_handler_list_low_tab`](#c.amdgpu_display_manager "amdgpu_display_manager")

They override the base driver’s IRQ table, and the effect can be seen
in the hooks that DM provides for `amdgpu_irq_src_funcs.process`. They
are all set to the DM generic handler [`amdgpu_dm_irq_handler()`](#c.amdgpu_dm_irq_handler "amdgpu_dm_irq_handler"), which looks up
DM’s IRQ tables. However, in order for base driver to recognize this hook, DM
still needs to register the IRQ with the base driver. See
`dce110_register_irq_handlers()` and `dcn10_register_irq_handlers()`.

To expose DC’s hardware interrupt toggle to the base driver, DM implements
`amdgpu_irq_src_funcs.set` hooks. Base driver calls it through
[`amdgpu_irq_update()`](../driver-core.html#c.amdgpu_irq_update "amdgpu_irq_update") to enable or disable the interrupt.

struct amdgpu\_dm\_irq\_handler\_data
:   Data for DM interrupt handlers.

**Definition**:

```
struct amdgpu_dm_irq_handler_data {
    struct list_head list;
    interrupt_handler handler;
    void *handler_arg;
    struct amdgpu_display_manager *dm;
    enum dc_irq_source irq_source;
    struct work_struct work;
};
```

**Members**

`list`
:   Linked list entry referencing the next/previous handler

`handler`
:   Handler function

`handler_arg`
:   Argument passed to the handler when triggered

`dm`
:   DM which this handler belongs to

`irq_source`
:   DC interrupt source that this handler is registered for

`work`
:   work struct

void dm\_irq\_work\_func(struct work\_struct \*work)
:   Handle an IRQ outside of the interrupt handler proper.

**Parameters**

`struct work_struct *work`
:   work struct

void unregister\_all\_irq\_handlers(struct amdgpu\_device \*adev)
:   Cleans up handlers from the DM IRQ table

**Parameters**

`struct amdgpu_device *adev`
:   The base driver device containing the DM device

**Description**

Go through low and high context IRQ tables and deallocate handlers.

void \*amdgpu\_dm\_irq\_register\_interrupt(struct amdgpu\_device \*adev, struct dc\_interrupt\_params \*int\_params, void (\*ih)(void\*), void \*handler\_args)
:   Register a handler within DM.

**Parameters**

`struct amdgpu_device *adev`
:   The base driver device containing the DM device.

`struct dc_interrupt_params *int_params`
:   Interrupt parameters containing the source, and handler context

`void (*ih)(void *)`
:   Function pointer to the interrupt handler to register

`void *handler_args`
:   Arguments passed to the handler when the interrupt occurs

**Description**

Register an interrupt handler for the given IRQ source, under the given
context. The context can either be high or low. High context handlers are
executed directly within ISR context, while low context is executed within a
workqueue, thereby allowing operations that sleep.

Registered handlers are called in a FIFO manner, i.e. the most recently
registered handler will be called first.

**Return**

Handler data [`struct amdgpu_dm_irq_handler_data`](#c.amdgpu_dm_irq_handler_data "amdgpu_dm_irq_handler_data") containing the IRQ
source, handler function, and args

void amdgpu\_dm\_irq\_unregister\_interrupt(struct amdgpu\_device \*adev, enum dc\_irq\_source irq\_source, void \*ih)
:   Remove a handler from the DM IRQ table

**Parameters**

`struct amdgpu_device *adev`
:   The base driver device containing the DM device

`enum dc_irq_source irq_source`
:   IRQ source to remove the given handler from

`void *ih`
:   Function pointer to the interrupt handler to unregister

**Description**

Go through both low and high context IRQ tables, and find the given handler
for the given irq source. If found, remove it. Otherwise, do nothing.

int amdgpu\_dm\_irq\_init(struct amdgpu\_device \*adev)
:   Initialize DM IRQ management

**Parameters**

`struct amdgpu_device *adev`
:   The base driver device containing the DM device

**Description**

Initialize DM’s high and low context IRQ tables.

The N by M table contains N IRQ sources, with M
[`struct amdgpu_dm_irq_handler_data`](#c.amdgpu_dm_irq_handler_data "amdgpu_dm_irq_handler_data") hooked together in a linked list. The
list\_heads are initialized here. When an interrupt n is triggered, all m
handlers are called in sequence, FIFO according to registration order.

The low context table requires special steps to initialize, since handlers
will be deferred to a workqueue. See `struct irq_list_head`.

void amdgpu\_dm\_irq\_fini(struct amdgpu\_device \*adev)
:   Tear down DM IRQ management

**Parameters**

`struct amdgpu_device *adev`
:   The base driver device containing the DM device

**Description**

Flush all work within the low context IRQ table.

int amdgpu\_dm\_irq\_handler(struct amdgpu\_device \*adev, struct amdgpu\_irq\_src \*source, struct amdgpu\_iv\_entry \*entry)
:   Generic DM IRQ handler

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu base driver device containing the DM device

`struct amdgpu_irq_src *source`
:   Unused

`struct amdgpu_iv_entry *entry`
:   Data about the triggered interrupt

**Description**

Calls all registered high irq work immediately, and schedules work for low
irq. The DM IRQ table is used to find the corresponding handlers.

void amdgpu\_dm\_hpd\_init(struct amdgpu\_device \*adev)
:   hpd setup callback.

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

**Description**

Setup the hpd pins used by the card (evergreen+).
Enable the pin, set the polarity, and enable the hpd interrupts.

void amdgpu\_dm\_hpd\_fini(struct amdgpu\_device \*adev)
:   hpd tear down callback.

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu\_device pointer

**Description**

Tear down the hpd pins used by the card (evergreen+).
Disable the hpd interrupts.

void dm\_pflip\_high\_irq(void \*interrupt\_params)
:   Handle pageflip interrupt

**Parameters**

`void *interrupt_params`
:   ignored

**Description**

Handles the pageflip interrupt by notifying all interested parties
that the pageflip has been completed.

void dm\_crtc\_high\_irq(void \*interrupt\_params)
:   Handles CRTC interrupt

**Parameters**

`void *interrupt_params`
:   used for determining the CRTC instance

**Description**

Handles the CRTC/VSYNC interrupt by notfying DRM’s VBLANK
event handler.

## [Atomic Implementation](#id4)

*WIP*

void amdgpu\_dm\_atomic\_commit\_tail(struct [drm\_atomic\_state](../../drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   AMDgpu DM’s commit tail implementation.

**Parameters**

`struct drm_atomic_state *state`
:   The atomic state to commit

**Description**

This will tell DC to commit the constructed DC state from atomic\_check,
programming the hardware. Any failures here implies a hardware failure, since
atomic check should have filtered anything non-kosher.

int amdgpu\_dm\_atomic\_check(struct [drm\_device](../../drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](../../drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   Atomic check implementation for AMDgpu DM.

**Parameters**

`struct drm_device *dev`
:   The DRM device

`struct drm_atomic_state *state`
:   The atomic state to commit

**Description**

Validate that the given atomic state is programmable by DC into hardware.
This involves constructing a `struct dc_state` reflecting the new hardware
state we wish to commit, then querying DC to see if it is programmable. It’s
important not to modify the existing DC state. Otherwise, atomic\_check
may unexpectedly commit hardware changes.

When validating the DC state, it’s important that the right locks are
acquired. For full updates case which removes/adds/updates streams on one
CRTC while flipping on another CRTC, acquiring global lock will guarantee
that any such full update commit will wait for completion of any outstanding
flip using DRMs synchronization events.

Note that DM adds the affected connectors for all CRTCs in state, when that
might not seem necessary. This is because DC stream creation requires the
DC sink, which is tied to the DRM connector state. Cleaning this up should
be possible but non-trivial - a possible TODO item.

**Return**

-Error code if validation failed.

## [Color Management Properties](#id5)

We have three types of color management in the AMD display driver.
1. the legacy [`drm_crtc`](../../drm-kms.html#c.drm_crtc "drm_crtc") DEGAMMA, CTM, and GAMMA properties
2. AMD driver private color management on [`drm_plane`](../../drm-kms.html#c.drm_plane "drm_plane") and [`drm_crtc`](../../drm-kms.html#c.drm_crtc "drm_crtc")
3. AMD plane color pipeline

The CRTC properties are the original color management. When they were
implemented per-plane color management was not a thing yet. Because
of that we could get away with plumbing the DEGAMMA and CTM
properties to pre-blending HW functions. This is incompatible with
per-plane color management, such as via the AMD private properties or
the new drm\_plane color pipeline. The only compatible CRTC property
with per-plane color management is the GAMMA property as it is
applied post-blending.

The AMD driver private color management properties are only exposed
when the kernel is built explicitly with -DAMD\_PRIVATE\_COLOR. They
are temporary building blocks on the path to full-fledged [`drm_plane`](../../drm-kms.html#c.drm_plane "drm_plane")
and [`drm_crtc`](../../drm-kms.html#c.drm_crtc "drm_crtc") color pipelines and lay the driver’s groundwork for the
color pipelines.

The AMD plane color pipeline describes AMD’s `drm_colorops` via the
[`drm_plane`](../../drm-kms.html#c.drm_plane "drm_plane")’s COLOR\_PIPELINE property.

### [drm\_crtc Properties](#id6)

The DC interface to HW gives us the following color management blocks
per pipe (surface):

* Input gamma LUT (de-normalized)
* Input CSC (normalized)
* Surface degamma LUT (normalized)
* Surface CSC (normalized)
* Surface regamma LUT (normalized)
* Output CSC (normalized)

But these aren’t a direct mapping to DRM color properties. The
current DRM interface exposes CRTC degamma, CRTC CTM and CRTC regamma
while our hardware is essentially giving:

Plane CTM -> Plane degamma -> Plane CTM -> Plane regamma -> Plane CTM

The input gamma LUT block isn’t really applicable here since it
operates on the actual input data itself rather than the HW fp
representation. The input and output CSC blocks are technically
available to use as part of the DC interface but are typically used
internally by DC for conversions between color spaces. These could be
blended together with user adjustments in the future but for now
these should remain untouched.

The pipe blending also happens after these blocks so we don’t
actually support any CRTC props with correct blending with multiple
planes - but we can still support CRTC color management properties in
DM in most single plane cases correctly with clever management of the
DC interface in DM.

As per DRM documentation, blocks should be in hardware bypass when
their respective property is set to NULL. A linear DGM/RGM LUT should
also considered as putting the respective block into bypass mode.

This means that the following configuration is assumed to be the
default:

Plane DGM Bypass -> Plane CTM Bypass -> Plane RGM Bypass -> ... CRTC
DGM Bypass -> CRTC CTM Bypass -> CRTC RGM Bypass

### [AMD Private Color Management on drm\_plane](#id7)

The AMD private color management properties on a [`drm_plane`](../../drm-kms.html#c.drm_plane "drm_plane") are:

* AMD\_PLANE\_DEGAMMA\_LUT
* AMD\_PLANE\_DEGAMMA\_LUT\_SIZE
* AMD\_PLANE\_DEGAMMA\_TF
* AMD\_PLANE\_HDR\_MULT
* AMD\_PLANE\_CTM
* AMD\_PLANE\_SHAPER\_LUT
* AMD\_PLANE\_SHAPER\_LUT\_SIZE
* AMD\_PLANE\_SHAPER\_TF
* AMD\_PLANE\_LUT3D
* AMD\_PLANE\_LUT3D\_SIZE
* AMD\_PLANE\_BLEND\_LUT
* AMD\_PLANE\_BLEND\_LUT\_SIZE
* AMD\_PLANE\_BLEND\_TF

The AMD private color management property on a [`drm_crtc`](../../drm-kms.html#c.drm_crtc "drm_crtc") is:

* AMD\_CRTC\_REGAMMA\_TF

Use of these properties is discouraged.

### [AMD plane color pipeline](#id8)

The AMD [`drm_plane`](../../drm-kms.html#c.drm_plane "drm_plane") color pipeline is advertised for DCN generations
3.0 and newer. It exposes these elements in this order:

1. 1D curve colorop
2. Multiplier
3. 3x4 CTM
4. 1D curve colorop
5. 1D LUT
6. 3D LUT
7. 1D curve colorop
8. 1D LUT

The multiplier (#2) is a simple multiplier that is applied to all
channels.

The 3x4 CTM (#3) is a simple 3x4 matrix.

#1, and #7 are non-linear to linear curves. #4 is a linear to
non-linear curve. They support sRGB, PQ, and BT.709/BT.2020 EOTFs or
their inverse.

The 1D LUTs (#5 and #8) are plain 4096 entry LUTs.

The 3DLUT (#6) is a tetrahedrally interpolated 17 cube LUT.

void amdgpu\_dm\_init\_color\_mod(void)
:   Initialize the color module.

**Parameters**

`void`
:   no arguments

**Description**

We’re not using the full color module, only certain components.
Only call setup functions for components that we need.

const struct drm\_color\_lut \*\_\_extract\_blob\_lut(const struct [drm\_property\_blob](../../drm-kms.html#c.drm_property_blob "drm_property_blob") \*blob, uint32\_t \*size)
:   Extracts the DRM lut and lut size from a blob.

**Parameters**

`const struct drm_property_blob *blob`
:   DRM color mgmt property blob

`uint32_t *size`
:   lut size

**Return**

DRM LUT or NULL

const struct drm\_color\_lut32 \*\_\_extract\_blob\_lut32(const struct [drm\_property\_blob](../../drm-kms.html#c.drm_property_blob "drm_property_blob") \*blob, uint32\_t \*size)
:   Extracts the DRM lut and lut size from a blob.

**Parameters**

`const struct drm_property_blob *blob`
:   DRM color mgmt property blob

`uint32_t *size`
:   lut size

**Return**

DRM LUT or NULL

bool \_\_is\_lut\_linear(const struct drm\_color\_lut \*lut, uint32\_t size)
:   check if the given lut is a linear mapping of values

**Parameters**

`const struct drm_color_lut *lut`
:   given lut to check values

`uint32_t size`
:   lut size

**Description**

It is considered linear if the lut represents:
f(a) = (0xFF00/MAX\_COLOR\_LUT\_ENTRIES-1)a; for integer a in [0,
MAX\_COLOR\_LUT\_ENTRIES)

**Return**

True if the given lut is a linear mapping of values, i.e. it acts like a
bypass LUT. Otherwise, false.

void \_\_drm\_lut\_to\_dc\_gamma(const struct drm\_color\_lut \*lut, struct dc\_gamma \*gamma, bool is\_legacy)
:   convert the drm\_color\_lut to dc\_gamma.

**Parameters**

`const struct drm_color_lut *lut`
:   DRM lookup table for color conversion

`struct dc_gamma *gamma`
:   DC gamma to set entries

`bool is_legacy`
:   legacy or atomic gamma

**Description**

The conversion depends on the size of the lut - whether or not it’s legacy.

void \_\_drm\_lut32\_to\_dc\_gamma(const struct drm\_color\_lut32 \*lut, struct dc\_gamma \*gamma)
:   convert the drm\_color\_lut to dc\_gamma.

**Parameters**

`const struct drm_color_lut32 *lut`
:   DRM lookup table for color conversion

`struct dc_gamma *gamma`
:   DC gamma to set entries

**Description**

The conversion depends on the size of the lut - whether or not it’s legacy.

void \_\_drm\_ctm\_to\_dc\_matrix(const struct drm\_color\_ctm \*ctm, struct fixed31\_32 \*matrix)
:   converts a DRM CTM to a DC CSC float matrix

**Parameters**

`const struct drm_color_ctm *ctm`
:   DRM color transformation matrix

`struct fixed31_32 *matrix`
:   DC CSC float matrix

**Description**

The matrix needs to be a 3x4 (12 entry) matrix.

void \_\_drm\_ctm\_3x4\_to\_dc\_matrix(const struct drm\_color\_ctm\_3x4 \*ctm, struct fixed31\_32 \*matrix)
:   converts a DRM CTM 3x4 to a DC CSC float matrix

**Parameters**

`const struct drm_color_ctm_3x4 *ctm`
:   DRM color transformation matrix with 3x4 dimensions

`struct fixed31_32 *matrix`
:   DC CSC float matrix

**Description**

The matrix needs to be a 3x4 (12 entry) matrix.

int \_\_set\_legacy\_tf(struct dc\_transfer\_func \*func, const struct drm\_color\_lut \*lut, uint32\_t lut\_size, bool has\_rom)
:   Calculates the legacy transfer function

**Parameters**

`struct dc_transfer_func *func`
:   transfer function

`const struct drm_color_lut *lut`
:   lookup table that defines the color space

`uint32_t lut_size`
:   size of respective lut

`bool has_rom`
:   if ROM can be used for hardcoded curve

**Description**

Only for sRGB input space

**Return**

0 in case of success, -ENOMEM if fails

int \_\_set\_output\_tf(struct dc\_transfer\_func \*func, const struct drm\_color\_lut \*lut, uint32\_t lut\_size, bool has\_rom)
:   calculates the output transfer function based on expected input space.

**Parameters**

`struct dc_transfer_func *func`
:   transfer function

`const struct drm_color_lut *lut`
:   lookup table that defines the color space

`uint32_t lut_size`
:   size of respective lut

`bool has_rom`
:   if ROM can be used for hardcoded curve

**Return**

0 in case of success. -ENOMEM if fails.

int \_\_set\_output\_tf\_32(struct dc\_transfer\_func \*func, const struct drm\_color\_lut32 \*lut, uint32\_t lut\_size, bool has\_rom)
:   calculates the output transfer function based on expected input space.

**Parameters**

`struct dc_transfer_func *func`
:   transfer function

`const struct drm_color_lut32 *lut`
:   lookup table that defines the color space

`uint32_t lut_size`
:   size of respective lut

`bool has_rom`
:   if ROM can be used for hardcoded curve

**Return**

0 in case of success. -ENOMEM if fails.

int \_\_set\_input\_tf(struct [dc\_color\_caps](#c.dc_color_caps "dc_color_caps") \*caps, struct dc\_transfer\_func \*func, const struct drm\_color\_lut \*lut, uint32\_t lut\_size)
:   calculates the input transfer function based on expected input space.

**Parameters**

`struct dc_color_caps *caps`
:   dc color capabilities

`struct dc_transfer_func *func`
:   transfer function

`const struct drm_color_lut *lut`
:   lookup table that defines the color space

`uint32_t lut_size`
:   size of respective lut.

**Return**

0 in case of success. -ENOMEM if fails.

int \_\_set\_input\_tf\_32(struct [dc\_color\_caps](#c.dc_color_caps "dc_color_caps") \*caps, struct dc\_transfer\_func \*func, const struct drm\_color\_lut32 \*lut, uint32\_t lut\_size)
:   calculates the input transfer function based on expected input space.

**Parameters**

`struct dc_color_caps *caps`
:   dc color capabilities

`struct dc_transfer_func *func`
:   transfer function

`const struct drm_color_lut32 *lut`
:   lookup table that defines the color space

`uint32_t lut_size`
:   size of respective lut.

**Return**

0 in case of success. -ENOMEM if fails.

int amdgpu\_dm\_verify\_lut3d\_size(struct amdgpu\_device \*adev, struct [drm\_plane\_state](../../drm-kms.html#c.drm_plane_state "drm_plane_state") \*plane\_state)
:   verifies if 3D LUT is supported and if user shaper and 3D LUTs match the hw supported size

**Parameters**

`struct amdgpu_device *adev`
:   amdgpu device

`struct drm_plane_state *plane_state`
:   the DRM plane state

**Description**

Verifies if pre-blending (DPP) 3D LUT is supported by the HW (DCN 2.0 or
newer) and if the user shaper and 3D LUTs match the supported size.

**Return**

0 on success. -EINVAL if lut size are invalid.

int amdgpu\_dm\_verify\_lut\_sizes(const struct [drm\_crtc\_state](../../drm-kms.html#c.drm_crtc_state "drm_crtc_state") \*crtc\_state)
:   verifies if DRM luts match the hw supported sizes

**Parameters**

`const struct drm_crtc_state *crtc_state`
:   the DRM CRTC state

**Description**

Verifies that the Degamma and Gamma LUTs attached to the `crtc_state`
are of the expected size.

**Return**

0 on success. -EINVAL if any lut sizes are invalid.

int amdgpu\_dm\_check\_crtc\_color\_mgmt(struct dm\_crtc\_state \*crtc, bool check\_only)
:   Check if DRM color props are programmable by DC.

**Parameters**

`struct dm_crtc_state *crtc`
:   amdgpu\_dm crtc state

`bool check_only`
:   only check color state without update dc stream

**Description**

This function just verifies CRTC LUT sizes, if there is enough space for
output transfer function and if its parameters can be calculated by AMD
color module. It also adjusts some settings for programming CRTC degamma at
plane stage, using plane DGM block.

The RGM block is typically more fully featured and accurate across
all ASICs - DCE can’t support a custom non-linear CRTC DGM.

For supporting both plane level color management and CRTC level color
management at once we have to either restrict the usage of some CRTC
properties or blend adjustments together.

**Return**

0 on success. Error code if validation fails.

int amdgpu\_dm\_update\_crtc\_color\_mgmt(struct dm\_crtc\_state \*crtc)
:   Maps DRM color management to DC stream.

**Parameters**

`struct dm_crtc_state *crtc`
:   amdgpu\_dm crtc state

**Description**

With no plane level color management properties we’re free to use any
of the HW blocks as long as the CRTC CTM always comes before the
CRTC RGM and after the CRTC DGM.

* The CRTC RGM block will be placed in the RGM LUT block if it is non-linear.
* The CRTC DGM block will be placed in the DGM LUT block if it is non-linear.
* The CRTC CTM will be placed in the gamut remap block if it is non-linear.

The RGM block is typically more fully featured and accurate across
all ASICs - DCE can’t support a custom non-linear CRTC DGM.

For supporting both plane level color management and CRTC level color
management at once we have to either restrict the usage of CRTC properties
or blend adjustments together.

**Return**

0 on success. Error code if setup fails.

int amdgpu\_dm\_update\_plane\_color\_mgmt(struct dm\_crtc\_state \*crtc, struct [drm\_plane\_state](../../drm-kms.html#c.drm_plane_state "drm_plane_state") \*plane\_state, struct [dc\_plane\_state](#c.amdgpu_dm_update_plane_color_mgmt "dc_plane_state") \*dc\_plane\_state)
:   Maps DRM color management to DC plane.

**Parameters**

`struct dm_crtc_state *crtc`
:   amdgpu\_dm crtc state

`struct drm_plane_state *plane_state`
:   DRM plane state

`struct dc_plane_state *dc_plane_state`
:   target DC surface

**Description**

Update the underlying dc\_stream\_state’s input transfer function (ITF) in
preparation for hardware commit. The transfer function used depends on
the preparation done on the stream for color management.

**Return**

0 on success. -ENOMEM if mem allocation fails.

### [DC Color Capabilities between DCN generations](#id9)

DRM/KMS framework defines three CRTC color correction properties: degamma,
color transformation matrix (CTM) and gamma, and two properties for degamma and
gamma LUT sizes. AMD DC programs some of the color correction features
pre-blending but DRM/KMS has not per-plane color correction properties.

In general, the DRM CRTC color properties are programmed to DC, as follows:
CRTC gamma after blending, and CRTC degamma pre-blending. Although CTM is
programmed after blending, it is mapped to DPP hw blocks (pre-blending). Other
color caps available in the hw is not currently exposed by DRM interface and
are bypassed.

**Color management caps (DPP and MPC)**

Modules/color calculates various color operations which are translated to
abstracted HW. DCE 5-12 had almost no important changes, but starting with
DCN1, every new generation comes with fairly major differences in color
pipeline. Therefore, we abstract color pipe capabilities so modules/DM can
decide mapping to HW block based on logical capabilities.

MAX\_SURFACES

`MAX_SURFACES`

> representative of the upper bound of surfaces that can be piped to a single CRTC

MAX\_PLANES

`MAX_PLANES`

> representative of the upper bound of planes that are supported by the HW

struct rom\_curve\_caps
:   predefined transfer function caps for degamma and regamma

**Definition**:

```
struct rom_curve_caps {
    uint16_t srgb : 1;
    uint16_t bt2020 : 1;
    uint16_t gamma2_2 : 1;
    uint16_t pq : 1;
    uint16_t hlg : 1;
};
```

**Members**

`srgb`
:   RGB color space transfer func

`bt2020`
:   BT.2020 transfer func

`gamma2_2`
:   standard gamma

`pq`
:   perceptual quantizer transfer function

`hlg`
:   hybrid log–gamma transfer function

struct dpp\_color\_caps
:   color pipeline capabilities for display pipe and plane blocks

**Definition**:

```
struct dpp_color_caps {
    uint16_t dcn_arch : 1;
    uint16_t input_lut_shared : 1;
    uint16_t icsc : 1;
    uint16_t dgam_ram : 1;
    uint16_t post_csc : 1;
    uint16_t gamma_corr : 1;
    uint16_t hw_3d_lut : 1;
    uint16_t ogam_ram : 1;
    uint16_t ocsc : 1;
    uint16_t dgam_rom_for_yuv : 1;
    struct rom_curve_caps dgam_rom_caps;
    struct rom_curve_caps ogam_rom_caps;
};
```

**Members**

`dcn_arch`
:   all DCE generations treated the same

`input_lut_shared`
:   shared with DGAM. Input LUT is different than most LUTs,
    just plain 256-entry lookup

`icsc`
:   input color space conversion

`dgam_ram`
:   programmable degamma LUT

`post_csc`
:   post color space conversion, before gamut remap

`gamma_corr`
:   degamma correction

`hw_3d_lut`
:   3D LUT support. It implies a shaper LUT before. It may be shared
    with MPC by setting mpc:shared\_3d\_lut flag

`ogam_ram`
:   programmable out/blend gamma LUT

`ocsc`
:   output color space conversion

`dgam_rom_for_yuv`
:   pre-defined degamma LUT for YUV planes

`dgam_rom_caps`
:   pre-definied curve caps for degamma 1D LUT

`ogam_rom_caps`
:   pre-definied curve caps for regamma 1D LUT

**Note**

hdr\_mult and gamut remap (CTM) are always available in DPP (in that order)

struct mpc\_color\_caps
:   color pipeline capabilities for multiple pipe and plane combined blocks

**Definition**:

```
struct mpc_color_caps {
    uint16_t gamut_remap : 1;
    uint16_t ogam_ram : 1;
    uint16_t ocsc : 1;
    uint16_t num_3dluts : 3;
    uint16_t num_rmcm_3dluts : 3;
    uint16_t shared_3d_lut:1;
    struct rom_curve_caps ogam_rom_caps;
    struct lut3d_caps mcm_3d_lut_caps;
    struct lut3d_caps rmcm_3d_lut_caps;
    bool preblend;
};
```

**Members**

`gamut_remap`
:   color transformation matrix

`ogam_ram`
:   programmable out gamma LUT

`ocsc`
:   output color space conversion matrix

`num_3dluts`
:   MPC 3D LUT; always assumes a preceding shaper LUT

`num_rmcm_3dluts`
:   number of RMCM 3D LUTS; always assumes a preceding shaper LUT

`shared_3d_lut`
:   shared 3D LUT flag. Can be either DPP or MPC, but single
    instance

`ogam_rom_caps`
:   pre-definied curve caps for regamma 1D LUT

`mcm_3d_lut_caps`
:   HW support cap for MCM LUT memory

`rmcm_3d_lut_caps`
:   HW support cap for RMCM LUT memory

`preblend`
:   whether color manager supports preblend with MPC

struct dc\_color\_caps
:   color pipes capabilities for DPP and MPC hw blocks

**Definition**:

```
struct dc_color_caps {
    struct dpp_color_caps dpp;
    struct mpc_color_caps mpc;
};
```

**Members**

`dpp`
:   color pipes caps for DPP

`mpc`
:   color pipes caps for MPC

enum pipe\_split\_policy
:   Pipe split strategy supported by DCN

**Constants**

`MPC_SPLIT_DYNAMIC`
:   DC will automatically decide how to split the
    pipe in order to bring the best trade-off between performance and
    power consumption. This is the recommended option.

`MPC_SPLIT_AVOID`
:   Avoid pipe split, which means that DC will not
    try any sort of split optimization.

`MPC_SPLIT_AVOID_MULT_DISP`
:   With this option, DC will only try to
    optimize the pipe utilization when using a single display; if the
    user connects to a second display, DC will avoid pipe split.

**Description**

This `enum is` used to define the pipe split policy supported by DCN. By
default, DC favors MPC\_SPLIT\_DYNAMIC.

struct dc\_validation\_set
:   Struct to store surface/stream associations for validation

**Definition**:

```
struct dc_validation_set {
    struct dc_stream_state *stream;
    struct dc_plane_state *plane_states[MAX_SURFACES];
    uint8_t plane_count;
};
```

**Members**

`stream`
:   Stream state properties

`plane_states`
:   Surface state

`plane_count`
:   Total of active planes

void dc\_get\_underflow\_debug\_data\_for\_otg(struct [dc](#c.dc_get_underflow_debug_data_for_otg "dc") \*dc, int primary\_otg\_inst, struct dc\_underflow\_debug\_data \*out\_data)
:   Retrieve underflow debug data.

**Parameters**

`struct dc *dc`
:   Pointer to the display core context.

`int primary_otg_inst`
:   Instance index of the primary OTG that underflowed.

`struct dc_underflow_debug_data *out_data`
:   Pointer to a dc\_underflow\_debug\_data `struct to` be filled with debug information.

**Description**

This function collects and logs underflow-related HW states when underflow happens,
including OTG underflow status, current read positions, frame count, and per-HUBP debug data.
The results are stored in the provided out\_data structure for further analysis or logging.

bool dc\_capture\_register\_software\_state(struct [dc](#c.dc_capture_register_software_state "dc") \*dc, struct dc\_register\_software\_state \*state)
:   Capture software state for register programming

**Parameters**

`struct dc *dc`
:   DC context containing current display configuration

`struct dc_register_software_state *state`
:   Pointer to dc\_register\_software\_state structure to populate

**Description**

Extracts all software state variables that are used to program hardware register
fields across the display driver pipeline. This provides a complete snapshot
of the software configuration that drives hardware register programming.

The function traverses the DC context and extracts values from:
- Stream configurations (timing, format, DSC settings)
- Plane states (surface format, rotation, scaling, cursor)
- Pipe contexts (resource allocation, blending, viewport)
- Clock manager (display clocks, DPP clocks, pixel clocks)
- Resource context (DET buffer allocation, ODM configuration)

This is essential for underflow debugging as it captures the exact software
state that determines how registers are programmed, allowing analysis of
whether underflow is caused by incorrect register programming or timing issues.

**Return**

true if state was successfully captured, false on error

bool dc\_get\_qos\_info(struct [dc](#c.dc_get_qos_info "dc") \*dc, struct dc\_qos\_info \*info)
:   Retrieve Quality of Service (QoS) information from display core

**Parameters**

`struct dc *dc`
:   DC context containing current display configuration

`struct dc_qos_info *info`
:   Pointer to dc\_qos\_info structure to populate with QoS metrics

**Description**

This function retrieves QoS metrics from the display core that can be used by
benchmark tools to analyze display system performance. The function may take
several milliseconds to execute due to hardware measurement requirements.

QoS information includes:
- Bandwidth bounds (lower limits in Mbps)
- Latency bounds (upper limits in nanoseconds)
- Hardware-measured bandwidth metrics (peak/average in Mbps)
- Hardware-measured latency metrics (maximum/average in nanoseconds)

The function will populate the provided dc\_qos\_info structure with current
QoS measurements. If hardware measurement functions are not available for
the current DCN version, the function returns false with zero’d info structure.

**Return**

true if QoS information was successfully retrieved, false if measurement
functions are unavailable or hardware measurements cannot be performed

The color pipeline has undergone major changes between DCN hardware
generations. What’s possible to do before and after blending depends on
hardware capabilities, as illustrated below by the DCN 2.0 and DCN 3.0 families
schemas.

**DCN 2.0 family color caps and mapping**

![../../../_images/dcn2_cm_drm_current.svg](../../../_images/dcn2_cm_drm_current.svg)

**DCN 3.0 family color caps and mapping**

![../../../_images/dcn3_cm_drm_current.svg](../../../_images/dcn3_cm_drm_current.svg)

## [Blend Mode Properties](#id10)

Pixel blend mode is a DRM plane composition property of [`drm_plane`](../../drm-kms.html#c.drm_plane "drm_plane") used to
describes how pixels from a foreground plane (fg) are composited with the
background plane (bg). Here, we present main concepts of DRM blend mode to help
to understand how this property is mapped to AMD DC interface. See more about
this DRM property and the alpha blending equations in [DRM Plane
Composition Properties](../../drm-kms.html#plane-composition-properties).

Basically, a blend mode sets the alpha blending equation for plane
composition that fits the mode in which the alpha channel affects the state of
pixel color values and, therefore, the resulted pixel color. For
example, consider the following elements of the alpha blending equation:

* *fg.rgb*: Each of the RGB component values from the foreground’s pixel.
* *fg.alpha*: Alpha component value from the foreground’s pixel.
* *bg.rgb*: Each of the RGB component values from the background.
* *plane\_alpha*: Plane alpha value set by the **plane “alpha” property**, see
  more in [DRM Plane Composition Properties](../../drm-kms.html#plane-composition-properties).

in the basic alpha blending equation:

```
out.rgb = alpha * fg.rgb + (1 - alpha) * bg.rgb
```

the alpha channel value of each pixel in a plane is ignored and only the plane
alpha affects the resulted pixel color values.

DRM has three blend mode to define the blend formula in the plane composition:

* **None**: Blend formula that ignores the pixel alpha.
* **Pre-multiplied**: Blend formula that assumes the pixel color values in a
  plane was already pre-multiplied by its own alpha channel before storage.
* **Coverage**: Blend formula that assumes the pixel color values were not
  pre-multiplied with the alpha channel values.

and pre-multiplied is the default pixel blend mode, that means, when no blend
mode property is created or defined, DRM considers the plane’s pixels has
pre-multiplied color values. On IGT GPU tools, the kms\_plane\_alpha\_blend test
provides a set of subtests to verify plane alpha and blend mode properties.

The DRM blend mode and its elements are then mapped by AMDGPU display manager
(DM) to program the blending configuration of the Multiple Pipe/Plane Combined
(MPC), as follows:

Therefore, the blending configuration for a single MPCC instance on the MPC
tree is defined by `mpcc_blnd_cfg`, where
`pre_multiplied_alpha` is the alpha pre-multiplied mode flag used to
set `MPCC_ALPHA_MULTIPLIED_MODE`. It controls whether alpha is
multiplied (true/false), being only true for DRM pre-multiplied blend mode.
`mpcc_alpha_blend_mode` defines the alpha blend mode regarding pixel
alpha and plane alpha values. It sets one of the three modes for
`MPCC_ALPHA_BLND_MODE`, as described below.

DM then maps the elements of `enum mpcc_alpha_blend_mode` to those in the DRM
blend formula, as follows:

* *MPC pixel alpha* matches *DRM fg.alpha* as the alpha component value
  from the plane’s pixel
* *MPC global alpha* matches *DRM plane\_alpha* when the pixel alpha should
  be ignored and, therefore, pixel values are not pre-multiplied
* *MPC global gain* assumes *MPC global alpha* value when both *DRM
  fg.alpha* and *DRM plane\_alpha* participate in the blend equation

In short, *fg.alpha* is ignored by selecting
`MPCC_ALPHA_BLEND_MODE_GLOBAL_ALPHA`. On the other hand, (plane\_alpha \*
fg.alpha) component becomes available by selecting
`MPCC_ALPHA_BLEND_MODE_PER_PIXEL_ALPHA_COMBINED_GLOBAL_GAIN`. And the
`MPCC_ALPHA_MULTIPLIED_MODE` defines if the pixel color values are
pre-multiplied by alpha or not.

### [Blend configuration flow](#id11)

The alpha blending equation is configured from DRM to DC interface by the
following path:

1. When updating a [`drm_plane_state`](../../drm-kms.html#c.drm_plane_state "drm_plane_state"), DM calls
   `amdgpu_dm_plane_fill_blending_from_plane_state()` that maps
   [`drm_plane_state`](../../drm-kms.html#c.drm_plane_state "drm_plane_state") attributes to
   `dc_plane_info` `struct to` be handled in the
   OS-agnostic component (DC).
2. On DC interface, `struct mpcc_blnd_cfg` programs the
   MPCC blend configuration considering the `dc_plane_info` input from DPP.
