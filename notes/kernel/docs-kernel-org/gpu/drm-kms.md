# Kernel Mode Setting (KMS)

> 출처(원문): https://docs.kernel.org/gpu/drm-kms.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel Mode Setting (KMS)

Drivers must initialize the mode setting core by calling
[`drmm_mode_config_init()`](#c.drmm_mode_config_init "drmm_mode_config_init") on the DRM device. The function
initializes the [`struct drm_device`](drm-internals.html#c.drm_device "drm_device")
mode\_config field and never fails. Once done, mode configuration must
be setup by initializing the following fields.

* int min\_width, min\_height; int max\_width, max\_height;
  Minimum and maximum width and height of the frame buffers in pixel
  units.
* [`struct drm_mode_config_funcs`](#c.drm_mode_config_funcs "drm_mode_config_funcs") \*funcs;
  Mode setting functions.

## Overview

![KMS Display Pipeline](../_images/DOT-dade12aa9127c64406e41cdf8d7f80694c134db2.svg)

KMS Display Pipeline Overview

The basic object structure KMS presents to userspace is fairly simple.
Framebuffers (represented by [`struct drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer"),
see [Frame Buffer Abstraction](#frame-buffer-abstraction)) feed into planes. Planes are represented by
[`struct drm_plane`](#c.drm_plane "drm_plane"), see [Plane Abstraction](#plane-abstraction) for more
details. One or more (or even no) planes feed their pixel data into a CRTC
(represented by [`struct drm_crtc`](#c.drm_crtc "drm_crtc"), see [CRTC Abstraction](#crtc-abstraction))
for blending. The precise blending step is explained in more detail in [Plane
Composition Properties](#id2) and related chapters.

For the output routing the first step is encoders (represented by
[`struct drm_encoder`](#c.drm_encoder "drm_encoder"), see [Encoder Abstraction](#encoder-abstraction)). Those
are really just internal artifacts of the helper libraries used to implement KMS
drivers. Besides that they make it unnecessarily more complicated for userspace
to figure out which connections between a CRTC and a connector are possible, and
what kind of cloning is supported, they serve no purpose in the userspace API.
Unfortunately encoders have been exposed to userspace, hence can’t remove them
at this point. Furthermore the exposed restrictions are often wrongly set by
drivers, and in many cases not powerful enough to express the real restrictions.
A CRTC can be connected to multiple encoders, and for an active CRTC there must
be at least one encoder.

The final, and real, endpoint in the display chain is the connector (represented
by [`struct drm_connector`](#c.drm_connector "drm_connector"), see [Connector
Abstraction](#connector-abstraction)). Connectors can have different possible encoders, but the kernel
driver selects which encoder to use for each connector. The use case is DVI,
which could switch between an analog and a digital encoder. Encoders can also
drive multiple different connectors. There is exactly one active connector for
every active encoder.

Internally the output pipeline is a bit more complex and matches today’s
hardware more closely:

![KMS Output Pipeline](../_images/DOT-6445c75fc4859992454fd377127d4d309e82f09a.svg)

KMS Output Pipeline

Internally two additional helper objects come into play. First, to be able to
share code for encoders (sometimes on the same SoC, sometimes off-chip) one or
more [Bridges](drm-kms-helpers.html#drm-bridges) (represented by [`struct drm_bridge`](drm-kms-helpers.html#c.drm_bridge "drm_bridge")) can be linked to an encoder. This link is static and cannot be
changed, which means the cross-bar (if there is any) needs to be mapped between
the CRTC and any encoders. Often for drivers with bridges there’s no code left
at the encoder level. Atomic drivers can leave out all the encoder callbacks to
essentially only leave a dummy routing object behind, which is needed for
backwards compatibility since encoders are exposed to userspace.

The second object is for panels, represented by [`struct drm_panel`](drm-kms-helpers.html#c.drm_panel "drm_panel"), see [Panel Helper Reference](drm-kms-helpers.html#drm-panel-helper). Panels do not have a fixed binding
point, but are generally linked to the driver private structure that embeds
[`struct drm_connector`](#c.drm_connector "drm_connector").

Note that currently the bridge chaining and interactions with connectors and
panels are still in-flux and not really fully sorted out yet.

## KMS Core Structures and Functions

struct drm\_mode\_config\_funcs
:   basic driver provided mode setting functions

**Definition**:

```
struct drm_mode_config_funcs {
    struct drm_framebuffer *(*fb_create)(struct drm_device *dev, struct drm_file *file_priv, const struct drm_format_info *info, const struct drm_mode_fb_cmd2 *mode_cmd);
    const struct drm_format_info *(*get_format_info)(u32 pixel_format, u64 modifier);
    enum drm_mode_status (*mode_valid)(struct drm_device *dev, const struct drm_display_mode *mode);
    int (*atomic_check)(struct drm_device *dev, struct drm_atomic_state *state);
    int (*atomic_commit)(struct drm_device *dev, struct drm_atomic_state *state, bool nonblock);
    struct drm_atomic_state *(*atomic_state_alloc)(struct drm_device *dev);
    void (*atomic_state_clear)(struct drm_atomic_state *state);
    void (*atomic_state_free)(struct drm_atomic_state *state);
};
```

**Members**

`fb_create`
:   Create a new framebuffer object. The core does basic checks on the
    requested metadata, but most of that is left to the driver. See
    [`struct drm_mode_fb_cmd2`](drm-uapi.html#c.drm_mode_fb_cmd2 "drm_mode_fb_cmd2") for details.

    To validate the pixel format and modifier drivers can use
    [`drm_any_plane_has_format()`](#c.drm_any_plane_has_format "drm_any_plane_has_format") to make sure at least one plane supports
    the requested values. Note that the driver must first determine the
    actual modifier used if the request doesn’t have it specified,
    ie. when (**mode\_cmd->flags** & DRM\_MODE\_FB\_MODIFIERS) == 0.

    IMPORTANT: These implied modifiers for legacy userspace must be
    stored in struct [`drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer"), including all relevant metadata
    like [`drm_framebuffer.pitches`](#c.drm_framebuffer "drm_framebuffer") and [`drm_framebuffer.offsets`](#c.drm_framebuffer "drm_framebuffer") if the
    modifier enables additional planes beyond the fourcc pixel format
    code. This is required by the GETFB2 ioctl.

    If the parameters are deemed valid and the backing storage objects in
    the underlying memory manager all exist, then the driver allocates
    a new [`drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer") structure, subclassed to contain
    driver-specific information (like the internal native buffer object
    references). It also needs to fill out all relevant metadata, which
    should be done by calling [`drm_helper_mode_fill_fb_struct()`](drm-kms-helpers.html#c.drm_helper_mode_fill_fb_struct "drm_helper_mode_fill_fb_struct").

    The initialization is finalized by calling [`drm_framebuffer_init()`](#c.drm_framebuffer_init "drm_framebuffer_init"),
    which registers the framebuffer and makes it accessible to other
    threads.

    RETURNS:

    A new framebuffer with an initial reference count of 1 or a negative
    error code encoded with [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR").

`get_format_info`
:   Allows a driver to return custom format information for special
    fb layouts (eg. ones with auxiliary compression control planes).

    RETURNS:

    The format information specific to the given fb metadata, or
    NULL if none is found.

`mode_valid`
:   Device specific validation of display modes. Can be used to reject
    modes that can never be supported. Only device wide constraints can
    be checked here. crtc/encoder/bridge/connector specific constraints
    should be checked in the .`mode_valid()` hook for each specific object.

`atomic_check`
:   This is the only hook to validate an atomic modeset update. This
    function must reject any modeset and state changes which the hardware
    or driver doesn’t support. This includes but is of course not limited
    to:

    > * Checking that the modes, framebuffers, scaling and placement
    >   requirements and so on are within the limits of the hardware.
    > * Checking that any hidden shared resources are not oversubscribed.
    >   This can be shared PLLs, shared lanes, overall memory bandwidth,
    >   display fifo space (where shared between planes or maybe even
    >   CRTCs).
    > * Checking that virtualized resources exported to userspace are not
    >   oversubscribed. For various reasons it can make sense to expose
    >   more planes, crtcs or encoders than which are physically there. One
    >   example is dual-pipe operations (which generally should be hidden
    >   from userspace if when lockstepped in hardware, exposed otherwise),
    >   where a plane might need 1 hardware plane (if it’s just on one
    >   pipe), 2 hardware planes (when it spans both pipes) or maybe even
    >   shared a hardware plane with a 2nd plane (if there’s a compatible
    >   plane requested on the area handled by the other pipe).
    > * Check that any transitional state is possible and that if
    >   requested, the update can indeed be done in the vblank period
    >   without temporarily disabling some functions.
    > * Check any other constraints the driver or hardware might have.
    > * This callback also needs to correctly fill out the [`drm_crtc_state`](#c.drm_crtc_state "drm_crtc_state")
    >   in this update to make sure that [`drm_atomic_crtc_needs_modeset()`](#c.drm_atomic_crtc_needs_modeset "drm_atomic_crtc_needs_modeset")
    >   reflects the nature of the possible update and returns true if and
    >   only if the update cannot be applied without tearing within one
    >   vblank on that CRTC. The core uses that information to reject
    >   updates which require a full modeset (i.e. blanking the screen, or
    >   at least pausing updates for a substantial amount of time) if
    >   userspace has disallowed that in its request.
    > * The driver also does not need to repeat basic input validation
    >   like done for the corresponding legacy entry points. The core does
    >   that before calling this hook.

    See the documentation of **atomic\_commit** for an exhaustive list of
    error conditions which don’t have to be checked at the in this
    callback.

    See the documentation for [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") for how exactly
    an atomic modeset update is described.

    Drivers using the atomic helpers can implement this hook using
    [`drm_atomic_helper_check()`](drm-kms-helpers.html#c.drm_atomic_helper_check "drm_atomic_helper_check"), or one of the exported sub-functions of
    it.

    RETURNS:

    0 on success or one of the below negative error codes:

    > * -EINVAL, if any of the above constraints are violated.
    > * -EDEADLK, when returned from an attempt to acquire an additional
    >   [`drm_modeset_lock`](#c.drm_modeset_lock "drm_modeset_lock") through [`drm_modeset_lock()`](#c.drm_modeset_lock "drm_modeset_lock").
    > * -ENOMEM, if allocating additional state sub-structures failed due
    >   to lack of memory.
    > * -EINTR, -EAGAIN or -ERESTARTSYS, if the IOCTL should be restarted.
    >   This can either be due to a pending signal, or because the driver
    >   needs to completely bail out to recover from an exceptional
    >   situation like a GPU hang. From a userspace point all errors are
    >   treated equally.

`atomic_commit`
:   This is the only hook to commit an atomic modeset update. The core
    guarantees that **atomic\_check** has been called successfully before
    calling this function, and that nothing has been changed in the
    interim.

    See the documentation for [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") for how exactly
    an atomic modeset update is described.

    Drivers using the atomic helpers can implement this hook using
    [`drm_atomic_helper_commit()`](drm-kms-helpers.html#c.drm_atomic_helper_commit "drm_atomic_helper_commit"), or one of the exported sub-functions of
    it.

    Nonblocking commits (as indicated with the nonblock parameter) must
    do any preparatory work which might result in an unsuccessful commit
    in the context of this callback. The only exceptions are hardware
    errors resulting in -EIO. But even in that case the driver must
    ensure that the display pipe is at least running, to avoid
    compositors crashing when pageflips don’t work. Anything else,
    specifically committing the update to the hardware, should be done
    without blocking the caller. For updates which do not require a
    modeset this must be guaranteed.

    The driver must wait for any pending rendering to the new
    framebuffers to complete before executing the flip. It should also
    wait for any pending rendering from other drivers if the underlying
    buffer is a shared dma-buf. Nonblocking commits must not wait for
    rendering in the context of this callback.

    An application can request to be notified when the atomic commit has
    completed. These events are per-CRTC and can be distinguished by the
    CRTC index supplied in [`drm_event`](drm-uapi.html#c.drm_event "drm_event") to userspace.

    The drm core will supply a [`struct drm_event`](drm-uapi.html#c.drm_event "drm_event") in each CRTC’s
    [`drm_crtc_state.event`](#c.drm_crtc_state "drm_crtc_state"). See the documentation for
    [`drm_crtc_state.event`](#c.drm_crtc_state "drm_crtc_state") for more details about the precise semantics of
    this event.

    NOTE:

    Drivers are not allowed to shut down any display pipe successfully
    enabled through an atomic commit on their own. Doing so can result in
    compositors crashing if a page flip is suddenly rejected because the
    pipe is off.

    RETURNS:

    0 on success or one of the below negative error codes:

    > * -EBUSY, if a nonblocking updated is requested and there is
    >   an earlier updated pending. Drivers are allowed to support a queue
    >   of outstanding updates, but currently no driver supports that.
    >   Note that drivers must wait for preceding updates to complete if a
    >   synchronous update is requested, they are not allowed to fail the
    >   commit in that case.
    > * -ENOMEM, if the driver failed to allocate memory. Specifically
    >   this can happen when trying to pin framebuffers, which must only
    >   be done when committing the state.
    > * -ENOSPC, as a refinement of the more generic -ENOMEM to indicate
    >   that the driver has run out of vram, iommu space or similar GPU
    >   address space needed for framebuffer.
    > * -EIO, if the hardware completely died.
    > * -EINTR, -EAGAIN or -ERESTARTSYS, if the IOCTL should be restarted.
    >   This can either be due to a pending signal, or because the driver
    >   needs to completely bail out to recover from an exceptional
    >   situation like a GPU hang. From a userspace point of view all errors are
    >   treated equally.

    This list is exhaustive. Specifically this hook is not allowed to
    return -EINVAL (any invalid requests should be caught in
    **atomic\_check**) or -EDEADLK (this function must not acquire
    additional modeset locks).

`atomic_state_alloc`
:   This optional hook can be used by drivers that want to subclass struct
    [`drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") to be able to track their own driver-private global
    state easily. If this hook is implemented, drivers must also
    implement **atomic\_state\_clear** and **atomic\_state\_free**.

    Subclassing of [`drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") is deprecated in favour of using
    [`drm_private_state`](#c.drm_private_state "drm_private_state") and [`drm_private_obj`](#c.drm_private_obj "drm_private_obj").

    RETURNS:

    A new [`drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") on success or NULL on failure.

`atomic_state_clear`
:   This hook must clear any driver private state duplicated into the
    passed-in [`drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state"). This hook is called when the caller
    encountered a [`drm_modeset_lock`](#c.drm_modeset_lock "drm_modeset_lock") deadlock and needs to drop all
    already acquired locks as part of the deadlock avoidance dance
    implemented in [`drm_modeset_backoff()`](#c.drm_modeset_backoff "drm_modeset_backoff").

    Any duplicated state must be invalidated since a concurrent atomic
    update might change it, and the drm atomic interfaces always apply
    updates as relative changes to the current state.

    Drivers that implement this must call [`drm_atomic_state_default_clear()`](#c.drm_atomic_state_default_clear "drm_atomic_state_default_clear")
    to clear common state.

    Subclassing of [`drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") is deprecated in favour of using
    [`drm_private_state`](#c.drm_private_state "drm_private_state") and [`drm_private_obj`](#c.drm_private_obj "drm_private_obj").

`atomic_state_free`
:   This hook needs driver private resources and the [`drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state")
    itself. Note that the core first calls [`drm_atomic_state_clear()`](#c.drm_atomic_state_clear "drm_atomic_state_clear") to
    avoid code duplicate between the clear and free hooks.

    Drivers that implement this must call
    [`drm_atomic_state_default_release()`](#c.drm_atomic_state_default_release "drm_atomic_state_default_release") to release common resources.

    Subclassing of [`drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") is deprecated in favour of using
    [`drm_private_state`](#c.drm_private_state "drm_private_state") and [`drm_private_obj`](#c.drm_private_obj "drm_private_obj").

**Description**

Some global (i.e. not per-CRTC, connector, etc) mode setting functions that
involve drivers.

struct drm\_mode\_config
:   Mode configuration control structure

**Definition**:

```
struct drm_mode_config {
    struct mutex mutex;
    struct drm_modeset_lock connection_mutex;
    struct drm_modeset_acquire_ctx *acquire_ctx;
    struct mutex idr_mutex;
    struct idr object_idr;
    struct idr tile_idr;
    struct mutex fb_lock;
    int num_fb;
    struct list_head fb_list;
    spinlock_t connector_list_lock;
    int num_connector;
    struct ida connector_ida;
    struct list_head connector_list;
    struct llist_head connector_free_list;
    struct work_struct connector_free_work;
    int num_encoder;
    struct list_head encoder_list;
    int num_total_plane;
    struct list_head plane_list;
    struct raw_spinlock panic_lock;
    int num_colorop;
    struct list_head colorop_list;
    int num_crtc;
    struct list_head crtc_list;
    struct list_head property_list;
    struct list_head privobj_list;
    unsigned int min_width, min_height;
    unsigned int max_width, max_height;
    const struct drm_mode_config_funcs *funcs;
    bool poll_enabled;
    bool poll_running;
    bool delayed_event;
    struct delayed_work output_poll_work;
    struct mutex blob_lock;
    struct list_head property_blob_list;
    struct drm_property *edid_property;
    struct drm_property *dpms_property;
    struct drm_property *path_property;
    struct drm_property *tile_property;
    struct drm_property *panel_type_property;
    struct drm_property *link_status_property;
    struct drm_property *plane_type_property;
    struct drm_property *prop_src_x;
    struct drm_property *prop_src_y;
    struct drm_property *prop_src_w;
    struct drm_property *prop_src_h;
    struct drm_property *prop_crtc_x;
    struct drm_property *prop_crtc_y;
    struct drm_property *prop_crtc_w;
    struct drm_property *prop_crtc_h;
    struct drm_property *prop_fb_id;
    struct drm_property *prop_in_fence_fd;
    struct drm_property *prop_out_fence_ptr;
    struct drm_property *prop_crtc_id;
    struct drm_property *prop_fb_damage_clips;
    struct drm_property *prop_active;
    struct drm_property *prop_mode_id;
    struct drm_property *prop_vrr_enabled;
    struct drm_property *dvi_i_subconnector_property;
    struct drm_property *dvi_i_select_subconnector_property;
    struct drm_property *dp_subconnector_property;
    struct drm_property *tv_subconnector_property;
    struct drm_property *tv_select_subconnector_property;
    struct drm_property *legacy_tv_mode_property;
    struct drm_property *tv_mode_property;
    struct drm_property *tv_left_margin_property;
    struct drm_property *tv_right_margin_property;
    struct drm_property *tv_top_margin_property;
    struct drm_property *tv_bottom_margin_property;
    struct drm_property *tv_brightness_property;
    struct drm_property *tv_contrast_property;
    struct drm_property *tv_flicker_reduction_property;
    struct drm_property *tv_overscan_property;
    struct drm_property *tv_saturation_property;
    struct drm_property *tv_hue_property;
    struct drm_property *scaling_mode_property;
    struct drm_property *aspect_ratio_property;
    struct drm_property *content_type_property;
    struct drm_property *degamma_lut_property;
    struct drm_property *degamma_lut_size_property;
    struct drm_property *ctm_property;
    struct drm_property *gamma_lut_property;
    struct drm_property *gamma_lut_size_property;
    struct drm_property *background_color_property;
    struct drm_property *suggested_x_property;
    struct drm_property *suggested_y_property;
    struct drm_property *non_desktop_property;
    struct drm_property *panel_orientation_property;
    struct drm_property *writeback_fb_id_property;
    struct drm_property *writeback_pixel_formats_property;
    struct drm_property *writeback_out_fence_ptr_property;
    struct drm_property *hdr_output_metadata_property;
    struct drm_property *content_protection_property;
    struct drm_property *hdcp_content_type_property;
    uint32_t preferred_depth, prefer_shadow;
    bool quirk_addfb_prefer_xbgr_30bpp;
    bool quirk_addfb_prefer_host_byte_order;
    bool async_page_flip;
    bool fb_modifiers_not_supported;
    bool normalize_zpos;
    struct drm_property *modifiers_property;
    struct drm_property *async_modifiers_property;
    struct drm_property *size_hints_property;
    uint32_t cursor_width, cursor_height;
    struct drm_atomic_state *suspend_state;
    const struct drm_mode_config_helper_funcs *helper_private;
};
```

**Members**

`mutex`
:   This is the big scary modeset BKL which protects everything that
    isn’t protect otherwise. Scope is unclear and fuzzy, try to remove
    anything from under its protection and move it into more well-scoped
    locks.

    The one important thing this protects is the use of **acquire\_ctx**.

`connection_mutex`
:   This protects connector state and the connector to encoder to CRTC
    routing chain.

    For atomic drivers specifically this protects [`drm_connector.state`](#c.drm_connector "drm_connector").

`acquire_ctx`
:   Global implicit acquire context used by atomic drivers for legacy
    IOCTLs. Deprecated, since implicit locking contexts make it
    impossible to use driver-private [`struct drm_modeset_lock`](#c.drm_modeset_lock "drm_modeset_lock"). Users of
    this must hold **mutex**.

`idr_mutex`
:   Mutex for KMS ID allocation and management. Protects both **object\_idr**
    and **tile\_idr**.

`object_idr`
:   Main KMS ID tracking object. Use this idr for all IDs, fb, crtc,
    connector, modes - just makes life easier to have only one.

`tile_idr`
:   Use this idr for allocating new IDs for tiled sinks like use in some
    high-res DP MST screens.

`fb_lock`
:   Mutex to protect fb the global **fb\_list** and **num\_fb**.

`num_fb`
:   Number of entries on **fb\_list**.

`fb_list`
:   List of all [`struct drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer").

`connector_list_lock`
:   Protects **num\_connector** and
    **connector\_list** and **connector\_free\_list**.

`num_connector`
:   Number of connectors on this device. Protected by
    **connector\_list\_lock**.

`connector_ida`
:   ID allocator for connector indices.

`connector_list`
:   List of connector objects linked with [`drm_connector.head`](#c.drm_connector "drm_connector"). Protected
    by **connector\_list\_lock**. Only use [`drm_for_each_connector_iter()`](#c.drm_for_each_connector_iter "drm_for_each_connector_iter") and
    [`struct drm_connector_list_iter`](#c.drm_connector_list_iter "drm_connector_list_iter") to walk this list.

`connector_free_list`
:   List of connector objects linked with [`drm_connector.free_head`](#c.drm_connector "drm_connector").
    Protected by **connector\_list\_lock**. Used by
    [`drm_for_each_connector_iter()`](#c.drm_for_each_connector_iter "drm_for_each_connector_iter") and
    [`struct drm_connector_list_iter`](#c.drm_connector_list_iter "drm_connector_list_iter") to savely free connectors using
    **connector\_free\_work**.

`connector_free_work`
:   Work to clean up **connector\_free\_list**.

`num_encoder`
:   Number of encoders on this device. This is invariant over the
    lifetime of a device and hence doesn’t need any locks.

`encoder_list`
:   List of encoder objects linked with [`drm_encoder.head`](#c.drm_encoder "drm_encoder"). This is
    invariant over the lifetime of a device and hence doesn’t need any
    locks.

`num_total_plane`
:   Number of universal (i.e. with primary/curso) planes on this device.
    This is invariant over the lifetime of a device and hence doesn’t
    need any locks.

`plane_list`
:   List of plane objects linked with [`drm_plane.head`](#c.drm_plane "drm_plane"). This is invariant
    over the lifetime of a device and hence doesn’t need any locks.

`panic_lock`
:   Raw spinlock used to protect critical sections of code that access
    the display hardware or modeset software state, which the panic
    printing code must be protected against. See [`drm_panic_trylock()`](#c.drm_panic_trylock "drm_panic_trylock"),
    [`drm_panic_lock()`](#c.drm_panic_lock "drm_panic_lock") and [`drm_panic_unlock()`](#c.drm_panic_unlock "drm_panic_unlock").

`num_colorop`
:   Number of colorop objects on this device.
    This is invariant over the lifetime of a device and hence doesn’t
    need any locks.

`colorop_list`
:   List of colorop objects linked with [`drm_colorop.head`](#c.drm_colorop "drm_colorop"). This is
    invariant over the lifetime of a device and hence doesn’t need any
    locks.

`num_crtc`
:   Number of CRTCs on this device linked with [`drm_crtc.head`](#c.drm_crtc "drm_crtc"). This is invariant over the lifetime
    of a device and hence doesn’t need any locks.

`crtc_list`
:   List of CRTC objects linked with [`drm_crtc.head`](#c.drm_crtc "drm_crtc"). This is invariant
    over the lifetime of a device and hence doesn’t need any locks.

`property_list`
:   List of property type objects linked with [`drm_property.head`](#c.drm_property "drm_property"). This is
    invariant over the lifetime of a device and hence doesn’t need any
    locks.

`privobj_list`
:   List of private objects linked with [`drm_private_obj.head`](#c.drm_private_obj "drm_private_obj"). This is
    invariant over the lifetime of a device and hence doesn’t need any
    locks.

`min_width`
:   minimum fb pixel width on this device

`min_height`
:   minimum fb pixel height on this device

`max_width`
:   maximum fb pixel width on this device

`max_height`
:   maximum fb pixel height on this device

`funcs`
:   core driver provided mode setting functions

`poll_enabled`
:   track polling support for this device

`poll_running`
:   track polling status for this device

`delayed_event`
:   track delayed poll uevent deliver for this device

`output_poll_work`
:   delayed work for polling in process context

`blob_lock`
:   Mutex for blob property allocation and management, protects
    **property\_blob\_list** and [`drm_file.blobs`](drm-internals.html#c.drm_file "drm_file").

`property_blob_list`
:   List of all the blob property objects linked with
    [`drm_property_blob.head`](#c.drm_property_blob "drm_property_blob"). Protected by **blob\_lock**.

`edid_property`
:   Default connector property to hold the EDID of the
    currently connected sink, if any.

`dpms_property`
:   Default connector property to control the
    connector’s DPMS state.

`path_property`
:   Default connector property to hold the DP MST path
    for the port.

`tile_property`
:   Default connector property to store the tile
    position of a tiled screen, for sinks which need to be driven with
    multiple CRTCs.

`panel_type_property`
:   Default connector property for panel type

`link_status_property`
:   Default connector property for link status
    of a connector

`plane_type_property`
:   Default plane property to differentiate
    CURSOR, PRIMARY and OVERLAY legacy uses of planes.

`prop_src_x`
:   Default atomic plane property for the plane source
    position in the connected [`drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer").

`prop_src_y`
:   Default atomic plane property for the plane source
    position in the connected [`drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer").

`prop_src_w`
:   Default atomic plane property for the plane source
    position in the connected [`drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer").

`prop_src_h`
:   Default atomic plane property for the plane source
    position in the connected [`drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer").

`prop_crtc_x`
:   Default atomic plane property for the plane destination
    position in the [`drm_crtc`](#c.drm_crtc "drm_crtc") is being shown on.

`prop_crtc_y`
:   Default atomic plane property for the plane destination
    position in the [`drm_crtc`](#c.drm_crtc "drm_crtc") is being shown on.

`prop_crtc_w`
:   Default atomic plane property for the plane destination
    position in the [`drm_crtc`](#c.drm_crtc "drm_crtc") is being shown on.

`prop_crtc_h`
:   Default atomic plane property for the plane destination
    position in the [`drm_crtc`](#c.drm_crtc "drm_crtc") is being shown on.

`prop_fb_id`
:   Default atomic plane property to specify the
    [`drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer").

`prop_in_fence_fd`
:   Sync File fd representing the incoming fences
    for a Plane.

`prop_out_fence_ptr`
:   Sync File fd pointer representing the
    outgoing fences for a CRTC. Userspace should provide a pointer to a
    value of type s32, and then cast that pointer to u64.

`prop_crtc_id`
:   Default atomic plane property to specify the
    [`drm_crtc`](#c.drm_crtc "drm_crtc").

`prop_fb_damage_clips`
:   Optional plane property to mark damaged
    regions on the plane in framebuffer coordinates of the framebuffer
    attached to the plane.

    The layout of blob data is simply an array of [`drm_mode_rect`](drm-uapi.html#c.drm_mode_rect "drm_mode_rect"). Unlike
    plane src coordinates, damage clips are not in 16.16 fixed point.

`prop_active`
:   Default atomic CRTC property to control the active
    state, which is the simplified implementation for DPMS in atomic
    drivers.

`prop_mode_id`
:   Default atomic CRTC property to set the mode for a
    CRTC. A 0 mode implies that the CRTC is entirely disabled - all
    connectors must be of and active must be set to disabled, too.

`prop_vrr_enabled`
:   Default atomic CRTC property to indicate
    whether variable refresh rate should be enabled on the CRTC.

`dvi_i_subconnector_property`
:   Optional DVI-I property to
    differentiate between analog or digital mode.

`dvi_i_select_subconnector_property`
:   Optional DVI-I property to
    select between analog or digital mode.

`dp_subconnector_property`
:   Optional DP property to differentiate
    between different DP downstream port types.

`tv_subconnector_property`
:   Optional TV property to differentiate
    between different TV connector types.

`tv_select_subconnector_property`
:   Optional TV property to select
    between different TV connector types.

`legacy_tv_mode_property`
:   Optional TV property to select
    the output TV mode.

    Superseded by **tv\_mode\_property**

`tv_mode_property`
:   Optional TV property to select the TV
    standard output on the connector.

`tv_left_margin_property`
:   Optional TV property to set the left
    margin (expressed in pixels).

`tv_right_margin_property`
:   Optional TV property to set the right
    margin (expressed in pixels).

`tv_top_margin_property`
:   Optional TV property to set the right
    margin (expressed in pixels).

`tv_bottom_margin_property`
:   Optional TV property to set the right
    margin (expressed in pixels).

`tv_brightness_property`
:   Optional TV property to set the
    brightness.

`tv_contrast_property`
:   Optional TV property to set the
    contrast.

`tv_flicker_reduction_property`
:   Optional TV property to control the
    flicker reduction mode.

`tv_overscan_property`
:   Optional TV property to control the overscan
    setting.

`tv_saturation_property`
:   Optional TV property to set the
    saturation.

`tv_hue_property`
:   Optional TV property to set the hue.

`scaling_mode_property`
:   Optional connector property to control the
    upscaling, mostly used for built-in panels.

`aspect_ratio_property`
:   Optional connector property to control the
    HDMI infoframe aspect ratio setting.

`content_type_property`
:   Optional connector property to control the
    HDMI infoframe content type setting.

`degamma_lut_property`
:   Optional CRTC property to set the LUT used to
    convert the framebuffer’s colors to linear gamma.

`degamma_lut_size_property`
:   Optional CRTC property for the size of
    the degamma LUT as supported by the driver (read-only).

`ctm_property`
:   Optional CRTC property to set the
    matrix used to convert colors after the lookup in the
    degamma LUT.

`gamma_lut_property`
:   Optional CRTC property to set the LUT used to
    convert the colors, after the CTM matrix, to the gamma space of the
    connected screen.

`gamma_lut_size_property`
:   Optional CRTC property for the size of the
    gamma LUT as supported by the driver (read-only).

`background_color_property`
:   Optional CRTC property to set the
    background color.

`suggested_x_property`
:   Optional connector property with a hint for
    the position of the output on the host’s screen.

`suggested_y_property`
:   Optional connector property with a hint for
    the position of the output on the host’s screen.

`non_desktop_property`
:   Optional connector property with a hint
    that device isn’t a standard display, and the console/desktop,
    should not be displayed on it.

`panel_orientation_property`
:   Optional connector property indicating
    how the lcd-panel is mounted inside the casing (e.g. normal or
    upside-down).

`writeback_fb_id_property`
:   Property for writeback connectors, storing
    the ID of the output framebuffer.
    See also: [`drm_writeback_connector_init()`](#c.drm_writeback_connector_init "drm_writeback_connector_init")

`writeback_pixel_formats_property`
:   Property for writeback connectors,
    storing an array of the supported pixel formats for the writeback
    engine (read-only).
    See also: [`drm_writeback_connector_init()`](#c.drm_writeback_connector_init "drm_writeback_connector_init")

`writeback_out_fence_ptr_property`
:   Property for writeback connectors,
    fd pointer representing the outgoing fences for a writeback
    connector. Userspace should provide a pointer to a value of type s32,
    and then cast that pointer to u64.
    See also: [`drm_writeback_connector_init()`](#c.drm_writeback_connector_init "drm_writeback_connector_init")

`hdr_output_metadata_property`
:   Connector property containing hdr
    metatada. This will be provided by userspace compositors based
    on HDR content

`content_protection_property`
:   DRM ENUM property for content
    protection. See [`drm_connector_attach_content_protection_property()`](drm-kms-helpers.html#c.drm_connector_attach_content_protection_property "drm_connector_attach_content_protection_property").

`hdcp_content_type_property`
:   DRM ENUM property for type of
    Protected Content.

`preferred_depth`
:   preferred RBG pixel depth, used by fb helpers

`prefer_shadow`
:   hint to userspace to prefer shadow-fb rendering

`quirk_addfb_prefer_xbgr_30bpp`
:   Special hack for legacy ADDFB to keep nouveau userspace happy. Should
    only ever be set by the nouveau kernel driver.

`quirk_addfb_prefer_host_byte_order`
:   When set to true `drm_mode_addfb()` will pick host byte order
    pixel\_format when calling `drm_mode_addfb2()`. This is how
    `drm_mode_addfb()` should have worked from day one. It
    didn’t though, so we ended up with quirks in both kernel
    and userspace drivers to deal with the broken behavior.
    Simply fixing `drm_mode_addfb()` unconditionally would break
    these drivers, so add a quirk bit here to allow drivers
    opt-in.

`async_page_flip`
:   Does this device support async flips on the primary
    plane?

`fb_modifiers_not_supported`
:   When this flag is set, the DRM device will not expose modifier
    support to userspace. This is only used by legacy drivers that infer
    the buffer layout through heuristics without using modifiers. New
    drivers shall not set fhis flag.

`normalize_zpos`
:   If true the drm core will call [`drm_atomic_normalize_zpos()`](#c.drm_atomic_normalize_zpos "drm_atomic_normalize_zpos") as part of
    atomic mode checking from [`drm_atomic_helper_check()`](drm-kms-helpers.html#c.drm_atomic_helper_check "drm_atomic_helper_check")

`modifiers_property`
:   Plane property to list support modifier/format
    combination.

`async_modifiers_property`
:   Plane property to list support modifier/format
    combination for asynchronous flips.

`size_hints_property`
:   Plane SIZE\_HINTS property.

`cursor_width`
:   hint to userspace for max cursor width

`cursor_height`
:   hint to userspace for max cursor height

`suspend_state`
:   Atomic state when suspended.
    Set by [`drm_mode_config_helper_suspend()`](drm-kms-helpers.html#c.drm_mode_config_helper_suspend "drm_mode_config_helper_suspend") and cleared by
    [`drm_mode_config_helper_resume()`](drm-kms-helpers.html#c.drm_mode_config_helper_resume "drm_mode_config_helper_resume").

`helper_private`
:   mid-layer private data

**Description**

Core mode resource tracking structure. All CRTC, encoders, and connectors
enumerated by the driver are added here, as are global properties. Some
global restrictions are also here, e.g. dimension restrictions.

Framebuffer sizes refer to the virtual screen that can be displayed by
the CRTC. This can be different from the physical resolution programmed.
The minimum width and height, stored in **min\_width** and **min\_height**,
describe the smallest size of the framebuffer. It correlates to the
minimum programmable resolution.
The maximum width, stored in **max\_width**, is typically limited by the
maximum pitch between two adjacent scanlines. The maximum height, stored
in **max\_height**, is usually only limited by the amount of addressable video
memory. For hardware that has no real maximum, drivers should pick a
reasonable default.

See also **DRM\_SHADOW\_PLANE\_MAX\_WIDTH** and **DRM\_SHADOW\_PLANE\_MAX\_HEIGHT**.

int drm\_mode\_config\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   DRM mode\_configuration structure initialization

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

This is the unmanaged version of [`drmm_mode_config_init()`](#c.drmm_mode_config_init "drmm_mode_config_init") for drivers which
still explicitly call [`drm_mode_config_cleanup()`](#c.drm_mode_config_cleanup "drm_mode_config_cleanup").

FIXME: This function is deprecated and drivers should be converted over to
[`drmm_mode_config_init()`](#c.drmm_mode_config_init "drmm_mode_config_init").

void drm\_mode\_config\_reset(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   call ->reset callbacks

**Parameters**

`struct drm_device *dev`
:   drm device

**Description**

This functions calls all the crtc’s, encoder’s and connector’s ->reset
callback. Drivers can use this in e.g. their driver load or resume code to
reset hardware and software state.

int drmm\_mode\_config\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   managed DRM mode\_configuration structure initialization

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

Initialize **dev**’s mode\_config structure, used for tracking the graphics
configuration of **dev**.

Since this initializes the modeset locks, no locking is possible. Which is no
problem, since this should happen single threaded at init time. It is the
driver’s problem to ensure this guarantee.

Cleanup is automatically handled through registering drm\_mode\_config\_cleanup
with [`drmm_add_action()`](drm-internals.html#c.drmm_add_action "drmm_add_action").

**Return**

0 on success, negative error value on failure.

void drm\_mode\_config\_cleanup(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   free up DRM mode\_config info

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

Free up all the connectors and CRTCs associated with this DRM device, then
free up the framebuffers and associated buffer objects.

Note that since this /should/ happen single-threaded at driver/device
teardown time, no locking is required. It’s the driver’s job to ensure that
this guarantee actually holds true.

FIXME: With the managed [`drmm_mode_config_init()`](#c.drmm_mode_config_init "drmm_mode_config_init") it is no longer necessary for
drivers to explicitly call this function.

## Modeset Base Object Abstraction

![Mode Objects and Properties](../_images/DOT-1eee3f74bb2de20b2b68c4aa6c9c1cabe5078857.svg)

Mode Objects and Properties

The base structure for all KMS objects is [`struct drm_mode_object`](#c.drm_mode_object "drm_mode_object"). One of the base services it provides is tracking properties,
which are especially important for the atomic IOCTL (see [Atomic Mode
Setting](#atomic-mode-setting)). The somewhat surprising part here is that properties are not
directly instantiated on each object, but free-standing mode objects themselves,
represented by [`struct drm_property`](#c.drm_property "drm_property"), which only specify
the type and value range of a property. Any given property can be attached
multiple times to different objects using [`drm_object_attach_property()`](#c.drm_object_attach_property "drm_object_attach_property").

struct drm\_mode\_object
:   base structure for modeset objects

**Definition**:

```
struct drm_mode_object {
    uint32_t id;
    uint32_t type;
    struct drm_object_properties *properties;
    struct kref refcount;
    void (*free_cb)(struct kref *kref);
};
```

**Members**

`id`
:   userspace visible identifier

`type`
:   type of the object, one of DRM\_MODE\_OBJECT\_\*

`properties`
:   properties attached to this object, including values

`refcount`
:   reference count for objects with dynamic lifetime

`free_cb`
:   free function callback, only set for objects with dynamic lifetime

**Description**

Base structure for modeset objects visible to userspace. Objects can be
looked up using [`drm_mode_object_find()`](#c.drm_mode_object_find "drm_mode_object_find"). Besides basic uapi interface
properties like **id** and **type** it provides two services:

* It tracks attached properties and their values. This is used by [`drm_crtc`](#c.drm_crtc "drm_crtc"),
  [`drm_plane`](#c.drm_plane "drm_plane") and [`drm_connector`](#c.drm_connector "drm_connector"). Properties are attached by calling
  [`drm_object_attach_property()`](#c.drm_object_attach_property "drm_object_attach_property") before the object is visible to userspace.
* For objects with dynamic lifetimes (as indicated by a non-NULL **free\_cb**) it
  provides reference counting through [`drm_mode_object_get()`](#c.drm_mode_object_get "drm_mode_object_get") and
  [`drm_mode_object_put()`](#c.drm_mode_object_put "drm_mode_object_put"). This is used by [`drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer"), [`drm_connector`](#c.drm_connector "drm_connector")
  and [`drm_property_blob`](#c.drm_property_blob "drm_property_blob"). These objects provide specialized reference
  counting wrappers.

struct drm\_object\_properties
:   property tracking for [`drm_mode_object`](#c.drm_mode_object "drm_mode_object")

**Definition**:

```
struct drm_object_properties {
    int count;
    struct drm_property *properties[DRM_OBJECT_MAX_PROPERTY];
    uint64_t values[DRM_OBJECT_MAX_PROPERTY];
};
```

**Members**

`count`
:   number of valid properties, must be less than or equal to
    DRM\_OBJECT\_MAX\_PROPERTY.

`properties`
:   Array of pointers to [`drm_property`](#c.drm_property "drm_property").

    NOTE: if we ever start dynamically destroying properties (ie.
    not at [`drm_mode_config_cleanup()`](#c.drm_mode_config_cleanup "drm_mode_config_cleanup") time), then we’d have to do
    a better job of detaching property from mode objects to avoid
    dangling property pointers:

`values`
:   Array to store the property values, matching **properties**. Do
    not read/write values directly, but use
    [`drm_object_property_get_value()`](#c.drm_object_property_get_value "drm_object_property_get_value") and [`drm_object_property_set_value()`](#c.drm_object_property_set_value "drm_object_property_set_value").

    Note that atomic drivers do not store mutable properties in this
    array, but only the decoded values in the corresponding state
    structure. The decoding is done using the [`drm_crtc.atomic_get_property`](#c.drm_crtc "drm_crtc") and
    [`drm_crtc.atomic_set_property`](#c.drm_crtc "drm_crtc") hooks for [`struct drm_crtc`](#c.drm_crtc "drm_crtc"). For
    [`struct drm_plane`](#c.drm_plane "drm_plane") the hooks are [`drm_plane_funcs.atomic_get_property`](#c.drm_plane_funcs "drm_plane_funcs") and
    [`drm_plane_funcs.atomic_set_property`](#c.drm_plane_funcs "drm_plane_funcs"). And for [`struct drm_connector`](#c.drm_connector "drm_connector")
    the hooks are [`drm_connector_funcs.atomic_get_property`](#c.drm_connector_funcs "drm_connector_funcs") and
    [`drm_connector_funcs.atomic_set_property`](#c.drm_connector_funcs "drm_connector_funcs") .

    Hence atomic drivers should not use [`drm_object_property_set_value()`](#c.drm_object_property_set_value "drm_object_property_set_value")
    and [`drm_object_property_get_value()`](#c.drm_object_property_get_value "drm_object_property_get_value") on mutable objects, i.e. those
    without the DRM\_MODE\_PROP\_IMMUTABLE flag set.

    For atomic drivers the default value of properties is stored in this
    array, so drm\_object\_property\_get\_default\_value can be used to
    retrieve it.

struct [drm\_mode\_object](#c.drm_mode_object "drm_mode_object") \*drm\_mode\_object\_find(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_priv, uint32\_t id, uint32\_t type)
:   look up a drm object with static lifetime

**Parameters**

`struct drm_device *dev`
:   drm device

`struct drm_file *file_priv`
:   drm file

`uint32_t id`
:   id of the mode object

`uint32_t type`
:   type of the mode object

**Description**

This function is used to look up a modeset object. It will acquire a
reference for reference counted objects. This reference must be dropped again
by callind [`drm_mode_object_put()`](#c.drm_mode_object_put "drm_mode_object_put").

void drm\_mode\_object\_put(struct [drm\_mode\_object](#c.drm_mode_object "drm_mode_object") \*obj)
:   release a mode object reference

**Parameters**

`struct drm_mode_object *obj`
:   DRM mode object

**Description**

This function decrements the object’s refcount if it is a refcounted modeset
object. It is a no-op on any other object. This is used to drop references
acquired with [`drm_mode_object_get()`](#c.drm_mode_object_get "drm_mode_object_get").

void drm\_mode\_object\_get(struct [drm\_mode\_object](#c.drm_mode_object "drm_mode_object") \*obj)
:   acquire a mode object reference

**Parameters**

`struct drm_mode_object *obj`
:   DRM mode object

**Description**

This function increments the object’s refcount if it is a refcounted modeset
object. It is a no-op on any other object. References should be dropped again
by calling [`drm_mode_object_put()`](#c.drm_mode_object_put "drm_mode_object_put").

void drm\_object\_attach\_property(struct [drm\_mode\_object](#c.drm_mode_object "drm_mode_object") \*obj, struct [drm\_property](#c.drm_property "drm_property") \*property, uint64\_t init\_val)
:   attach a property to a modeset object

**Parameters**

`struct drm_mode_object *obj`
:   drm modeset object

`struct drm_property *property`
:   property to attach

`uint64_t init_val`
:   initial value of the property

**Description**

This attaches the given property to the modeset object with the given initial
value. Currently this function cannot fail since the properties are stored in
a statically sized array.

Note that all properties must be attached before the object itself is
registered and accessible from userspace.

int drm\_object\_property\_set\_value(struct [drm\_mode\_object](#c.drm_mode_object "drm_mode_object") \*obj, struct [drm\_property](#c.drm_property "drm_property") \*property, uint64\_t val)
:   set the value of a property

**Parameters**

`struct drm_mode_object *obj`
:   drm mode object to set property value for

`struct drm_property *property`
:   property to set

`uint64_t val`
:   value the property should be set to

**Description**

This function sets a given property on a given object. This function only
changes the software state of the property, it does not call into the
driver’s ->set\_property callback.

Note that atomic drivers should not have any need to call this, the core will
ensure consistency of values reported back to userspace through the
appropriate ->atomic\_get\_property callback. Only legacy drivers should call
this function to update the tracked value (after clamping and other
restrictions have been applied).

**Return**

Zero on success, error code on failure.

int drm\_object\_property\_get\_value(struct [drm\_mode\_object](#c.drm_mode_object "drm_mode_object") \*obj, struct [drm\_property](#c.drm_property "drm_property") \*property, uint64\_t \*val)
:   retrieve the value of a property

**Parameters**

`struct drm_mode_object *obj`
:   drm mode object to get property value from

`struct drm_property *property`
:   property to retrieve

`uint64_t *val`
:   storage for the property value

**Description**

This function retrieves the softare state of the given property for the given
property. Since there is no driver callback to retrieve the current property
value this might be out of sync with the hardware, depending upon the driver
and property.

Atomic drivers should never call this function directly, the core will read
out property values through the various ->atomic\_get\_property callbacks.

**Return**

Zero on success, error code on failure.

int drm\_object\_property\_get\_default\_value(struct [drm\_mode\_object](#c.drm_mode_object "drm_mode_object") \*obj, struct [drm\_property](#c.drm_property "drm_property") \*property, uint64\_t \*val)
:   retrieve the default value of a property when in atomic mode.

**Parameters**

`struct drm_mode_object *obj`
:   drm mode object to get property value from

`struct drm_property *property`
:   property to retrieve

`uint64_t *val`
:   storage for the property value

**Description**

This function retrieves the default state of the given property as passed in
to drm\_object\_attach\_property

Only atomic drivers should call this function directly, as for non-atomic
drivers it will return the current value.

**Return**

Zero on success, error code on failure.

int drm\_object\_immutable\_property\_get\_value(struct [drm\_mode\_object](#c.drm_mode_object "drm_mode_object") \*obj, struct [drm\_property](#c.drm_property "drm_property") \*property, uint64\_t \*val)
:   retrieve the value of a property

**Parameters**

`struct drm_mode_object *obj`
:   drm mode object to get property value from

`struct drm_property *property`
:   property to retrieve

`uint64_t *val`
:   storage for the property value

**Description**

This function retrieves the software state of the given immutable property
for the given mode object.

This function can be called by both atomic and non-atomic drivers.

**Return**

Zero on success, error code on failure.

## Atomic Mode Setting

![Mode Objects and Properties](../_images/DOT-69c6997054dbfdfb0892fa0cab076cfd8074c7ed.svg)

Mode Objects and Properties

Atomic provides transactional modeset (including planes) updates, but a
bit differently from the usual transactional approach of try-commit and
rollback:

* Firstly, no hardware changes are allowed when the commit would fail. This
  allows us to implement the DRM\_MODE\_ATOMIC\_TEST\_ONLY mode, which allows
  userspace to explore whether certain configurations would work or not.
* This would still allow setting and rollback of just the software state,
  simplifying conversion of existing drivers. But auditing drivers for
  correctness of the atomic\_check code becomes really hard with that: Rolling
  back changes in data structures all over the place is hard to get right.
* Lastly, for backwards compatibility and to support all use-cases, atomic
  updates need to be incremental and be able to execute in parallel. Hardware
  doesn’t always allow it, but where possible plane updates on different CRTCs
  should not interfere, and not get stalled due to output routing changing on
  different CRTCs.

Taken all together there’s two consequences for the atomic design:

* The overall state is split up into per-object state structures:
  [`struct drm_plane_state`](#c.drm_plane_state "drm_plane_state") for planes, [`struct
  drm_crtc_state`](#c.drm_crtc_state "drm_crtc_state") for CRTCs and [`struct
  drm_connector_state`](#c.drm_connector_state "drm_connector_state") for connectors. These are the only
  objects with userspace-visible and settable state. For internal state drivers
  can subclass these structures through embedding, or add entirely new state
  structures for their globally shared hardware functions, see [`struct
  drm_private_state`](#c.drm_private_state "drm_private_state").
* An atomic update is assembled and validated as an entirely free-standing pile
  of structures within the [`drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state")
  container. Driver private state structures are also tracked in the same
  structure; see the next chapter. Only when a state is committed is it applied
  to the driver and modeset objects. This way rolling back an update boils down
  to releasing memory and unreferencing objects like framebuffers.

Locking of atomic state structures is internally using [`struct
drm_modeset_lock`](#c.drm_modeset_lock "drm_modeset_lock"). As a general rule the locking shouldn’t be
exposed to drivers, instead the right locks should be automatically acquired by
any function that duplicates or peeks into a state, like e.g.
[`drm_atomic_get_crtc_state()`](#c.drm_atomic_get_crtc_state "drm_atomic_get_crtc_state"). Locking only protects the software data
structure, ordering of committing state changes to hardware is sequenced using
[`struct drm_crtc_commit`](#c.drm_crtc_commit "drm_crtc_commit").

Read on in this chapter, and also in [Atomic Modeset Helper Functions Reference](drm-kms-helpers.html#drm-atomic-helper) for more detailed
coverage of specific topics.

### Handling Driver Private State

Very often the DRM objects exposed to userspace in the atomic modeset api
([`drm_connector`](#c.drm_connector "drm_connector"), [`drm_crtc`](#c.drm_crtc "drm_crtc") and [`drm_plane`](#c.drm_plane "drm_plane")) do not map neatly to the
underlying hardware. Especially for any kind of shared resources (e.g. shared
clocks, scaler units, bandwidth and fifo limits shared among a group of
planes or CRTCs, and so on) it makes sense to model these as independent
objects. Drivers then need to do similar state tracking and commit ordering for
such private (since not exposed to userspace) objects as the atomic core and
helpers already provide for connectors, planes and CRTCs.

To make this easier on drivers the atomic core provides some support to track
driver private state objects using struct [`drm_private_obj`](#c.drm_private_obj "drm_private_obj"), with the
associated state struct [`drm_private_state`](#c.drm_private_state "drm_private_state").

Similar to userspace-exposed objects, private state structures can be
acquired by calling [`drm_atomic_get_private_obj_state()`](#c.drm_atomic_get_private_obj_state "drm_atomic_get_private_obj_state"). This also takes care
of locking, hence drivers should not have a need to call [`drm_modeset_lock()`](#c.drm_modeset_lock "drm_modeset_lock")
directly. Sequence of the actual hardware state commit is not handled,
drivers might need to keep track of [`struct drm_crtc_commit`](#c.drm_crtc_commit "drm_crtc_commit") within subclassed
structure of [`drm_private_state`](#c.drm_private_state "drm_private_state") as necessary, e.g. similar to
[`drm_plane_state.commit`](#c.drm_plane_state "drm_plane_state"). See also [`drm_atomic_state.fake_commit`](#c.drm_atomic_state "drm_atomic_state").

All private state structures contained in a [`drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") update can be
iterated using [`for_each_oldnew_private_obj_in_state()`](#c.for_each_oldnew_private_obj_in_state "for_each_oldnew_private_obj_in_state"),
[`for_each_new_private_obj_in_state()`](#c.for_each_new_private_obj_in_state "for_each_new_private_obj_in_state") and [`for_each_old_private_obj_in_state()`](#c.for_each_old_private_obj_in_state "for_each_old_private_obj_in_state").
Drivers are recommended to wrap these for each type of driver private state
object they have, filtering on [`drm_private_obj.funcs`](#c.drm_private_obj "drm_private_obj") using [`for_each_if()`](../driver-api/basics.html#c.for_each_if "for_each_if"), at
least if they want to iterate over all objects of a given type.

An earlier way to handle driver private state was by subclassing struct
[`drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state"). But since that encourages non-standard ways to implement
the check/commit split atomic requires (by using e.g. “check and rollback or
commit instead” of “duplicate state, check, then either commit or release
duplicated state) it is deprecated in favour of using [`drm_private_state`](#c.drm_private_state "drm_private_state").

### Atomic Mode Setting Function Reference

struct drm\_crtc\_commit
:   track modeset commits on a CRTC

**Definition**:

```
struct drm_crtc_commit {
    struct drm_crtc *crtc;
    struct kref ref;
    struct completion flip_done;
    struct completion hw_done;
    struct completion cleanup_done;
    struct list_head commit_entry;
    struct drm_pending_vblank_event *event;
    bool abort_completion;
};
```

**Members**

`crtc`
:   DRM CRTC for this commit.

`ref`
:   Reference count for this structure. Needed to allow blocking on
    completions without the risk of the completion disappearing
    meanwhile.

`flip_done`
:   Will be signaled when the hardware has flipped to the new set of
    buffers. Signals at the same time as when the drm event for this
    commit is sent to userspace, or when an out-fence is singalled. Note
    that for most hardware, in most cases this happens after **hw\_done** is
    signalled.

    Completion of this stage is signalled implicitly by calling
    [`drm_crtc_send_vblank_event()`](#c.drm_crtc_send_vblank_event "drm_crtc_send_vblank_event") on [`drm_crtc_state.event`](#c.drm_crtc_state "drm_crtc_state").

`hw_done`
:   Will be signalled when all hw register changes for this commit have
    been written out. Especially when disabling a pipe this can be much
    later than **flip\_done**, since that can signal already when the
    screen goes black, whereas to fully shut down a pipe more register
    I/O is required.

    Note that this does not need to include separately reference-counted
    resources like backing storage buffer pinning, or runtime pm
    management.

    Drivers should call [`drm_atomic_helper_commit_hw_done()`](drm-kms-helpers.html#c.drm_atomic_helper_commit_hw_done "drm_atomic_helper_commit_hw_done") to signal
    completion of this stage.

`cleanup_done`
:   Will be signalled after old buffers have been cleaned up by calling
    [`drm_atomic_helper_cleanup_planes()`](drm-kms-helpers.html#c.drm_atomic_helper_cleanup_planes "drm_atomic_helper_cleanup_planes"). Since this can only happen after
    a vblank wait completed it might be a bit later. This completion is
    useful to throttle updates and avoid hardware updates getting ahead
    of the buffer cleanup too much.

    Drivers should call [`drm_atomic_helper_commit_cleanup_done()`](drm-kms-helpers.html#c.drm_atomic_helper_commit_cleanup_done "drm_atomic_helper_commit_cleanup_done") to signal
    completion of this stage.

`commit_entry`
:   Entry on the per-CRTC [`drm_crtc.commit_list`](#c.drm_crtc "drm_crtc"). Protected by
    $drm\_crtc.commit\_lock.

`event`
:   [`drm_pending_vblank_event`](#c.drm_pending_vblank_event "drm_pending_vblank_event") pointer to clean up private events.

`abort_completion`
:   A flag that’s set after [`drm_atomic_helper_setup_commit()`](drm-kms-helpers.html#c.drm_atomic_helper_setup_commit "drm_atomic_helper_setup_commit") takes a
    second reference for the completion of $drm\_crtc\_state.event. It’s
    used by the free code to remove the second reference if commit fails.

**Description**

This structure is used to track pending modeset changes and atomic commit on
a per-CRTC basis. Since updating the list should never block, this structure
is reference counted to allow waiters to safely wait on an event to complete,
without holding any locks.

It has 3 different events in total to allow a fine-grained synchronization
between outstanding updates:

```
atomic commit thread                    hardware

write new state into hardware   ---->   ...
signal hw_done
                                        switch to new state on next
...                                     v/hblank

wait for buffers to show up             ...

...                                     send completion irq
                                        irq handler signals flip_done
cleanup old buffers

signal cleanup_done

wait for flip_done              <----
clean up atomic state
```

The important bit to know is that `cleanup_done` is the terminal event, but the
ordering between `flip_done` and `hw_done` is entirely up to the specific driver
and modeset state change.

For an implementation of how to use this look at
[`drm_atomic_helper_setup_commit()`](drm-kms-helpers.html#c.drm_atomic_helper_setup_commit "drm_atomic_helper_setup_commit") from the atomic helper library.

See also [`drm_crtc_commit_wait()`](#c.drm_crtc_commit_wait "drm_crtc_commit_wait").

struct drm\_private\_state\_funcs
:   atomic state functions for private objects

**Definition**:

```
struct drm_private_state_funcs {
    struct drm_private_state *(*atomic_create_state)(struct drm_private_obj *obj);
    struct drm_private_state *(*atomic_duplicate_state)(struct drm_private_obj *obj);
    void (*atomic_destroy_state)(struct drm_private_obj *obj, struct drm_private_state *state);
    void (*atomic_print_state)(struct drm_printer *p, const struct drm_private_state *state);
};
```

**Members**

`atomic_create_state`
:   Allocates a pristine, initialized, state for the private
    object and returns it.

    RETURNS:

    A new, pristine, private state instance or an error pointer
    on failure.

`atomic_duplicate_state`
:   Duplicate the current state of the private object and return it. It
    is an error to call this before obj->state has been initialized.

    RETURNS:

    Duplicated atomic state or NULL when obj->state is not
    initialized or allocation failed.

`atomic_destroy_state`
:   Frees the private object state created with **atomic\_duplicate\_state**.

`atomic_print_state`
:   If driver subclasses [`struct drm_private_state`](#c.drm_private_state "drm_private_state"), it should implement
    this optional hook for printing additional driver specific state.

    Do not call this directly, use `drm_atomic_private_obj_print_state()`
    instead.

**Description**

These hooks are used by atomic helpers to create, swap and destroy states of
private objects. The structure itself is used as a vtable to identify the
associated private object type. Each private object type that needs to be
added to the atomic states is expected to have an implementation of these
hooks and pass a pointer to its drm\_private\_state\_funcs `struct to`
[`drm_atomic_get_private_obj_state()`](#c.drm_atomic_get_private_obj_state "drm_atomic_get_private_obj_state").

struct drm\_private\_obj
:   base struct for driver private atomic object

**Definition**:

```
struct drm_private_obj {
    struct drm_device *dev;
    struct list_head head;
    struct drm_modeset_lock lock;
    struct drm_private_state *state;
    const struct drm_private_state_funcs *funcs;
};
```

**Members**

`dev`
:   parent DRM device

`head`
:   List entry used to attach a private object to a [`drm_device`](drm-internals.html#c.drm_device "drm_device")
    (queued to [`drm_mode_config.privobj_list`](#c.drm_mode_config "drm_mode_config")).

`lock`
:   Modeset lock to protect the state object.

`state`
:   Current atomic state for this driver private object.

`funcs`
:   Functions to manipulate the state of this driver private object, see
    [`drm_private_state_funcs`](#c.drm_private_state_funcs "drm_private_state_funcs").

**Description**

A driver private object is initialized by calling
[`drm_atomic_private_obj_init()`](#c.drm_atomic_private_obj_init "drm_atomic_private_obj_init") and cleaned up by calling
[`drm_atomic_private_obj_fini()`](#c.drm_atomic_private_obj_fini "drm_atomic_private_obj_fini").

Currently only tracks the state update functions and the opaque driver
private state itself, but in the future might also track which
[`drm_modeset_lock`](#c.drm_modeset_lock "drm_modeset_lock") is required to duplicate and update this object’s state.

All private objects must be initialized before the DRM device they are
attached to is registered to the DRM subsystem (call to [`drm_dev_register()`](drm-internals.html#c.drm_dev_register "drm_dev_register"))
and should stay around until this DRM device is unregistered (call to
[`drm_dev_unregister()`](drm-internals.html#c.drm_dev_unregister "drm_dev_unregister")). In other words, private objects lifetime is tied
to the DRM device lifetime. This implies that:

1/ all calls to drm\_atomic\_private\_obj\_init() must be done before calling
:   [`drm_dev_register()`](drm-internals.html#c.drm_dev_register "drm_dev_register")

2/ all calls to drm\_atomic\_private\_obj\_fini() must be done after calling
:   [`drm_dev_unregister()`](drm-internals.html#c.drm_dev_unregister "drm_dev_unregister")

If that private object is used to store a state shared by multiple
CRTCs, proper care must be taken to ensure that non-blocking commits are
properly ordered to avoid a use-after-free issue.

Indeed, assuming a sequence of two non-blocking [`drm_atomic_commit`](#c.drm_atomic_commit "drm_atomic_commit") on two
different [`drm_crtc`](#c.drm_crtc "drm_crtc") using different [`drm_plane`](#c.drm_plane "drm_plane") and [`drm_connector`](#c.drm_connector "drm_connector"), so with no
resources shared, there’s no guarantee on which commit is going to happen
first. However, the second [`drm_atomic_commit`](#c.drm_atomic_commit "drm_atomic_commit") will consider the first
[`drm_private_obj`](#c.drm_private_obj "drm_private_obj") its old state, and will be in charge of freeing it whenever
the second [`drm_atomic_commit`](#c.drm_atomic_commit "drm_atomic_commit") is done.

If the first [`drm_atomic_commit`](#c.drm_atomic_commit "drm_atomic_commit") happens after it, it will consider its
[`drm_private_obj`](#c.drm_private_obj "drm_private_obj") the new state and will be likely to access it, resulting in
an access to a freed memory region. Drivers should store (and get a reference
to) the [`drm_crtc_commit`](#c.drm_crtc_commit "drm_crtc_commit") structure in our private state in
[`drm_mode_config_helper_funcs.atomic_commit_setup`](drm-kms-helpers.html#c.drm_mode_config_helper_funcs "drm_mode_config_helper_funcs"), and then wait for that
commit to complete as the first step of
[`drm_mode_config_helper_funcs.atomic_commit_tail`](drm-kms-helpers.html#c.drm_mode_config_helper_funcs "drm_mode_config_helper_funcs"), similar to
[`drm_atomic_helper_wait_for_dependencies()`](drm-kms-helpers.html#c.drm_atomic_helper_wait_for_dependencies "drm_atomic_helper_wait_for_dependencies").

drm\_for\_each\_privobj

`drm_for_each_privobj (privobj, dev)`

> private object iterator

**Parameters**

`privobj`
:   pointer to the current private object. Updated after each
    iteration

`dev`
:   the DRM device we want get private objects from

**Description**

Allows one to iterate over all private objects attached to **dev**

struct drm\_private\_state
:   base struct for driver private object state

**Definition**:

```
struct drm_private_state {
    struct drm_atomic_state *state;
    struct drm_private_obj *obj;
};
```

**Members**

`state`
:   backpointer to global drm\_atomic\_state

`obj`
:   backpointer to the private object

**Description**

Currently only contains a backpointer to the overall atomic update,
and the relevant private object but in the future also might hold
synchronization information similar to e.g. [`drm_crtc.commit`](#c.drm_crtc "drm_crtc").

struct drm\_atomic\_state
:   Atomic commit structure

**Definition**:

```
struct drm_atomic_state {
    struct kref ref;
    struct drm_device *dev;
    bool allow_modeset : 1;
    bool legacy_cursor_update : 1;
    bool async_update : 1;
    bool duplicated : 1;
    bool checked : 1;
    bool plane_color_pipeline : 1;
    struct __drm_colorops_state *colorops;
    struct __drm_planes_state *planes;
    struct __drm_crtcs_state *crtcs;
    int num_connector;
    struct __drm_connnectors_state *connectors;
    int num_private_objs;
    struct __drm_private_objs_state *private_objs;
    struct drm_modeset_acquire_ctx *acquire_ctx;
    struct drm_crtc_commit *fake_commit;
    struct work_struct commit_work;
};
```

**Members**

`ref`
:   Count of all references to this update (will not be freed until zero).

`dev`
:   Parent DRM Device.

`allow_modeset`
:   Allow full modeset. This is used by the ATOMIC IOCTL handler to
    implement the DRM\_MODE\_ATOMIC\_ALLOW\_MODESET flag. Drivers should
    generally not consult this flag, but instead look at the output of
    [`drm_atomic_crtc_needs_modeset()`](#c.drm_atomic_crtc_needs_modeset "drm_atomic_crtc_needs_modeset"). The detailed rules are:

    * Drivers must not consult **allow\_modeset** in the atomic commit path.
      Use [`drm_atomic_crtc_needs_modeset()`](#c.drm_atomic_crtc_needs_modeset "drm_atomic_crtc_needs_modeset") instead.
    * Drivers must consult **allow\_modeset** before adding unrelated [`struct
      drm_crtc_state`](#c.drm_crtc_state "drm_crtc_state") to this commit by calling
      [`drm_atomic_get_crtc_state()`](#c.drm_atomic_get_crtc_state "drm_atomic_get_crtc_state"). See also the warning in the
      documentation for that function.
    * Drivers must never change this flag, it is under the exclusive
      control of userspace.
    * Drivers may consult **allow\_modeset** in the atomic check path, if
      they have the choice between an optimal hardware configuration
      which requires a modeset, and a less optimal configuration which
      can be committed without a modeset. An example would be suboptimal
      scanout FIFO allocation resulting in increased idle power
      consumption. This allows userspace to avoid flickering and delays
      for the normal composition loop at reasonable cost.

`legacy_cursor_update`
:   Hint to enforce legacy cursor IOCTL semantics.

    WARNING: This is thoroughly broken and pretty much impossible to
    implement correctly. Drivers must ignore this and should instead
    implement [`drm_plane_helper_funcs.atomic_async_check`](drm-kms-helpers.html#c.drm_plane_helper_funcs "drm_plane_helper_funcs") and
    [`drm_plane_helper_funcs.atomic_async_commit`](drm-kms-helpers.html#c.drm_plane_helper_funcs "drm_plane_helper_funcs") hooks. New users of this
    flag are not allowed.

`async_update`
:   hint for asynchronous plane update

`duplicated`
:   Indicates whether or not this atomic state was duplicated using
    [`drm_atomic_helper_duplicate_state()`](drm-kms-helpers.html#c.drm_atomic_helper_duplicate_state "drm_atomic_helper_duplicate_state"). Drivers and atomic helpers
    should use this to fixup normal inconsistencies in duplicated
    states.

`checked`
:   Indicates the state has been checked and thus must no longer
    be mutated. For internal use only, do not consult from drivers.

`plane_color_pipeline`
:   Indicates whether this atomic state originated with a client that
    set the DRM\_CLIENT\_CAP\_PLANE\_COLOR\_PIPELINE.

    Drivers and helper functions should use this to ignore legacy
    properties that are incompatible with the drm\_plane COLOR\_PIPELINE
    behavior, such as:

    > * COLOR\_RANGE
    > * COLOR\_ENCODING

    or any other driver-specific properties that might affect pixel
    values.

`colorops`
:   Pointer to array of **drm\_colorop** and **drm\_colorop\_state** part of this
    update.

`planes`
:   Pointer to array of **drm\_plane** and **drm\_plane\_state** part of this
    update.

`crtcs`
:   Pointer to array of **drm\_crtc** and **drm\_crtc\_state** part of this
    update.

`num_connector`
:   size of the **connectors** array

`connectors`
:   Pointer to array of **drm\_connector** and **drm\_connector\_state** part of
    this update.

`num_private_objs`
:   size of the **private\_objs** array

`private_objs`
:   Pointer to array of **drm\_private\_obj** and **drm\_private\_obj\_state** part
    of this update.

`acquire_ctx`
:   acquire context for this atomic modeset state update

`fake_commit`
:   Used for signaling unbound planes/connectors.
    When a connector or plane is not bound to any CRTC, it’s still important
    to preserve linearity to prevent the atomic states from being freed too early.

    This commit (if set) is not bound to any CRTC, but will be completed when
    [`drm_atomic_helper_commit_hw_done()`](drm-kms-helpers.html#c.drm_atomic_helper_commit_hw_done "drm_atomic_helper_commit_hw_done") is called.

`commit_work`
:   Work item which can be used by the driver or helpers to execute the
    commit without blocking.

**Description**

This structure is the kernel counterpart of **drm\_mode\_atomic** and represents
an atomic commit that transitions from an old to a new display state. It
contains all the objects affected by the atomic commit and both the new
state structures and pointers to the old state structures for
these.

States are added to an atomic update by calling [`drm_atomic_get_crtc_state()`](#c.drm_atomic_get_crtc_state "drm_atomic_get_crtc_state"),
[`drm_atomic_get_plane_state()`](#c.drm_atomic_get_plane_state "drm_atomic_get_plane_state"), [`drm_atomic_get_connector_state()`](#c.drm_atomic_get_connector_state "drm_atomic_get_connector_state"), or for
private state structures, [`drm_atomic_get_private_obj_state()`](#c.drm_atomic_get_private_obj_state "drm_atomic_get_private_obj_state").

**NOTE**

[`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") first started as a single collection of
entities state pointers (drm\_plane\_state, drm\_crtc\_state, etc.).

At atomic\_check time, you could get the state about to be committed
from drm\_atomic\_state, and the one currently running from the
entities state pointer (drm\_crtc.state, for example). After the call
to [`drm_atomic_helper_swap_state()`](drm-kms-helpers.html#c.drm_atomic_helper_swap_state "drm_atomic_helper_swap_state"), the entities state pointer would
contain the state previously checked, and the drm\_atomic\_state
structure the old state.

Over time, and in order to avoid confusion, drm\_atomic\_state has
grown to have both the old state (ie, the state we replace) and the
new state (ie, the state we want to apply). Those names are stable
during the commit process, which makes it easier to reason about.

You can still find some traces of that evolution through some hooks
or callbacks taking a drm\_atomic\_state parameter called names like
“old\_state”. This doesn’t necessarily mean that the previous
drm\_atomic\_state is passed, but rather that this used to be the state
collection we were replacing after [`drm_atomic_helper_swap_state()`](drm-kms-helpers.html#c.drm_atomic_helper_swap_state "drm_atomic_helper_swap_state"),
but the variable name was never updated.

Some atomic operations implementations followed a similar process. We
first started to pass the entity state only. However, it was pretty
cumbersome for drivers, and especially CRTCs, to retrieve the states
of other components. Thus, we switched to passing the whole
drm\_atomic\_state as a parameter to those operations. Similarly, the
transition isn’t complete yet, and one might still find atomic
operations taking a drm\_atomic\_state pointer, or a component state
pointer. The former is the preferred form.

struct [drm\_crtc\_commit](#c.drm_crtc_commit "drm_crtc_commit") \*drm\_crtc\_commit\_get(struct [drm\_crtc\_commit](#c.drm_crtc_commit "drm_crtc_commit") \*commit)
:   acquire a reference to the CRTC commit

**Parameters**

`struct drm_crtc_commit *commit`
:   CRTC commit

**Description**

Increases the reference of **commit**.

**Return**

The pointer to **commit**, with reference increased.

void drm\_crtc\_commit\_put(struct [drm\_crtc\_commit](#c.drm_crtc_commit "drm_crtc_commit") \*commit)
:   release a reference to the CRTC commmit

**Parameters**

`struct drm_crtc_commit *commit`
:   CRTC commit

**Description**

This releases a reference to **commit** which is freed after removing the
final reference. No locking required and callable from any context.

struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*drm\_atomic\_state\_get(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state)
:   acquire a reference to the atomic state

**Parameters**

`struct drm_atomic_state *state`
:   The atomic state

**Description**

Returns a new reference to the **state**

void drm\_atomic\_state\_put(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state)
:   release a reference to the atomic state

**Parameters**

`struct drm_atomic_state *state`
:   The atomic state

**Description**

This releases a reference to **state** which is freed after removing the
final reference. No locking required and callable from any context.

struct [drm\_crtc\_state](#c.drm_crtc_state "drm_crtc_state") \*drm\_atomic\_get\_old\_crtc\_state(const struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   get old CRTC state, if it exists

**Parameters**

`const struct drm_atomic_state *state`
:   global atomic state object

`struct drm_crtc *crtc`
:   CRTC to grab

**Description**

This function returns the old CRTC state for the given CRTC, or
NULL if the CRTC is not part of the global atomic state.

struct [drm\_crtc\_state](#c.drm_crtc_state "drm_crtc_state") \*drm\_atomic\_get\_new\_crtc\_state(const struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   get new CRTC state, if it exists

**Parameters**

`const struct drm_atomic_state *state`
:   global atomic state object

`struct drm_crtc *crtc`
:   CRTC to grab

**Description**

This function returns the new CRTC state for the given CRTC, or
NULL if the CRTC is not part of the global atomic state.

struct [drm\_plane\_state](#c.drm_plane_state "drm_plane_state") \*drm\_atomic\_get\_old\_plane\_state(const struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_plane](#c.drm_plane "drm_plane") \*plane)
:   get plane state, if it exists

**Parameters**

`const struct drm_atomic_state *state`
:   global atomic state object

`struct drm_plane *plane`
:   plane to grab

**Description**

This function returns the old plane state for the given plane, or
NULL if the plane is not part of the global atomic state.

struct [drm\_plane\_state](#c.drm_plane_state "drm_plane_state") \*drm\_atomic\_get\_new\_plane\_state(const struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_plane](#c.drm_plane "drm_plane") \*plane)
:   get plane state, if it exists

**Parameters**

`const struct drm_atomic_state *state`
:   global atomic state object

`struct drm_plane *plane`
:   plane to grab

**Description**

This function returns the new plane state for the given plane, or
NULL if the plane is not part of the global atomic state.

struct [drm\_connector\_state](#c.drm_connector_state "drm_connector_state") \*drm\_atomic\_get\_old\_connector\_state(const struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   get connector state, if it exists

**Parameters**

`const struct drm_atomic_state *state`
:   global atomic state object

`struct drm_connector *connector`
:   connector to grab

**Description**

This function returns the old connector state for the given connector,
or NULL if the connector is not part of the global atomic state.

struct [drm\_connector\_state](#c.drm_connector_state "drm_connector_state") \*drm\_atomic\_get\_new\_connector\_state(const struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   get connector state, if it exists

**Parameters**

`const struct drm_atomic_state *state`
:   global atomic state object

`struct drm_connector *connector`
:   connector to grab

**Description**

This function returns the new connector state for the given connector,
or NULL if the connector is not part of the global atomic state.

const struct [drm\_plane\_state](#c.drm_plane_state "drm_plane_state") \*\_\_drm\_atomic\_get\_current\_plane\_state(const struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_plane](#c.drm_plane "drm_plane") \*plane)
:   get current plane state

**Parameters**

`const struct drm_atomic_state *state`
:   global atomic state object

`struct drm_plane *plane`
:   plane to grab

**Description**

This function returns the plane state for the given plane, either the
new plane state from **state**, or if the plane isn’t part of the atomic
state update, from **plane**. This is useful in atomic check callbacks,
when drivers need to peek at, but not change, state of other planes,
since it avoids threading an error code back up the call chain.

WARNING:

Note that this function is in general unsafe since it doesn’t check for the
required locking for access state structures. Drivers must ensure that it is
safe to access the returned state structure through other means. One common
example is when planes are fixed to a single CRTC, and the driver knows that
the CRTC lock is held already. In that case holding the CRTC lock gives a
read-lock on all planes connected to that CRTC. But if planes can be
reassigned things get more tricky. In that case it’s better to use
drm\_atomic\_get\_plane\_state and wire up full error handling.

Read-only pointer to the current plane state.

for\_each\_oldnew\_connector\_in\_state

`for_each_oldnew_connector_in_state (__state, connector, old_connector_state, new_connector_state, __i)`

> iterate over all connectors in an atomic update

**Parameters**

`__state`
:   [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") pointer

`connector`
:   [`struct drm_connector`](#c.drm_connector "drm_connector") iteration cursor

`old_connector_state`
:   [`struct drm_connector_state`](#c.drm_connector_state "drm_connector_state") iteration cursor for the
    old state

`new_connector_state`
:   [`struct drm_connector_state`](#c.drm_connector_state "drm_connector_state") iteration cursor for the
    new state

`__i`
:   int iteration cursor, for macro-internal use

**Description**

This iterates over all connectors in an atomic update, tracking both old and
new state. This is useful in places where the state delta needs to be
considered, for example in atomic check functions.

for\_each\_old\_connector\_in\_state

`for_each_old_connector_in_state (__state, connector, old_connector_state, __i)`

> iterate over all connectors in an atomic update

**Parameters**

`__state`
:   [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") pointer

`connector`
:   [`struct drm_connector`](#c.drm_connector "drm_connector") iteration cursor

`old_connector_state`
:   [`struct drm_connector_state`](#c.drm_connector_state "drm_connector_state") iteration cursor for the
    old state

`__i`
:   int iteration cursor, for macro-internal use

**Description**

This iterates over all connectors in an atomic update, tracking only the old
state. This is useful in disable functions, where we need the old state the
hardware is still in.

for\_each\_new\_connector\_in\_state

`for_each_new_connector_in_state (__state, connector, new_connector_state, __i)`

> iterate over all connectors in an atomic update

**Parameters**

`__state`
:   [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") pointer

`connector`
:   [`struct drm_connector`](#c.drm_connector "drm_connector") iteration cursor

`new_connector_state`
:   [`struct drm_connector_state`](#c.drm_connector_state "drm_connector_state") iteration cursor for the
    new state

`__i`
:   int iteration cursor, for macro-internal use

**Description**

This iterates over all connectors in an atomic update, tracking only the new
state. This is useful in enable functions, where we need the new state the
hardware should be in when the atomic commit operation has completed.

for\_each\_oldnew\_crtc\_in\_state

`for_each_oldnew_crtc_in_state (__state, crtc, old_crtc_state, new_crtc_state, __i)`

> iterate over all CRTCs in an atomic update

**Parameters**

`__state`
:   [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") pointer

`crtc`
:   [`struct drm_crtc`](#c.drm_crtc "drm_crtc") iteration cursor

`old_crtc_state`
:   [`struct drm_crtc_state`](#c.drm_crtc_state "drm_crtc_state") iteration cursor for the old state

`new_crtc_state`
:   [`struct drm_crtc_state`](#c.drm_crtc_state "drm_crtc_state") iteration cursor for the new state

`__i`
:   int iteration cursor, for macro-internal use

**Description**

This iterates over all CRTCs in an atomic update, tracking both old and
new state. This is useful in places where the state delta needs to be
considered, for example in atomic check functions.

for\_each\_old\_crtc\_in\_state

`for_each_old_crtc_in_state (__state, crtc, old_crtc_state, __i)`

> iterate over all CRTCs in an atomic update

**Parameters**

`__state`
:   [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") pointer

`crtc`
:   [`struct drm_crtc`](#c.drm_crtc "drm_crtc") iteration cursor

`old_crtc_state`
:   [`struct drm_crtc_state`](#c.drm_crtc_state "drm_crtc_state") iteration cursor for the old state

`__i`
:   int iteration cursor, for macro-internal use

**Description**

This iterates over all CRTCs in an atomic update, tracking only the old
state. This is useful in disable functions, where we need the old state the
hardware is still in.

for\_each\_new\_crtc\_in\_state

`for_each_new_crtc_in_state (__state, crtc, new_crtc_state, __i)`

> iterate over all CRTCs in an atomic update

**Parameters**

`__state`
:   [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") pointer

`crtc`
:   [`struct drm_crtc`](#c.drm_crtc "drm_crtc") iteration cursor

`new_crtc_state`
:   [`struct drm_crtc_state`](#c.drm_crtc_state "drm_crtc_state") iteration cursor for the new state

`__i`
:   int iteration cursor, for macro-internal use

**Description**

This iterates over all CRTCs in an atomic update, tracking only the new
state. This is useful in enable functions, where we need the new state the
hardware should be in when the atomic commit operation has completed.

for\_each\_oldnew\_colorop\_in\_state

`for_each_oldnew_colorop_in_state (__state, colorop, old_colorop_state, new_colorop_state, __i)`

> iterate over all colorops in an atomic update

**Parameters**

`__state`
:   [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") pointer

`colorop`
:   [`struct drm_colorop`](#c.drm_colorop "drm_colorop") iteration cursor

`old_colorop_state`
:   [`struct drm_colorop_state`](#c.drm_colorop_state "drm_colorop_state") iteration cursor for the old state

`new_colorop_state`
:   [`struct drm_colorop_state`](#c.drm_colorop_state "drm_colorop_state") iteration cursor for the new state

`__i`
:   int iteration cursor, for macro-internal use

**Description**

This iterates over all colorops in an atomic update, tracking both old and
new state. This is useful in places where the state delta needs to be
considered, for example in atomic check functions.

for\_each\_new\_colorop\_in\_state

`for_each_new_colorop_in_state (__state, colorop, new_colorop_state, __i)`

> iterate over all colorops in an atomic update

**Parameters**

`__state`
:   [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") pointer

`colorop`
:   [`struct drm_colorop`](#c.drm_colorop "drm_colorop") iteration cursor

`new_colorop_state`
:   [`struct drm_colorop_state`](#c.drm_colorop_state "drm_colorop_state") iteration cursor for the new state

`__i`
:   int iteration cursor, for macro-internal use

**Description**

This iterates over all colorops in an atomic update, tracking new state. This is
useful in places where the state delta needs to be considered, for example in
atomic check functions.

for\_each\_oldnew\_plane\_in\_state

`for_each_oldnew_plane_in_state (__state, plane, old_plane_state, new_plane_state, __i)`

> iterate over all planes in an atomic update

**Parameters**

`__state`
:   [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") pointer

`plane`
:   [`struct drm_plane`](#c.drm_plane "drm_plane") iteration cursor

`old_plane_state`
:   [`struct drm_plane_state`](#c.drm_plane_state "drm_plane_state") iteration cursor for the old state

`new_plane_state`
:   [`struct drm_plane_state`](#c.drm_plane_state "drm_plane_state") iteration cursor for the new state

`__i`
:   int iteration cursor, for macro-internal use

**Description**

This iterates over all planes in an atomic update, tracking both old and
new state. This is useful in places where the state delta needs to be
considered, for example in atomic check functions.

for\_each\_oldnew\_plane\_in\_state\_reverse

`for_each_oldnew_plane_in_state_reverse (__state, plane, old_plane_state, new_plane_state, __i)`

> iterate over all planes in an atomic update in reverse order

**Parameters**

`__state`
:   [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") pointer

`plane`
:   [`struct drm_plane`](#c.drm_plane "drm_plane") iteration cursor

`old_plane_state`
:   [`struct drm_plane_state`](#c.drm_plane_state "drm_plane_state") iteration cursor for the old state

`new_plane_state`
:   [`struct drm_plane_state`](#c.drm_plane_state "drm_plane_state") iteration cursor for the new state

`__i`
:   int iteration cursor, for macro-internal use

**Description**

This iterates over all planes in an atomic update in reverse order,
tracking both old and new state. This is useful in places where the
state delta needs to be considered, for example in atomic check functions.

for\_each\_new\_plane\_in\_state\_reverse

`for_each_new_plane_in_state_reverse (__state, plane, new_plane_state, __i)`

> other than only tracking new state, it’s the same as for\_each\_oldnew\_plane\_in\_state\_reverse

**Parameters**

`__state`
:   [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") pointer

`plane`
:   [`struct drm_plane`](#c.drm_plane "drm_plane") iteration cursor

`new_plane_state`
:   [`struct drm_plane_state`](#c.drm_plane_state "drm_plane_state") iteration cursor for the new state

`__i`
:   int iteration cursor, for macro-internal use

for\_each\_old\_plane\_in\_state

`for_each_old_plane_in_state (__state, plane, old_plane_state, __i)`

> iterate over all planes in an atomic update

**Parameters**

`__state`
:   [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") pointer

`plane`
:   [`struct drm_plane`](#c.drm_plane "drm_plane") iteration cursor

`old_plane_state`
:   [`struct drm_plane_state`](#c.drm_plane_state "drm_plane_state") iteration cursor for the old state

`__i`
:   int iteration cursor, for macro-internal use

**Description**

This iterates over all planes in an atomic update, tracking only the old
state. This is useful in disable functions, where we need the old state the
hardware is still in.

for\_each\_new\_plane\_in\_state

`for_each_new_plane_in_state (__state, plane, new_plane_state, __i)`

> iterate over all planes in an atomic update

**Parameters**

`__state`
:   [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") pointer

`plane`
:   [`struct drm_plane`](#c.drm_plane "drm_plane") iteration cursor

`new_plane_state`
:   [`struct drm_plane_state`](#c.drm_plane_state "drm_plane_state") iteration cursor for the new state

`__i`
:   int iteration cursor, for macro-internal use

**Description**

This iterates over all planes in an atomic update, tracking only the new
state. This is useful in enable functions, where we need the new state the
hardware should be in when the atomic commit operation has completed.

for\_each\_oldnew\_private\_obj\_in\_state

`for_each_oldnew_private_obj_in_state (__state, obj, old_obj_state, new_obj_state, __i)`

> iterate over all private objects in an atomic update

**Parameters**

`__state`
:   [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") pointer

`obj`
:   [`struct drm_private_obj`](#c.drm_private_obj "drm_private_obj") iteration cursor

`old_obj_state`
:   [`struct drm_private_state`](#c.drm_private_state "drm_private_state") iteration cursor for the old state

`new_obj_state`
:   [`struct drm_private_state`](#c.drm_private_state "drm_private_state") iteration cursor for the new state

`__i`
:   int iteration cursor, for macro-internal use

**Description**

This iterates over all private objects in an atomic update, tracking both
old and new state. This is useful in places where the state delta needs
to be considered, for example in atomic check functions.

for\_each\_old\_private\_obj\_in\_state

`for_each_old_private_obj_in_state (__state, obj, old_obj_state, __i)`

> iterate over all private objects in an atomic update

**Parameters**

`__state`
:   [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") pointer

`obj`
:   [`struct drm_private_obj`](#c.drm_private_obj "drm_private_obj") iteration cursor

`old_obj_state`
:   [`struct drm_private_state`](#c.drm_private_state "drm_private_state") iteration cursor for the old state

`__i`
:   int iteration cursor, for macro-internal use

**Description**

This iterates over all private objects in an atomic update, tracking only
the old state. This is useful in disable functions, where we need the old
state the hardware is still in.

for\_each\_new\_private\_obj\_in\_state

`for_each_new_private_obj_in_state (__state, obj, new_obj_state, __i)`

> iterate over all private objects in an atomic update

**Parameters**

`__state`
:   [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") pointer

`obj`
:   [`struct drm_private_obj`](#c.drm_private_obj "drm_private_obj") iteration cursor

`new_obj_state`
:   [`struct drm_private_state`](#c.drm_private_state "drm_private_state") iteration cursor for the new state

`__i`
:   int iteration cursor, for macro-internal use

**Description**

This iterates over all private objects in an atomic update, tracking only
the new state. This is useful in enable functions, where we need the new state the
hardware should be in when the atomic commit operation has completed.

bool drm\_atomic\_crtc\_needs\_modeset(const struct [drm\_crtc\_state](#c.drm_crtc_state "drm_crtc_state") \*state)
:   compute combined modeset need

**Parameters**

`const struct drm_crtc_state *state`
:   [`drm_crtc_state`](#c.drm_crtc_state "drm_crtc_state") for the CRTC

**Description**

To give drivers flexibility [`struct drm_crtc_state`](#c.drm_crtc_state "drm_crtc_state") has 3 booleans to track
whether the state CRTC changed enough to need a full modeset cycle:
mode\_changed, active\_changed and connectors\_changed. This helper simply
combines these three to compute the overall need for a modeset for **state**.

The atomic helper code sets these booleans, but drivers can and should
change them appropriately to accurately represent whether a modeset is
really needed. In general, drivers should avoid full modesets whenever
possible.

For example if the CRTC mode has changed, and the hardware is able to enact
the requested mode change without going through a full modeset, the driver
should clear mode\_changed in its [`drm_mode_config_funcs.atomic_check`](#c.drm_mode_config_funcs "drm_mode_config_funcs")
implementation.

bool drm\_atomic\_crtc\_effectively\_active(const struct [drm\_crtc\_state](#c.drm_crtc_state "drm_crtc_state") \*state)
:   compute whether CRTC is actually active

**Parameters**

`const struct drm_crtc_state *state`
:   [`drm_crtc_state`](#c.drm_crtc_state "drm_crtc_state") for the CRTC

**Description**

When in self refresh mode, the crtc\_state->active value will be false, since
the CRTC is off. However in some cases we’re interested in whether the CRTC
is active, or effectively active (ie: it’s connected to an active display).
In these cases, use this function instead of just checking active.

struct drm\_bus\_cfg
:   bus configuration

**Definition**:

```
struct drm_bus_cfg {
    u32 format;
    u32 flags;
};
```

**Members**

`format`
:   format used on this bus (one of the MEDIA\_BUS\_FMT\_\* format)

    This field should not be directly modified by drivers
    (`drm_atomic_bridge_chain_select_bus_fmts()` takes care of the bus
    format negotiation).

`flags`
:   DRM\_BUS\_\* flags used on this bus

**Description**

This structure stores the configuration of a physical bus between two
components in an output pipeline, usually between two bridges, an encoder
and a bridge, or a bridge and a connector.

The bus configuration is stored in [`drm_bridge_state`](#c.drm_bridge_state "drm_bridge_state") separately for the
input and output buses, as seen from the point of view of each bridge. The
bus configuration of a bridge output is usually identical to the
configuration of the next bridge’s input, but may differ if the signals are
modified between the two bridges, for instance by an inverter on the board.
The input and output configurations of a bridge may differ if the bridge
modifies the signals internally, for instance by performing format
conversion, or modifying signals polarities.

struct drm\_bridge\_state
:   Atomic bridge state object

**Definition**:

```
struct drm_bridge_state {
    struct drm_private_state base;
    struct drm_bridge *bridge;
    struct drm_bus_cfg input_bus_cfg;
    struct drm_bus_cfg output_bus_cfg;
};
```

**Members**

`base`
:   inherit from [`drm_private_state`](#c.drm_private_state "drm_private_state")

`bridge`
:   the bridge this state refers to

`input_bus_cfg`
:   input bus configuration

`output_bus_cfg`
:   output bus configuration

int drm\_crtc\_commit\_wait(struct [drm\_crtc\_commit](#c.drm_crtc_commit "drm_crtc_commit") \*commit)
:   Waits for a commit to complete

**Parameters**

`struct drm_crtc_commit *commit`
:   [`drm_crtc_commit`](#c.drm_crtc_commit "drm_crtc_commit") to wait for

**Description**

Waits for a given [`drm_crtc_commit`](#c.drm_crtc_commit "drm_crtc_commit") to be programmed into the
hardware and flipped to.

**Return**

0 on success, a negative error code otherwise.

void drm\_atomic\_state\_default\_release(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state)
:   release memory initialized by drm\_atomic\_state\_init

**Parameters**

`struct drm_atomic_state *state`
:   atomic state

**Description**

Free all the memory allocated by drm\_atomic\_state\_init.
This should only be used by drivers which are still subclassing
[`drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") and haven’t switched to [`drm_private_state`](#c.drm_private_state "drm_private_state") yet.

int drm\_atomic\_state\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state)
:   init new atomic state

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   atomic state

**Description**

Default implementation for filling in a new atomic state.
This should only be used by drivers which are still subclassing
[`drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") and haven’t switched to [`drm_private_state`](#c.drm_private_state "drm_private_state") yet.

struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*drm\_atomic\_state\_alloc(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   allocate atomic state

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

This allocates an empty atomic state to track updates.

void drm\_atomic\_state\_default\_clear(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state)
:   clear base atomic state

**Parameters**

`struct drm_atomic_state *state`
:   atomic state

**Description**

Default implementation for clearing atomic state.
This should only be used by drivers which are still subclassing
[`drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") and haven’t switched to [`drm_private_state`](#c.drm_private_state "drm_private_state") yet.

void drm\_atomic\_state\_clear(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state)
:   clear state object

**Parameters**

`struct drm_atomic_state *state`
:   atomic state

**Description**

When the w/w mutex algorithm detects a deadlock we need to back off and drop
all locks. So someone else could sneak in and change the current modeset
configuration. Which means that all the state assembled in **state** is no
longer an atomic update to the current state, but to some arbitrary earlier
state. Which could break assumptions the driver’s
[`drm_mode_config_funcs.atomic_check`](#c.drm_mode_config_funcs "drm_mode_config_funcs") likely relies on.

Hence we must clear all cached state and completely start over, using this
function.

void \_\_drm\_atomic\_state\_free(struct kref \*ref)
:   free all memory for an atomic state

**Parameters**

`struct kref *ref`
:   This atomic state to deallocate

**Description**

This frees all memory associated with an atomic state, including all the
per-object state for planes, CRTCs and connectors.

struct [drm\_crtc\_state](#c.drm_crtc_state "drm_crtc_state") \*drm\_atomic\_get\_crtc\_state(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   get CRTC state

**Parameters**

`struct drm_atomic_state *state`
:   global atomic state object

`struct drm_crtc *crtc`
:   CRTC to get state object for

**Description**

This function returns the CRTC state for the given CRTC, allocating it if
needed. It will also grab the relevant CRTC lock to make sure that the state
is consistent.

WARNING: Drivers may only add new CRTC states to a **state** if
drm\_atomic\_state.allow\_modeset is set, or if it’s a driver-internal commit
not created by userspace through an IOCTL call.

**Return**

Either the allocated state or the error code encoded into the pointer. When
the error is EDEADLK then the w/w mutex code has detected a deadlock and the
entire atomic sequence must be restarted. All other errors are fatal.

struct [drm\_plane\_state](#c.drm_plane_state "drm_plane_state") \*drm\_atomic\_get\_plane\_state(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_plane](#c.drm_plane "drm_plane") \*plane)
:   get plane state

**Parameters**

`struct drm_atomic_state *state`
:   global atomic state object

`struct drm_plane *plane`
:   plane to get state object for

**Description**

This function returns the plane state for the given plane, allocating it if
needed. It will also grab the relevant plane lock to make sure that the state
is consistent.

**Return**

Either the allocated state or the error code encoded into the pointer. When
the error is EDEADLK then the w/w mutex code has detected a deadlock and the
entire atomic sequence must be restarted. All other errors are fatal.

struct [drm\_colorop\_state](#c.drm_colorop_state "drm_colorop_state") \*drm\_atomic\_get\_colorop\_state(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_colorop](#c.drm_colorop "drm_colorop") \*colorop)
:   get colorop state

**Parameters**

`struct drm_atomic_state *state`
:   global atomic state object

`struct drm_colorop *colorop`
:   colorop to get state object for

**Description**

This function returns the colorop state for the given colorop, allocating it
if needed. It will also grab the relevant plane lock to make sure that the
state is consistent.

Either the allocated state or the error code encoded into the pointer. When
the error is EDEADLK then the w/w mutex code has detected a deadlock and the
entire atomic sequence must be restarted. All other errors are fatal.

struct [drm\_colorop\_state](#c.drm_colorop_state "drm_colorop_state") \*drm\_atomic\_get\_old\_colorop\_state(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_colorop](#c.drm_colorop "drm_colorop") \*colorop)
:   get colorop state, if it exists

**Parameters**

`struct drm_atomic_state *state`
:   global atomic state object

`struct drm_colorop *colorop`
:   colorop to grab

**Description**

This function returns the old colorop state for the given colorop, or
NULL if the colorop is not part of the global atomic state.

struct [drm\_colorop\_state](#c.drm_colorop_state "drm_colorop_state") \*drm\_atomic\_get\_new\_colorop\_state(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_colorop](#c.drm_colorop "drm_colorop") \*colorop)
:   get colorop state, if it exists

**Parameters**

`struct drm_atomic_state *state`
:   global atomic state object

`struct drm_colorop *colorop`
:   colorop to grab

**Description**

This function returns the new colorop state for the given colorop, or
NULL if the colorop is not part of the global atomic state.

int drm\_atomic\_private\_obj\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_private\_obj](#c.drm_private_obj "drm_private_obj") \*obj, const struct [drm\_private\_state\_funcs](#c.drm_private_state_funcs "drm_private_state_funcs") \*funcs)
:   initialize private object

**Parameters**

`struct drm_device *dev`
:   DRM device this object will be attached to

`struct drm_private_obj *obj`
:   private object

`const struct drm_private_state_funcs *funcs`
:   pointer to the `struct of` function pointers that identify the object
    type

**Description**

Initialize the private object, which can be embedded into any
driver private object that needs its own atomic state.

**Return**

Zero on success, error code on failure

void drm\_atomic\_private\_obj\_fini(struct [drm\_private\_obj](#c.drm_private_obj "drm_private_obj") \*obj)
:   finalize private object

**Parameters**

`struct drm_private_obj *obj`
:   private object

**Description**

Finalize the private object.

struct [drm\_private\_state](#c.drm_private_state "drm_private_state") \*drm\_atomic\_get\_private\_obj\_state(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_private\_obj](#c.drm_private_obj "drm_private_obj") \*obj)
:   get private object state

**Parameters**

`struct drm_atomic_state *state`
:   global atomic state

`struct drm_private_obj *obj`
:   private object to get the state for

**Description**

This function returns the private object state for the given private object,
allocating the state if needed. It will also grab the relevant private
object lock to make sure that the state is consistent.

**Return**

Either the allocated state or the error code encoded into a pointer.

struct [drm\_private\_state](#c.drm_private_state "drm_private_state") \*drm\_atomic\_get\_old\_private\_obj\_state(const struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_private\_obj](#c.drm_private_obj "drm_private_obj") \*obj)

**Parameters**

`const struct drm_atomic_state *state`
:   global atomic state object

`struct drm_private_obj *obj`
:   private\_obj to grab

**Description**

This function returns the old private object state for the given private\_obj,
or NULL if the private\_obj is not part of the global atomic state.

struct [drm\_private\_state](#c.drm_private_state "drm_private_state") \*drm\_atomic\_get\_new\_private\_obj\_state(const struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_private\_obj](#c.drm_private_obj "drm_private_obj") \*obj)

**Parameters**

`const struct drm_atomic_state *state`
:   global atomic state object

`struct drm_private_obj *obj`
:   private\_obj to grab

**Description**

This function returns the new private object state for the given private\_obj,
or NULL if the private\_obj is not part of the global atomic state.

struct [drm\_connector](#c.drm_connector "drm_connector") \*drm\_atomic\_get\_old\_connector\_for\_encoder(const struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_encoder](#c.drm_encoder "drm_encoder") \*encoder)
:   Get old connector for an encoder

**Parameters**

`const struct drm_atomic_state *state`
:   Atomic state

`struct drm_encoder *encoder`
:   The encoder to fetch the connector state for

**Description**

This function finds and returns the connector that was connected to **encoder**
as specified by the **state**.

If there is no connector in **state** which previously had **encoder** connected to
it, this function will return NULL. While this may seem like an invalid use
case, it is sometimes useful to differentiate commits which had no prior
connectors attached to **encoder** vs ones that did (and to inspect their
state). This is especially true in enable hooks because the pipeline has
changed.

If you don’t have access to the atomic state, see
[`drm_atomic_get_connector_for_encoder()`](#c.drm_atomic_get_connector_for_encoder "drm_atomic_get_connector_for_encoder").

**Return**

The old connector connected to **encoder**, or NULL if the encoder is
not connected.

struct [drm\_connector](#c.drm_connector "drm_connector") \*drm\_atomic\_get\_new\_connector\_for\_encoder(const struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_encoder](#c.drm_encoder "drm_encoder") \*encoder)
:   Get new connector for an encoder

**Parameters**

`const struct drm_atomic_state *state`
:   Atomic state

`struct drm_encoder *encoder`
:   The encoder to fetch the connector state for

**Description**

This function finds and returns the connector that will be connected to
**encoder** as specified by the **state**.

If there is no connector in **state** which will have **encoder** connected to it,
this function will return NULL. While this may seem like an invalid use case,
it is sometimes useful to differentiate commits which have no connectors
attached to **encoder** vs ones that do (and to inspect their state). This is
especially true in disable hooks because the pipeline will change.

If you don’t have access to the atomic state, see
[`drm_atomic_get_connector_for_encoder()`](#c.drm_atomic_get_connector_for_encoder "drm_atomic_get_connector_for_encoder").

**Return**

The new connector connected to **encoder**, or NULL if the encoder is
not connected.

struct [drm\_connector](#c.drm_connector "drm_connector") \*drm\_atomic\_get\_connector\_for\_encoder(const struct [drm\_encoder](#c.drm_encoder "drm_encoder") \*encoder, struct [drm\_modeset\_acquire\_ctx](#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx)
:   Get connector currently assigned to an encoder

**Parameters**

`const struct drm_encoder *encoder`
:   The encoder to find the connector of

`struct drm_modeset_acquire_ctx *ctx`
:   Modeset locking context

**Description**

This function finds and returns the connector currently assigned to
an **encoder**.

It is similar to the [`drm_atomic_get_old_connector_for_encoder()`](#c.drm_atomic_get_old_connector_for_encoder "drm_atomic_get_old_connector_for_encoder") and
[`drm_atomic_get_new_connector_for_encoder()`](#c.drm_atomic_get_new_connector_for_encoder "drm_atomic_get_new_connector_for_encoder") helpers, but doesn’t
require access to the atomic state. If you have access to it, prefer
using these. This helper is typically useful in situations where you
don’t have access to the atomic state, like detect, link repair,
threaded interrupt handlers, or hooks from other frameworks (ALSA,
CEC, etc.).

**Return**

The connector connected to **encoder**, or an error pointer otherwise.
When the error is EDEADLK, a deadlock has been detected and the
sequence must be restarted.

struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*drm\_atomic\_get\_old\_crtc\_for\_encoder(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_encoder](#c.drm_encoder "drm_encoder") \*encoder)
:   Get old crtc for an encoder

**Parameters**

`struct drm_atomic_state *state`
:   Atomic state

`struct drm_encoder *encoder`
:   The encoder to fetch the crtc state for

**Description**

This function finds and returns the crtc that was connected to **encoder**
as specified by the **state**.

**Return**

The old crtc connected to **encoder**, or NULL if the encoder is
not connected.

struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*drm\_atomic\_get\_new\_crtc\_for\_encoder(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_encoder](#c.drm_encoder "drm_encoder") \*encoder)
:   Get new crtc for an encoder

**Parameters**

`struct drm_atomic_state *state`
:   Atomic state

`struct drm_encoder *encoder`
:   The encoder to fetch the crtc state for

**Description**

This function finds and returns the crtc that will be connected to **encoder**
as specified by the **state**.

**Return**

The new crtc connected to **encoder**, or NULL if the encoder is
not connected.

struct [drm\_connector\_state](#c.drm_connector_state "drm_connector_state") \*drm\_atomic\_get\_connector\_state(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   get connector state

**Parameters**

`struct drm_atomic_state *state`
:   global atomic state object

`struct drm_connector *connector`
:   connector to get state object for

**Description**

This function returns the connector state for the given connector,
allocating it if needed. It will also grab the relevant connector lock to
make sure that the state is consistent.

**Return**

Either the allocated state or the error code encoded into the pointer. When
the error is EDEADLK then the w/w mutex code has detected a deadlock and the
entire atomic sequence must be restarted. All other errors are fatal.

struct [drm\_bridge\_state](#c.drm_bridge_state "drm_bridge_state") \*drm\_atomic\_get\_bridge\_state(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_bridge](drm-kms-helpers.html#c.drm_bridge "drm_bridge") \*bridge)
:   get bridge state

**Parameters**

`struct drm_atomic_state *state`
:   global atomic state object

`struct drm_bridge *bridge`
:   bridge to get state object for

**Description**

This function returns the bridge state for the given bridge, allocating it
if needed. It will also grab the relevant bridge lock to make sure that the
state is consistent.

**Return**

Either the allocated state or the error code encoded into the pointer. When
the error is EDEADLK then the w/w mutex code has detected a deadlock and the
entire atomic sequence must be restarted.

struct [drm\_bridge\_state](#c.drm_bridge_state "drm_bridge_state") \*drm\_atomic\_get\_old\_bridge\_state(const struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_bridge](drm-kms-helpers.html#c.drm_bridge "drm_bridge") \*bridge)
:   get old bridge state, if it exists

**Parameters**

`const struct drm_atomic_state *state`
:   global atomic state object

`struct drm_bridge *bridge`
:   bridge to grab

**Description**

This function returns the old bridge state for the given bridge, or NULL if
the bridge is not part of the global atomic state.

struct [drm\_bridge\_state](#c.drm_bridge_state "drm_bridge_state") \*drm\_atomic\_get\_new\_bridge\_state(const struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_bridge](drm-kms-helpers.html#c.drm_bridge "drm_bridge") \*bridge)
:   get new bridge state, if it exists

**Parameters**

`const struct drm_atomic_state *state`
:   global atomic state object

`struct drm_bridge *bridge`
:   bridge to grab

**Description**

This function returns the new bridge state for the given bridge, or NULL if
the bridge is not part of the global atomic state.

int drm\_atomic\_add\_encoder\_bridges(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_encoder](#c.drm_encoder "drm_encoder") \*encoder)
:   add bridges attached to an encoder

**Parameters**

`struct drm_atomic_state *state`
:   atomic state

`struct drm_encoder *encoder`
:   DRM encoder

**Description**

This function adds all bridges attached to **encoder**. This is needed to add
bridge states to **state** and make them available when
[`drm_bridge_funcs.atomic_check()`](drm-kms-helpers.html#c.drm_bridge_funcs "drm_bridge_funcs"), [`drm_bridge_funcs.atomic_pre_enable()`](drm-kms-helpers.html#c.drm_bridge_funcs "drm_bridge_funcs"),
[`drm_bridge_funcs.atomic_enable()`](drm-kms-helpers.html#c.drm_bridge_funcs "drm_bridge_funcs"),
[`drm_bridge_funcs.atomic_disable_post_disable()`](drm-kms-helpers.html#c.drm_bridge_funcs "drm_bridge_funcs") are called.

**Return**

0 on success or can fail with -EDEADLK or -ENOMEM. When the error is EDEADLK
then the w/w mutex code has detected a deadlock and the entire atomic
sequence must be restarted. All other errors are fatal.

int drm\_atomic\_add\_affected\_connectors(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   add connectors for CRTC

**Parameters**

`struct drm_atomic_state *state`
:   atomic state

`struct drm_crtc *crtc`
:   DRM CRTC

**Description**

This function walks the current configuration and adds all connectors
currently using **crtc** to the atomic configuration **state**. Note that this
function must acquire the connection mutex. This can potentially cause
unneeded serialization if the update is just for the planes on one CRTC. Hence
drivers and helpers should only call this when really needed (e.g. when a
full modeset needs to happen due to some change).

**Return**

0 on success or can fail with -EDEADLK or -ENOMEM. When the error is EDEADLK
then the w/w mutex code has detected a deadlock and the entire atomic
sequence must be restarted. All other errors are fatal.

int drm\_atomic\_add\_affected\_planes(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   add planes for CRTC

**Parameters**

`struct drm_atomic_state *state`
:   atomic state

`struct drm_crtc *crtc`
:   DRM CRTC

**Description**

This function walks the current configuration and adds all planes
currently used by **crtc** to the atomic configuration **state**. This is useful
when an atomic commit also needs to check all currently enabled plane on
**crtc**, e.g. when changing the mode. It’s also useful when re-enabling a CRTC
to avoid special code to force-enable all planes.

Since acquiring a plane state will always also acquire the w/w mutex of the
current CRTC for that plane (if there is any) adding all the plane states for
a CRTC will not reduce parallelism of atomic updates.

**Return**

0 on success or can fail with -EDEADLK or -ENOMEM. When the error is EDEADLK
then the w/w mutex code has detected a deadlock and the entire atomic
sequence must be restarted. All other errors are fatal.

int drm\_atomic\_add\_affected\_colorops(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_plane](#c.drm_plane "drm_plane") \*plane)
:   add colorops for plane

**Parameters**

`struct drm_atomic_state *state`
:   atomic state

`struct drm_plane *plane`
:   DRM plane

**Description**

This function walks the current configuration and adds all colorops
currently used by **plane** to the atomic configuration **state**. This is useful
when an atomic commit also needs to check all currently enabled colorop on
**plane**, e.g. when changing the mode. It’s also useful when re-enabling a plane
to avoid special code to force-enable all colorops.

Since acquiring a colorop state will always also acquire the w/w mutex of the
current plane for that colorop (if there is any) adding all the colorop states for
a plane will not reduce parallelism of atomic updates.

**Return**

0 on success or can fail with -EDEADLK or -ENOMEM. When the error is EDEADLK
then the w/w mutex code has detected a deadlock and the entire atomic
sequence must be restarted. All other errors are fatal.

int drm\_atomic\_check\_only(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state)
:   check whether a given config would work

**Parameters**

`struct drm_atomic_state *state`
:   atomic configuration to check

**Description**

Note that this function can return -EDEADLK if the driver needed to acquire
more locks but encountered a deadlock. The caller must then do the usual w/w
backoff dance and restart. All other errors are fatal.

**Return**

0 on success, negative error code on failure.

int drm\_atomic\_commit(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state)
:   commit configuration atomically

**Parameters**

`struct drm_atomic_state *state`
:   atomic configuration to check

**Description**

Note that this function can return -EDEADLK if the driver needed to acquire
more locks but encountered a deadlock. The caller must then do the usual w/w
backoff dance and restart. All other errors are fatal.

This function will take its own reference on **state**.
Callers should always release their reference with [`drm_atomic_state_put()`](#c.drm_atomic_state_put "drm_atomic_state_put").

**Return**

0 on success, negative error code on failure.

int drm\_atomic\_nonblocking\_commit(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state)
:   atomic nonblocking commit

**Parameters**

`struct drm_atomic_state *state`
:   atomic configuration to check

**Description**

Note that this function can return -EDEADLK if the driver needed to acquire
more locks but encountered a deadlock. The caller must then do the usual w/w
backoff dance and restart. All other errors are fatal.

This function will take its own reference on **state**.
Callers should always release their reference with [`drm_atomic_state_put()`](#c.drm_atomic_state_put "drm_atomic_state_put").

**Return**

0 on success, negative error code on failure.

void drm\_atomic\_print\_new\_state(const struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_printer](drm-internals.html#c.drm_printer "drm_printer") \*p)
:   prints drm atomic state

**Parameters**

`const struct drm_atomic_state *state`
:   atomic configuration to check

`struct drm_printer *p`
:   drm printer

**Description**

This functions prints the drm atomic state snapshot using the drm printer
which is passed to it. This snapshot can be used for debugging purposes.

Note that this function looks into the new state objects and hence its not
safe to be used after the call to [`drm_atomic_helper_commit_hw_done()`](drm-kms-helpers.html#c.drm_atomic_helper_commit_hw_done "drm_atomic_helper_commit_hw_done").

void drm\_state\_dump(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_printer](drm-internals.html#c.drm_printer "drm_printer") \*p)
:   dump entire device atomic state

**Parameters**

`struct drm_device *dev`
:   the drm device

`struct drm_printer *p`
:   where to print the state to

**Description**

Just for debugging. Drivers might want an option to dump state
to dmesg in case of error irq’s. (Hint, you probably want to
ratelimit this!)

The caller must wrap this [`drm_modeset_lock_all_ctx()`](#c.drm_modeset_lock_all_ctx "drm_modeset_lock_all_ctx") and
[`drm_modeset_drop_locks()`](#c.drm_modeset_drop_locks "drm_modeset_drop_locks"). If this is called from error irq handler, it should
not be enabled by default - if you are debugging errors you might
not care that this is racey, but calling this without all modeset locks held
is inherently unsafe.

### Atomic Mode Setting IOCTL and UAPI Functions

This file contains the marshalling and demarshalling glue for the atomic UAPI
in all its forms: The monster ATOMIC IOCTL itself, code for GET\_PROPERTY and
SET\_PROPERTY IOCTLs. Plus interface functions for compatibility helpers and
drivers which have special needs to construct their own atomic updates, e.g.
for load detect or similar.

int drm\_atomic\_set\_mode\_for\_crtc(struct [drm\_crtc\_state](#c.drm_crtc_state "drm_crtc_state") \*state, const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode)
:   set mode for CRTC

**Parameters**

`struct drm_crtc_state *state`
:   the CRTC whose incoming state to update

`const struct drm_display_mode *mode`
:   kernel-internal mode to use for the CRTC, or NULL to disable

**Description**

Set a mode (originating from the kernel) on the desired CRTC state and update
the enable property.

**Return**

Zero on success, error code on failure. Cannot return -EDEADLK.

int drm\_atomic\_set\_mode\_prop\_for\_crtc(struct [drm\_crtc\_state](#c.drm_crtc_state "drm_crtc_state") \*state, struct [drm\_property\_blob](#c.drm_property_blob "drm_property_blob") \*blob)
:   set mode for CRTC

**Parameters**

`struct drm_crtc_state *state`
:   the CRTC whose incoming state to update

`struct drm_property_blob *blob`
:   pointer to blob property to use for mode

**Description**

Set a mode (originating from a blob property) on the desired CRTC state.
This function will take a reference on the blob property for the CRTC state,
and release the reference held on the state’s existing mode property, if any
was set.

**Return**

Zero on success, error code on failure. Cannot return -EDEADLK.

int drm\_atomic\_set\_crtc\_for\_plane(struct [drm\_plane\_state](#c.drm_plane_state "drm_plane_state") \*plane\_state, struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   set CRTC for plane

**Parameters**

`struct drm_plane_state *plane_state`
:   the plane whose incoming state to update

`struct drm_crtc *crtc`
:   CRTC to use for the plane

**Description**

Changing the assigned CRTC for a plane requires us to grab the lock and state
for the new CRTC, as needed. This function takes care of all these details
besides updating the pointer in the state object itself.

**Return**

0 on success or can fail with -EDEADLK or -ENOMEM. When the error is EDEADLK
then the w/w mutex code has detected a deadlock and the entire atomic
sequence must be restarted. All other errors are fatal.

void drm\_atomic\_set\_fb\_for\_plane(struct [drm\_plane\_state](#c.drm_plane_state "drm_plane_state") \*plane\_state, struct [drm\_framebuffer](#c.drm_framebuffer "drm_framebuffer") \*fb)
:   set framebuffer for plane

**Parameters**

`struct drm_plane_state *plane_state`
:   atomic state object for the plane

`struct drm_framebuffer *fb`
:   fb to use for the plane

**Description**

Changing the assigned framebuffer for a plane requires us to grab a reference
to the new fb and drop the reference to the old fb, if there is one. This
function takes care of all these details besides updating the pointer in the
state object itself.

bool drm\_atomic\_set\_colorop\_for\_plane(struct [drm\_plane\_state](#c.drm_plane_state "drm_plane_state") \*plane\_state, struct [drm\_colorop](#c.drm_colorop "drm_colorop") \*colorop)
:   set colorop for plane

**Parameters**

`struct drm_plane_state *plane_state`
:   atomic state object for the plane

`struct drm_colorop *colorop`
:   colorop to use for the plane

**Description**

Helper function to select the color pipeline on a plane by setting
it to the first drm\_colorop element of the pipeline.

**Return**

true if plane color pipeline value changed, false otherwise.

int drm\_atomic\_set\_crtc\_for\_connector(struct [drm\_connector\_state](#c.drm_connector_state "drm_connector_state") \*conn\_state, struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   set CRTC for connector

**Parameters**

`struct drm_connector_state *conn_state`
:   atomic state object for the connector

`struct drm_crtc *crtc`
:   CRTC to use for the connector

**Description**

Changing the assigned CRTC for a connector requires us to grab the lock and
state for the new CRTC, as needed. This function takes care of all these
details besides updating the pointer in the state object itself.

**Return**

0 on success or can fail with -EDEADLK or -ENOMEM. When the error is EDEADLK
then the w/w mutex code has detected a deadlock and the entire atomic
sequence must be restarted. All other errors are fatal.

## CRTC Abstraction

A CRTC represents the overall display pipeline. It receives pixel data from
[`drm_plane`](#c.drm_plane "drm_plane") and blends them together. The [`drm_display_mode`](#c.drm_display_mode "drm_display_mode") is also attached
to the CRTC, specifying display timings. On the output side the data is fed
to one or more [`drm_encoder`](#c.drm_encoder "drm_encoder"), which are then each connected to one
[`drm_connector`](#c.drm_connector "drm_connector").

To create a CRTC, a KMS driver allocates and zeroes an instance of
[`struct drm_crtc`](#c.drm_crtc "drm_crtc") (possibly as part of a larger structure) and registers it
with a call to [`drm_crtc_init_with_planes()`](#c.drm_crtc_init_with_planes "drm_crtc_init_with_planes").

The CRTC is also the entry point for legacy modeset operations (see
[`drm_crtc_funcs.set_config`](#c.drm_crtc_funcs "drm_crtc_funcs")), legacy plane operations (see
[`drm_crtc_funcs.page_flip`](#c.drm_crtc_funcs "drm_crtc_funcs") and [`drm_crtc_funcs.cursor_set2`](#c.drm_crtc_funcs "drm_crtc_funcs")), and other legacy
operations like [`drm_crtc_funcs.gamma_set`](#c.drm_crtc_funcs "drm_crtc_funcs"). For atomic drivers all these
features are controlled through [`drm_property`](#c.drm_property "drm_property") and
[`drm_mode_config_funcs.atomic_check`](#c.drm_mode_config_funcs "drm_mode_config_funcs").

### CRTC Functions Reference

struct drm\_crtc\_state
:   mutable CRTC state

**Definition**:

```
struct drm_crtc_state {
    struct drm_crtc *crtc;
    bool enable;
    bool active;
    bool planes_changed : 1;
    bool mode_changed : 1;
    bool active_changed : 1;
    bool connectors_changed : 1;
    bool zpos_changed : 1;
    bool color_mgmt_changed : 1;
    bool no_vblank;
    u32 plane_mask;
    u32 connector_mask;
    u32 encoder_mask;
    struct drm_display_mode adjusted_mode;
    struct drm_display_mode mode;
    struct drm_property_blob *mode_blob;
    struct drm_property_blob *degamma_lut;
    struct drm_property_blob *ctm;
    struct drm_property_blob *gamma_lut;
    u64 background_color;
    u32 target_vblank;
    bool async_flip;
    bool vrr_enabled;
    bool self_refresh_active;
    enum drm_scaling_filter scaling_filter;
    u8 sharpness_strength;
    struct drm_pending_vblank_event *event;
    struct drm_crtc_commit *commit;
    struct drm_atomic_state *state;
};
```

**Members**

`crtc`
:   backpointer to the CRTC

`enable`
:   Whether the CRTC should be enabled, gates all other state.
    This controls reservations of shared resources. Actual hardware state
    is controlled by **active**.

`active`
:   Whether the CRTC is actively displaying (used for DPMS).
    Implies that **enable** is set. The driver must not release any shared
    resources if **active** is set to false but **enable** still true, because
    userspace expects that a DPMS ON always succeeds.

    Hence drivers must not consult **active** in their various
    [`drm_mode_config_funcs.atomic_check`](#c.drm_mode_config_funcs "drm_mode_config_funcs") callback to reject an atomic
    commit. They can consult it to aid in the computation of derived
    hardware state, since even in the DPMS OFF state the display hardware
    should be as much powered down as when the CRTC is completely
    disabled through setting **enable** to false.

`planes_changed`
:   Planes on this crtc are updated. Used by the atomic
    helpers and drivers to steer the atomic commit control flow.

`mode_changed`
:   **mode** or **enable** has been changed. Used by the atomic
    helpers and drivers to steer the atomic commit control flow. See also
    [`drm_atomic_crtc_needs_modeset()`](#c.drm_atomic_crtc_needs_modeset "drm_atomic_crtc_needs_modeset").

    Drivers are supposed to set this for any CRTC state changes that
    require a full modeset. They can also reset it to false if e.g. a
    **mode** change can be done without a full modeset by only changing
    scaler settings.

`active_changed`
:   **active** has been toggled. Used by the atomic
    helpers and drivers to steer the atomic commit control flow. See also
    [`drm_atomic_crtc_needs_modeset()`](#c.drm_atomic_crtc_needs_modeset "drm_atomic_crtc_needs_modeset").

`connectors_changed`
:   Connectors to this crtc have been updated,
    either in their state or routing. Used by the atomic
    helpers and drivers to steer the atomic commit control flow. See also
    [`drm_atomic_crtc_needs_modeset()`](#c.drm_atomic_crtc_needs_modeset "drm_atomic_crtc_needs_modeset").

    Drivers are supposed to set this as-needed from their own atomic
    check code, e.g. from [`drm_encoder_helper_funcs.atomic_check`](drm-kms-helpers.html#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs")

`zpos_changed`
:   zpos values of planes on this crtc have been updated.
    Used by the atomic helpers and drivers to steer the atomic commit
    control flow.

`color_mgmt_changed`
:   Color management properties have changed
    (**gamma\_lut**, **degamma\_lut** or **ctm**). Used by the atomic helpers and
    drivers to steer the atomic commit control flow.

`no_vblank`
:   Reflects the ability of a CRTC to send VBLANK events. This state
    usually depends on the pipeline configuration. If set to true, DRM
    atomic helpers will send out a fake VBLANK event during display
    updates after all hardware changes have been committed. This is
    implemented in [`drm_atomic_helper_fake_vblank()`](drm-kms-helpers.html#c.drm_atomic_helper_fake_vblank "drm_atomic_helper_fake_vblank").

    One usage is for drivers and/or hardware without support for VBLANK
    interrupts. Such drivers typically do not initialize vblanking
    (i.e., call [`drm_vblank_init()`](#c.drm_vblank_init "drm_vblank_init") with the number of CRTCs). For CRTCs
    without initialized vblanking, this field is set to true in
    [`drm_atomic_helper_check_modeset()`](drm-kms-helpers.html#c.drm_atomic_helper_check_modeset "drm_atomic_helper_check_modeset"), and a fake VBLANK event will be
    send out on each update of the display pipeline by
    [`drm_atomic_helper_fake_vblank()`](drm-kms-helpers.html#c.drm_atomic_helper_fake_vblank "drm_atomic_helper_fake_vblank").

    Another usage is CRTCs feeding a writeback connector operating in
    oneshot mode. In this case the fake VBLANK event is only generated
    when a job is queued to the writeback connector, and we want the
    core to fake VBLANK events when this part of the pipeline hasn’t
    changed but others had or when the CRTC and connectors are being
    disabled.

    [`__drm_atomic_helper_crtc_duplicate_state()`](drm-kms-helpers.html#c.__drm_atomic_helper_crtc_duplicate_state "__drm_atomic_helper_crtc_duplicate_state") will not reset the value
    from the current state, the CRTC driver is then responsible for
    updating this field when needed.

    Note that the combination of [`drm_crtc_state.event`](#c.drm_crtc_state "drm_crtc_state") == NULL and
    [`drm_crtc_state.no_blank`](#c.drm_crtc_state "drm_crtc_state") == true is valid and usually used when the
    writeback connector attached to the CRTC has a new job queued. In
    this case the driver will send the VBLANK event on its own when the
    writeback job is complete.

`plane_mask`
:   Bitmask of drm\_plane\_mask(plane) of planes attached to
    this CRTC.

`connector_mask`
:   Bitmask of drm\_connector\_mask(connector) of
    connectors attached to this CRTC.

`encoder_mask`
:   Bitmask of drm\_encoder\_mask(encoder) of encoders
    attached to this CRTC.

`adjusted_mode`
:   Internal display timings which can be used by the driver to handle
    differences between the mode requested by userspace in **mode** and what
    is actually programmed into the hardware.

    For drivers using [`drm_bridge`](drm-kms-helpers.html#c.drm_bridge "drm_bridge"), this stores hardware display timings
    used between the CRTC and the first bridge. For other drivers, the
    meaning of the adjusted\_mode field is purely driver implementation
    defined information, and will usually be used to store the hardware
    display timings used between the CRTC and encoder blocks.

`mode`
:   Display timings requested by userspace. The driver should try to
    match the refresh rate as close as possible (but note that it’s
    undefined what exactly is close enough, e.g. some of the HDMI modes
    only differ in less than 1% of the refresh rate). The active width
    and height as observed by userspace for positioning planes must match
    exactly.

    For external connectors where the sink isn’t fixed (like with a
    built-in panel), this mode here should match the physical mode on the
    wire to the last details (i.e. including sync polarities and
    everything).

`mode_blob`
:   [`drm_property_blob`](#c.drm_property_blob "drm_property_blob") for **mode**, for exposing the mode to
    atomic userspace.

`degamma_lut`
:   Lookup table for converting framebuffer pixel data before apply the
    color conversion matrix **ctm**. See [`drm_crtc_enable_color_mgmt()`](#c.drm_crtc_enable_color_mgmt "drm_crtc_enable_color_mgmt"). The
    blob (if not NULL) is an array of `struct drm_color_lut`.

`ctm`
:   Color transformation matrix. See [`drm_crtc_enable_color_mgmt()`](#c.drm_crtc_enable_color_mgmt "drm_crtc_enable_color_mgmt"). The
    blob (if not NULL) is a `struct drm_color_ctm`.

`gamma_lut`
:   Lookup table for converting pixel data after the color conversion
    matrix **ctm**. See [`drm_crtc_enable_color_mgmt()`](#c.drm_crtc_enable_color_mgmt "drm_crtc_enable_color_mgmt"). The blob (if not
    NULL) is an array of `struct drm_color_lut`.

    Note that for mostly historical reasons stemming from Xorg heritage,
    this is also used to store the color map (also sometimes color lut,
    CLUT or color palette) for indexed formats like DRM\_FORMAT\_C8.

`background_color`
:   RGB value representing the CRTC’s background color. The background
    color (aka “canvas color”) of a CRTC is the color that will be used
    for pixels not covered by a plane, or covered by transparent pixels
    of a plane. The value here should be built using DRM\_ARGB64\_PREP\*()
    helpers, while the individual color components can be extracted with
    desired precision via the DRM\_ARGB64\_GET\*() macros.

`target_vblank`
:   Target vertical blank period when a page flip
    should take effect.

`async_flip`
:   This is set when DRM\_MODE\_PAGE\_FLIP\_ASYNC is set in the legacy
    PAGE\_FLIP IOCTL. It’s not wired up for the atomic IOCTL itself yet.

`vrr_enabled`
:   Indicates if variable refresh rate should be enabled for the CRTC.
    Support for the requested vrr state will depend on driver and
    hardware capabiltiy - lacking support is not treated as failure.

`self_refresh_active`
:   Used by the self refresh helpers to denote when a self refresh
    transition is occurring. This will be set on enable/disable callbacks
    when self refresh is being enabled or disabled. In some cases, it may
    not be desirable to fully shut off the crtc during self refresh.
    CRTC’s can inspect this flag and determine the best course of action.

`scaling_filter`
:   Scaling filter to be applied

`sharpness_strength`
:   Used by the user to set the sharpness intensity.
    The value ranges from 0-255.
    Default value is 0 which disable the sharpness feature.
    Any value greater than 0 enables sharpening with the
    specified strength.

`event`
:   Optional pointer to a DRM event to signal upon completion of the
    state update. The driver must send out the event when the atomic
    commit operation completes. There are two cases:

    > * The event is for a CRTC which is being disabled through this
    >   atomic commit. In that case the event can be send out any time
    >   after the hardware has stopped scanning out the current
    >   framebuffers. It should contain the timestamp and counter for the
    >   last vblank before the display pipeline was shut off. The simplest
    >   way to achieve that is calling [`drm_crtc_send_vblank_event()`](#c.drm_crtc_send_vblank_event "drm_crtc_send_vblank_event")
    >   somewhen after [`drm_crtc_vblank_off()`](#c.drm_crtc_vblank_off "drm_crtc_vblank_off") has been called.
    > * For a CRTC which is enabled at the end of the commit (even when it
    >   undergoes an full modeset) the vblank timestamp and counter must
    >   be for the vblank right before the first frame that scans out the
    >   new set of buffers. Again the event can only be sent out after the
    >   hardware has stopped scanning out the old buffers.
    > * Events for disabled CRTCs are not allowed, and drivers can ignore
    >   that case.

    For very simple hardware without VBLANK interrupt, enabling
    [`struct drm_crtc_state`](#c.drm_crtc_state "drm_crtc_state").no\_vblank makes DRM’s atomic commit helpers
    send a fake VBLANK event at the end of the display update after all
    hardware changes have been applied. See
    [`drm_atomic_helper_fake_vblank()`](drm-kms-helpers.html#c.drm_atomic_helper_fake_vblank "drm_atomic_helper_fake_vblank").

    For more complex hardware this
    can be handled by the [`drm_crtc_send_vblank_event()`](#c.drm_crtc_send_vblank_event "drm_crtc_send_vblank_event") function,
    which the driver should call on the provided event upon completion of
    the atomic commit. Note that if the driver supports vblank signalling
    and timestamping the vblank counters and timestamps must agree with
    the ones returned from page flip events. With the current vblank
    helper infrastructure this can be achieved by holding a vblank
    reference while the page flip is pending, acquired through
    [`drm_crtc_vblank_get()`](#c.drm_crtc_vblank_get "drm_crtc_vblank_get") and released with [`drm_crtc_vblank_put()`](#c.drm_crtc_vblank_put "drm_crtc_vblank_put").
    Drivers are free to implement their own vblank counter and timestamp
    tracking though, e.g. if they have accurate timestamp registers in
    hardware.

    For hardware which supports some means to synchronize vblank
    interrupt delivery with committing display state there’s also
    [`drm_crtc_arm_vblank_event()`](#c.drm_crtc_arm_vblank_event "drm_crtc_arm_vblank_event"). See the documentation of that function
    for a detailed discussion of the constraints it needs to be used
    safely.

    If the device can’t notify of flip completion in a race-free way
    at all, then the event should be armed just after the page flip is
    committed. In the worst case the driver will send the event to
    userspace one frame too late. This doesn’t allow for a real atomic
    update, but it should avoid tearing.

`commit`
:   This tracks how the commit for this update proceeds through the
    various phases. This is never cleared, except when we destroy the
    state, so that subsequent commits can synchronize with previous ones.

`state`
:   backpointer to global drm\_atomic\_state

**Description**

Note that the distinction between **enable** and **active** is rather subtle:
Flipping **active** while **enable** is set without changing anything else may
never return in a failure from the [`drm_mode_config_funcs.atomic_check`](#c.drm_mode_config_funcs "drm_mode_config_funcs")
callback. Userspace assumes that a DPMS On will always succeed. In other
words: **enable** controls resource assignment, **active** controls the actual
hardware state.

The three booleans active\_changed, connectors\_changed and mode\_changed are
intended to indicate whether a full modeset is needed, rather than strictly
describing what has changed in a commit. See also:
[`drm_atomic_crtc_needs_modeset()`](#c.drm_atomic_crtc_needs_modeset "drm_atomic_crtc_needs_modeset")

struct drm\_crtc\_funcs
:   control CRTCs for a given device

**Definition**:

```
struct drm_crtc_funcs {
    void (*reset)(struct drm_crtc *crtc);
    int (*cursor_set)(struct drm_crtc *crtc, struct drm_file *file_priv, uint32_t handle, uint32_t width, uint32_t height);
    int (*cursor_set2)(struct drm_crtc *crtc, struct drm_file *file_priv, uint32_t handle, uint32_t width, uint32_t height, int32_t hot_x, int32_t hot_y);
    int (*cursor_move)(struct drm_crtc *crtc, int x, int y);
    int (*gamma_set)(struct drm_crtc *crtc, u16 *r, u16 *g, u16 *b, uint32_t size, struct drm_modeset_acquire_ctx *ctx);
    void (*destroy)(struct drm_crtc *crtc);
    int (*set_config)(struct drm_mode_set *set, struct drm_modeset_acquire_ctx *ctx);
    int (*page_flip)(struct drm_crtc *crtc, struct drm_framebuffer *fb, struct drm_pending_vblank_event *event, uint32_t flags, struct drm_modeset_acquire_ctx *ctx);
    int (*page_flip_target)(struct drm_crtc *crtc, struct drm_framebuffer *fb, struct drm_pending_vblank_event *event, uint32_t flags, uint32_t target, struct drm_modeset_acquire_ctx *ctx);
    int (*set_property)(struct drm_crtc *crtc, struct drm_property *property, uint64_t val);
    struct drm_crtc_state *(*atomic_duplicate_state)(struct drm_crtc *crtc);
    void (*atomic_destroy_state)(struct drm_crtc *crtc, struct drm_crtc_state *state);
    int (*atomic_set_property)(struct drm_crtc *crtc, struct drm_crtc_state *state, struct drm_property *property, uint64_t val);
    int (*atomic_get_property)(struct drm_crtc *crtc, const struct drm_crtc_state *state, struct drm_property *property, uint64_t *val);
    int (*late_register)(struct drm_crtc *crtc);
    void (*early_unregister)(struct drm_crtc *crtc);
    int (*set_crc_source)(struct drm_crtc *crtc, const char *source);
    int (*verify_crc_source)(struct drm_crtc *crtc, const char *source, size_t *values_cnt);
    const char *const *(*get_crc_sources)(struct drm_crtc *crtc, size_t *count);
    void (*atomic_print_state)(struct drm_printer *p, const struct drm_crtc_state *state);
    u32 (*get_vblank_counter)(struct drm_crtc *crtc);
    int (*enable_vblank)(struct drm_crtc *crtc);
    void (*disable_vblank)(struct drm_crtc *crtc);
    bool (*get_vblank_timestamp)(struct drm_crtc *crtc, int *max_error, ktime_t *vblank_time, bool in_vblank_irq);
};
```

**Members**

`reset`
:   Reset CRTC hardware and software state to off. This function isn’t
    called by the core directly, only through [`drm_mode_config_reset()`](#c.drm_mode_config_reset "drm_mode_config_reset").
    It’s not a helper hook only for historical reasons.

    Atomic drivers can use [`drm_atomic_helper_crtc_reset()`](drm-kms-helpers.html#c.drm_atomic_helper_crtc_reset "drm_atomic_helper_crtc_reset") to reset
    atomic state using this hook.

`cursor_set`
:   Update the cursor image. The cursor position is relative to the CRTC
    and can be partially or fully outside of the visible area.

    Note that contrary to all other KMS functions the legacy cursor entry
    points don’t take a framebuffer object, but instead take directly a
    raw buffer object id from the driver’s buffer manager (which is
    either GEM or TTM for current drivers).

    This entry point is deprecated, drivers should instead implement
    universal plane support and register a proper cursor plane using
    [`drm_crtc_init_with_planes()`](#c.drm_crtc_init_with_planes "drm_crtc_init_with_planes").

    This callback is optional

    RETURNS:

    0 on success or a negative error code on failure.

`cursor_set2`
:   Update the cursor image, including hotspot information. The hotspot
    must not affect the cursor position in CRTC coordinates, but is only
    meant as a hint for virtualized display hardware to coordinate the
    guests and hosts cursor position. The cursor hotspot is relative to
    the cursor image. Otherwise this works exactly like **cursor\_set**.

    This entry point is deprecated, drivers should instead implement
    universal plane support and register a proper cursor plane using
    [`drm_crtc_init_with_planes()`](#c.drm_crtc_init_with_planes "drm_crtc_init_with_planes").

    This callback is optional.

    RETURNS:

    0 on success or a negative error code on failure.

`cursor_move`
:   Update the cursor position. The cursor does not need to be visible
    when this hook is called.

    This entry point is deprecated, drivers should instead implement
    universal plane support and register a proper cursor plane using
    [`drm_crtc_init_with_planes()`](#c.drm_crtc_init_with_planes "drm_crtc_init_with_planes").

    This callback is optional.

    RETURNS:

    0 on success or a negative error code on failure.

`gamma_set`
:   Set gamma on the CRTC.

    This callback is optional.

    Atomic drivers who want to support gamma tables should implement the
    atomic color management support, enabled by calling
    [`drm_crtc_enable_color_mgmt()`](#c.drm_crtc_enable_color_mgmt "drm_crtc_enable_color_mgmt"), which then supports the legacy gamma
    interface through the `drm_atomic_helper_legacy_gamma_set()`
    compatibility implementation.

`destroy`
:   Clean up CRTC resources. This is only called at driver unload time
    through [`drm_mode_config_cleanup()`](#c.drm_mode_config_cleanup "drm_mode_config_cleanup") since a CRTC cannot be hotplugged
    in DRM.

`set_config`
:   This is the main legacy entry point to change the modeset state on a
    CRTC. All the details of the desired configuration are passed in a
    [`struct drm_mode_set`](#c.drm_mode_set "drm_mode_set") - see there for details.

    Drivers implementing atomic modeset should use
    [`drm_atomic_helper_set_config()`](drm-kms-helpers.html#c.drm_atomic_helper_set_config "drm_atomic_helper_set_config") to implement this hook.

    RETURNS:

    0 on success or a negative error code on failure.

`page_flip`
:   Legacy entry point to schedule a flip to the given framebuffer.

    Page flipping is a synchronization mechanism that replaces the frame
    buffer being scanned out by the CRTC with a new frame buffer during
    vertical blanking, avoiding tearing (except when requested otherwise
    through the DRM\_MODE\_PAGE\_FLIP\_ASYNC flag). When an application
    requests a page flip the DRM core verifies that the new frame buffer
    is large enough to be scanned out by the CRTC in the currently
    configured mode and then calls this hook with a pointer to the new
    frame buffer.

    The driver must wait for any pending rendering to the new framebuffer
    to complete before executing the flip. It should also wait for any
    pending rendering from other drivers if the underlying buffer is a
    shared dma-buf.

    An application can request to be notified when the page flip has
    completed. The drm core will supply a [`struct drm_event`](drm-uapi.html#c.drm_event "drm_event") in the event
    parameter in this case. This can be handled by the
    [`drm_crtc_send_vblank_event()`](#c.drm_crtc_send_vblank_event "drm_crtc_send_vblank_event") function, which the driver should call on
    the provided event upon completion of the flip. Note that if
    the driver supports vblank signalling and timestamping the vblank
    counters and timestamps must agree with the ones returned from page
    flip events. With the current vblank helper infrastructure this can
    be achieved by holding a vblank reference while the page flip is
    pending, acquired through [`drm_crtc_vblank_get()`](#c.drm_crtc_vblank_get "drm_crtc_vblank_get") and released with
    [`drm_crtc_vblank_put()`](#c.drm_crtc_vblank_put "drm_crtc_vblank_put"). Drivers are free to implement their own vblank
    counter and timestamp tracking though, e.g. if they have accurate
    timestamp registers in hardware.

    This callback is optional.

    NOTE:

    Very early versions of the KMS ABI mandated that the driver must
    block (but not reject) any rendering to the old framebuffer until the
    flip operation has completed and the old framebuffer is no longer
    visible. This requirement has been lifted, and userspace is instead
    expected to request delivery of an event and wait with recycling old
    buffers until such has been received.

    RETURNS:

    0 on success or a negative error code on failure. Note that if a
    page flip operation is already pending the callback should return
    -EBUSY. Pageflips on a disabled CRTC (either by setting a NULL mode
    or just runtime disabled through DPMS respectively the new atomic
    “ACTIVE” state) should result in an -EINVAL error code. Note that
    [`drm_atomic_helper_page_flip()`](drm-kms-helpers.html#c.drm_atomic_helper_page_flip "drm_atomic_helper_page_flip") checks this already for atomic drivers.

`page_flip_target`
:   Same as **page\_flip** but with an additional parameter specifying the
    absolute target vertical blank period (as reported by
    [`drm_crtc_vblank_count()`](#c.drm_crtc_vblank_count "drm_crtc_vblank_count")) when the flip should take effect.

    Note that the core code calls drm\_crtc\_vblank\_get before this entry
    point, and will call drm\_crtc\_vblank\_put if this entry point returns
    any non-0 error code. It’s the driver’s responsibility to call
    drm\_crtc\_vblank\_put after this entry point returns 0, typically when
    the flip completes.

`set_property`
:   This is the legacy entry point to update a property attached to the
    CRTC.

    This callback is optional if the driver does not support any legacy
    driver-private properties. For atomic drivers it is not used because
    property handling is done entirely in the DRM core.

    RETURNS:

    0 on success or a negative error code on failure.

`atomic_duplicate_state`
:   Duplicate the current atomic state for this CRTC and return it.
    The core and helpers guarantee that any atomic state duplicated with
    this hook and still owned by the caller (i.e. not transferred to the
    driver by calling [`drm_mode_config_funcs.atomic_commit`](#c.drm_mode_config_funcs "drm_mode_config_funcs")) will be
    cleaned up by calling the **atomic\_destroy\_state** hook in this
    structure.

    This callback is mandatory for atomic drivers.

    Atomic drivers which don’t subclass [`struct drm_crtc_state`](#c.drm_crtc_state "drm_crtc_state") should use
    [`drm_atomic_helper_crtc_duplicate_state()`](drm-kms-helpers.html#c.drm_atomic_helper_crtc_duplicate_state "drm_atomic_helper_crtc_duplicate_state"). Drivers that subclass the
    state structure to extend it with driver-private state should use
    [`__drm_atomic_helper_crtc_duplicate_state()`](drm-kms-helpers.html#c.__drm_atomic_helper_crtc_duplicate_state "__drm_atomic_helper_crtc_duplicate_state") to make sure shared state is
    duplicated in a consistent fashion across drivers.

    It is an error to call this hook before [`drm_crtc.state`](#c.drm_crtc "drm_crtc") has been
    initialized correctly.

    NOTE:

    If the duplicate state references refcounted resources this hook must
    acquire a reference for each of them. The driver must release these
    references again in **atomic\_destroy\_state**.

    RETURNS:

    Duplicated atomic state or NULL when the allocation failed.

`atomic_destroy_state`
:   Destroy a state duplicated with **atomic\_duplicate\_state** and release
    or unreference all resources it references

    This callback is mandatory for atomic drivers.

`atomic_set_property`
:   Decode a driver-private property value and store the decoded value
    into the passed-in state structure. Since the atomic core decodes all
    standardized properties (even for extensions beyond the core set of
    properties which might not be implemented by all drivers) this
    requires drivers to subclass the state structure.

    Such driver-private properties should really only be implemented for
    truly hardware/vendor specific state. Instead it is preferred to
    standardize atomic extension and decode the properties used to expose
    such an extension in the core.

    Do not call this function directly, use
    `drm_atomic_crtc_set_property()` instead.

    This callback is optional if the driver does not support any
    driver-private atomic properties.

    NOTE:

    This function is called in the state assembly phase of atomic
    modesets, which can be aborted for any reason (including on
    userspace’s request to just check whether a configuration would be
    possible). Drivers MUST NOT touch any persistent state (hardware or
    software) or data structures except the passed in **state** parameter.

    Also since userspace controls in which order properties are set this
    function must not do any input validation (since the state update is
    incomplete and hence likely inconsistent). Instead any such input
    validation must be done in the various atomic\_check callbacks.

    RETURNS:

    0 if the property has been found, -EINVAL if the property isn’t
    implemented by the driver (which should never happen, the core only
    asks for properties attached to this CRTC). No other validation is
    allowed by the driver. The core already checks that the property
    value is within the range (integer, valid `enum value`, ...) the driver
    set when registering the property.

`atomic_get_property`
:   Reads out the decoded driver-private property. This is used to
    implement the GETCRTC IOCTL.

    Do not call this function directly, use
    `drm_atomic_crtc_get_property()` instead.

    This callback is optional if the driver does not support any
    driver-private atomic properties.

    RETURNS:

    0 on success, -EINVAL if the property isn’t implemented by the
    driver (which should never happen, the core only asks for
    properties attached to this CRTC).

`late_register`
:   This optional hook can be used to register additional userspace
    interfaces attached to the crtc like debugfs interfaces.
    It is called late in the driver load sequence from [`drm_dev_register()`](drm-internals.html#c.drm_dev_register "drm_dev_register").
    Everything added from this callback should be unregistered in
    the early\_unregister callback.

    Returns:

    0 on success, or a negative error code on failure.

`early_unregister`
:   This optional hook should be used to unregister the additional
    userspace interfaces attached to the crtc from
    **late\_register**. It is called from [`drm_dev_unregister()`](drm-internals.html#c.drm_dev_unregister "drm_dev_unregister"),
    early in the driver unload sequence to disable userspace access
    before data structures are torndown.

`set_crc_source`
:   Changes the source of CRC checksums of frames at the request of
    userspace, typically for testing purposes. The sources available are
    specific of each driver and a `NULL` value indicates that CRC
    generation is to be switched off.

    When CRC generation is enabled, the driver should call
    [`drm_crtc_add_crc_entry()`](drm-uapi.html#c.drm_crtc_add_crc_entry "drm_crtc_add_crc_entry") at each frame, providing any information
    that characterizes the frame contents in the crcN arguments, as
    provided from the configured source. Drivers must accept an “auto”
    source name that will select a default source for this CRTC.

    This may trigger an atomic modeset commit if necessary, to enable CRC
    generation.

    Note that “auto” can depend upon the current modeset configuration,
    e.g. it could pick an encoder or output specific CRC sampling point.

    This callback is optional if the driver does not support any CRC
    generation functionality.

    RETURNS:

    0 on success or a negative error code on failure.

`verify_crc_source`
:   verifies the source of CRC checksums of frames before setting the
    source for CRC and during crc open. Source parameter can be NULL
    while disabling crc source.

    This callback is optional if the driver does not support any CRC
    generation functionality.

    RETURNS:

    0 on success or a negative error code on failure.

`get_crc_sources`
:   Driver callback for getting a list of all the available sources for
    CRC generation. This callback depends upon verify\_crc\_source, So
    verify\_crc\_source callback should be implemented before implementing
    this. Driver can pass full list of available crc sources, this
    callback does the verification on each crc-source before passing it
    to userspace.

    This callback is optional if the driver does not support exporting of
    possible CRC sources list.

    RETURNS:

    a constant character pointer to the list of all the available CRC
    sources. On failure driver should return NULL. count should be
    updated with number of sources in list. if zero we don’t process any
    source from the list.

`atomic_print_state`
:   If driver subclasses [`struct drm_crtc_state`](#c.drm_crtc_state "drm_crtc_state"), it should implement
    this optional hook for printing additional driver specific state.

    Do not call this directly, use `drm_atomic_crtc_print_state()`
    instead.

`get_vblank_counter`
:   Driver callback for fetching a raw hardware vblank counter for the
    CRTC. It’s meant to be used by new drivers as the replacement of
    [`drm_driver.get_vblank_counter`](drm-internals.html#c.drm_driver "drm_driver") hook.

    This callback is optional. If a device doesn’t have a hardware
    counter, the driver can simply leave the hook as NULL. The DRM core
    will account for missed vblank events while interrupts where disabled
    based on system timestamps.

    Wraparound handling and loss of events due to modesetting is dealt
    with in the DRM core code, as long as drivers call
    [`drm_crtc_vblank_off()`](#c.drm_crtc_vblank_off "drm_crtc_vblank_off") and [`drm_crtc_vblank_on()`](#c.drm_crtc_vblank_on "drm_crtc_vblank_on") when disabling or
    enabling a CRTC.

    See also [`drm_device.vblank_disable_immediate`](drm-internals.html#c.drm_device "drm_device") and
    [`drm_device.max_vblank_count`](drm-internals.html#c.drm_device "drm_device").

    Returns:

    Raw vblank counter value.

`enable_vblank`
:   Enable vblank interrupts for the CRTC. It’s meant to be used by
    new drivers as the replacement of [`drm_driver.enable_vblank`](drm-internals.html#c.drm_driver "drm_driver") hook.

    Returns:

    Zero on success, appropriate errno if the vblank interrupt cannot
    be enabled.

`disable_vblank`
:   Disable vblank interrupts for the CRTC. It’s meant to be used by
    new drivers as the replacement of [`drm_driver.disable_vblank`](drm-internals.html#c.drm_driver "drm_driver") hook.

`get_vblank_timestamp`
:   Called by `drm_get_last_vbltimestamp()`. Should return a precise
    timestamp when the most recent vblank interval ended or will end.

    Specifically, the timestamp in **vblank\_time** should correspond as
    closely as possible to the time when the first video scanline of
    the video frame after the end of vblank will start scanning out,
    the time immediately after end of the vblank interval. If the
    **crtc** is currently inside vblank, this will be a time in the future.
    If the **crtc** is currently scanning out a frame, this will be the
    past start time of the current scanout. This is meant to adhere
    to the OpenML OML\_sync\_control extension specification.

    Parameters:

    crtc:
    :   CRTC for which timestamp should be returned.

    max\_error:
    :   Maximum allowable timestamp error in nanoseconds.
        Implementation should strive to provide timestamp
        with an error of at most max\_error nanoseconds.
        Returns true upper bound on error for timestamp.

    vblank\_time:
    :   Target location for returned vblank timestamp.

    in\_vblank\_irq:
    :   True when called from [`drm_crtc_handle_vblank()`](#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank"). Some drivers
        need to apply some workarounds for gpu-specific vblank irq quirks
        if flag is set.

    Returns:

    True on success, false on failure, which means the core should
    fallback to a simple timestamp taken in [`drm_crtc_handle_vblank()`](#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank").

**Description**

The drm\_crtc\_funcs structure is the central CRTC management structure
in the DRM. Each CRTC controls one or more connectors (note that the name
CRTC is simply historical, a CRTC may control LVDS, VGA, DVI, TV out, etc.
connectors, not just CRTs).

Each driver is responsible for filling out this structure at startup time,
in addition to providing other modesetting features, like i2c and DDC
bus accessors.

struct drm\_crtc
:   central CRTC control structure

**Definition**:

```
struct drm_crtc {
    struct drm_device *dev;
    struct device_node *port;
    struct list_head head;
    char *name;
    struct drm_modeset_lock mutex;
    struct drm_mode_object base;
    struct drm_plane *primary;
    struct drm_plane *cursor;
    unsigned index;
    int cursor_x;
    int cursor_y;
    bool enabled;
    struct drm_display_mode mode;
    struct drm_display_mode hwmode;
    int x;
    int y;
    const struct drm_crtc_funcs *funcs;
    uint32_t gamma_size;
    uint16_t *gamma_store;
    const struct drm_crtc_helper_funcs *helper_private;
    struct drm_object_properties properties;
    struct drm_property *scaling_filter_property;
    struct drm_property *sharpness_strength_property;
    struct drm_crtc_state *state;
    struct list_head commit_list;
    spinlock_t commit_lock;
    struct dentry *debugfs_entry;
    struct drm_crtc_crc crc;
    unsigned int fence_context;
    spinlock_t fence_lock;
    unsigned long fence_seqno;
    char timeline_name[32];
    struct drm_self_refresh_data *self_refresh_data;
};
```

**Members**

`dev`
:   parent DRM device

`port`
:   OF node used by [`drm_of_find_possible_crtcs()`](drm-kms-helpers.html#c.drm_of_find_possible_crtcs "drm_of_find_possible_crtcs").

`head`
:   List of all CRTCs on **dev**, linked from [`drm_mode_config.crtc_list`](#c.drm_mode_config "drm_mode_config").
    Invariant over the lifetime of **dev** and therefore does not need
    locking.

`name`
:   human readable name, can be overwritten by the driver

`mutex`
:   This provides a read lock for the overall CRTC state (mode, dpms
    state, ...) and a write lock for everything which can be update
    without a full modeset (fb, cursor data, CRTC properties ...). A full
    modeset also need to grab [`drm_mode_config.connection_mutex`](#c.drm_mode_config "drm_mode_config").

    For atomic drivers specifically this protects **state**.

`base`
:   base KMS object for ID tracking etc.

`primary`
:   Primary plane for this CRTC. Note that this is only
    relevant for legacy IOCTL, it specifies the plane implicitly used by
    the SETCRTC and PAGE\_FLIP IOCTLs. It does not have any significance
    beyond that.

`cursor`
:   Cursor plane for this CRTC. Note that this is only relevant for
    legacy IOCTL, it specifies the plane implicitly used by the SETCURSOR
    and SETCURSOR2 IOCTLs. It does not have any significance
    beyond that.

`index`
:   Position inside the mode\_config.list, can be used as an array
    index. It is invariant over the lifetime of the CRTC.

`cursor_x`
:   Current x position of the cursor, used for universal
    cursor planes because the SETCURSOR IOCTL only can update the
    framebuffer without supplying the coordinates. Drivers should not use
    this directly, atomic drivers should look at [`drm_plane_state.crtc_x`](#c.drm_plane_state "drm_plane_state")
    of the cursor plane instead.

`cursor_y`
:   Current y position of the cursor, used for universal
    cursor planes because the SETCURSOR IOCTL only can update the
    framebuffer without supplying the coordinates. Drivers should not use
    this directly, atomic drivers should look at [`drm_plane_state.crtc_y`](#c.drm_plane_state "drm_plane_state")
    of the cursor plane instead.

`enabled`
:   Is this CRTC enabled? Should only be used by legacy drivers, atomic
    drivers should instead consult [`drm_crtc_state.enable`](#c.drm_crtc_state "drm_crtc_state") and
    [`drm_crtc_state.active`](#c.drm_crtc_state "drm_crtc_state"). Atomic drivers can update this by calling
    [`drm_atomic_helper_update_legacy_modeset_state()`](drm-kms-helpers.html#c.drm_atomic_helper_update_legacy_modeset_state "drm_atomic_helper_update_legacy_modeset_state").

`mode`
:   Current mode timings. Should only be used by legacy drivers, atomic
    drivers should instead consult [`drm_crtc_state.mode`](#c.drm_crtc_state "drm_crtc_state"). Atomic drivers
    can update this by calling
    [`drm_atomic_helper_update_legacy_modeset_state()`](drm-kms-helpers.html#c.drm_atomic_helper_update_legacy_modeset_state "drm_atomic_helper_update_legacy_modeset_state").

`hwmode`
:   Programmed mode in hw, after adjustments for encoders, crtc, panel
    scaling etc. Should only be used by legacy drivers, for high
    precision vblank timestamps in
    [`drm_crtc_vblank_helper_get_vblank_timestamp()`](#c.drm_crtc_vblank_helper_get_vblank_timestamp "drm_crtc_vblank_helper_get_vblank_timestamp").

    Note that atomic drivers should not use this, but instead use
    [`drm_crtc_state.adjusted_mode`](#c.drm_crtc_state "drm_crtc_state"). And for high-precision timestamps
    [`drm_crtc_vblank_helper_get_vblank_timestamp()`](#c.drm_crtc_vblank_helper_get_vblank_timestamp "drm_crtc_vblank_helper_get_vblank_timestamp") used
    [`drm_vblank_crtc.hwmode`](#c.drm_vblank_crtc "drm_vblank_crtc"),
    which is filled out by calling [`drm_calc_timestamping_constants()`](#c.drm_calc_timestamping_constants "drm_calc_timestamping_constants").

`x`
:   x position on screen. Should only be used by legacy drivers, atomic
    drivers should look at [`drm_plane_state.crtc_x`](#c.drm_plane_state "drm_plane_state") of the primary plane
    instead. Updated by calling
    [`drm_atomic_helper_update_legacy_modeset_state()`](drm-kms-helpers.html#c.drm_atomic_helper_update_legacy_modeset_state "drm_atomic_helper_update_legacy_modeset_state").

`y`
:   y position on screen. Should only be used by legacy drivers, atomic
    drivers should look at [`drm_plane_state.crtc_y`](#c.drm_plane_state "drm_plane_state") of the primary plane
    instead. Updated by calling
    [`drm_atomic_helper_update_legacy_modeset_state()`](drm-kms-helpers.html#c.drm_atomic_helper_update_legacy_modeset_state "drm_atomic_helper_update_legacy_modeset_state").

`funcs`
:   CRTC control functions

`gamma_size`
:   Size of legacy gamma ramp reported to userspace. Set up
    by calling [`drm_mode_crtc_set_gamma_size()`](#c.drm_mode_crtc_set_gamma_size "drm_mode_crtc_set_gamma_size").

    Note that atomic drivers need to instead use
    [`drm_crtc_state.gamma_lut`](#c.drm_crtc_state "drm_crtc_state"). See [`drm_crtc_enable_color_mgmt()`](#c.drm_crtc_enable_color_mgmt "drm_crtc_enable_color_mgmt").

`gamma_store`
:   Gamma ramp values used by the legacy SETGAMMA and
    GETGAMMA IOCTls. Set up by calling [`drm_mode_crtc_set_gamma_size()`](#c.drm_mode_crtc_set_gamma_size "drm_mode_crtc_set_gamma_size").

    Note that atomic drivers need to instead use
    [`drm_crtc_state.gamma_lut`](#c.drm_crtc_state "drm_crtc_state"). See [`drm_crtc_enable_color_mgmt()`](#c.drm_crtc_enable_color_mgmt "drm_crtc_enable_color_mgmt").

`helper_private`
:   mid-layer private data

`properties`
:   property tracking for this CRTC

`scaling_filter_property`
:   property to apply a particular filter while
    scaling.

`sharpness_strength_property`
:   property to apply
    the intensity of the sharpness requested.

`state`
:   Current atomic state for this CRTC.

    This is protected by **mutex**. Note that nonblocking atomic commits
    access the current CRTC state without taking locks. Either by going
    through the [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") pointers, see
    [`for_each_oldnew_crtc_in_state()`](#c.for_each_oldnew_crtc_in_state "for_each_oldnew_crtc_in_state"), [`for_each_old_crtc_in_state()`](#c.for_each_old_crtc_in_state "for_each_old_crtc_in_state") and
    [`for_each_new_crtc_in_state()`](#c.for_each_new_crtc_in_state "for_each_new_crtc_in_state"). Or through careful ordering of atomic
    commit operations as implemented in the atomic helpers, see
    [`struct drm_crtc_commit`](#c.drm_crtc_commit "drm_crtc_commit").

`commit_list`
:   List of [`drm_crtc_commit`](#c.drm_crtc_commit "drm_crtc_commit") structures tracking pending commits.
    Protected by **commit\_lock**. This list holds its own full reference,
    as does the ongoing commit.

    “Note that the commit for a state change is also tracked in
    [`drm_crtc_state.commit`](#c.drm_crtc_state "drm_crtc_state"). For accessing the immediately preceding
    commit in an atomic update it is recommended to just use that
    pointer in the old CRTC state, since accessing that doesn’t need
    any locking or list-walking. **commit\_list** should only be used to
    stall for framebuffer cleanup that’s signalled through
    [`drm_crtc_commit.cleanup_done`](#c.drm_crtc_commit "drm_crtc_commit").”

`commit_lock`
:   Spinlock to protect **commit\_list**.

`debugfs_entry`
:   Debugfs directory for this CRTC.

`crc`
:   Configuration settings of CRC capture.

`fence_context`
:   timeline context used for fence operations.

`fence_lock`
:   spinlock to protect the fences in the fence\_context.

`fence_seqno`
:   Seqno variable used as monotonic counter for the fences
    created on the CRTC’s timeline.

`timeline_name`
:   The name of the CRTC’s fence timeline.

`self_refresh_data`
:   Holds the state for the self refresh helpers

    Initialized via [`drm_self_refresh_helper_init()`](drm-kms-helpers.html#c.drm_self_refresh_helper_init "drm_self_refresh_helper_init").

**Description**

Each CRTC may have one or more connectors associated with it. This structure
allows the CRTC to be controlled.

struct drm\_mode\_set
:   new values for a CRTC config change

**Definition**:

```
struct drm_mode_set {
    struct drm_framebuffer *fb;
    struct drm_crtc *crtc;
    struct drm_display_mode *mode;
    uint32_t x;
    uint32_t y;
    struct drm_connector **connectors;
    size_t num_connectors;
};
```

**Members**

`fb`
:   framebuffer to use for new config

`crtc`
:   CRTC whose configuration we’re about to change

`mode`
:   mode timings to use

`x`
:   position of this CRTC relative to **fb**

`y`
:   position of this CRTC relative to **fb**

`connectors`
:   array of connectors to drive with this CRTC if possible

`num_connectors`
:   size of **connectors** array

**Description**

This represents a modeset configuration for the legacy SETCRTC ioctl and is
also used internally. Atomic drivers instead use [`drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state").

drmm\_crtc\_alloc\_with\_planes

`drmm_crtc_alloc_with_planes (dev, type, member, primary, cursor, funcs, name, ...)`

> Allocate and initialize a new CRTC object with specified primary and cursor planes.

**Parameters**

`dev`
:   DRM device

`type`
:   the type of the struct which contains struct [`drm_crtc`](#c.drm_crtc "drm_crtc")

`member`
:   the name of the [`drm_crtc`](#c.drm_crtc "drm_crtc") within **type**.

`primary`
:   Primary plane for CRTC

`cursor`
:   Cursor plane for CRTC

`funcs`
:   callbacks for the new CRTC

`name`
:   printf style format string for the CRTC name, or NULL for default name

`...`
:   variable arguments

**Description**

Allocates and initializes a new crtc object. Cleanup is automatically
handled through registering `drmm_crtc_cleanup()` with [`drmm_add_action()`](drm-internals.html#c.drmm_add_action "drmm_add_action").

The **drm\_crtc\_funcs.destroy** hook must be NULL.

**Return**

Pointer to new crtc, or ERR\_PTR on failure.

unsigned int drm\_crtc\_index(const struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   find the index of a registered CRTC

**Parameters**

`const struct drm_crtc *crtc`
:   CRTC to find index for

**Description**

Given a registered CRTC, return the index of that CRTC within a DRM
device’s list of CRTCs.

uint32\_t drm\_crtc\_mask(const struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   find the mask of a registered CRTC

**Parameters**

`const struct drm_crtc *crtc`
:   CRTC to find mask for

**Description**

Given a registered CRTC, return the mask bit of that CRTC for the
[`drm_encoder.possible_crtcs`](#c.drm_encoder "drm_encoder") and [`drm_plane.possible_crtcs`](#c.drm_plane "drm_plane") fields.

struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*drm\_crtc\_find(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_priv, uint32\_t id)
:   look up a CRTC object from its ID

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_file *file_priv`
:   drm file to check for lease against.

`uint32_t id`
:   [`drm_mode_object`](#c.drm_mode_object "drm_mode_object") ID

**Description**

This can be used to look up a CRTC from its userspace ID. Only used by
drivers for legacy IOCTLs and interface, nowadays extensions to the KMS
userspace interface should be done using [`drm_property`](#c.drm_property "drm_property").

drm\_for\_each\_crtc

`drm_for_each_crtc (crtc, dev)`

> iterate over all CRTCs

**Parameters**

`crtc`
:   a [`struct drm_crtc`](#c.drm_crtc "drm_crtc") as the loop cursor

`dev`
:   the [`struct drm_device`](drm-internals.html#c.drm_device "drm_device")

**Description**

Iterate over all CRTCs of **dev**.

drm\_for\_each\_crtc\_reverse

`drm_for_each_crtc_reverse (crtc, dev)`

> iterate over all CRTCs in reverse order

**Parameters**

`crtc`
:   a [`struct drm_crtc`](#c.drm_crtc "drm_crtc") as the loop cursor

`dev`
:   the [`struct drm_device`](drm-internals.html#c.drm_device "drm_device")

**Description**

Iterate over all CRTCs of **dev**.

struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*drm\_crtc\_from\_index(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, int idx)
:   find the registered CRTC at an index

**Parameters**

`struct drm_device *dev`
:   DRM device

`int idx`
:   index of registered CRTC to find for

**Description**

Given a CRTC index, return the registered CRTC from DRM device’s
list of CRTCs with matching index. This is the inverse of [`drm_crtc_index()`](#c.drm_crtc_index "drm_crtc_index").
It’s useful in the vblank callbacks (like [`drm_driver.enable_vblank`](drm-internals.html#c.drm_driver "drm_driver") or
[`drm_driver.disable_vblank`](drm-internals.html#c.drm_driver "drm_driver")), since that still deals with indices instead
of pointers to [`struct drm_crtc`](#c.drm_crtc "drm_crtc").”

int drm\_crtc\_init\_with\_planes(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_plane](#c.drm_plane "drm_plane") \*primary, struct [drm\_plane](#c.drm_plane "drm_plane") \*cursor, const struct [drm\_crtc\_funcs](#c.drm_crtc_funcs "drm_crtc_funcs") \*funcs, const char \*name, ...)
:   Initialise a new CRTC object with specified primary and cursor planes.

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_crtc *crtc`
:   CRTC object to init

`struct drm_plane *primary`
:   Primary plane for CRTC

`struct drm_plane *cursor`
:   Cursor plane for CRTC

`const struct drm_crtc_funcs *funcs`
:   callbacks for the new CRTC

`const char *name`
:   printf style format string for the CRTC name, or NULL for default name

`...`
:   variable arguments

**Description**

Inits a new object created as base part of a driver crtc object. Drivers
should use this function instead of [`drm_crtc_init()`](drm-kms-helpers.html#c.drm_crtc_init "drm_crtc_init"), which is only provided
for backwards compatibility with drivers which do not yet support universal
planes).
The [`drm_crtc_funcs.destroy`](#c.drm_crtc_funcs "drm_crtc_funcs") hook should call [`drm_crtc_cleanup()`](#c.drm_crtc_cleanup "drm_crtc_cleanup") and [`kfree()`](../core-api/mm-api.html#c.kfree "kfree")
the crtc structure. The crtc structure should not be allocated with
`devm_kzalloc()`.

The **primary** and **cursor** planes are only relevant for legacy uAPI, see
[`drm_crtc.primary`](#c.drm_crtc "drm_crtc") and [`drm_crtc.cursor`](#c.drm_crtc "drm_crtc").

**Note**

consider using [`drmm_crtc_alloc_with_planes()`](#c.drmm_crtc_alloc_with_planes "drmm_crtc_alloc_with_planes") or
[`drmm_crtc_init_with_planes()`](#c.drmm_crtc_init_with_planes "drmm_crtc_init_with_planes") instead of [`drm_crtc_init_with_planes()`](#c.drm_crtc_init_with_planes "drm_crtc_init_with_planes")
to let the DRM managed resource infrastructure take care of cleanup
and deallocation.

**Return**

Zero on success, error code on failure.

int drmm\_crtc\_init\_with\_planes(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_plane](#c.drm_plane "drm_plane") \*primary, struct [drm\_plane](#c.drm_plane "drm_plane") \*cursor, const struct [drm\_crtc\_funcs](#c.drm_crtc_funcs "drm_crtc_funcs") \*funcs, const char \*name, ...)
:   Initialise a new CRTC object with specified primary and cursor planes.

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_crtc *crtc`
:   CRTC object to init

`struct drm_plane *primary`
:   Primary plane for CRTC

`struct drm_plane *cursor`
:   Cursor plane for CRTC

`const struct drm_crtc_funcs *funcs`
:   callbacks for the new CRTC

`const char *name`
:   printf style format string for the CRTC name, or NULL for default name

`...`
:   variable arguments

**Description**

Inits a new object created as base part of a driver crtc object. Drivers
should use this function instead of [`drm_crtc_init()`](drm-kms-helpers.html#c.drm_crtc_init "drm_crtc_init"), which is only provided
for backwards compatibility with drivers which do not yet support universal
planes).

Cleanup is automatically handled through registering
`drmm_crtc_cleanup()` with [`drmm_add_action()`](drm-internals.html#c.drmm_add_action "drmm_add_action"). The crtc structure should
be allocated with [`drmm_kzalloc()`](drm-internals.html#c.drmm_kzalloc "drmm_kzalloc").

The **drm\_crtc\_funcs.destroy** hook must be NULL.

The **primary** and **cursor** planes are only relevant for legacy uAPI, see
[`drm_crtc.primary`](#c.drm_crtc "drm_crtc") and [`drm_crtc.cursor`](#c.drm_crtc "drm_crtc").

**Return**

Zero on success, error code on failure.

void drm\_crtc\_cleanup(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   Clean up the core crtc usage

**Parameters**

`struct drm_crtc *crtc`
:   CRTC to cleanup

**Description**

This function cleans up **crtc** and removes it from the DRM mode setting
core. Note that the function does *not* free the crtc structure itself,
this is the responsibility of the caller.

int drm\_mode\_set\_config\_internal(struct [drm\_mode\_set](#c.drm_mode_set "drm_mode_set") \*set)
:   helper to call [`drm_mode_config_funcs.set_config`](#c.drm_mode_config_funcs "drm_mode_config_funcs")

**Parameters**

`struct drm_mode_set *set`
:   modeset config to set

**Description**

This is a little helper to wrap internal calls to the
[`drm_mode_config_funcs.set_config`](#c.drm_mode_config_funcs "drm_mode_config_funcs") driver interface. The only thing it adds is
correct refcounting dance.

This should only be used by non-atomic legacy drivers.

**Return**

Zero on success, negative errno on failure.

int drm\_crtc\_check\_viewport(const struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, int x, int y, const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode, const struct [drm\_framebuffer](#c.drm_framebuffer "drm_framebuffer") \*fb)
:   Checks that a framebuffer is big enough for the CRTC viewport

**Parameters**

`const struct drm_crtc *crtc`
:   CRTC that framebuffer will be displayed on

`int x`
:   x panning

`int y`
:   y panning

`const struct drm_display_mode *mode`
:   mode that framebuffer will be displayed under

`const struct drm_framebuffer *fb`
:   framebuffer to check size of

int drm\_crtc\_create\_scaling\_filter\_property(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, unsigned int supported\_filters)
:   create a new scaling filter property

**Parameters**

`struct drm_crtc *crtc`
:   drm CRTC

`unsigned int supported_filters`
:   bitmask of supported scaling filters, must include
    BIT(DRM\_SCALING\_FILTER\_DEFAULT).

**Description**

This function lets driver to enable the scaling filter property on a given
CRTC.

**Return**

Zero for success or -errno

bool drm\_crtc\_in\_clone\_mode(struct [drm\_crtc\_state](#c.drm_crtc_state "drm_crtc_state") \*crtc\_state)
:   check if the given CRTC state is in clone mode

**Parameters**

`struct drm_crtc_state *crtc_state`
:   CRTC state to check

**Description**

This function determines if the given CRTC state is being cloned by multiple
encoders.

**Return**

True if the CRTC state is in clone mode. False otherwise

### Color Management Functions Reference

u64 drm\_color\_ctm\_s31\_32\_to\_qm\_n(u64 user\_input, u32 m, u32 n)

**Parameters**

`u64 user_input`
:   input value

`u32 m`
:   number of integer bits, only support m <= 32, include the sign-bit

`u32 n`
:   number of fractional bits, only support n <= 32

**Description**

Convert and clamp S31.32 sign-magnitude to Qm.n (signed 2’s complement).
The sign-bit BIT(m+n-1) and above are 0 for positive value and 1 for negative
the range of value is [-2^(m-1), 2^(m-1) - 2^-n]

For example
A Q3.12 format number:
- required bit: 3 + 12 = 15bits
- range: [-2^2, 2^2 - 2^−15]

**NOTE**

the m can be zero if all bit\_precision are used to present fractional
:   bits like Q0.32

void drm\_crtc\_enable\_color\_mgmt(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, uint degamma\_lut\_size, bool has\_ctm, uint gamma\_lut\_size)
:   enable color management properties

**Parameters**

`struct drm_crtc *crtc`
:   DRM CRTC

`uint degamma_lut_size`
:   the size of the degamma lut (before CSC)

`bool has_ctm`
:   whether to attach ctm\_property for CSC matrix

`uint gamma_lut_size`
:   the size of the gamma lut (after CSC)

**Description**

This function lets the driver enable the color correction
properties on a CRTC. This includes 3 degamma, csc and gamma
properties that userspace can set and 2 size properties to inform
the userspace of the lut sizes. Each of the properties are
optional. The gamma and degamma properties are only attached if
their size is not 0 and ctm\_property is only attached if has\_ctm is
true.

int drm\_mode\_crtc\_set\_gamma\_size(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, int gamma\_size)
:   set the gamma table size

**Parameters**

`struct drm_crtc *crtc`
:   CRTC to set the gamma table size for

`int gamma_size`
:   size of the gamma table

**Description**

Drivers which support gamma tables should set this to the supported gamma
table size when initializing the CRTC. Currently the drm core only supports a
fixed gamma table size.

**Return**

Zero on success, negative errno on failure.

int drm\_plane\_create\_color\_properties(struct [drm\_plane](#c.drm_plane "drm_plane") \*plane, u32 supported\_encodings, u32 supported\_ranges, enum drm\_color\_encoding default\_encoding, enum drm\_color\_range default\_range)
:   color encoding related plane properties

**Parameters**

`struct drm_plane *plane`
:   plane object

`u32 supported_encodings`
:   bitfield indicating supported color encodings

`u32 supported_ranges`
:   bitfileld indicating supported color ranges

`enum drm_color_encoding default_encoding`
:   default color encoding

`enum drm_color_range default_range`
:   default color range

**Description**

Create and attach plane specific COLOR\_ENCODING and COLOR\_RANGE
properties to **plane**. The supported encodings and ranges should
be provided in supported\_encodings and supported\_ranges bitmasks.
Each bit set in the bitmask indicates that its number as `enum
value` is supported.

int drm\_color\_lut\_check(const struct [drm\_property\_blob](#c.drm_property_blob "drm_property_blob") \*lut, u32 tests)
:   check validity of lookup table

**Parameters**

`const struct drm_property_blob *lut`
:   property blob containing LUT to check

`u32 tests`
:   bitmask of tests to run

**Description**

Helper to check whether a userspace-provided lookup table is valid and
satisfies hardware requirements. Drivers pass a bitmask indicating which of
the tests in [`drm_color_lut_tests`](#c.drm_color_lut_tests "drm_color_lut_tests") should be performed.

Returns 0 on success, -EINVAL on failure.

void drm\_crtc\_load\_gamma\_888(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, const struct drm\_color\_lut \*lut, drm\_crtc\_set\_lut\_func set\_gamma)
:   Programs gamma ramp for RGB888-like formats

**Parameters**

`struct drm_crtc *crtc`
:   The displaying CRTC

`const struct drm_color_lut *lut`
:   The gamma ramp to program

`drm_crtc_set_lut_func set_gamma`
:   Callback for programming the hardware gamma LUT

**Description**

Programs the gamma ramp specified in **lut** to hardware. The input gamma
ramp must have 256 entries per color component.

void drm\_crtc\_load\_gamma\_565\_from\_888(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, const struct drm\_color\_lut \*lut, drm\_crtc\_set\_lut\_func set\_gamma)
:   Programs gamma ramp for RGB565-like formats

**Parameters**

`struct drm_crtc *crtc`
:   The displaying CRTC

`const struct drm_color_lut *lut`
:   The gamma ramp to program

`drm_crtc_set_lut_func set_gamma`
:   Callback for programming the hardware gamma LUT

**Description**

Programs the gamma ramp specified in **lut** to hardware. The input gamma
ramp must have 256 entries per color component. The helper interpolates
the individual color components to reduce the number of entries to 5/6/5.

void drm\_crtc\_load\_gamma\_555\_from\_888(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, const struct drm\_color\_lut \*lut, drm\_crtc\_set\_lut\_func set\_gamma)
:   Programs gamma ramp for RGB555-like formats

**Parameters**

`struct drm_crtc *crtc`
:   The displaying CRTC

`const struct drm_color_lut *lut`
:   The gamma ramp to program

`drm_crtc_set_lut_func set_gamma`
:   Callback for programming the hardware gamma LUT

**Description**

Programs the gamma ramp specified in **lut** to hardware. The input gamma
ramp must have 256 entries per color component. The helper interpolates
the individual color components to reduce the number of entries to 5/5/5.

void drm\_crtc\_fill\_gamma\_888(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, drm\_crtc\_set\_lut\_func set\_gamma)
:   Programs a default gamma ramp for RGB888-like formats

**Parameters**

`struct drm_crtc *crtc`
:   The displaying CRTC

`drm_crtc_set_lut_func set_gamma`
:   Callback for programming the hardware gamma LUT

**Description**

Programs a default gamma ramp to hardware.

void drm\_crtc\_fill\_gamma\_565(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, drm\_crtc\_set\_lut\_func set\_gamma)
:   Programs a default gamma ramp for RGB565-like formats

**Parameters**

`struct drm_crtc *crtc`
:   The displaying CRTC

`drm_crtc_set_lut_func set_gamma`
:   Callback for programming the hardware gamma LUT

**Description**

Programs a default gamma ramp to hardware.

void drm\_crtc\_fill\_gamma\_555(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, drm\_crtc\_set\_lut\_func set\_gamma)
:   Programs a default gamma ramp for RGB555-like formats

**Parameters**

`struct drm_crtc *crtc`
:   The displaying CRTC

`drm_crtc_set_lut_func set_gamma`
:   Callback for programming the hardware gamma LUT

**Description**

Programs a default gamma ramp to hardware.

void drm\_crtc\_load\_palette\_8(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, const struct drm\_color\_lut \*lut, drm\_crtc\_set\_lut\_func set\_palette)
:   Programs palette for C8-like formats

**Parameters**

`struct drm_crtc *crtc`
:   The displaying CRTC

`const struct drm_color_lut *lut`
:   The palette to program

`drm_crtc_set_lut_func set_palette`
:   Callback for programming the hardware palette

**Description**

Programs the palette specified in **lut** to hardware. The input palette
must have 256 entries per color component.

void drm\_crtc\_fill\_palette\_332(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, drm\_crtc\_set\_lut\_func set\_palette)
:   Programs a default palette for RGB332-like formats

**Parameters**

`struct drm_crtc *crtc`
:   The displaying CRTC

`drm_crtc_set_lut_func set_palette`
:   Callback for programming the hardware gamma LUT

**Description**

Programs an RGB332 palette to hardware.

void drm\_crtc\_fill\_palette\_8(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, drm\_crtc\_set\_lut\_func set\_palette)
:   Programs a default palette for C8-like formats

**Parameters**

`struct drm_crtc *crtc`
:   The displaying CRTC

`drm_crtc_set_lut_func set_palette`
:   Callback for programming the hardware gamma LUT

**Description**

Programs a default palette to hardware.

int drm\_color\_lut32\_check(const struct [drm\_property\_blob](#c.drm_property_blob "drm_property_blob") \*lut, u32 tests)
:   check validity of extended lookup table

**Parameters**

`const struct drm_property_blob *lut`
:   property blob containing extended LUT to check

`u32 tests`
:   bitmask of tests to run

**Description**

Helper to check whether a userspace-provided extended lookup table is valid and
satisfies hardware requirements. Drivers pass a bitmask indicating which of
the tests in [`drm_color_lut_tests`](#c.drm_color_lut_tests "drm_color_lut_tests") should be performed.

Returns 0 on success, -EINVAL on failure.

u32 drm\_color\_lut\_extract(u32 user\_input, int bit\_precision)
:   clamp and round LUT entries

**Parameters**

`u32 user_input`
:   input value

`int bit_precision`
:   number of bits the hw LUT supports

**Description**

Extract a degamma/gamma LUT value provided by user (in the form of
`drm_color_lut` entries) and round it to the precision supported by the
hardware, following OpenGL int<->float conversion rules
(see eg. OpenGL 4.6 specification - 2.3.5 Fixed-Point Data Conversions).

u32 drm\_color\_lut32\_extract(u32 user\_input, int bit\_precision)
:   clamp and round LUT entries

**Parameters**

`u32 user_input`
:   input value

`int bit_precision`
:   number of bits the hw LUT supports

**Description**

Extract U0.bit\_precision from a U0.32 LUT value.

int drm\_color\_lut\_size(const struct [drm\_property\_blob](#c.drm_property_blob "drm_property_blob") \*blob)
:   calculate the number of entries in the LUT

**Parameters**

`const struct drm_property_blob *blob`
:   blob containing the LUT

**Return**

The number of entries in the color LUT stored in **blob**.

int drm\_color\_lut32\_size(const struct [drm\_property\_blob](#c.drm_property_blob "drm_property_blob") \*blob)
:   calculate the number of entries in the extended LUT

**Parameters**

`const struct drm_property_blob *blob`
:   blob containing the LUT

**Return**

The number of entries in the color LUT stored in **blob**.

enum drm\_color\_lut\_tests
:   hw-specific LUT tests to perform

**Constants**

`DRM_COLOR_LUT_EQUAL_CHANNELS`
:   Checks whether the entries of a LUT all have equal values for the
    red, green, and blue channels. Intended for hardware that only
    accepts a single value per LUT entry and assumes that value applies
    to all three color components.

`DRM_COLOR_LUT_NON_DECREASING`
:   Checks whether the entries of a LUT are always flat or increasing
    (never decreasing).

**Description**

The [`drm_color_lut_check()`](#c.drm_color_lut_check "drm_color_lut_check") function takes a bitmask of the values here to
determine which tests to apply to a userspace-provided LUT.

## Frame Buffer Abstraction

Frame buffers are abstract memory objects that provide a source of pixels to
scanout to a CRTC. Applications explicitly request the creation of frame
buffers through the DRM\_IOCTL\_MODE\_ADDFB(2) ioctls and receive an opaque
handle that can be passed to the KMS CRTC control, plane configuration and
page flip functions.

Frame buffers rely on the underlying memory manager for allocating backing
storage. When creating a frame buffer applications pass a memory handle
(or a list of memory handles for multi-planar formats) through the
[`struct drm_mode_fb_cmd2`](drm-uapi.html#c.drm_mode_fb_cmd2 "drm_mode_fb_cmd2") argument. For drivers using GEM as their userspace
buffer management interface this would be a GEM handle. Drivers are however
free to use their own backing storage object handles, e.g. vmwgfx directly
exposes special TTM handles to userspace and so expects TTM handles in the
create ioctl and not GEM handles.

Framebuffers are tracked with [`struct drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer"). They are published
using [`drm_framebuffer_init()`](#c.drm_framebuffer_init "drm_framebuffer_init") - after calling that function userspace can use
and access the framebuffer object. The helper function
[`drm_helper_mode_fill_fb_struct()`](drm-kms-helpers.html#c.drm_helper_mode_fill_fb_struct "drm_helper_mode_fill_fb_struct") can be used to pre-fill the required
metadata fields.

The lifetime of a drm framebuffer is controlled with a reference count,
drivers can grab additional references with [`drm_framebuffer_get()`](#c.drm_framebuffer_get "drm_framebuffer_get") and drop
them again with [`drm_framebuffer_put()`](#c.drm_framebuffer_put "drm_framebuffer_put"). For driver-private framebuffers for
which the last reference is never dropped (e.g. for the fbdev framebuffer
when the struct [`struct drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer") is embedded into the fbdev helper
struct) drivers can manually clean up a framebuffer at module unload time
with [`drm_framebuffer_unregister_private()`](#c.drm_framebuffer_unregister_private "drm_framebuffer_unregister_private"). But doing this is not
recommended, and it’s better to have a normal free-standing [`struct
drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer").

### Frame Buffer Functions Reference

struct drm\_framebuffer\_funcs
:   framebuffer hooks

**Definition**:

```
struct drm_framebuffer_funcs {
    void (*destroy)(struct drm_framebuffer *framebuffer);
    int (*create_handle)(struct drm_framebuffer *fb, struct drm_file *file_priv, unsigned int *handle);
    int (*dirty)(struct drm_framebuffer *framebuffer, struct drm_file *file_priv, unsigned flags, unsigned color, struct drm_clip_rect *clips, unsigned num_clips);
};
```

**Members**

`destroy`
:   Clean up framebuffer resources, specifically also unreference the
    backing storage. The core guarantees to call this function for every
    framebuffer successfully created by calling
    [`drm_mode_config_funcs.fb_create`](#c.drm_mode_config_funcs "drm_mode_config_funcs"). Drivers must also call
    [`drm_framebuffer_cleanup()`](#c.drm_framebuffer_cleanup "drm_framebuffer_cleanup") to release DRM core resources for this
    framebuffer.

`create_handle`
:   Create a buffer handle in the driver-specific buffer manager (either
    GEM or TTM) valid for the passed-in [`struct drm_file`](drm-internals.html#c.drm_file "drm_file"). This is used by
    the core to implement the GETFB IOCTL, which returns (for
    sufficiently priviledged user) also a native buffer handle. This can
    be used for seamless transitions between modesetting clients by
    copying the current screen contents to a private buffer and blending
    between that and the new contents.

    GEM based drivers should call [`drm_gem_handle_create()`](drm-mm.html#c.drm_gem_handle_create "drm_gem_handle_create") to create the
    handle.

    RETURNS:

    0 on success or a negative error code on failure.

`dirty`
:   Optional callback for the dirty fb IOCTL.

    Userspace can notify the driver via this callback that an area of the
    framebuffer has changed and should be flushed to the display
    hardware. This can also be used internally, e.g. by the fbdev
    emulation, though that’s not the case currently.

    See documentation in drm\_mode.h for the `struct drm_mode_fb_dirty_cmd`
    for more information as all the semantics and arguments have a one to
    one mapping on this function.

    Atomic drivers should use [`drm_atomic_helper_dirtyfb()`](#c.drm_atomic_helper_dirtyfb "drm_atomic_helper_dirtyfb") to implement
    this hook.

    RETURNS:

    0 on success or a negative error code on failure.

struct drm\_framebuffer
:   frame buffer object

**Definition**:

```
struct drm_framebuffer {
    struct drm_device *dev;
    struct list_head head;
    struct drm_mode_object base;
    char comm[TASK_COMM_LEN];
    const struct drm_format_info *format;
    const struct drm_framebuffer_funcs *funcs;
    unsigned int pitches[DRM_FORMAT_MAX_PLANES];
    unsigned int offsets[DRM_FORMAT_MAX_PLANES];
    uint64_t modifier;
    unsigned int width;
    unsigned int height;
    int flags;
    unsigned int internal_flags;
    struct list_head filp_head;
    struct drm_gem_object *obj[DRM_FORMAT_MAX_PLANES];
};
```

**Members**

`dev`
:   DRM device this framebuffer belongs to

`head`
:   Place on the [`drm_mode_config.fb_list`](#c.drm_mode_config "drm_mode_config"), access protected by
    [`drm_mode_config.fb_lock`](#c.drm_mode_config "drm_mode_config").

`base`
:   base modeset object structure, contains the reference count.

`comm`
:   Name of the process allocating the fb, used for fb dumping.

`format`
:   framebuffer format information

`funcs`
:   framebuffer vfunc table

`pitches`
:   Line stride per buffer. For userspace created object this
    is copied from drm\_mode\_fb\_cmd2.

`offsets`
:   Offset from buffer start to the actual pixel data in bytes,
    per buffer. For userspace created object this is copied from
    drm\_mode\_fb\_cmd2.

    Note that this is a linear offset and does not take into account
    tiling or buffer layout per **modifier**. It is meant to be used when
    the actual pixel data for this framebuffer plane starts at an offset,
    e.g. when multiple planes are allocated within the same backing
    storage buffer object. For tiled layouts this generally means its
    **offsets** must at least be tile-size aligned, but hardware often has
    stricter requirements.

    This should not be used to specifiy x/y pixel offsets into the buffer
    data (even for linear buffers). Specifying an x/y pixel offset is
    instead done through the source rectangle in [`struct drm_plane_state`](#c.drm_plane_state "drm_plane_state").

`modifier`
:   Data layout modifier. This is used to describe
    tiling, or also special layouts (like compression) of auxiliary
    buffers. For userspace created object this is copied from
    drm\_mode\_fb\_cmd2.

`width`
:   Logical width of the visible area of the framebuffer, in
    pixels.

`height`
:   Logical height of the visible area of the framebuffer, in
    pixels.

`flags`
:   Framebuffer flags like DRM\_MODE\_FB\_INTERLACED or
    DRM\_MODE\_FB\_MODIFIERS.

`internal_flags`
:   Framebuffer flags like DRM\_FRAMEBUFFER\_HAS\_HANDLE\_REF.

`filp_head`
:   Placed on [`drm_file.fbs`](drm-internals.html#c.drm_file "drm_file"), protected by [`drm_file.fbs_lock`](drm-internals.html#c.drm_file "drm_file").

`obj`
:   GEM objects backing the framebuffer, one per plane (optional).

    This is used by the GEM framebuffer helpers, see e.g.
    [`drm_gem_fb_create()`](drm-kms-helpers.html#c.drm_gem_fb_create "drm_gem_fb_create").

**Description**

Note that the fb is refcounted for the benefit of driver internals,
for example some hw, disabling a CRTC/plane is asynchronous, and
scanout does not actually complete until the next vblank. So some
cleanup (like releasing the reference(s) on the backing GEM bo(s))
should be deferred. In cases like this, the driver would like to
hold a ref to the fb even though it has already been removed from
userspace perspective. See [`drm_framebuffer_get()`](#c.drm_framebuffer_get "drm_framebuffer_get") and
[`drm_framebuffer_put()`](#c.drm_framebuffer_put "drm_framebuffer_put").

The refcount is stored inside the mode object **base**.

void drm\_framebuffer\_get(struct [drm\_framebuffer](#c.drm_framebuffer "drm_framebuffer") \*fb)
:   acquire a framebuffer reference

**Parameters**

`struct drm_framebuffer *fb`
:   DRM framebuffer

**Description**

This function increments the framebuffer’s reference count.

void drm\_framebuffer\_put(struct [drm\_framebuffer](#c.drm_framebuffer "drm_framebuffer") \*fb)
:   release a framebuffer reference

**Parameters**

`struct drm_framebuffer *fb`
:   DRM framebuffer

**Description**

This function decrements the framebuffer’s reference count and frees the
framebuffer if the reference count drops to zero.

uint32\_t drm\_framebuffer\_read\_refcount(const struct [drm\_framebuffer](#c.drm_framebuffer "drm_framebuffer") \*fb)
:   read the framebuffer reference count.

**Parameters**

`const struct drm_framebuffer *fb`
:   framebuffer

**Description**

This functions returns the framebuffer’s reference count.

void drm\_framebuffer\_assign(struct [drm\_framebuffer](#c.drm_framebuffer "drm_framebuffer") \*\*p, struct [drm\_framebuffer](#c.drm_framebuffer "drm_framebuffer") \*fb)
:   store a reference to the fb

**Parameters**

`struct drm_framebuffer **p`
:   location to store framebuffer

`struct drm_framebuffer *fb`
:   new framebuffer (maybe NULL)

**Description**

This functions sets the location to store a reference to the framebuffer,
unreferencing the framebuffer that was previously stored in that location.

struct drm\_afbc\_framebuffer
:   a special afbc frame buffer object

**Definition**:

```
struct drm_afbc_framebuffer {
    struct drm_framebuffer base;
    u32 block_width;
    u32 block_height;
    u32 aligned_width;
    u32 aligned_height;
    u32 offset;
    u32 afbc_size;
};
```

**Members**

`base`
:   base framebuffer structure.

`block_width`
:   width of a single afbc block

`block_height`
:   height of a single afbc block

`aligned_width`
:   aligned frame buffer width

`aligned_height`
:   aligned frame buffer height

`offset`
:   offset of the first afbc header

`afbc_size`
:   minimum size of afbc buffer

**Description**

A derived class of [`struct drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer"), dedicated for afbc use cases.

int drm\_framebuffer\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_framebuffer](#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_framebuffer\_funcs](#c.drm_framebuffer_funcs "drm_framebuffer_funcs") \*funcs)
:   initialize a framebuffer

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_framebuffer *fb`
:   framebuffer to be initialized

`const struct drm_framebuffer_funcs *funcs`
:   ... with these functions

**Description**

Allocates an ID for the framebuffer’s parent mode object, sets its mode
functions & device file and adds it to the master fd list.

IMPORTANT:
This functions publishes the fb and makes it available for concurrent access
by other users. Which means by this point the fb \_must\_ be fully set up -
since all the fb attributes are invariant over its lifetime, no further
locking but only correct reference counting is required.

**Return**

Zero on success, error code on failure.

struct [drm\_framebuffer](#c.drm_framebuffer "drm_framebuffer") \*drm\_framebuffer\_lookup(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_priv, uint32\_t id)
:   look up a drm framebuffer and grab a reference

**Parameters**

`struct drm_device *dev`
:   drm device

`struct drm_file *file_priv`
:   drm file to check for lease against.

`uint32_t id`
:   id of the fb object

**Description**

If successful, this grabs an additional reference to the framebuffer -
callers need to make sure to eventually unreference the returned framebuffer
again, using [`drm_framebuffer_put()`](#c.drm_framebuffer_put "drm_framebuffer_put").

void drm\_framebuffer\_unregister\_private(struct [drm\_framebuffer](#c.drm_framebuffer "drm_framebuffer") \*fb)
:   unregister a private fb from the lookup idr

**Parameters**

`struct drm_framebuffer *fb`
:   fb to unregister

**Description**

Drivers need to call this when cleaning up driver-private framebuffers, e.g.
those used for fbdev. Note that the caller must hold a reference of its own,
i.e. the object may not be destroyed through this call (since it’ll lead to a
locking inversion).

**NOTE**

This function is deprecated. For driver-private framebuffers it is not
recommended to embed a framebuffer `struct info` fbdev struct, instead, a
framebuffer pointer is preferred and [`drm_framebuffer_put()`](#c.drm_framebuffer_put "drm_framebuffer_put") should be called
when the framebuffer is to be cleaned up.

void drm\_framebuffer\_cleanup(struct [drm\_framebuffer](#c.drm_framebuffer "drm_framebuffer") \*fb)
:   remove a framebuffer object

**Parameters**

`struct drm_framebuffer *fb`
:   framebuffer to remove

**Description**

Cleanup framebuffer. This function is intended to be used from the drivers
[`drm_framebuffer_funcs.destroy`](#c.drm_framebuffer_funcs "drm_framebuffer_funcs") callback. It can also be used to clean up
driver private framebuffers embedded into a larger structure.

Note that this function does not remove the fb from active usage - if it is
still used anywhere, hilarity can ensue since userspace could call getfb on
the id and get back -EINVAL. Obviously no concern at driver unload time.

Also, the framebuffer will not be removed from the lookup idr - for
user-created framebuffers this will happen in the rmfb ioctl. For
driver-private objects (e.g. for fbdev) drivers need to explicitly call
drm\_framebuffer\_unregister\_private.

void drm\_framebuffer\_remove(struct [drm\_framebuffer](#c.drm_framebuffer "drm_framebuffer") \*fb)
:   remove and unreference a framebuffer object

**Parameters**

`struct drm_framebuffer *fb`
:   framebuffer to remove

**Description**

Scans all the CRTCs and planes in **dev**’s mode\_config. If they’re
using **fb**, removes it, setting it to NULL. Then drops the reference to the
passed-in framebuffer. Might take the modeset locks.

Note that this function optimizes the cleanup away if the caller holds the
last reference to the framebuffer. It is also guaranteed to not take the
modeset locks in this case.

## DRM Format Handling

In the DRM subsystem, framebuffer pixel formats are described using the
fourcc codes defined in include/uapi/drm/drm\_fourcc.h. In addition to the
fourcc code, a Format Modifier may optionally be provided, in order to
further describe the buffer’s format - for example tiling or compression.

### Format Modifiers

Format modifiers are used in conjunction with a fourcc code, forming a
unique fourcc:modifier pair. This format:modifier pair must fully define the
format and data layout of the buffer, and should be the only way to describe
that particular buffer.

Having multiple fourcc:modifier pairs which describe the same layout should
be avoided, as such aliases run the risk of different drivers exposing
different names for the same data format, forcing userspace to understand
that they are aliases.

Format modifiers may change any property of the buffer, including the number
of planes and/or the required allocation size. Format modifiers are
vendor-namespaced, and as such the relationship between a fourcc code and a
modifier is specific to the modifier being used. For example, some modifiers
may preserve meaning - such as number of planes - from the fourcc code,
whereas others may not.

Modifiers must uniquely encode buffer layout. In other words, a buffer must
match only a single modifier. A modifier must not be a subset of layouts of
another modifier. For instance, it’s incorrect to encode pitch alignment in
a modifier: a buffer may match a 64-pixel aligned modifier and a 32-pixel
aligned modifier. That said, modifiers can have implicit minimal
requirements.

For modifiers where the combination of fourcc code and modifier can alias,
a canonical pair needs to be defined and used by all drivers. Preferred
combinations are also encouraged where all combinations might lead to
confusion and unnecessarily reduced interoperability. An example for the
latter is AFBC, where the ABGR layouts are preferred over ARGB layouts.

There are two kinds of modifier users:

* Kernel and user-space drivers: for drivers it’s important that modifiers
  don’t alias, otherwise two drivers might support the same format but use
  different aliases, preventing them from sharing buffers in an efficient
  format.
* Higher-level programs interfacing with KMS/GBM/EGL/Vulkan/etc: these users
  see modifiers as opaque tokens they can check for equality and intersect.
  These users mustn’t need to know to reason about the modifier value
  (i.e. they are not expected to extract information out of the modifier).

Vendors should document their modifier usage in as much detail as
possible, to ensure maximum compatibility across devices, drivers and
applications.

The authoritative list of format modifier codes is found in
include/uapi/drm/drm\_fourcc.h

### Open Source User Waiver

Because this is the authoritative source for pixel formats and modifiers
referenced by GL, Vulkan extensions and other standards and hence used both
by open source and closed source driver stacks, the usual requirement for an
upstream in-kernel or open source userspace user does not apply.

To ensure, as much as feasible, compatibility across stacks and avoid
confusion with incompatible enumerations stakeholders for all relevant driver
stacks should approve additions.

### Format Functions Reference

DRM\_FORMAT\_MAX\_PLANES

`DRM_FORMAT_MAX_PLANES`

> maximum number of planes a DRM format can have

struct drm\_format\_info
:   information about a DRM format

**Definition**:

```
struct drm_format_info {
    u32 format;
    u8 depth;
    u8 num_planes;
    union {
        u8 cpp[DRM_FORMAT_MAX_PLANES];
        u8 char_per_block[DRM_FORMAT_MAX_PLANES];
    };
    u8 block_w[DRM_FORMAT_MAX_PLANES];
    u8 block_h[DRM_FORMAT_MAX_PLANES];
    u8 hsub;
    u8 vsub;
    bool has_alpha;
    bool is_yuv;
    bool is_color_indexed;
};
```

**Members**

`format`
:   4CC format identifier (DRM\_FORMAT\_\*)

`depth`
:   Color depth (number of bits per pixel excluding padding bits),
    valid for a subset of RGB formats only. This is a legacy field, do
    not use in new code and set to 0 for new formats.

`num_planes`
:   Number of color planes (1 to 3)

`{unnamed_union}`
:   anonymous

`cpp`
:   Number of bytes per pixel (per plane), this is aliased with
    **char\_per\_block**. It is deprecated in favour of using the
    triplet **char\_per\_block**, **block\_w**, **block\_h** for better
    describing the pixel format.

`char_per_block`
:   Number of bytes per block (per plane), where blocks are
    defined as a rectangle of pixels which are stored next to
    each other in a byte aligned memory region. Together with
    **block\_w** and **block\_h** this is used to properly describe tiles
    in tiled formats or to describe groups of pixels in packed
    formats for which the memory needed for a single pixel is not
    byte aligned.

    **cpp** has been kept for historical reasons because there are
    a lot of places in drivers where it’s used. In drm core for
    generic code paths the preferred way is to use
    **char\_per\_block**, [`drm_format_info_block_width()`](#c.drm_format_info_block_width "drm_format_info_block_width") and
    [`drm_format_info_block_height()`](#c.drm_format_info_block_height "drm_format_info_block_height") which allows handling both
    block and non-block formats in the same way.

    For formats that are intended to be used only with non-linear
    modifiers both **cpp** and **char\_per\_block** must be 0 in the
    generic format table. Drivers could supply accurate
    information from their drm\_mode\_config.get\_format\_info hook
    if they want the core to be validating the pitch.

`block_w`
:   Block width in pixels, this is intended to be accessed through
    [`drm_format_info_block_width()`](#c.drm_format_info_block_width "drm_format_info_block_width")

`block_h`
:   Block height in pixels, this is intended to be accessed through
    [`drm_format_info_block_height()`](#c.drm_format_info_block_height "drm_format_info_block_height")

`hsub`
:   Horizontal chroma subsampling factor

`vsub`
:   Vertical chroma subsampling factor

`has_alpha`
:   Does the format embeds an alpha component?

`is_yuv`
:   Is it a YUV format?

`is_color_indexed`
:   Is it a color-indexed format?

bool drm\_format\_info\_is\_yuv\_packed(const struct [drm\_format\_info](#c.drm_format_info "drm_format_info") \*info)
:   check that the format info matches a YUV format with data laid in a single plane

**Parameters**

`const struct drm_format_info *info`
:   format info

**Return**

A boolean indicating whether the format info matches a packed YUV format.

bool drm\_format\_info\_is\_yuv\_semiplanar(const struct [drm\_format\_info](#c.drm_format_info "drm_format_info") \*info)
:   check that the format info matches a YUV format with data laid in two planes (luminance and chrominance)

**Parameters**

`const struct drm_format_info *info`
:   format info

**Return**

A boolean indicating whether the format info matches a semiplanar YUV format.

bool drm\_format\_info\_is\_yuv\_planar(const struct [drm\_format\_info](#c.drm_format_info "drm_format_info") \*info)
:   check that the format info matches a YUV format with data laid in three planes (one for each YUV component)

**Parameters**

`const struct drm_format_info *info`
:   format info

**Return**

A boolean indicating whether the format info matches a planar YUV format.

bool drm\_format\_info\_is\_yuv\_sampling\_410(const struct [drm\_format\_info](#c.drm_format_info "drm_format_info") \*info)
:   check that the format info matches a YUV format with 4:1:0 sub-sampling

**Parameters**

`const struct drm_format_info *info`
:   format info

**Return**

A boolean indicating whether the format info matches a YUV format with 4:1:0
sub-sampling.

bool drm\_format\_info\_is\_yuv\_sampling\_411(const struct [drm\_format\_info](#c.drm_format_info "drm_format_info") \*info)
:   check that the format info matches a YUV format with 4:1:1 sub-sampling

**Parameters**

`const struct drm_format_info *info`
:   format info

**Return**

A boolean indicating whether the format info matches a YUV format with 4:1:1
sub-sampling.

bool drm\_format\_info\_is\_yuv\_sampling\_420(const struct [drm\_format\_info](#c.drm_format_info "drm_format_info") \*info)
:   check that the format info matches a YUV format with 4:2:0 sub-sampling

**Parameters**

`const struct drm_format_info *info`
:   format info

**Return**

A boolean indicating whether the format info matches a YUV format with 4:2:0
sub-sampling.

bool drm\_format\_info\_is\_yuv\_sampling\_422(const struct [drm\_format\_info](#c.drm_format_info "drm_format_info") \*info)
:   check that the format info matches a YUV format with 4:2:2 sub-sampling

**Parameters**

`const struct drm_format_info *info`
:   format info

**Return**

A boolean indicating whether the format info matches a YUV format with 4:2:2
sub-sampling.

bool drm\_format\_info\_is\_yuv\_sampling\_444(const struct [drm\_format\_info](#c.drm_format_info "drm_format_info") \*info)
:   check that the format info matches a YUV format with 4:4:4 sub-sampling

**Parameters**

`const struct drm_format_info *info`
:   format info

**Return**

A boolean indicating whether the format info matches a YUV format with 4:4:4
sub-sampling.

int drm\_format\_info\_plane\_width(const struct [drm\_format\_info](#c.drm_format_info "drm_format_info") \*info, int width, int plane)
:   width of the plane given the first plane

**Parameters**

`const struct drm_format_info *info`
:   pixel format info

`int width`
:   width of the first plane

`int plane`
:   plane index

**Return**

The width of **plane**, given that the width of the first plane is **width**.

int drm\_format\_info\_plane\_height(const struct [drm\_format\_info](#c.drm_format_info "drm_format_info") \*info, int height, int plane)
:   height of the plane given the first plane

**Parameters**

`const struct drm_format_info *info`
:   pixel format info

`int height`
:   height of the first plane

`int plane`
:   plane index

**Return**

The height of **plane**, given that the height of the first plane is **height**.

uint32\_t drm\_mode\_legacy\_fb\_format(uint32\_t bpp, uint32\_t depth)
:   compute drm fourcc code from legacy description

**Parameters**

`uint32_t bpp`
:   bits per pixels

`uint32_t depth`
:   bit depth per pixel

**Description**

Computes a drm fourcc pixel format code for the given **bpp**/**depth** values.

uint32\_t drm\_driver\_legacy\_fb\_format(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, uint32\_t bpp, uint32\_t depth)
:   compute drm fourcc code from legacy description

**Parameters**

`struct drm_device *dev`
:   DRM device

`uint32_t bpp`
:   bits per pixels

`uint32_t depth`
:   bit depth per pixel

**Description**

Computes a drm fourcc pixel format code for the given **bpp**/**depth** values.
Unlike [`drm_mode_legacy_fb_format()`](#c.drm_mode_legacy_fb_format "drm_mode_legacy_fb_format") this looks at the drivers mode\_config,
and depending on the [`drm_mode_config.quirk_addfb_prefer_host_byte_order`](#c.drm_mode_config "drm_mode_config") flag
it returns little endian byte order or host byte order framebuffer formats.

uint32\_t drm\_driver\_color\_mode\_format(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, unsigned int color\_mode)
:   Compute DRM 4CC code from color mode

**Parameters**

`struct drm_device *dev`
:   DRM device

`unsigned int color_mode`
:   command-line color mode

**Description**

Computes a DRM 4CC pixel format code for the given color mode using
`drm_driver_color_mode()`. The color mode is in the format used and the
kernel command line. It specifies the number of bits per pixel
and color depth in a single value.

Useful in fbdev emulation code, since that deals in those values. The
helper does not consider YUV or other complicated formats. This means
only legacy formats are supported (fmt->depth is a legacy field), but
the framebuffer emulation can only deal with such formats, specifically
RGB/BGA formats.

const struct [drm\_format\_info](#c.drm_format_info "drm_format_info") \*drm\_format\_info(u32 format)
:   query information for a given format

**Parameters**

`u32 format`
:   pixel format (DRM\_FORMAT\_\*)

**Description**

The caller should only pass a supported pixel format to this function.
Unsupported pixel formats will generate a warning in the kernel log.

**Return**

The instance of [`struct drm_format_info`](#c.drm_format_info "drm_format_info") that describes the pixel format, or
NULL if the format is unsupported.

const struct [drm\_format\_info](#c.drm_format_info "drm_format_info") \*drm\_get\_format\_info(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, u32 pixel\_format, u64 modifier)
:   query information for a given framebuffer configuration

**Parameters**

`struct drm_device *dev`
:   DRM device

`u32 pixel_format`
:   pixel format (DRM\_FORMAT\_\*)

`u64 modifier`
:   modifier

**Return**

The instance of [`struct drm_format_info`](#c.drm_format_info "drm_format_info") that describes the pixel format, or
NULL if the format is unsupported.

unsigned int drm\_format\_info\_block\_width(const struct [drm\_format\_info](#c.drm_format_info "drm_format_info") \*info, int plane)
:   width in pixels of block.

**Parameters**

`const struct drm_format_info *info`
:   pixel format info

`int plane`
:   plane index

**Return**

The width in pixels of a block, depending on the plane index.

unsigned int drm\_format\_info\_block\_height(const struct [drm\_format\_info](#c.drm_format_info "drm_format_info") \*info, int plane)
:   height in pixels of a block

**Parameters**

`const struct drm_format_info *info`
:   pixel format info

`int plane`
:   plane index

**Return**

The height in pixels of a block, depending on the plane index.

unsigned int drm\_format\_info\_bpp(const struct [drm\_format\_info](#c.drm_format_info "drm_format_info") \*info, int plane)
:   number of bits per pixel

**Parameters**

`const struct drm_format_info *info`
:   pixel format info

`int plane`
:   plane index

**Return**

The actual number of bits per pixel, depending on the plane index.

uint64\_t drm\_format\_info\_min\_pitch(const struct [drm\_format\_info](#c.drm_format_info "drm_format_info") \*info, int plane, unsigned int buffer\_width)
:   computes the minimum required pitch in bytes

**Parameters**

`const struct drm_format_info *info`
:   pixel format info

`int plane`
:   plane index

`unsigned int buffer_width`
:   buffer width in pixels

**Return**

The minimum required pitch in bytes for a buffer by taking into consideration
the pixel format information and the buffer width.

## Dumb Buffer Objects

The KMS API doesn’t standardize backing storage object creation and leaves it
to driver-specific ioctls. Furthermore actually creating a buffer object even
for GEM-based drivers is done through a driver-specific ioctl - GEM only has
a common userspace interface for sharing and destroying objects. While not an
issue for full-fledged graphics stacks that include device-specific userspace
components (in libdrm for instance), this limit makes DRM-based early boot
graphics unnecessarily complex.

Dumb objects partly alleviate the problem by providing a standard API to
create dumb buffers suitable for scanout, which can then be used to create
KMS frame buffers.

To support dumb objects drivers must implement the [`drm_driver.dumb_create`](drm-internals.html#c.drm_driver "drm_driver")
and [`drm_driver.dumb_map_offset`](drm-internals.html#c.drm_driver "drm_driver") operations (the latter defaults to
[`drm_gem_dumb_map_offset()`](drm-mm.html#c.drm_gem_dumb_map_offset "drm_gem_dumb_map_offset") if not set). Drivers that don’t use GEM handles
additionally need to implement the [`drm_driver.dumb_destroy`](drm-internals.html#c.drm_driver "drm_driver") operation. See
the callbacks for further details.

Note that dumb objects may not be used for gpu acceleration, as has been
attempted on some ARM embedded platforms. Such drivers really must have
a hardware-specific ioctl to allocate suitable buffer objects.

## Plane Abstraction

A plane represents an image source that can be blended with or overlaid on
top of a CRTC during the scanout process. Planes take their input data from a
[`drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer") object. The plane itself specifies the cropping and scaling
of that image, and where it is placed on the visible area of a display
pipeline, represented by [`drm_crtc`](#c.drm_crtc "drm_crtc"). A plane can also have additional
properties that specify how the pixels are positioned and blended, like
rotation or Z-position. All these properties are stored in [`drm_plane_state`](#c.drm_plane_state "drm_plane_state").

Unless explicitly specified (via CRTC property or otherwise), the active area
of a CRTC will be black by default. This means portions of the active area
which are not covered by a plane will be black, and alpha blending of any
planes with the CRTC background will blend with black at the lowest zpos.

To create a plane, a KMS drivers allocates and zeroes an instances of
[`struct drm_plane`](#c.drm_plane "drm_plane") (possibly as part of a larger structure) and registers it
with a call to [`drm_universal_plane_init()`](#c.drm_universal_plane_init "drm_universal_plane_init").

Each plane has a type, see [`enum drm_plane_type`](#c.drm_plane_type "drm_plane_type"). A plane can be compatible
with multiple CRTCs, see [`drm_plane.possible_crtcs`](#c.drm_plane "drm_plane").

Each CRTC must have a unique primary plane userspace can attach to enable
the CRTC. In other words, userspace must be able to attach a different
primary plane to each CRTC at the same time. Primary planes can still be
compatible with multiple CRTCs. There must be exactly as many primary planes
as there are CRTCs.

Legacy uAPI doesn’t expose the primary and cursor planes directly. DRM core
relies on the driver to set the primary and optionally the cursor plane used
for legacy IOCTLs. This is done by calling [`drm_crtc_init_with_planes()`](#c.drm_crtc_init_with_planes "drm_crtc_init_with_planes"). All
drivers must provide one primary plane per CRTC to avoid surprising legacy
userspace too much.

### Plane Functions Reference

struct drm\_plane\_state
:   mutable plane state

**Definition**:

```
struct drm_plane_state {
    struct drm_plane *plane;
    struct drm_crtc *crtc;
    struct drm_framebuffer *fb;
    struct dma_fence *fence;
    int32_t crtc_x;
    int32_t crtc_y;
    uint32_t crtc_w, crtc_h;
    uint32_t src_x;
    uint32_t src_y;
    uint32_t src_h, src_w;
    int32_t hotspot_x, hotspot_y;
    u16 alpha;
    uint16_t pixel_blend_mode;
    unsigned int rotation;
    unsigned int zpos;
    unsigned int normalized_zpos;
    enum drm_color_encoding color_encoding;
    enum drm_color_range color_range;
    struct drm_property_blob *fb_damage_clips;
    bool ignore_damage_clips;
    struct drm_rect src, dst;
    bool visible;
    enum drm_scaling_filter scaling_filter;
    struct drm_colorop *color_pipeline;
    struct drm_crtc_commit *commit;
    struct drm_atomic_state *state;
    bool color_mgmt_changed : 1;
};
```

**Members**

`plane`
:   backpointer to the plane

`crtc`
:   Currently bound CRTC, NULL if disabled. Do not write this directly,
    use [`drm_atomic_set_crtc_for_plane()`](#c.drm_atomic_set_crtc_for_plane "drm_atomic_set_crtc_for_plane")

`fb`
:   Currently bound framebuffer. Do not write this directly, use
    [`drm_atomic_set_fb_for_plane()`](#c.drm_atomic_set_fb_for_plane "drm_atomic_set_fb_for_plane")

`fence`
:   Optional fence to wait for before scanning out **fb**. The core atomic
    code will set this when userspace is using explicit fencing. Do not
    write this field directly for a driver’s implicit fence.

    Drivers should store any implicit fence in this from their
    [`drm_plane_helper_funcs.prepare_fb`](drm-kms-helpers.html#c.drm_plane_helper_funcs "drm_plane_helper_funcs") callback. See
    [`drm_gem_plane_helper_prepare_fb()`](drm-kms-helpers.html#c.drm_gem_plane_helper_prepare_fb "drm_gem_plane_helper_prepare_fb") for a suitable helper.

`crtc_x`
:   Left position of visible portion of plane on crtc, signed dest
    location allows it to be partially off screen.

`crtc_y`
:   Upper position of visible portion of plane on crtc, signed dest
    location allows it to be partially off screen.

`crtc_w`
:   width of visible portion of plane on crtc

`crtc_h`
:   height of visible portion of plane on crtc

`src_x`
:   left position of visible portion of plane within plane (in
    16.16 fixed point).

`src_y`
:   upper position of visible portion of plane within plane (in
    16.16 fixed point).

`src_h`
:   height of visible portion of plane (in 16.16)

`src_w`
:   width of visible portion of plane (in 16.16)

`hotspot_x`
:   x offset to mouse cursor hotspot

`hotspot_y`
:   y offset to mouse cursor hotspot

`alpha`
:   Opacity of the plane with 0 as completely transparent and 0xffff as
    completely opaque. See [`drm_plane_create_alpha_property()`](#c.drm_plane_create_alpha_property "drm_plane_create_alpha_property") for more
    details.

`pixel_blend_mode`
:   The alpha blending equation selection, describing how the pixels from
    the current plane are composited with the background. Value can be
    one of DRM\_MODE\_BLEND\_\*

`rotation`
:   Rotation of the plane. See [`drm_plane_create_rotation_property()`](#c.drm_plane_create_rotation_property "drm_plane_create_rotation_property") for
    more details.

`zpos`
:   Priority of the given plane on crtc (optional).

    User-space may set mutable zpos properties so that multiple active
    planes on the same CRTC have identical zpos values. This is a
    user-space bug, but drivers can solve the conflict by comparing the
    plane object IDs; the plane with a higher ID is stacked on top of a
    plane with a lower ID.

    See [`drm_plane_create_zpos_property()`](#c.drm_plane_create_zpos_property "drm_plane_create_zpos_property") and
    [`drm_plane_create_zpos_immutable_property()`](#c.drm_plane_create_zpos_immutable_property "drm_plane_create_zpos_immutable_property") for more details.

`normalized_zpos`
:   Normalized value of zpos: unique, range from 0 to N-1 where N is the
    number of active planes for given crtc. Note that the driver must set
    [`drm_mode_config.normalize_zpos`](#c.drm_mode_config "drm_mode_config") or call [`drm_atomic_normalize_zpos()`](#c.drm_atomic_normalize_zpos "drm_atomic_normalize_zpos") to
    update this before it can be trusted.

`color_encoding`
:   Color encoding for non RGB formats

`color_range`
:   Color range for non RGB formats

`fb_damage_clips`
:   Blob representing damage (area in plane framebuffer that changed
    since last plane update) as an array of [`drm_mode_rect`](drm-uapi.html#c.drm_mode_rect "drm_mode_rect") in framebuffer
    coodinates of the attached framebuffer. Note that unlike plane src,
    damage clips are not in 16.16 fixed point.

    See [`drm_plane_get_damage_clips()`](#c.drm_plane_get_damage_clips "drm_plane_get_damage_clips") and
    [`drm_plane_get_damage_clips_count()`](#c.drm_plane_get_damage_clips_count "drm_plane_get_damage_clips_count") for accessing these.

`ignore_damage_clips`
:   Set by drivers to indicate the [`drm_atomic_helper_damage_iter_init()`](#c.drm_atomic_helper_damage_iter_init "drm_atomic_helper_damage_iter_init")
    helper that the **fb\_damage\_clips** blob property should be ignored.

    See [Damage Tracking Properties](#damage-tracking-properties) for more information.

`src`
:   source coordinates of the plane (in 16.16).

    When using [`drm_atomic_helper_check_plane_state()`](drm-kms-helpers.html#c.drm_atomic_helper_check_plane_state "drm_atomic_helper_check_plane_state"),
    the coordinates are clipped, but the driver may choose
    to use unclipped coordinates instead when the hardware
    performs the clipping automatically.

`dst`
:   clipped destination coordinates of the plane.

    When using [`drm_atomic_helper_check_plane_state()`](drm-kms-helpers.html#c.drm_atomic_helper_check_plane_state "drm_atomic_helper_check_plane_state"),
    the coordinates are clipped, but the driver may choose
    to use unclipped coordinates instead when the hardware
    performs the clipping automatically.

`visible`
:   Visibility of the plane. This can be false even if fb!=NULL and
    crtc!=NULL, due to clipping.

`scaling_filter`
:   Scaling filter to be applied

`color_pipeline`
:   The first colorop of the active color pipeline, or NULL, if no
    color pipeline is active.

`commit`
:   Tracks the pending commit to prevent use-after-free conditions,
    and for async plane updates.

    May be NULL.

`state`
:   backpointer to global drm\_atomic\_state

`color_mgmt_changed`
:   Color management properties have changed. Used
    by the atomic helpers and drivers to steer the atomic commit control
    flow.

**Description**

Please note that the destination coordinates **crtc\_x**, **crtc\_y**, **crtc\_h** and
**crtc\_w** and the source coordinates **src\_x**, **src\_y**, **src\_h** and **src\_w** are the
raw coordinates provided by userspace. Drivers should use
[`drm_atomic_helper_check_plane_state()`](drm-kms-helpers.html#c.drm_atomic_helper_check_plane_state "drm_atomic_helper_check_plane_state") and only use the derived rectangles in
**src** and **dst** to program the hardware.

struct drm\_plane\_funcs
:   driver plane control functions

**Definition**:

```
struct drm_plane_funcs {
    int (*update_plane)(struct drm_plane *plane, struct drm_crtc *crtc, struct drm_framebuffer *fb, int crtc_x, int crtc_y, unsigned int crtc_w, unsigned int crtc_h, uint32_t src_x, uint32_t src_y, uint32_t src_w, uint32_t src_h, struct drm_modeset_acquire_ctx *ctx);
    int (*disable_plane)(struct drm_plane *plane, struct drm_modeset_acquire_ctx *ctx);
    void (*destroy)(struct drm_plane *plane);
    void (*reset)(struct drm_plane *plane);
    int (*set_property)(struct drm_plane *plane, struct drm_property *property, uint64_t val);
    struct drm_plane_state *(*atomic_duplicate_state)(struct drm_plane *plane);
    void (*atomic_destroy_state)(struct drm_plane *plane, struct drm_plane_state *state);
    int (*atomic_set_property)(struct drm_plane *plane, struct drm_plane_state *state, struct drm_property *property, uint64_t val);
    int (*atomic_get_property)(struct drm_plane *plane, const struct drm_plane_state *state, struct drm_property *property, uint64_t *val);
    int (*late_register)(struct drm_plane *plane);
    void (*early_unregister)(struct drm_plane *plane);
    void (*atomic_print_state)(struct drm_printer *p, const struct drm_plane_state *state);
    bool (*format_mod_supported)(struct drm_plane *plane, uint32_t format, uint64_t modifier);
    bool (*format_mod_supported_async)(struct drm_plane *plane, u32 format, u64 modifier);
};
```

**Members**

`update_plane`
:   This is the legacy entry point to enable and configure the plane for
    the given CRTC and framebuffer. It is never called to disable the
    plane, i.e. the passed-in crtc and fb paramters are never NULL.

    The source rectangle in frame buffer memory coordinates is given by
    the src\_x, src\_y, src\_w and src\_h parameters (as 16.16 fixed point
    values). Devices that don’t support subpixel plane coordinates can
    ignore the fractional part.

    The destination rectangle in CRTC coordinates is given by the
    crtc\_x, crtc\_y, crtc\_w and crtc\_h parameters (as integer values).
    Devices scale the source rectangle to the destination rectangle. If
    scaling is not supported, and the source rectangle size doesn’t match
    the destination rectangle size, the driver must return a
    -<errorname>EINVAL</errorname> error.

    Drivers implementing atomic modeset should use
    [`drm_atomic_helper_update_plane()`](drm-kms-helpers.html#c.drm_atomic_helper_update_plane "drm_atomic_helper_update_plane") to implement this hook.

    RETURNS:

    0 on success or a negative error code on failure.

`disable_plane`
:   This is the legacy entry point to disable the plane. The DRM core
    calls this method in response to a DRM\_IOCTL\_MODE\_SETPLANE IOCTL call
    with the frame buffer ID set to 0. Disabled planes must not be
    processed by the CRTC.

    Drivers implementing atomic modeset should use
    [`drm_atomic_helper_disable_plane()`](drm-kms-helpers.html#c.drm_atomic_helper_disable_plane "drm_atomic_helper_disable_plane") to implement this hook.

    RETURNS:

    0 on success or a negative error code on failure.

`destroy`
:   Clean up plane resources. This is only called at driver unload time
    through [`drm_mode_config_cleanup()`](#c.drm_mode_config_cleanup "drm_mode_config_cleanup") since a plane cannot be hotplugged
    in DRM.

`reset`
:   Reset plane hardware and software state to off. This function isn’t
    called by the core directly, only through [`drm_mode_config_reset()`](#c.drm_mode_config_reset "drm_mode_config_reset").
    It’s not a helper hook only for historical reasons.

    Atomic drivers can use [`drm_atomic_helper_plane_reset()`](drm-kms-helpers.html#c.drm_atomic_helper_plane_reset "drm_atomic_helper_plane_reset") to reset
    atomic state using this hook.

`set_property`
:   This is the legacy entry point to update a property attached to the
    plane.

    This callback is optional if the driver does not support any legacy
    driver-private properties. For atomic drivers it is not used because
    property handling is done entirely in the DRM core.

    RETURNS:

    0 on success or a negative error code on failure.

`atomic_duplicate_state`
:   Duplicate the current atomic state for this plane and return it.
    The core and helpers guarantee that any atomic state duplicated with
    this hook and still owned by the caller (i.e. not transferred to the
    driver by calling [`drm_mode_config_funcs.atomic_commit`](#c.drm_mode_config_funcs "drm_mode_config_funcs")) will be
    cleaned up by calling the **atomic\_destroy\_state** hook in this
    structure.

    This callback is mandatory for atomic drivers.

    Atomic drivers which don’t subclass [`struct drm_plane_state`](#c.drm_plane_state "drm_plane_state") should use
    [`drm_atomic_helper_plane_duplicate_state()`](drm-kms-helpers.html#c.drm_atomic_helper_plane_duplicate_state "drm_atomic_helper_plane_duplicate_state"). Drivers that subclass the
    state structure to extend it with driver-private state should use
    [`__drm_atomic_helper_plane_duplicate_state()`](drm-kms-helpers.html#c.__drm_atomic_helper_plane_duplicate_state "__drm_atomic_helper_plane_duplicate_state") to make sure shared state is
    duplicated in a consistent fashion across drivers.

    It is an error to call this hook before [`drm_plane.state`](#c.drm_plane "drm_plane") has been
    initialized correctly.

    NOTE:

    If the duplicate state references refcounted resources this hook must
    acquire a reference for each of them. The driver must release these
    references again in **atomic\_destroy\_state**.

    RETURNS:

    Duplicated atomic state or NULL when the allocation failed.

`atomic_destroy_state`
:   Destroy a state duplicated with **atomic\_duplicate\_state** and release
    or unreference all resources it references

    This callback is mandatory for atomic drivers.

`atomic_set_property`
:   Decode a driver-private property value and store the decoded value
    into the passed-in state structure. Since the atomic core decodes all
    standardized properties (even for extensions beyond the core set of
    properties which might not be implemented by all drivers) this
    requires drivers to subclass the state structure.

    Such driver-private properties should really only be implemented for
    truly hardware/vendor specific state. Instead it is preferred to
    standardize atomic extension and decode the properties used to expose
    such an extension in the core.

    Do not call this function directly, use
    `drm_atomic_plane_set_property()` instead.

    This callback is optional if the driver does not support any
    driver-private atomic properties.

    NOTE:

    This function is called in the state assembly phase of atomic
    modesets, which can be aborted for any reason (including on
    userspace’s request to just check whether a configuration would be
    possible). Drivers MUST NOT touch any persistent state (hardware or
    software) or data structures except the passed in **state** parameter.

    Also since userspace controls in which order properties are set this
    function must not do any input validation (since the state update is
    incomplete and hence likely inconsistent). Instead any such input
    validation must be done in the various atomic\_check callbacks.

    RETURNS:

    0 if the property has been found, -EINVAL if the property isn’t
    implemented by the driver (which shouldn’t ever happen, the core only
    asks for properties attached to this plane). No other validation is
    allowed by the driver. The core already checks that the property
    value is within the range (integer, valid `enum value`, ...) the driver
    set when registering the property.

`atomic_get_property`
:   Reads out the decoded driver-private property. This is used to
    implement the GETPLANE IOCTL.

    Do not call this function directly, use
    `drm_atomic_plane_get_property()` instead.

    This callback is optional if the driver does not support any
    driver-private atomic properties.

    RETURNS:

    0 on success, -EINVAL if the property isn’t implemented by the
    driver (which should never happen, the core only asks for
    properties attached to this plane).

`late_register`
:   This optional hook can be used to register additional userspace
    interfaces attached to the plane like debugfs interfaces.
    It is called late in the driver load sequence from [`drm_dev_register()`](drm-internals.html#c.drm_dev_register "drm_dev_register").
    Everything added from this callback should be unregistered in
    the early\_unregister callback.

    Returns:

    0 on success, or a negative error code on failure.

`early_unregister`
:   This optional hook should be used to unregister the additional
    userspace interfaces attached to the plane from
    **late\_register**. It is called from [`drm_dev_unregister()`](drm-internals.html#c.drm_dev_unregister "drm_dev_unregister"),
    early in the driver unload sequence to disable userspace access
    before data structures are torndown.

`atomic_print_state`
:   If driver subclasses [`struct drm_plane_state`](#c.drm_plane_state "drm_plane_state"), it should implement
    this optional hook for printing additional driver specific state.

    Do not call this directly, use `drm_atomic_plane_print_state()`
    instead.

`format_mod_supported`
:   This optional hook is used for the DRM to determine if the given
    format/modifier combination is valid for the plane. This allows the
    DRM to generate the correct format bitmask (which formats apply to
    which modifier), and to validate modifiers at atomic\_check time.

    If not present, then any modifier in the plane’s modifier
    list is allowed with any of the plane’s formats.

    Returns:

    True if the given modifier is valid for that format on the plane.
    False otherwise.

`format_mod_supported_async`
:   This optional hook is used for the DRM to determine if for
    asynchronous flip the given format/modifier combination is valid for
    the plane. This allows the DRM to generate the correct format
    bitmask (which formats apply to which modifier), and to validate
    modifiers at atomic\_check time.

    Returns:

    True if the given modifier is valid for that format on the plane.
    False otherwise.

enum drm\_plane\_type
:   uapi plane type enumeration

**Constants**

`DRM_PLANE_TYPE_OVERLAY`
:   Overlay planes represent all non-primary, non-cursor planes. Some
    drivers refer to these types of planes as “sprites” internally.

`DRM_PLANE_TYPE_PRIMARY`
:   A primary plane attached to a CRTC is the most likely to be able to
    light up the CRTC when no scaling/cropping is used and the plane
    covers the whole CRTC.

`DRM_PLANE_TYPE_CURSOR`
:   A cursor plane attached to a CRTC is more likely to be able to be
    enabled when no scaling/cropping is used and the framebuffer has the
    size indicated by [`drm_mode_config.cursor_width`](#c.drm_mode_config "drm_mode_config") and
    [`drm_mode_config.cursor_height`](#c.drm_mode_config "drm_mode_config"). Additionally, if the driver doesn’t
    support modifiers, the framebuffer should have a linear layout.

**Description**

For historical reasons not all planes are made the same. This enumeration is
used to tell the different types of planes apart to implement the different
uapi semantics for them. For userspace which is universal plane aware and
which is using that atomic IOCTL there’s no difference between these planes
(beyong what the driver and hardware can support of course).

For compatibility with legacy userspace, only overlay planes are made
available to userspace by default. Userspace clients may set the
[`DRM_CLIENT_CAP_UNIVERSAL_PLANES`](drm-uapi.html#c.DRM_CLIENT_CAP_UNIVERSAL_PLANES "DRM_CLIENT_CAP_UNIVERSAL_PLANES") client capability bit to indicate that they
wish to receive a universal plane list containing all plane types. See also
[`drm_for_each_legacy_plane()`](#c.drm_for_each_legacy_plane "drm_for_each_legacy_plane").

In addition to setting each plane’s type, drivers need to setup the
[`drm_crtc.primary`](#c.drm_crtc "drm_crtc") and optionally [`drm_crtc.cursor`](#c.drm_crtc "drm_crtc") pointers for legacy
IOCTLs. See [`drm_crtc_init_with_planes()`](#c.drm_crtc_init_with_planes "drm_crtc_init_with_planes").

WARNING: The values of this `enum is` UABI since they’re exposed in the “type”
property.

struct drm\_plane
:   central DRM plane control structure

**Definition**:

```
struct drm_plane {
    struct drm_device *dev;
    struct list_head head;
    char *name;
    struct drm_modeset_lock mutex;
    struct drm_mode_object base;
    uint32_t possible_crtcs;
    uint32_t *format_types;
    unsigned int format_count;
    bool format_default;
    uint64_t *modifiers;
    unsigned int modifier_count;
    struct drm_crtc *crtc;
    struct drm_framebuffer *fb;
    struct drm_framebuffer *old_fb;
    const struct drm_plane_funcs *funcs;
    struct drm_object_properties properties;
    enum drm_plane_type type;
    unsigned index;
    const struct drm_plane_helper_funcs *helper_private;
    struct drm_plane_state *state;
    struct drm_property *alpha_property;
    struct drm_property *zpos_property;
    struct drm_property *rotation_property;
    struct drm_property *blend_mode_property;
    struct drm_property *color_encoding_property;
    struct drm_property *color_range_property;
    struct drm_property *color_pipeline_property;
    struct drm_property *scaling_filter_property;
    struct drm_property *hotspot_x_property;
    struct drm_property *hotspot_y_property;
    struct kmsg_dumper kmsg_panic;
};
```

**Members**

`dev`
:   DRM device this plane belongs to

`head`
:   List of all planes on **dev**, linked from [`drm_mode_config.plane_list`](#c.drm_mode_config "drm_mode_config").
    Invariant over the lifetime of **dev** and therefore does not need
    locking.

`name`
:   human readable name, can be overwritten by the driver

`mutex`
:   Protects modeset plane state, together with the [`drm_crtc.mutex`](#c.drm_crtc "drm_crtc") of
    CRTC this plane is linked to (when active, getting activated or
    getting disabled).

    For atomic drivers specifically this protects **state**.

`base`
:   base mode object

`possible_crtcs`
:   pipes this plane can be bound to constructed from
    [`drm_crtc_mask()`](#c.drm_crtc_mask "drm_crtc_mask")

`format_types`
:   array of formats supported by this plane

`format_count`
:   Size of the array pointed at by **format\_types**.

`format_default`
:   driver hasn’t supplied supported formats for the
    plane. Used by the non-atomic driver compatibility wrapper only.

`modifiers`
:   array of modifiers supported by this plane

`modifier_count`
:   Size of the array pointed at by **modifier\_count**.

`crtc`
:   Currently bound CRTC, only meaningful for non-atomic drivers. For
    atomic drivers this is forced to be NULL, atomic drivers should
    instead check [`drm_plane_state.crtc`](#c.drm_plane_state "drm_plane_state").

`fb`
:   Currently bound framebuffer, only meaningful for non-atomic drivers.
    For atomic drivers this is forced to be NULL, atomic drivers should
    instead check [`drm_plane_state.fb`](#c.drm_plane_state "drm_plane_state").

`old_fb`
:   Temporary tracking of the old fb while a modeset is ongoing. Only
    used by non-atomic drivers, forced to be NULL for atomic drivers.

`funcs`
:   plane control functions

`properties`
:   property tracking for this plane

`type`
:   Type of plane, see [`enum drm_plane_type`](#c.drm_plane_type "drm_plane_type") for details.

`index`
:   Position inside the mode\_config.list, can be used as an array
    index. It is invariant over the lifetime of the plane.

`helper_private`
:   mid-layer private data

`state`
:   Current atomic state for this plane.

    This is protected by **mutex**. Note that nonblocking atomic commits
    access the current plane state without taking locks. Either by going
    through the [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state") pointers, see
    [`for_each_oldnew_plane_in_state()`](#c.for_each_oldnew_plane_in_state "for_each_oldnew_plane_in_state"), [`for_each_old_plane_in_state()`](#c.for_each_old_plane_in_state "for_each_old_plane_in_state") and
    [`for_each_new_plane_in_state()`](#c.for_each_new_plane_in_state "for_each_new_plane_in_state"). Or through careful ordering of atomic
    commit operations as implemented in the atomic helpers, see
    [`struct drm_crtc_commit`](#c.drm_crtc_commit "drm_crtc_commit").

`alpha_property`
:   Optional alpha property for this plane. See
    [`drm_plane_create_alpha_property()`](#c.drm_plane_create_alpha_property "drm_plane_create_alpha_property").

`zpos_property`
:   Optional zpos property for this plane. See
    [`drm_plane_create_zpos_property()`](#c.drm_plane_create_zpos_property "drm_plane_create_zpos_property").

`rotation_property`
:   Optional rotation property for this plane. See
    [`drm_plane_create_rotation_property()`](#c.drm_plane_create_rotation_property "drm_plane_create_rotation_property").

`blend_mode_property`
:   Optional “pixel blend mode” `enum property` for this plane.
    Blend mode property represents the alpha blending equation selection,
    describing how the pixels from the current plane are composited with
    the background.

`color_encoding_property`
:   Optional “COLOR\_ENCODING” `enum property` for specifying
    color encoding for non RGB formats.
    See [`drm_plane_create_color_properties()`](#c.drm_plane_create_color_properties "drm_plane_create_color_properties").

`color_range_property`
:   Optional “COLOR\_RANGE” `enum property` for specifying
    color range for non RGB formats.
    See [`drm_plane_create_color_properties()`](#c.drm_plane_create_color_properties "drm_plane_create_color_properties").

`color_pipeline_property`
:   Optional “COLOR\_PIPELINE” `enum property` for specifying
    a color pipeline to use on the plane.

`scaling_filter_property`
:   property to apply a particular filter while
    scaling.

`hotspot_x_property`
:   property to set mouse hotspot x offset.

`hotspot_y_property`
:   property to set mouse hotspot y offset.

`kmsg_panic`
:   Used to register a panic notifier for this plane

**Description**

Planes represent the scanout hardware of a display block. They receive their
input data from a [`drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer") and feed it to a [`drm_crtc`](#c.drm_crtc "drm_crtc"). Planes control
the color conversion, see [Plane Composition Properties](#id2) for more details,
and are also involved in the color conversion of input pixels, see [Color
Management Properties](#color-management-properties) for details on that.

drmm\_universal\_plane\_alloc

`drmm_universal_plane_alloc (dev, type, member, possible_crtcs, funcs, formats, format_count, format_modifiers, plane_type, name, ...)`

> Allocate and initialize an universal plane object

**Parameters**

`dev`
:   DRM device

`type`
:   the type of the struct which contains struct [`drm_plane`](#c.drm_plane "drm_plane")

`member`
:   the name of the [`drm_plane`](#c.drm_plane "drm_plane") within **type**

`possible_crtcs`
:   bitmask of possible CRTCs

`funcs`
:   callbacks for the new plane

`formats`
:   array of supported formats (DRM\_FORMAT\_\*)

`format_count`
:   number of elements in **formats**

`format_modifiers`
:   array of `struct drm_format` modifiers terminated by
    DRM\_FORMAT\_MOD\_INVALID

`plane_type`
:   type of plane (overlay, primary, cursor)

`name`
:   printf style format string for the plane name, or NULL for default name

`...`
:   variable arguments

**Description**

Allocates and initializes a plane object of type **type**. Cleanup is
automatically handled through registering [`drm_plane_cleanup()`](#c.drm_plane_cleanup "drm_plane_cleanup") with
[`drmm_add_action()`](drm-internals.html#c.drmm_add_action "drmm_add_action").

The **drm\_plane\_funcs.destroy** hook must be NULL.

Drivers that only support the DRM\_FORMAT\_MOD\_LINEAR modifier support may set
**format\_modifiers** to NULL. The plane will advertise the linear modifier.

**Return**

Pointer to new plane, or ERR\_PTR on failure.

drm\_universal\_plane\_alloc

`drm_universal_plane_alloc (dev, type, member, possible_crtcs, funcs, formats, format_count, format_modifiers, plane_type, name, ...)`

> Allocate and initialize an universal plane object

**Parameters**

`dev`
:   DRM device

`type`
:   the type of the struct which contains struct [`drm_plane`](#c.drm_plane "drm_plane")

`member`
:   the name of the [`drm_plane`](#c.drm_plane "drm_plane") within **type**

`possible_crtcs`
:   bitmask of possible CRTCs

`funcs`
:   callbacks for the new plane

`formats`
:   array of supported formats (DRM\_FORMAT\_\*)

`format_count`
:   number of elements in **formats**

`format_modifiers`
:   array of `struct drm_format` modifiers terminated by
    DRM\_FORMAT\_MOD\_INVALID

`plane_type`
:   type of plane (overlay, primary, cursor)

`name`
:   printf style format string for the plane name, or NULL for default name

`...`
:   variable arguments

**Description**

Allocates and initializes a plane object of type **type**. The caller
is responsible for releasing the allocated memory with [`kfree()`](../core-api/mm-api.html#c.kfree "kfree").

Drivers are encouraged to use [`drmm_universal_plane_alloc()`](#c.drmm_universal_plane_alloc "drmm_universal_plane_alloc") instead.

Drivers that only support the DRM\_FORMAT\_MOD\_LINEAR modifier support may set
**format\_modifiers** to NULL. The plane will advertise the linear modifier.

**Return**

Pointer to new plane, or ERR\_PTR on failure.

unsigned int drm\_plane\_index(const struct [drm\_plane](#c.drm_plane "drm_plane") \*plane)
:   find the index of a registered plane

**Parameters**

`const struct drm_plane *plane`
:   plane to find index for

**Description**

Given a registered plane, return the index of that plane within a DRM
device’s list of planes.

u32 drm\_plane\_mask(const struct [drm\_plane](#c.drm_plane "drm_plane") \*plane)
:   find the mask of a registered plane

**Parameters**

`const struct drm_plane *plane`
:   plane to find mask for

struct [drm\_plane](#c.drm_plane "drm_plane") \*drm\_plane\_find(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_priv, uint32\_t id)
:   find a [`drm_plane`](#c.drm_plane "drm_plane")

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_file *file_priv`
:   drm file to check for lease against.

`uint32_t id`
:   plane id

**Description**

Returns the plane with **id**, NULL if it doesn’t exist. Simple wrapper around
[`drm_mode_object_find()`](#c.drm_mode_object_find "drm_mode_object_find").

drm\_for\_each\_plane\_mask

`drm_for_each_plane_mask (plane, dev, plane_mask)`

> iterate over planes specified by bitmask

**Parameters**

`plane`
:   the loop cursor

`dev`
:   the DRM device

`plane_mask`
:   bitmask of plane indices

**Description**

Iterate over all planes specified by bitmask.

drm\_for\_each\_legacy\_plane

`drm_for_each_legacy_plane (plane, dev)`

> iterate over all planes for legacy userspace

**Parameters**

`plane`
:   the loop cursor

`dev`
:   the DRM device

**Description**

Iterate over all legacy planes of **dev**, excluding primary and cursor planes.
This is useful for implementing userspace apis when userspace is not
universal plane aware. See also [`enum drm_plane_type`](#c.drm_plane_type "drm_plane_type").

drm\_for\_each\_plane

`drm_for_each_plane (plane, dev)`

> iterate over all planes

**Parameters**

`plane`
:   the loop cursor

`dev`
:   the DRM device

**Description**

Iterate over all planes of **dev**, include primary and cursor planes.

int drm\_universal\_plane\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_plane](#c.drm_plane "drm_plane") \*plane, uint32\_t possible\_crtcs, const struct [drm\_plane\_funcs](#c.drm_plane_funcs "drm_plane_funcs") \*funcs, const uint32\_t \*formats, unsigned int format\_count, const uint64\_t \*format\_modifiers, enum [drm\_plane\_type](#c.drm_plane_type "drm_plane_type") type, const char \*name, ...)
:   Initialize a new universal plane object

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_plane *plane`
:   plane object to init

`uint32_t possible_crtcs`
:   bitmask of possible CRTCs

`const struct drm_plane_funcs *funcs`
:   callbacks for the new plane

`const uint32_t *formats`
:   array of supported formats (DRM\_FORMAT\_\*)

`unsigned int format_count`
:   number of elements in **formats**

`const uint64_t *format_modifiers`
:   array of `struct drm_format` modifiers terminated by
    DRM\_FORMAT\_MOD\_INVALID

`enum drm_plane_type type`
:   type of plane (overlay, primary, cursor)

`const char *name`
:   printf style format string for the plane name, or NULL for default name

`...`
:   variable arguments

**Description**

Initializes a plane object of type **type**. The [`drm_plane_funcs.destroy`](#c.drm_plane_funcs "drm_plane_funcs") hook
should call [`drm_plane_cleanup()`](#c.drm_plane_cleanup "drm_plane_cleanup") and [`kfree()`](../core-api/mm-api.html#c.kfree "kfree") the plane structure. The plane
structure should not be allocated with `devm_kzalloc()`.

**Note**

consider using [`drmm_universal_plane_alloc()`](#c.drmm_universal_plane_alloc "drmm_universal_plane_alloc") instead of
[`drm_universal_plane_init()`](#c.drm_universal_plane_init "drm_universal_plane_init") to let the DRM managed resource infrastructure
take care of cleanup and deallocation.

Drivers that only support the DRM\_FORMAT\_MOD\_LINEAR modifier support may set
**format\_modifiers** to NULL. The plane will advertise the linear modifier.

**Return**

Zero on success, error code on failure.

void drm\_plane\_cleanup(struct [drm\_plane](#c.drm_plane "drm_plane") \*plane)
:   Clean up the core plane usage

**Parameters**

`struct drm_plane *plane`
:   plane to cleanup

**Description**

This function cleans up **plane** and removes it from the DRM mode setting
core. Note that the function does *not* free the plane structure itself,
this is the responsibility of the caller.

struct [drm\_plane](#c.drm_plane "drm_plane") \*drm\_plane\_from\_index(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, int idx)
:   find the registered plane at an index

**Parameters**

`struct drm_device *dev`
:   DRM device

`int idx`
:   index of registered plane to find for

**Description**

Given a plane index, return the registered plane from DRM device’s
list of planes with matching index. This is the inverse of [`drm_plane_index()`](#c.drm_plane_index "drm_plane_index").

void drm\_plane\_force\_disable(struct [drm\_plane](#c.drm_plane "drm_plane") \*plane)
:   Forcibly disable a plane

**Parameters**

`struct drm_plane *plane`
:   plane to disable

**Description**

Forces the plane to be disabled.

Used when the plane’s current framebuffer is destroyed,
and when restoring fbdev mode.

Note that this function is not suitable for atomic drivers, since it doesn’t
wire through the lock acquisition context properly and hence can’t handle
retries or driver private locks. You probably want to use
[`drm_atomic_helper_disable_plane()`](drm-kms-helpers.html#c.drm_atomic_helper_disable_plane "drm_atomic_helper_disable_plane") or
[`drm_atomic_helper_disable_planes_on_crtc()`](drm-kms-helpers.html#c.drm_atomic_helper_disable_planes_on_crtc "drm_atomic_helper_disable_planes_on_crtc") instead.

int drm\_mode\_plane\_set\_obj\_prop(struct [drm\_plane](#c.drm_plane "drm_plane") \*plane, struct [drm\_property](#c.drm_property "drm_property") \*property, uint64\_t value)
:   set the value of a property

**Parameters**

`struct drm_plane *plane`
:   drm plane object to set property value for

`struct drm_property *property`
:   property to set

`uint64_t value`
:   value the property should be set to

**Description**

This functions sets a given property on a given plane object. This function
calls the driver’s ->set\_property callback and changes the software state of
the property if the callback succeeds.

**Return**

Zero on success, error code on failure.

bool drm\_plane\_has\_format(struct [drm\_plane](#c.drm_plane "drm_plane") \*plane, u32 format, u64 modifier)
:   Check whether the plane supports this format and modifier combination

**Parameters**

`struct drm_plane *plane`
:   drm plane

`u32 format`
:   pixel format (DRM\_FORMAT\_\*)

`u64 modifier`
:   data layout modifier

**Return**

Whether the plane supports the specified format and modifier combination.

bool drm\_any\_plane\_has\_format(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, u32 format, u64 modifier)
:   Check whether any plane supports this format and modifier combination

**Parameters**

`struct drm_device *dev`
:   DRM device

`u32 format`
:   pixel format (DRM\_FORMAT\_\*)

`u64 modifier`
:   data layout modifier

**Return**

Whether at least one plane supports the specified format and modifier combination.

void drm\_plane\_enable\_fb\_damage\_clips(struct [drm\_plane](#c.drm_plane "drm_plane") \*plane)
:   Enables plane fb damage clips property.

**Parameters**

`struct drm_plane *plane`
:   Plane on which to enable damage clips property.

**Description**

This function lets driver to enable the damage clips property on a plane.

unsigned int drm\_plane\_get\_damage\_clips\_count(const struct [drm\_plane\_state](#c.drm_plane_state "drm_plane_state") \*state)
:   Returns damage clips count.

**Parameters**

`const struct drm_plane_state *state`
:   Plane state.

**Description**

Simple helper to get the number of [`drm_mode_rect`](drm-uapi.html#c.drm_mode_rect "drm_mode_rect") clips set by user-space
during plane update.

**Return**

Number of clips in plane fb\_damage\_clips blob property.

struct [drm\_mode\_rect](drm-uapi.html#c.drm_mode_rect "drm_mode_rect") \*drm\_plane\_get\_damage\_clips(const struct [drm\_plane\_state](#c.drm_plane_state "drm_plane_state") \*state)
:   Returns damage clips.

**Parameters**

`const struct drm_plane_state *state`
:   Plane state.

**Description**

Note that this function returns uapi type [`drm_mode_rect`](drm-uapi.html#c.drm_mode_rect "drm_mode_rect"). Drivers might want
to use the helper functions [`drm_atomic_helper_damage_iter_init()`](#c.drm_atomic_helper_damage_iter_init "drm_atomic_helper_damage_iter_init") and
[`drm_atomic_helper_damage_iter_next()`](#c.drm_atomic_helper_damage_iter_next "drm_atomic_helper_damage_iter_next") or [`drm_atomic_helper_damage_merged()`](#c.drm_atomic_helper_damage_merged "drm_atomic_helper_damage_merged") if
the driver can only handle a single damage region at most.

**Return**

Damage clips in plane fb\_damage\_clips blob property.

int drm\_plane\_create\_scaling\_filter\_property(struct [drm\_plane](#c.drm_plane "drm_plane") \*plane, unsigned int supported\_filters)
:   create a new scaling filter property

**Parameters**

`struct drm_plane *plane`
:   drm plane

`unsigned int supported_filters`
:   bitmask of supported scaling filters, must include
    BIT(DRM\_SCALING\_FILTER\_DEFAULT).

**Description**

This function lets driver to enable the scaling filter property on a given
plane.

**Return**

Zero for success or -errno

int drm\_plane\_add\_size\_hints\_property(struct [drm\_plane](#c.drm_plane "drm_plane") \*plane, const struct [drm\_plane\_size\_hint](drm-uapi.html#c.drm_plane_size_hint "drm_plane_size_hint") \*hints, int num\_hints)
:   create a size hints property

**Parameters**

`struct drm_plane *plane`
:   drm plane

`const struct drm_plane_size_hint *hints`
:   size hints

`int num_hints`
:   number of size hints

**Description**

Create a size hints property for the plane.

**Return**

Zero for success or -errno

int drm\_plane\_create\_color\_pipeline\_property(struct [drm\_plane](#c.drm_plane "drm_plane") \*plane, const struct drm\_prop\_enum\_list \*pipelines, int num\_pipelines)
:   create a new color pipeline property

**Parameters**

`struct drm_plane *plane`
:   drm plane

`const struct drm_prop_enum_list *pipelines`
:   list of pipelines

`int num_pipelines`
:   number of pipelines

**Description**

Create the COLOR\_PIPELINE plane property to specific color pipelines on
the plane.

**Return**

Zero for success or -errno

### Plane Composition Functions Reference

int drm\_plane\_create\_alpha\_property(struct [drm\_plane](#c.drm_plane "drm_plane") \*plane)
:   create a new alpha property

**Parameters**

`struct drm_plane *plane`
:   drm plane

**Description**

This function creates a generic, mutable, alpha property and enables support
for it in the DRM core. It is attached to **plane**.

The alpha property will be allowed to be within the bounds of 0
(transparent) to 0xffff (opaque).

**Return**

0 on success, negative error code on failure.

int drm\_plane\_create\_rotation\_property(struct [drm\_plane](#c.drm_plane "drm_plane") \*plane, unsigned int rotation, unsigned int supported\_rotations)
:   create a new rotation property

**Parameters**

`struct drm_plane *plane`
:   drm plane

`unsigned int rotation`
:   initial value of the rotation property

`unsigned int supported_rotations`
:   bitmask of supported rotations and reflections

**Description**

This creates a new property with the selected support for transformations.

Since a rotation by 180° degress is the same as reflecting both along the x
and the y axis the rotation property is somewhat redundant. Drivers can use
[`drm_rotation_simplify()`](#c.drm_rotation_simplify "drm_rotation_simplify") to normalize values of this property.

The property exposed to userspace is a bitmask property (see
[`drm_property_create_bitmask()`](#c.drm_property_create_bitmask "drm_property_create_bitmask")) called “rotation” and has the following
bitmask enumaration values:

DRM\_MODE\_ROTATE\_0:
:   “rotate-0”

DRM\_MODE\_ROTATE\_90:
:   “rotate-90”

DRM\_MODE\_ROTATE\_180:
:   “rotate-180”

DRM\_MODE\_ROTATE\_270:
:   “rotate-270”

DRM\_MODE\_REFLECT\_X:
:   “reflect-x”

DRM\_MODE\_REFLECT\_Y:
:   “reflect-y”

Rotation is the specified amount in degrees in counter clockwise direction,
the X and Y axis are within the source rectangle, i.e. the X/Y axis before
rotation. After reflection, the rotation is applied to the image sampled from
the source rectangle, before scaling it to fit the destination rectangle.

unsigned int drm\_rotation\_simplify(unsigned int rotation, unsigned int supported\_rotations)
:   Try to simplify the rotation

**Parameters**

`unsigned int rotation`
:   Rotation to be simplified

`unsigned int supported_rotations`
:   Supported rotations

**Description**

Attempt to simplify the rotation to a form that is supported.
Eg. if the hardware supports everything except DRM\_MODE\_REFLECT\_X
one could call this function like this:

drm\_rotation\_simplify(rotation, DRM\_MODE\_ROTATE\_0 |
:   DRM\_MODE\_ROTATE\_90 | DRM\_MODE\_ROTATE\_180 |
    DRM\_MODE\_ROTATE\_270 | DRM\_MODE\_REFLECT\_Y);

to eliminate the DRM\_MODE\_REFLECT\_X flag. Depending on what kind of
transforms the hardware supports, this function may not
be able to produce a supported transform, so the caller should
check the result afterwards.

int drm\_plane\_create\_zpos\_property(struct [drm\_plane](#c.drm_plane "drm_plane") \*plane, unsigned int zpos, unsigned int min, unsigned int max)
:   create mutable zpos property

**Parameters**

`struct drm_plane *plane`
:   drm plane

`unsigned int zpos`
:   initial value of zpos property

`unsigned int min`
:   minimal possible value of zpos property

`unsigned int max`
:   maximal possible value of zpos property

**Description**

This function initializes generic mutable zpos property and enables support
for it in drm core. Drivers can then attach this property to planes to enable
support for configurable planes arrangement during blending operation.
Drivers that attach a mutable zpos property to any plane should call the
[`drm_atomic_normalize_zpos()`](#c.drm_atomic_normalize_zpos "drm_atomic_normalize_zpos") helper during their implementation of
[`drm_mode_config_funcs.atomic_check()`](#c.drm_mode_config_funcs "drm_mode_config_funcs"), which will update the normalized zpos
values and store them in [`drm_plane_state.normalized_zpos`](#c.drm_plane_state "drm_plane_state"). Usually min
should be set to 0 and max to maximal number of planes for given crtc - 1.

If zpos of some planes cannot be changed (like fixed background or
cursor/topmost planes), drivers shall adjust the min/max values and assign
those planes immutable zpos properties with lower or higher values (for more
information, see [`drm_plane_create_zpos_immutable_property()`](#c.drm_plane_create_zpos_immutable_property "drm_plane_create_zpos_immutable_property") function). In such
case drivers shall also assign proper initial zpos values for all planes in
its `plane_reset()` callback, so the planes will be always sorted properly.

See also [`drm_atomic_normalize_zpos()`](#c.drm_atomic_normalize_zpos "drm_atomic_normalize_zpos").

The property exposed to userspace is called “zpos”.

**Return**

Zero on success, negative errno on failure.

int drm\_plane\_create\_zpos\_immutable\_property(struct [drm\_plane](#c.drm_plane "drm_plane") \*plane, unsigned int zpos)
:   create immuttable zpos property

**Parameters**

`struct drm_plane *plane`
:   drm plane

`unsigned int zpos`
:   value of zpos property

**Description**

This function initializes generic immutable zpos property and enables
support for it in drm core. Using this property driver lets userspace
to get the arrangement of the planes for blending operation and notifies
it that the hardware (or driver) doesn’t support changing of the planes’
order. For mutable zpos see [`drm_plane_create_zpos_property()`](#c.drm_plane_create_zpos_property "drm_plane_create_zpos_property").

The property exposed to userspace is called “zpos”.

**Return**

Zero on success, negative errno on failure.

int drm\_atomic\_normalize\_zpos(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state)
:   calculate normalized zpos values for all crtcs

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   atomic state of DRM device

**Description**

This function calculates normalized zpos value for all modified planes in
the provided atomic state of DRM device.

For every CRTC this function checks new states of all planes assigned to
it and calculates normalized zpos value for these planes. Planes are compared
first by their zpos values, then by plane id (if zpos is equal). The plane
with lowest zpos value is at the bottom. The [`drm_plane_state.normalized_zpos`](#c.drm_plane_state "drm_plane_state")
is then filled with unique values from 0 to number of active planes in crtc
minus one.

RETURNS
Zero for success or -errno

int drm\_plane\_create\_blend\_mode\_property(struct [drm\_plane](#c.drm_plane "drm_plane") \*plane, unsigned int supported\_modes)
:   create a new blend mode property

**Parameters**

`struct drm_plane *plane`
:   drm plane

`unsigned int supported_modes`
:   bitmask of supported modes, must include
    BIT(DRM\_MODE\_BLEND\_PREMULTI). Current DRM assumption is
    that alpha is premultiplied, and old userspace can break if
    the property defaults to anything else.

**Description**

This creates a new property describing the blend mode.

The property exposed to userspace is an enumeration property (see
[`drm_property_create_enum()`](#c.drm_property_create_enum "drm_property_create_enum")) called “pixel blend mode” and has the
following enumeration values:

“None”:
:   Blend formula that ignores the pixel alpha.

“Pre-multiplied”:
:   Blend formula that assumes the pixel color values have been already
    pre-multiplied with the alpha channel values.

“Coverage”:
:   Blend formula that assumes the pixel color values have not been
    pre-multiplied and will do so when blending them to the background color
    values.

**Return**

Zero for success or -errno

void drm\_crtc\_attach\_background\_color\_property(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   attach background color property

**Parameters**

`struct drm_crtc *crtc`
:   drm crtc

**Description**

Attaches the background color property to **crtc**. The property defaults to
solid black and will accept 64-bit ARGB values in the format generated by
DRM\_ARGB64\_PREP\*() helpers.

### Plane Damage Tracking Functions Reference

void drm\_atomic\_helper\_check\_plane\_damage(struct [drm\_atomic\_state](#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_plane\_state](#c.drm_plane_state "drm_plane_state") \*plane\_state)
:   Verify plane damage on atomic\_check.

**Parameters**

`struct drm_atomic_state *state`
:   The driver state object.

`struct drm_plane_state *plane_state`
:   Plane state for which to verify damage.

**Description**

This helper function makes sure that damage from plane state is discarded
for full modeset. If there are more reasons a driver would want to do a full
plane update rather than processing individual damage regions, then those
cases should be taken care of here.

Note that [`drm_plane_state.fb_damage_clips`](#c.drm_plane_state "drm_plane_state") == NULL in plane state means that
full plane update should happen. It also ensure helper iterator will return
[`drm_plane_state.src`](#c.drm_plane_state "drm_plane_state") as damage.

int drm\_atomic\_helper\_dirtyfb(struct [drm\_framebuffer](#c.drm_framebuffer "drm_framebuffer") \*fb, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_priv, unsigned int flags, unsigned int color, struct drm\_clip\_rect \*clips, unsigned int num\_clips)
:   Helper for dirtyfb.

**Parameters**

`struct drm_framebuffer *fb`
:   DRM framebuffer.

`struct drm_file *file_priv`
:   Drm file for the ioctl call.

`unsigned int flags`
:   Dirty fb annotate flags.

`unsigned int color`
:   Color for annotate fill.

`struct drm_clip_rect *clips`
:   Dirty region.

`unsigned int num_clips`
:   Count of clip in clips.

**Description**

A helper to implement [`drm_framebuffer_funcs.dirty`](#c.drm_framebuffer_funcs "drm_framebuffer_funcs") using damage interface
during plane update. If num\_clips is 0 then this helper will do a full plane
update. This is the same behaviour expected by DIRTFB IOCTL.

Note that this helper is blocking implementation. This is what current
drivers and userspace expect in their DIRTYFB IOCTL implementation, as a way
to rate-limit userspace and make sure its rendering doesn’t get ahead of
uploading new data too much.

**Return**

Zero on success, negative errno on failure.

void drm\_atomic\_helper\_damage\_iter\_init(struct [drm\_atomic\_helper\_damage\_iter](#c.drm_atomic_helper_damage_iter "drm_atomic_helper_damage_iter") \*iter, const struct [drm\_plane\_state](#c.drm_plane_state "drm_plane_state") \*old\_state, const struct [drm\_plane\_state](#c.drm_plane_state "drm_plane_state") \*state)
:   Initialize the damage iterator.

**Parameters**

`struct drm_atomic_helper_damage_iter *iter`
:   The iterator to initialize.

`const struct drm_plane_state *old_state`
:   Old plane state for validation.

`const struct drm_plane_state *state`
:   Plane state from which to iterate the damage clips.

**Description**

Initialize an iterator, which clips plane damage
[`drm_plane_state.fb_damage_clips`](#c.drm_plane_state "drm_plane_state") to plane [`drm_plane_state.src`](#c.drm_plane_state "drm_plane_state"). This iterator
returns full plane src in case damage is not present because either
user-space didn’t sent or driver discarded it (it want to do full plane
update). Currently this iterator returns full plane src in case plane src
changed but that can be changed in future to return damage.

For the case when plane is not visible or plane update should not happen the
first call to iter\_next will return false. Note that this helper use clipped
[`drm_plane_state.src`](#c.drm_plane_state "drm_plane_state"), so driver calling this helper should have called
[`drm_atomic_helper_check_plane_state()`](drm-kms-helpers.html#c.drm_atomic_helper_check_plane_state "drm_atomic_helper_check_plane_state") earlier.

bool drm\_atomic\_helper\_damage\_iter\_next(struct [drm\_atomic\_helper\_damage\_iter](#c.drm_atomic_helper_damage_iter "drm_atomic_helper_damage_iter") \*iter, struct [drm\_rect](drm-kms-helpers.html#c.drm_rect "drm_rect") \*rect)
:   Advance the damage iterator.

**Parameters**

`struct drm_atomic_helper_damage_iter *iter`
:   The iterator to advance.

`struct drm_rect *rect`
:   Return a rectangle in fb coordinate clipped to plane src.

**Description**

Since plane src is in 16.16 fixed point and damage clips are whole number,
this iterator round off clips that intersect with plane src. Round down for
x1/y1 and round up for x2/y2 for the intersected coordinate. Similar rounding
off for full plane src, in case it’s returned as damage. This iterator will
skip damage clips outside of plane src.

If the first call to iterator next returns false then it means no need to
update the plane.

**Return**

True if the output is valid, false if reached the end.

bool drm\_atomic\_helper\_damage\_merged(const struct [drm\_plane\_state](#c.drm_plane_state "drm_plane_state") \*old\_state, const struct [drm\_plane\_state](#c.drm_plane_state "drm_plane_state") \*state, struct [drm\_rect](drm-kms-helpers.html#c.drm_rect "drm_rect") \*rect)
:   Merged plane damage

**Parameters**

`const struct drm_plane_state *old_state`
:   Old plane state for validation.

`const struct drm_plane_state *state`
:   Plane state from which to iterate the damage clips.

`struct drm_rect *rect`
:   Returns the merged damage rectangle

**Description**

This function merges any valid plane damage clips into one rectangle and
returns it in **rect**.

For details see: [`drm_atomic_helper_damage_iter_init()`](#c.drm_atomic_helper_damage_iter_init "drm_atomic_helper_damage_iter_init") and
[`drm_atomic_helper_damage_iter_next()`](#c.drm_atomic_helper_damage_iter_next "drm_atomic_helper_damage_iter_next").

**Return**

True if there is valid plane damage otherwise false.

drm\_atomic\_for\_each\_plane\_damage

`drm_atomic_for_each_plane_damage (iter, rect)`

> Iterator macro for plane damage.

**Parameters**

`iter`
:   The iterator to advance.

`rect`
:   Return a rectangle in fb coordinate clipped to plane src.

**Description**

Note that if the first call to iterator macro return false then no need to do
plane update. Iterator will return full plane src when damage is not passed
by user-space.

struct drm\_atomic\_helper\_damage\_iter
:   Closure structure for damage iterator.

**Definition**:

```
struct drm_atomic_helper_damage_iter {
};
```

**Members**

**Description**

This structure tracks state needed to walk the list of plane damage clips.

### Plane Panic Feature

To enable DRM panic for a driver, the primary plane must implement a
[`drm_plane_helper_funcs.get_scanout_buffer`](drm-kms-helpers.html#c.drm_plane_helper_funcs "drm_plane_helper_funcs") helper function. It is then
automatically registered to the drm panic handler.
When a panic occurs, the [`drm_plane_helper_funcs.get_scanout_buffer`](drm-kms-helpers.html#c.drm_plane_helper_funcs "drm_plane_helper_funcs") will be
called, and the driver can provide a framebuffer so the panic handler can
draw the panic screen on it. Currently only linear buffer and a few color
formats are supported.
Optionally the driver can also provide a [`drm_plane_helper_funcs.panic_flush`](drm-kms-helpers.html#c.drm_plane_helper_funcs "drm_plane_helper_funcs")
callback, that will be called after that, to send additional commands to the
hardware to make the scanout buffer visible.

### Plane Panic Functions Reference

struct drm\_scanout\_buffer
:   DRM scanout buffer

**Definition**:

```
struct drm_scanout_buffer {
    const struct drm_format_info *format;
    struct iosys_map map[DRM_FORMAT_MAX_PLANES];
    struct page **pages;
    unsigned int width;
    unsigned int height;
    unsigned int pitch[DRM_FORMAT_MAX_PLANES];
    void (*set_pixel)(struct drm_scanout_buffer *sb, unsigned int x, unsigned int y, u32 color);
    void *private;
};
```

**Members**

`format`
:   drm format of the scanout buffer.

`map`
:   Virtual address of the scanout buffer, either in memory or iomem.
    The scanout buffer should be in linear format, and can be directly
    sent to the display hardware. Tearing is not an issue for the panic
    screen.

`pages`
:   Optional, if the scanout buffer is not mapped, set this field
    to the array of pages of the scanout buffer. The panic code will use
    `kmap_local_page_try_from_panic()` to map one page at a time to write
    all the pixels. This array shouldn’t be allocated from the
    `get_scanoutbuffer()` callback.
    The scanout buffer should be in linear format.

`width`
:   Width of the scanout buffer, in pixels.

`height`
:   Height of the scanout buffer, in pixels.

`pitch`
:   Length in bytes between the start of two consecutive lines.

`set_pixel`
:   Optional function, to set a pixel color on the
    framebuffer. It allows to handle special tiling format inside the
    driver. It takes precedence over the **map** and **pages** fields.

`private`
:   private pointer that you can use in the callbacks
    `set_pixel()`

**Description**

This structure holds the information necessary for drm\_panic to draw the
panic screen, and display it.

drm\_panic\_trylock

`drm_panic_trylock (dev, flags)`

> try to enter the panic printing critical section

**Parameters**

`dev`
:   [`struct drm_device`](drm-internals.html#c.drm_device "drm_device")

`flags`
:   unsigned long irq flags you need to pass to the `unlock()` counterpart

**Description**

This function must be called by any panic printing code. The panic printing
attempt must be aborted if the trylock fails.

Panic printing code can make the following assumptions while holding the
panic lock:

* Anything protected by [`drm_panic_lock()`](#c.drm_panic_lock "drm_panic_lock") and [`drm_panic_unlock()`](#c.drm_panic_unlock "drm_panic_unlock") pairs is safe
  to access.
* Furthermore the panic printing code only registers in [`drm_dev_unregister()`](drm-internals.html#c.drm_dev_unregister "drm_dev_unregister")
  and gets removed in [`drm_dev_unregister()`](drm-internals.html#c.drm_dev_unregister "drm_dev_unregister"). This allows the panic code to
  safely access any state which is invariant in between these two function
  calls, like the list of planes [`drm_mode_config.plane_list`](#c.drm_mode_config "drm_mode_config") or most of the
  [`struct drm_plane`](#c.drm_plane "drm_plane") structure.

Specifically thanks to the protection around plane updates in
[`drm_atomic_helper_swap_state()`](drm-kms-helpers.html#c.drm_atomic_helper_swap_state "drm_atomic_helper_swap_state") the following additional guarantees hold:

* It is safe to deference the drm\_plane.state pointer.
* Anything in [`struct drm_plane_state`](#c.drm_plane_state "drm_plane_state") or the driver’s subclass thereof which
  stays invariant after the atomic check code has finished is safe to access.
  Specifically this includes the reference counted pointers to framebuffer
  and buffer objects.
* Anything set up by [`drm_plane_helper_funcs.fb_prepare`](drm-kms-helpers.html#c.drm_plane_helper_funcs "drm_plane_helper_funcs") and cleaned up
  [`drm_plane_helper_funcs.fb_cleanup`](drm-kms-helpers.html#c.drm_plane_helper_funcs "drm_plane_helper_funcs") is safe to access, as long as it stays
  invariant between these two calls. This also means that for drivers using
  dynamic buffer management the framebuffer is pinned, and therefer all
  relevant datastructures can be accessed without taking any further locks
  (which would be impossible in panic context anyway).
* Importantly, software and hardware state set up by
  [`drm_plane_helper_funcs.begin_fb_access`](drm-kms-helpers.html#c.drm_plane_helper_funcs "drm_plane_helper_funcs") and
  [`drm_plane_helper_funcs.end_fb_access`](drm-kms-helpers.html#c.drm_plane_helper_funcs "drm_plane_helper_funcs") is not safe to access.

Drivers must not make any assumptions about the actual state of the hardware,
unless they explicitly protected these hardware access with [`drm_panic_lock()`](#c.drm_panic_lock "drm_panic_lock")
and [`drm_panic_unlock()`](#c.drm_panic_unlock "drm_panic_unlock").

**Return**

`0` when failing to acquire the raw spinlock, nonzero on success.

drm\_panic\_lock

`drm_panic_lock (dev, flags)`

> protect panic printing relevant state

**Parameters**

`dev`
:   [`struct drm_device`](drm-internals.html#c.drm_device "drm_device")

`flags`
:   unsigned long irq flags you need to pass to the `unlock()` counterpart

**Description**

This function must be called to protect software and hardware state that the
panic printing code must be able to rely on. The protected sections must be
as small as possible. It uses the irqsave/irqrestore variant, and can be
called from irq handler. Examples include:

* Access to peek/poke or other similar registers, if that is the way the
  driver prints the pixels into the scanout buffer at panic time.
* Updates to pointers like [`drm_plane.state`](#c.drm_plane "drm_plane"), allowing the panic handler to
  safely deference these. This is done in [`drm_atomic_helper_swap_state()`](drm-kms-helpers.html#c.drm_atomic_helper_swap_state "drm_atomic_helper_swap_state").
* An state that isn’t invariant and that the driver must be able to access
  during panic printing.

drm\_panic\_unlock

`drm_panic_unlock (dev, flags)`

> end of the panic printing critical section

**Parameters**

`dev`
:   [`struct drm_device`](drm-internals.html#c.drm_device "drm_device")

`flags`
:   irq flags that were returned when acquiring the lock

**Description**

Unlocks the raw spinlock acquired by either [`drm_panic_lock()`](#c.drm_panic_lock "drm_panic_lock") or
[`drm_panic_trylock()`](#c.drm_panic_trylock "drm_panic_trylock").

bool drm\_panic\_is\_enabled(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)

**Parameters**

`struct drm_device *dev`
:   the drm device that may supports drm\_panic

**Description**

returns true if the drm device supports drm\_panic

## Colorop Abstraction

When userspace signals the [`DRM_CLIENT_CAP_PLANE_COLOR_PIPELINE`](drm-uapi.html#c.DRM_CLIENT_CAP_PLANE_COLOR_PIPELINE "DRM_CLIENT_CAP_PLANE_COLOR_PIPELINE") it
should use the COLOR\_PIPELINE plane property and associated colorops
for any color operation on the [`drm_plane`](#c.drm_plane "drm_plane"). Setting of all old color
properties, such as COLOR\_ENCODING and COLOR\_RANGE, will be rejected
and the values of the properties will be ignored.

Colorops are only advertised and valid for atomic drivers and atomic
userspace that signals the [`DRM_CLIENT_CAP_PLANE_COLOR_PIPELINE`](drm-uapi.html#c.DRM_CLIENT_CAP_PLANE_COLOR_PIPELINE "DRM_CLIENT_CAP_PLANE_COLOR_PIPELINE")
client cap.

A colorop represents a single color operation. Colorops are chained
via the NEXT property and make up color pipelines. Color pipelines
are advertised and selected via the COLOR\_PIPELINE [`drm_plane`](#c.drm_plane "drm_plane")
property.

A colorop will be of a certain type, advertised by the read-only TYPE
property. Each type of colorop will advertise a different set of
properties and is programmed in a different manner. Types can be
enumerated 1D curves, 1D LUTs, 3D LUTs, matrices, etc. See the
[`drm_colorop_type`](drm-uapi.html#c.drm_colorop_type "drm_colorop_type") documentation for information on each type.

If a colorop advertises the BYPASS property it can be bypassed.

Information about colorop and color pipeline design decisions can be
found at [Linux Color Pipeline API](rfc/color_pipeline.html), but note that this document will
grow stale over time.

### Colorop Functions Reference

enum drm\_colorop\_curve\_1d\_type
:   type of 1D curve

**Constants**

`DRM_COLOROP_1D_CURVE_SRGB_EOTF`
:   `enum string` “sRGB EOTF”

    sRGB piece-wise electro-optical transfer function. Transfer
    characteristics as defined by IEC 61966-2-1 sRGB. Equivalent
    to H.273 TransferCharacteristics code point 13 with
    MatrixCoefficients set to 0.

`DRM_COLOROP_1D_CURVE_SRGB_INV_EOTF`
:   `enum string` “sRGB Inverse EOTF”

    The inverse of `DRM_COLOROP_1D_CURVE_SRGB_EOTF`

`DRM_COLOROP_1D_CURVE_PQ_125_EOTF`
:   `enum string` “PQ 125 EOTF”

    The PQ transfer function, scaled by 125.0f, so that 10,000
    nits correspond to 125.0f.

    Transfer characteristics of the PQ function as defined by
    SMPTE ST 2084 (2014) for 10-, 12-, 14-, and 16-bit systems
    and Rec. ITU-R BT.2100-2 perceptual quantization (PQ) system,
    represented by H.273 TransferCharacteristics code point 16.

`DRM_COLOROP_1D_CURVE_PQ_125_INV_EOTF`
:   `enum string` “PQ 125 Inverse EOTF”

    The inverse of DRM\_COLOROP\_1D\_CURVE\_PQ\_125\_EOTF.

`DRM_COLOROP_1D_CURVE_BT2020_INV_OETF`
:   `enum string` “BT.2020 Inverse OETF”

    The inverse of `DRM_COLOROP_1D_CURVE_BT2020_OETF`

`DRM_COLOROP_1D_CURVE_BT2020_OETF`
:   `enum string` “BT.2020 OETF”

    The BT.2020/BT.709 transfer function. The BT.709 and BT.2020
    transfer functions are the same, the only difference is that
    BT.2020 is defined with more precision for 10 and 12-bit
    encodings.

`DRM_COLOROP_1D_CURVE_GAMMA22`
:   `enum string` “Gamma 2.2”

    A gamma 2.2 power function. This applies a power curve with
    gamma value of 2.2 to the input values.

`DRM_COLOROP_1D_CURVE_GAMMA22_INV`
:   `enum string` “Gamma 2.2 Inverse”

    The inverse of `DRM_COLOROP_1D_CURVE_GAMMA22`

`DRM_COLOROP_1D_CURVE_COUNT`
:   `enum value` denoting the size of the enum

**Description**

Describes a 1D curve to be applied by the DRM\_COLOROP\_1D\_CURVE colorop.

struct drm\_colorop\_state
:   mutable colorop state

**Definition**:

```
struct drm_colorop_state {
    struct drm_colorop *colorop;
    bool bypass;
    enum drm_colorop_curve_1d_type curve_1d_type;
    uint64_t multiplier;
    struct drm_property_blob *data;
    enum drm_colorop_lut1d_interpolation_type lut1d_interpolation;
    enum drm_colorop_lut3d_interpolation_type lut3d_interpolation;
    struct drm_atomic_state *state;
};
```

**Members**

`colorop`
:   backpointer to the colorop

`bypass`
:   When the property BYPASS exists on this colorop, this stores
    the requested bypass state: true if colorop shall be bypassed,
    false if colorop is enabled.

`curve_1d_type`
:   Type of 1D curve.

`multiplier`
:   Multiplier to ‘gain’ the plane. Format is S31.32 sign-magnitude.

`data`
:   Data blob for any TYPE that requires such a blob. The
    interpretation of the blob is TYPE-specific.

    See the [`drm_colorop_type`](drm-uapi.html#c.drm_colorop_type "drm_colorop_type") documentation for how blob is laid
    out.

`lut1d_interpolation`
:   Interpolation for DRM\_COLOROP\_1D\_LUT

`lut3d_interpolation`
:   Interpolation for DRM\_COLOROP\_3D\_LUT

`state`
:   backpointer to global drm\_atomic\_state

struct drm\_colorop\_funcs
:   driver colorop control functions

**Definition**:

```
struct drm_colorop_funcs {
    void (*destroy)(struct drm_colorop *colorop);
};
```

**Members**

`destroy`
:   Clean up colorop resources. This is called at driver unload time
    through [`drm_mode_config_cleanup()`](#c.drm_mode_config_cleanup "drm_mode_config_cleanup")

struct drm\_colorop
:   DRM color operation control structure

**Definition**:

```
struct drm_colorop {
    struct drm_device *dev;
    struct list_head head;
    unsigned int index;
    struct drm_mode_object base;
    struct drm_plane *plane;
    struct drm_colorop_state *state;
    struct drm_object_properties properties;
    enum drm_colorop_type type;
    struct drm_colorop *next;
    struct drm_property *type_property;
    struct drm_property *bypass_property;
    uint32_t size;
    struct drm_property *lut1d_interpolation_property;
    struct drm_property *curve_1d_type_property;
    struct drm_property *multiplier_property;
    struct drm_property *size_property;
    struct drm_property *lut3d_interpolation_property;
    struct drm_property *data_property;
    struct drm_property *next_property;
    const struct drm_colorop_funcs *funcs;
};
```

**Members**

`dev`
:   parent DRM device

`head`
:   List of all colorops on **dev**, linked from [`drm_mode_config.colorop_list`](#c.drm_mode_config "drm_mode_config").
    Invariant over the lifetime of **dev** and therefore does not need
    locking.

`index`
:   Position inside the mode\_config.list, can be used as an array
    index. It is invariant over the lifetime of the colorop.

`base`
:   base mode object

`plane`
:   The plane on which the colorop sits. A drm\_colorop is always unique
    to a plane.

`state`
:   Current atomic state for this colorop.

    This is protected by **mutex**. Note that nonblocking atomic commits
    access the current colorop state without taking locks.

`properties`
:   property tracking for this colorop

`type`
:   Read-only
    Type of color operation

`next`
:   Read-only
    Pointer to next drm\_colorop in pipeline

`type_property`
:   Read-only “TYPE” property for specifying the type of
    this color operation. The type is [`enum drm_colorop_type`](drm-uapi.html#c.drm_colorop_type "drm_colorop_type").

`bypass_property`
:   Boolean property to control enablement of the color
    operation. Only present if DRM\_COLOROP\_FLAG\_ALLOW\_BYPASS
    flag is set. When present, setting bypass to “true” shall
    always be supported to allow compositors to quickly fall
    back to alternate methods of color processing. This is
    important since setting color operations can fail due to
    unique HW constraints.

`size`
:   Number of entries of the custom LUT. This should be read-only.

`lut1d_interpolation_property`
:   Property for DRM\_COLOROP\_1D\_LUT interpolation

`curve_1d_type_property`
:   Sub-type for DRM\_COLOROP\_1D\_CURVE type.

`multiplier_property`
:   Multiplier property for plane gain

`size_property`
:   Size property for custom LUT from userspace.

`lut3d_interpolation_property`
:   Property for DRM\_COLOROP\_3D\_LUT interpolation

`data_property`
:   blob property for any TYPE that requires a blob of data,
    such as 1DLUT, CTM, 3DLUT, etc.

    The way this blob is interpreted depends on the TYPE of
    this

`next_property`
:   Read-only property to next colorop in the pipeline

`funcs`
:   colorop control functions

**Description**

A colorop represents one color operation. They can be chained via
the ‘next’ pointer to build a color pipeline.

Since colorops cannot stand-alone and are used to describe colorop
operations on a plane they don’t have their own locking mechanism but
are locked and programmed along with their associated [`drm_plane`](#c.drm_plane "drm_plane").

struct [drm\_colorop](#c.drm_colorop "drm_colorop") \*drm\_colorop\_find(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_priv, uint32\_t id)
:   look up a Colorop object from its ID

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_file *file_priv`
:   drm file to check for lease against.

`uint32_t id`
:   [`drm_mode_object`](#c.drm_mode_object "drm_mode_object") ID

**Description**

This can be used to look up a Colorop from its userspace ID. Only used by
drivers for legacy IOCTLs and interface, nowadays extensions to the KMS
userspace interface should be done using [`drm_property`](#c.drm_property "drm_property").

void drm\_colorop\_reset(struct [drm\_colorop](#c.drm_colorop "drm_colorop") \*colorop)
:   reset colorop atomic state

**Parameters**

`struct drm_colorop *colorop`
:   drm colorop

**Description**

Resets the atomic state for **colorop** by freeing the state pointer (which might
be NULL, e.g. at driver load time) and allocating a new empty state object.

unsigned int drm\_colorop\_index(const struct [drm\_colorop](#c.drm_colorop "drm_colorop") \*colorop)
:   find the index of a registered colorop

**Parameters**

`const struct drm_colorop *colorop`
:   colorop to find index for

**Description**

Given a registered colorop, return the index of that colorop within a DRM
device’s list of colorops.

const char \*drm\_get\_colorop\_type\_name(enum [drm\_colorop\_type](drm-uapi.html#c.drm_colorop_type "drm_colorop_type") type)
:   return a string for colorop type

**Parameters**

`enum drm_colorop_type type`
:   colorop type to compute name of

**Description**

In contrast to the other drm\_get\_\*\_name functions this one here returns a
const pointer and hence is threadsafe.

const char \*drm\_get\_colorop\_curve\_1d\_type\_name(enum [drm\_colorop\_curve\_1d\_type](#c.drm_colorop_curve_1d_type "drm_colorop_curve_1d_type") type)
:   return a string for 1D curve type

**Parameters**

`enum drm_colorop_curve_1d_type type`
:   1d curve type to compute name of

**Description**

In contrast to the other drm\_get\_\*\_name functions this one here returns a
const pointer and hence is threadsafe.

void drm\_colorop\_cleanup(struct [drm\_colorop](#c.drm_colorop "drm_colorop") \*colorop)
:   Cleanup a drm\_colorop object in color\_pipeline

**Parameters**

`struct drm_colorop *colorop`
:   The drm\_colorop object to be cleaned

void drm\_colorop\_destroy(struct [drm\_colorop](#c.drm_colorop "drm_colorop") \*colorop)
:   destroy colorop

**Parameters**

`struct drm_colorop *colorop`
:   drm colorop

**Description**

Destroys **colorop** by performing common DRM cleanup and freeing the
colorop object. This can be used by drivers if they do not
require any driver-specific teardown.

void drm\_colorop\_pipeline\_destroy(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   Helper for color pipeline destruction

**Parameters**

`struct drm_device *dev`
:   * The drm\_device containing the drm\_planes with the color\_pipelines

**Description**

Provides a default color pipeline destroy handler for drm\_device.

int drm\_plane\_colorop\_curve\_1d\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_colorop](#c.drm_colorop "drm_colorop") \*colorop, struct [drm\_plane](#c.drm_plane "drm_plane") \*plane, const struct [drm\_colorop\_funcs](#c.drm_colorop_funcs "drm_colorop_funcs") \*funcs, u64 supported\_tfs, uint32\_t flags)
:   Initialize a DRM\_COLOROP\_1D\_CURVE

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_colorop *colorop`
:   The drm\_colorop object to initialize

`struct drm_plane *plane`
:   The associated drm\_plane

`const struct drm_colorop_funcs *funcs`
:   control functions for the new colorop

`u64 supported_tfs`
:   A bitfield of supported drm\_plane\_colorop\_curve\_1d\_init `enum values`,
    created using BIT(curve\_type) and combined with the OR ‘|’
    operator.

`uint32_t flags`
:   bitmask of misc, see DRM\_COLOROP\_FLAG\_\* defines.
    **return** zero on success, -E value on failure

int drm\_plane\_colorop\_curve\_1d\_lut\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_colorop](#c.drm_colorop "drm_colorop") \*colorop, struct [drm\_plane](#c.drm_plane "drm_plane") \*plane, const struct [drm\_colorop\_funcs](#c.drm_colorop_funcs "drm_colorop_funcs") \*funcs, uint32\_t lut\_size, enum [drm\_colorop\_lut1d\_interpolation\_type](drm-uapi.html#c.drm_colorop_lut1d_interpolation_type "drm_colorop_lut1d_interpolation_type") interpolation, uint32\_t flags)
:   Initialize a DRM\_COLOROP\_1D\_LUT

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_colorop *colorop`
:   The drm\_colorop object to initialize

`struct drm_plane *plane`
:   The associated drm\_plane

`const struct drm_colorop_funcs *funcs`
:   control functions for new colorop

`uint32_t lut_size`
:   LUT size supported by driver

`enum drm_colorop_lut1d_interpolation_type interpolation`
:   1D LUT interpolation type

`uint32_t flags`
:   bitmask of misc, see DRM\_COLOROP\_FLAG\_\* defines.
    **return** zero on success, -E value on failure

int drm\_plane\_colorop\_mult\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_colorop](#c.drm_colorop "drm_colorop") \*colorop, struct [drm\_plane](#c.drm_plane "drm_plane") \*plane, const struct [drm\_colorop\_funcs](#c.drm_colorop_funcs "drm_colorop_funcs") \*funcs, uint32\_t flags)
:   Initialize a DRM\_COLOROP\_MULTIPLIER

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_colorop *colorop`
:   The drm\_colorop object to initialize

`struct drm_plane *plane`
:   The associated drm\_plane

`const struct drm_colorop_funcs *funcs`
:   control functions for the new colorop

`uint32_t flags`
:   bitmask of misc, see DRM\_COLOROP\_FLAG\_\* defines.
    **return** zero on success, -E value on failure

void drm\_colorop\_set\_next\_property(struct [drm\_colorop](#c.drm_colorop "drm_colorop") \*colorop, struct [drm\_colorop](#c.drm_colorop "drm_colorop") \*next)
:   sets the next pointer

**Parameters**

`struct drm_colorop *colorop`
:   drm colorop

`struct drm_colorop *next`
:   next colorop

**Description**

Should be used when constructing the color pipeline

## Display Modes Function Reference

enum drm\_mode\_status
:   hardware support status of a mode

**Constants**

`MODE_OK`
:   Mode OK

`MODE_HSYNC`
:   hsync out of range

`MODE_VSYNC`
:   vsync out of range

`MODE_H_ILLEGAL`
:   mode has illegal horizontal timings

`MODE_V_ILLEGAL`
:   mode has illegal vertical timings

`MODE_BAD_WIDTH`
:   requires an unsupported linepitch

`MODE_NOMODE`
:   no mode with a matching name

`MODE_NO_INTERLACE`
:   interlaced mode not supported

`MODE_NO_DBLESCAN`
:   doublescan mode not supported

`MODE_NO_VSCAN`
:   multiscan mode not supported

`MODE_MEM`
:   insufficient video memory

`MODE_VIRTUAL_X`
:   mode width too large for specified virtual size

`MODE_VIRTUAL_Y`
:   mode height too large for specified virtual size

`MODE_MEM_VIRT`
:   insufficient video memory given virtual size

`MODE_NOCLOCK`
:   no fixed clock available

`MODE_CLOCK_HIGH`
:   clock required is too high

`MODE_CLOCK_LOW`
:   clock required is too low

`MODE_CLOCK_RANGE`
:   clock/mode isn’t in a ClockRange

`MODE_BAD_HVALUE`
:   horizontal timing was out of range

`MODE_BAD_VVALUE`
:   vertical timing was out of range

`MODE_BAD_VSCAN`
:   VScan value out of range

`MODE_HSYNC_NARROW`
:   horizontal sync too narrow

`MODE_HSYNC_WIDE`
:   horizontal sync too wide

`MODE_HBLANK_NARROW`
:   horizontal blanking too narrow

`MODE_HBLANK_WIDE`
:   horizontal blanking too wide

`MODE_VSYNC_NARROW`
:   vertical sync too narrow

`MODE_VSYNC_WIDE`
:   vertical sync too wide

`MODE_VBLANK_NARROW`
:   vertical blanking too narrow

`MODE_VBLANK_WIDE`
:   vertical blanking too wide

`MODE_PANEL`
:   exceeds panel dimensions

`MODE_INTERLACE_WIDTH`
:   width too large for interlaced mode

`MODE_ONE_WIDTH`
:   only one width is supported

`MODE_ONE_HEIGHT`
:   only one height is supported

`MODE_ONE_SIZE`
:   only one resolution is supported

`MODE_NO_REDUCED`
:   monitor doesn’t accept reduced blanking

`MODE_NO_STEREO`
:   stereo modes not supported

`MODE_NO_420`
:   ycbcr 420 modes not supported

`MODE_STALE`
:   mode has become stale

`MODE_BAD`
:   unspecified reason

`MODE_ERROR`
:   error condition

**Description**

This `enum is` used to filter out modes not supported by the driver/hardware
combination.

DRM\_MODE\_RES\_MM

`DRM_MODE_RES_MM (res, dpi)`

> Calculates the display size from resolution and DPI

**Parameters**

`res`
:   The resolution in pixel

`dpi`
:   The number of dots per inch

DRM\_MODE\_INIT

`DRM_MODE_INIT (hz, hd, vd, hd_mm, vd_mm)`

> Initialize display mode

**Parameters**

`hz`
:   Vertical refresh rate in Hertz

`hd`
:   Horizontal resolution, width

`vd`
:   Vertical resolution, height

`hd_mm`
:   Display width in millimeters

`vd_mm`
:   Display height in millimeters

**Description**

This macro initializes a [`drm_display_mode`](#c.drm_display_mode "drm_display_mode") that contains information about
refresh rate, resolution and physical size.

DRM\_SIMPLE\_MODE

`DRM_SIMPLE_MODE (hd, vd, hd_mm, vd_mm)`

> Simple display mode

**Parameters**

`hd`
:   Horizontal resolution, width

`vd`
:   Vertical resolution, height

`hd_mm`
:   Display width in millimeters

`vd_mm`
:   Display height in millimeters

**Description**

This macro initializes a [`drm_display_mode`](#c.drm_display_mode "drm_display_mode") that only contains info about
resolution and physical size.

struct drm\_display\_mode
:   DRM kernel-internal display mode structure

**Definition**:

```
struct drm_display_mode {
    int clock;
    u16 hdisplay;
    u16 hsync_start;
    u16 hsync_end;
    u16 htotal;
    u16 hskew;
    u16 vdisplay;
    u16 vsync_start;
    u16 vsync_end;
    u16 vtotal;
    u16 vscan;
    u32 flags;
    int crtc_clock;
    u16 crtc_hdisplay;
    u16 crtc_hblank_start;
    u16 crtc_hblank_end;
    u16 crtc_hsync_start;
    u16 crtc_hsync_end;
    u16 crtc_htotal;
    u16 crtc_hskew;
    u16 crtc_vdisplay;
    u16 crtc_vblank_start;
    u16 crtc_vblank_end;
    u16 crtc_vsync_start;
    u16 crtc_vsync_end;
    u16 crtc_vtotal;
    u16 width_mm;
    u16 height_mm;
    u8 type;
    bool expose_to_userspace;
    struct list_head head;
    char name[DRM_DISPLAY_MODE_LEN];
    enum drm_mode_status status;
    enum hdmi_picture_aspect picture_aspect_ratio;
};
```

**Members**

`clock`
:   Pixel clock in kHz.

`hdisplay`
:   horizontal display size

`hsync_start`
:   horizontal sync start

`hsync_end`
:   horizontal sync end

`htotal`
:   horizontal total size

`hskew`
:   horizontal skew?!

`vdisplay`
:   vertical display size

`vsync_start`
:   vertical sync start

`vsync_end`
:   vertical sync end

`vtotal`
:   vertical total size

`vscan`
:   vertical scan?!

`flags`
:   Sync and timing flags:

    > * DRM\_MODE\_FLAG\_PHSYNC: horizontal sync is active high.
    > * DRM\_MODE\_FLAG\_NHSYNC: horizontal sync is active low.
    > * DRM\_MODE\_FLAG\_PVSYNC: vertical sync is active high.
    > * DRM\_MODE\_FLAG\_NVSYNC: vertical sync is active low.
    > * DRM\_MODE\_FLAG\_INTERLACE: mode is interlaced.
    > * DRM\_MODE\_FLAG\_DBLSCAN: mode uses doublescan.
    > * DRM\_MODE\_FLAG\_CSYNC: mode uses composite sync.
    > * DRM\_MODE\_FLAG\_PCSYNC: composite sync is active high.
    > * DRM\_MODE\_FLAG\_NCSYNC: composite sync is active low.
    > * DRM\_MODE\_FLAG\_HSKEW: hskew provided (not used?).
    > * DRM\_MODE\_FLAG\_BCAST: <deprecated>
    > * DRM\_MODE\_FLAG\_PIXMUX: <deprecated>
    > * DRM\_MODE\_FLAG\_DBLCLK: double-clocked mode.
    > * DRM\_MODE\_FLAG\_CLKDIV2: half-clocked mode.

    Additionally there’s flags to specify how 3D modes are packed:

    > * DRM\_MODE\_FLAG\_3D\_NONE: normal, non-3D mode.
    > * DRM\_MODE\_FLAG\_3D\_FRAME\_PACKING: 2 full frames for left and right.
    > * DRM\_MODE\_FLAG\_3D\_FIELD\_ALTERNATIVE: interleaved like fields.
    > * DRM\_MODE\_FLAG\_3D\_LINE\_ALTERNATIVE: interleaved lines.
    > * DRM\_MODE\_FLAG\_3D\_SIDE\_BY\_SIDE\_FULL: side-by-side full frames.
    > * DRM\_MODE\_FLAG\_3D\_L\_DEPTH: ?
    > * DRM\_MODE\_FLAG\_3D\_L\_DEPTH\_GFX\_GFX\_DEPTH: ?
    > * DRM\_MODE\_FLAG\_3D\_TOP\_AND\_BOTTOM: frame split into top and bottom
    >   parts.
    > * DRM\_MODE\_FLAG\_3D\_SIDE\_BY\_SIDE\_HALF: frame split into left and
    >   right parts.

`crtc_clock`
:   Actual pixel or dot clock in the hardware. This differs from the
    logical **clock** when e.g. using interlacing, double-clocking, stereo
    modes or other fancy stuff that changes the timings and signals
    actually sent over the wire.

    This is again in kHz.

    Note that with digital outputs like HDMI or DP there’s usually a
    massive confusion between the dot clock and the signal clock at the
    bit encoding level. Especially when a 8b/10b encoding is used and the
    difference is exactly a factor of 10.

`crtc_hdisplay`
:   hardware mode horizontal display size

`crtc_hblank_start`
:   hardware mode horizontal blank start

`crtc_hblank_end`
:   hardware mode horizontal blank end

`crtc_hsync_start`
:   hardware mode horizontal sync start

`crtc_hsync_end`
:   hardware mode horizontal sync end

`crtc_htotal`
:   hardware mode horizontal total size

`crtc_hskew`
:   hardware mode horizontal skew?!

`crtc_vdisplay`
:   hardware mode vertical display size

`crtc_vblank_start`
:   hardware mode vertical blank start

`crtc_vblank_end`
:   hardware mode vertical blank end

`crtc_vsync_start`
:   hardware mode vertical sync start

`crtc_vsync_end`
:   hardware mode vertical sync end

`crtc_vtotal`
:   hardware mode vertical total size

`width_mm`
:   Addressable size of the output in mm, projectors should set this to
    0.

`height_mm`
:   Addressable size of the output in mm, projectors should set this to
    0.

`type`
:   A bitmask of flags, mostly about the source of a mode. Possible flags
    are:

    > * DRM\_MODE\_TYPE\_PREFERRED: Preferred mode, usually the native
    >   resolution of an LCD panel. There should only be one preferred
    >   mode per connector at any given time.
    > * DRM\_MODE\_TYPE\_DRIVER: Mode created by the driver, which is all of
    >   them really. Drivers must set this bit for all modes they create
    >   and expose to userspace.
    > * DRM\_MODE\_TYPE\_USERDEF: Mode defined or selected via the kernel
    >   command line.

    Plus a big list of flags which shouldn’t be used at all, but are
    still around since these flags are also used in the userspace ABI.
    We no longer accept modes with these types though:

    > * DRM\_MODE\_TYPE\_BUILTIN: Meant for hard-coded modes, unused.
    >   Use DRM\_MODE\_TYPE\_DRIVER instead.
    > * DRM\_MODE\_TYPE\_DEFAULT: Again a leftover, use
    >   DRM\_MODE\_TYPE\_PREFERRED instead.
    > * DRM\_MODE\_TYPE\_CLOCK\_C and DRM\_MODE\_TYPE\_CRTC\_C: Define leftovers
    >   which are stuck around for hysterical raisins only. No one has an
    >   idea what they were meant for. Don’t use.

`expose_to_userspace`
:   Indicates whether the mode is to be exposed to the userspace.
    This is to maintain a set of exposed modes while preparing
    user-mode’s list in drm\_mode\_getconnector ioctl. The purpose of
    this only lies in the ioctl function, and is not to be used
    outside the function.

`head`
:   `struct list_head` for mode lists.

`name`
:   Human-readable name of the mode, filled out with [`drm_mode_set_name()`](#c.drm_mode_set_name "drm_mode_set_name").

`status`
:   Status of the mode, used to filter out modes not supported by the
    hardware. See enum [`drm_mode_status`](#c.drm_mode_status "drm_mode_status").

`picture_aspect_ratio`
:   Field for setting the HDMI picture aspect ratio of a mode.

**Description**

This is the kernel API display mode information structure. For the
user-space version see [`struct drm_mode_modeinfo`](drm-uapi.html#c.drm_mode_modeinfo "drm_mode_modeinfo").

The horizontal and vertical timings are defined per the following diagram.

```
          Active                 Front           Sync           Back
         Region                 Porch                          Porch
<-----------------------><----------------><-------------><-------------->
  //////////////////////|
 ////////////////////// |
//////////////////////  |..................               ................
                                           _______________
<----- [hv]display ----->
<------------- [hv]sync_start ------------>
<--------------------- [hv]sync_end --------------------->
<-------------------------------- [hv]total ----------------------------->*
```

This structure contains two copies of timings. First are the plain timings,
which specify the logical mode, as it would be for a progressive 1:1 scanout
at the refresh rate userspace can observe through vblank timestamps. Then
there’s the hardware timings, which are corrected for interlacing,
double-clocking and similar things. They are provided as a convenience, and
can be appropriately computed using [`drm_mode_set_crtcinfo()`](#c.drm_mode_set_crtcinfo "drm_mode_set_crtcinfo").

For printing you can use `DRM_MODE_FMT` and [`DRM_MODE_ARG()`](#c.DRM_MODE_ARG "DRM_MODE_ARG").

DRM\_MODE\_FMT

`DRM_MODE_FMT`

> printf string for [`struct drm_display_mode`](#c.drm_display_mode "drm_display_mode")

DRM\_MODE\_ARG

`DRM_MODE_ARG (m)`

> printf arguments for [`struct drm_display_mode`](#c.drm_display_mode "drm_display_mode")

**Parameters**

`m`
:   display mode

bool drm\_mode\_is\_stereo(const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode)
:   check for stereo mode flags

**Parameters**

`const struct drm_display_mode *mode`
:   drm\_display\_mode to check

**Return**

True if the mode is one of the stereo modes (like side-by-side), false if
not.

void drm\_mode\_debug\_printmodeline(const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode)
:   print a mode to dmesg

**Parameters**

`const struct drm_display_mode *mode`
:   mode to print

**Description**

Describe **mode** using DRM\_DEBUG.

struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*drm\_mode\_create(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   create a new display mode

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

Create a new, cleared drm\_display\_mode with kzalloc, allocate an ID for it
and return it.

**Return**

Pointer to new mode on success, NULL on error.

void drm\_mode\_destroy(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode)
:   remove a mode

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_display_mode *mode`
:   mode to remove

**Description**

Release **mode**’s unique ID, then free it **mode** structure itself using kfree.

void drm\_mode\_probed\_add(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode)
:   add a mode to a connector’s probed\_mode list

**Parameters**

`struct drm_connector *connector`
:   connector the new mode

`struct drm_display_mode *mode`
:   mode data

**Description**

Add **mode** to **connector**’s probed\_mode list for later use. This list should
then in a second step get filtered and all the modes actually supported by
the hardware moved to the **connector**’s modes list.

struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*drm\_analog\_tv\_mode(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, enum [drm\_connector\_tv\_mode](#c.drm_connector_tv_mode "drm_connector_tv_mode") tv\_mode, unsigned long pixel\_clock\_hz, unsigned int hdisplay, unsigned int vdisplay, bool interlace)
:   create a display mode for an analog TV

**Parameters**

`struct drm_device *dev`
:   drm device

`enum drm_connector_tv_mode tv_mode`
:   TV Mode standard to create a mode for. See DRM\_MODE\_TV\_MODE\_\*.

`unsigned long pixel_clock_hz`
:   Pixel Clock Frequency, in Hertz

`unsigned int hdisplay`
:   hdisplay size

`unsigned int vdisplay`
:   vdisplay size

`bool interlace`
:   whether to compute an interlaced mode

**Description**

This function creates a [`struct drm_display_mode`](#c.drm_display_mode "drm_display_mode") instance suited for
an analog TV output, for one of the usual analog TV modes. Where
this is DRM\_MODE\_TV\_MODE\_MONOCHROME, a 625-line mode will be created.

Note that **hdisplay** is larger than the usual constraints for the PAL
and NTSC timings, and we’ll choose to ignore most timings constraints
to reach those resolutions.

**Return**

A pointer to the mode, allocated with [`drm_mode_create()`](#c.drm_mode_create "drm_mode_create"). Returns NULL
on error.

struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*drm\_cvt\_mode(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, int hdisplay, int vdisplay, int vrefresh, bool reduced, bool interlaced, bool margins)
:   create a modeline based on the CVT algorithm

**Parameters**

`struct drm_device *dev`
:   drm device

`int hdisplay`
:   hdisplay size

`int vdisplay`
:   vdisplay size

`int vrefresh`
:   vrefresh rate

`bool reduced`
:   whether to use reduced blanking

`bool interlaced`
:   whether to compute an interlaced mode

`bool margins`
:   whether to add margins (borders)

**Description**

This function is called to generate the modeline based on CVT algorithm
according to the hdisplay, vdisplay, vrefresh.
It is based from the VESA(TM) Coordinated Video Timing Generator by
Graham Loveridge April 9, 2003 available at
<http://www.elo.utfsm.cl/~elo212/docs/CVTd6r1.xls>

And it is copied from xf86CVTmode in xserver/hw/xfree86/modes/xf86cvt.c.
What I have done is to translate it by using integer calculation.

**Return**

The modeline based on the CVT algorithm stored in a drm\_display\_mode object.
The display mode object is allocated with [`drm_mode_create()`](#c.drm_mode_create "drm_mode_create"). Returns NULL
when no mode could be allocated.

struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*drm\_gtf\_mode\_complex(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, int hdisplay, int vdisplay, int vrefresh, bool interlaced, int margins, int GTF\_M, int GTF\_2C, int GTF\_K, int GTF\_2J)
:   create the modeline based on the full GTF algorithm

**Parameters**

`struct drm_device *dev`
:   drm device

`int hdisplay`
:   hdisplay size

`int vdisplay`
:   vdisplay size

`int vrefresh`
:   vrefresh rate.

`bool interlaced`
:   whether to compute an interlaced mode

`int margins`
:   desired margin (borders) size

`int GTF_M`
:   extended GTF formula parameters

`int GTF_2C`
:   extended GTF formula parameters

`int GTF_K`
:   extended GTF formula parameters

`int GTF_2J`
:   extended GTF formula parameters

**Description**

GTF feature blocks specify C and J in multiples of 0.5, so we pass them
in here multiplied by two. For a C of 40, pass in 80.

**Return**

The modeline based on the full GTF algorithm stored in a drm\_display\_mode object.
The display mode object is allocated with [`drm_mode_create()`](#c.drm_mode_create "drm_mode_create"). Returns NULL
when no mode could be allocated.

struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*drm\_gtf\_mode(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, int hdisplay, int vdisplay, int vrefresh, bool interlaced, int margins)
:   create the modeline based on the GTF algorithm

**Parameters**

`struct drm_device *dev`
:   drm device

`int hdisplay`
:   hdisplay size

`int vdisplay`
:   vdisplay size

`int vrefresh`
:   vrefresh rate.

`bool interlaced`
:   whether to compute an interlaced mode

`int margins`
:   desired margin (borders) size

**Description**

return the modeline based on GTF algorithm

This function is to create the modeline based on the GTF algorithm.
Generalized Timing Formula is derived from:

> GTF Spreadsheet by Andy Morrish (1/5/97)
> available at <https://www.vesa.org>

And it is copied from the file of xserver/hw/xfree86/modes/xf86gtf.c.
What I have done is to translate it by using integer calculation.
I also refer to the function of fb\_get\_mode in the file of
drivers/video/fbmon.c

Standard GTF parameters:

```
M = 600
C = 40
K = 128
J = 20
```

**Return**

The modeline based on the GTF algorithm stored in a drm\_display\_mode object.
The display mode object is allocated with [`drm_mode_create()`](#c.drm_mode_create "drm_mode_create"). Returns NULL
when no mode could be allocated.

void drm\_display\_mode\_from\_videomode(const struct videomode \*vm, struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*dmode)
:   fill in **dmode** using **vm**,

**Parameters**

`const struct videomode *vm`
:   videomode structure to use as source

`struct drm_display_mode *dmode`
:   drm\_display\_mode structure to use as destination

**Description**

Fills out **dmode** using the display mode specified in **vm**.

void drm\_display\_mode\_to\_videomode(const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*dmode, struct videomode \*vm)
:   fill in **vm** using **dmode**,

**Parameters**

`const struct drm_display_mode *dmode`
:   drm\_display\_mode structure to use as source

`struct videomode *vm`
:   videomode structure to use as destination

**Description**

Fills out **vm** using the display mode specified in **dmode**.

void drm\_bus\_flags\_from\_videomode(const struct videomode \*vm, u32 \*bus\_flags)
:   extract information about pixelclk and DE polarity from videomode and store it in a separate variable

**Parameters**

`const struct videomode *vm`
:   videomode structure to use

`u32 *bus_flags`
:   information about pixelclk, sync and DE polarity will be stored
    here

**Description**

Sets DRM\_BUS\_FLAG\_DE\_(LOW|HIGH), DRM\_BUS\_FLAG\_PIXDATA\_DRIVE\_(POS|NEG)EDGE
and DISPLAY\_FLAGS\_SYNC\_(POS|NEG)EDGE in **bus\_flags** according to DISPLAY\_FLAGS
found in **vm**

int of\_get\_drm\_display\_mode(struct device\_node \*np, struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*dmode, u32 \*bus\_flags, int index)
:   get a drm\_display\_mode from devicetree

**Parameters**

`struct device_node *np`
:   device\_node with the timing specification

`struct drm_display_mode *dmode`
:   will be set to the return value

`u32 *bus_flags`
:   information about pixelclk, sync and DE polarity

`int index`
:   index into the list of display timings in devicetree

**Description**

This function is expensive and should only be used, if only one mode is to be
read from DT. To get multiple modes start with of\_get\_display\_timings and
work with that instead.

**Return**

0 on success, a negative errno code when no of videomode node was found.

int of\_get\_drm\_panel\_display\_mode(struct device\_node \*np, struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*dmode, u32 \*bus\_flags)
:   get a panel-timing drm\_display\_mode from devicetree

**Parameters**

`struct device_node *np`
:   device\_node with the panel-timing specification

`struct drm_display_mode *dmode`
:   will be set to the return value

`u32 *bus_flags`
:   information about pixelclk, sync and DE polarity

**Description**

The mandatory Device Tree properties width-mm and height-mm
are read and set on the display mode.

**Return**

Zero on success, negative error code on failure.

void drm\_mode\_set\_name(struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode)
:   set the name on a mode

**Parameters**

`struct drm_display_mode *mode`
:   name will be set in this mode

**Description**

Set the name of **mode** to a standard format which is <hdisplay>x<vdisplay>
with an optional ‘i’ suffix for interlaced modes.

int drm\_mode\_vrefresh(const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode)
:   get the vrefresh of a mode

**Parameters**

`const struct drm_display_mode *mode`
:   mode

**Return**

**modes**’s vrefresh rate in Hz, rounded to the nearest integer.

void drm\_mode\_get\_hv\_timing(const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode, int \*hdisplay, int \*vdisplay)
:   Fetches hdisplay/vdisplay for given mode

**Parameters**

`const struct drm_display_mode *mode`
:   mode to query

`int *hdisplay`
:   hdisplay value to fill in

`int *vdisplay`
:   vdisplay value to fill in

**Description**

The vdisplay value will be doubled if the specified mode is a stereo mode of
the appropriate layout.

void drm\_mode\_set\_crtcinfo(struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*p, int adjust\_flags)
:   set CRTC modesetting timing parameters

**Parameters**

`struct drm_display_mode *p`
:   mode

`int adjust_flags`
:   a combination of adjustment flags

**Description**

Setup the CRTC modesetting timing parameters for **p**, adjusting if necessary.

* The CRTC\_INTERLACE\_HALVE\_V flag can be used to halve vertical timings of
  interlaced modes.
* The CRTC\_STEREO\_DOUBLE flag can be used to compute the timings for
  buffers containing two eyes (only adjust the timings when needed, eg. for
  “frame packing” or “side by side full”).
* The CRTC\_NO\_DBLSCAN and CRTC\_NO\_VSCAN flags request that adjustment *not*
  be performed for doublescan and vscan > 1 modes respectively.

void drm\_mode\_copy(struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*dst, const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*src)
:   copy the mode

**Parameters**

`struct drm_display_mode *dst`
:   mode to overwrite

`const struct drm_display_mode *src`
:   mode to copy

**Description**

Copy an existing mode into another mode, preserving the
list head of the destination mode.

void drm\_mode\_init(struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*dst, const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*src)
:   initialize the mode from another mode

**Parameters**

`struct drm_display_mode *dst`
:   mode to overwrite

`const struct drm_display_mode *src`
:   mode to copy

**Description**

Copy an existing mode into another mode, zeroing the
list head of the destination mode. Typically used
to guarantee the list head is not left with stack
garbage in on-stack modes.

struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*drm\_mode\_duplicate(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode)
:   allocate and duplicate an existing mode

**Parameters**

`struct drm_device *dev`
:   drm\_device to allocate the duplicated mode for

`const struct drm_display_mode *mode`
:   mode to duplicate

**Description**

Just allocate a new mode, copy the existing mode into it, and return
a pointer to it. Used to create new instances of established modes.

**Return**

Pointer to duplicated mode on success, NULL on error.

bool drm\_mode\_match(const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode1, const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode2, unsigned int match\_flags)
:   test modes for (partial) equality

**Parameters**

`const struct drm_display_mode *mode1`
:   first mode

`const struct drm_display_mode *mode2`
:   second mode

`unsigned int match_flags`
:   which parts need to match (DRM\_MODE\_MATCH\_\*)

**Description**

Check to see if **mode1** and **mode2** are equivalent.

**Return**

True if the modes are (partially) equal, false otherwise.

bool drm\_mode\_equal(const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode1, const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode2)
:   test modes for equality

**Parameters**

`const struct drm_display_mode *mode1`
:   first mode

`const struct drm_display_mode *mode2`
:   second mode

**Description**

Check to see if **mode1** and **mode2** are equivalent.

**Return**

True if the modes are equal, false otherwise.

bool drm\_mode\_equal\_no\_clocks(const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode1, const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode2)
:   test modes for equality

**Parameters**

`const struct drm_display_mode *mode1`
:   first mode

`const struct drm_display_mode *mode2`
:   second mode

**Description**

Check to see if **mode1** and **mode2** are equivalent, but
don’t check the pixel clocks.

**Return**

True if the modes are equal, false otherwise.

bool drm\_mode\_equal\_no\_clocks\_no\_stereo(const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode1, const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode2)
:   test modes for equality

**Parameters**

`const struct drm_display_mode *mode1`
:   first mode

`const struct drm_display_mode *mode2`
:   second mode

**Description**

Check to see if **mode1** and **mode2** are equivalent, but
don’t check the pixel clocks nor the stereo layout.

**Return**

True if the modes are equal, false otherwise.

enum [drm\_mode\_status](#c.drm_mode_status "drm_mode_status") drm\_mode\_validate\_driver(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode)
:   make sure the mode is somewhat sane

**Parameters**

`struct drm_device *dev`
:   drm device

`const struct drm_display_mode *mode`
:   mode to check

**Description**

First do basic validation on the mode, and then allow the driver
to check for device/driver specific limitations via the optional
[`drm_mode_config_helper_funcs.mode_valid`](drm-kms-helpers.html#c.drm_mode_config_helper_funcs "drm_mode_config_helper_funcs") hook.

**Return**

The mode status

enum [drm\_mode\_status](#c.drm_mode_status "drm_mode_status") drm\_mode\_validate\_size(const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode, int maxX, int maxY)
:   make sure modes adhere to size constraints

**Parameters**

`const struct drm_display_mode *mode`
:   mode to check

`int maxX`
:   maximum width

`int maxY`
:   maximum height

**Description**

This function is a helper which can be used to validate modes against size
limitations of the DRM device/connector. If a mode is too big its status
member is updated with the appropriate validation failure code. The list
itself is not changed.

**Return**

The mode status

enum [drm\_mode\_status](#c.drm_mode_status "drm_mode_status") drm\_mode\_validate\_ycbcr420(const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode, struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   add ‘ycbcr420-only’ modes only when allowed

**Parameters**

`const struct drm_display_mode *mode`
:   mode to check

`struct drm_connector *connector`
:   drm connector under action

**Description**

This function is a helper which can be used to filter out any YCBCR420
only mode, when the source doesn’t support it.

**Return**

The mode status

void drm\_mode\_prune\_invalid(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct list\_head \*mode\_list, bool verbose)
:   remove invalid modes from mode list

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct list_head *mode_list`
:   list of modes to check

`bool verbose`
:   be verbose about it

**Description**

This helper function can be used to prune a display mode list after
validation has been completed. All modes whose status is not MODE\_OK will be
removed from the list, and if **verbose** the status code and mode name is also
printed to dmesg.

void drm\_mode\_sort(struct list\_head \*mode\_list)
:   sort mode list

**Parameters**

`struct list_head *mode_list`
:   list of drm\_display\_mode structures to sort

**Description**

Sort **mode\_list** by favorability, moving good modes to the head of the list.

void drm\_connector\_list\_update(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   update the mode list for the connector

**Parameters**

`struct drm_connector *connector`
:   the connector to update

**Description**

This moves the modes from the **connector** probed\_modes list
to the actual mode list. It compares the probed mode against the current
list and only adds different/new modes.

This is just a helper functions doesn’t validate any modes itself and also
doesn’t prune any invalid modes. Callers need to do that themselves.

bool drm\_mode\_parse\_command\_line\_for\_connector(const char \*mode\_option, const struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, struct [drm\_cmdline\_mode](#c.drm_cmdline_mode "drm_cmdline_mode") \*mode)
:   parse command line modeline for connector

**Parameters**

`const char *mode_option`
:   optional per connector mode option

`const struct drm_connector *connector`
:   connector to parse modeline for

`struct drm_cmdline_mode *mode`
:   preallocated drm\_cmdline\_mode structure to fill out

**Description**

This parses **mode\_option** command line modeline for modes and options to
configure the connector.

This uses the same parameters as the fb modedb.c, except for an extra
force-enable, force-enable-digital and force-disable bit at the end:

```
<xres>x<yres>[M][R][-<bpp>][@<refresh>][i][m][eDd]
```

Additionals options can be provided following the mode, using a comma to
separate each option. Valid options can be found in
[modedb default video mode support](../fb/modedb.html).

The intermediate drm\_cmdline\_mode structure is required to store additional
options from the command line modline like the force-enable/disable flag.

**Return**

True if a valid modeline has been parsed, false otherwise.

struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*drm\_mode\_create\_from\_cmdline\_mode(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_cmdline\_mode](#c.drm_cmdline_mode "drm_cmdline_mode") \*cmd)
:   convert a command line modeline into a DRM display mode

**Parameters**

`struct drm_device *dev`
:   DRM device to create the new mode for

`struct drm_cmdline_mode *cmd`
:   input command line modeline

**Return**

Pointer to converted mode on success, NULL on error.

bool drm\_mode\_is\_420\_only(const struct [drm\_display\_info](#c.drm_display_info "drm_display_info") \*display, const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode)
:   if a given videomode can be only supported in YCBCR420 output format

**Parameters**

`const struct drm_display_info *display`
:   display under action

`const struct drm_display_mode *mode`
:   video mode to be tested.

**Return**

true if the mode can be supported in YCBCR420 format
false if not.

bool drm\_mode\_is\_420\_also(const struct [drm\_display\_info](#c.drm_display_info "drm_display_info") \*display, const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode)
:   if a given videomode can be supported in YCBCR420 output format also (along with RGB/YCBCR444/422)

**Parameters**

`const struct drm_display_info *display`
:   display under action.

`const struct drm_display_mode *mode`
:   video mode to be tested.

**Return**

true if the mode can be support YCBCR420 format
false if not.

bool drm\_mode\_is\_420(const struct [drm\_display\_info](#c.drm_display_info "drm_display_info") \*display, const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode)
:   if a given videomode can be supported in YCBCR420 output format

**Parameters**

`const struct drm_display_info *display`
:   display under action.

`const struct drm_display_mode *mode`
:   video mode to be tested.

**Return**

true if the mode can be supported in YCBCR420 format
false if not.

void drm\_set\_preferred\_mode(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, int hpref, int vpref)
:   Sets the preferred mode of a connector

**Parameters**

`struct drm_connector *connector`
:   connector whose mode list should be processed

`int hpref`
:   horizontal resolution of preferred mode

`int vpref`
:   vertical resolution of preferred mode

**Description**

Marks a mode as preferred if it matches the resolution specified by **hpref**
and **vpref**.

## Connector Abstraction

In DRM connectors are the general abstraction for display sinks, and include
also fixed panels or anything else that can display pixels in some form. As
opposed to all other KMS objects representing hardware (like CRTC, encoder or
plane abstractions) connectors can be hotplugged and unplugged at runtime.
Hence they are reference-counted using [`drm_connector_get()`](#c.drm_connector_get "drm_connector_get") and
[`drm_connector_put()`](#c.drm_connector_put "drm_connector_put").

KMS driver must create, initialize, register and attach at a [`struct
drm_connector`](#c.drm_connector "drm_connector") for each such sink. The instance is created as other KMS
objects and initialized by setting the following fields. The connector is
initialized with a call to [`drm_connector_init()`](#c.drm_connector_init "drm_connector_init") with a pointer to the
[`struct drm_connector_funcs`](#c.drm_connector_funcs "drm_connector_funcs") and a connector type, and then exposed to
userspace with a call to [`drm_connector_register()`](#c.drm_connector_register "drm_connector_register").

Connectors must be attached to an encoder to be used. For devices that map
connectors to encoders 1:1, the connector should be attached at
initialization time with a call to [`drm_connector_attach_encoder()`](#c.drm_connector_attach_encoder "drm_connector_attach_encoder"). The
driver must also set the [`drm_connector.encoder`](#c.drm_connector "drm_connector") field to point to the
attached encoder.

For connectors which are not fixed (like built-in panels) the driver needs to
support hotplug notifications. The simplest way to do that is by using the
probe helpers, see [`drm_kms_helper_poll_init()`](drm-kms-helpers.html#c.drm_kms_helper_poll_init "drm_kms_helper_poll_init") for connectors which don’t have
hardware support for hotplug interrupts. Connectors with hardware hotplug
support can instead use e.g. [`drm_helper_hpd_irq_event()`](drm-kms-helpers.html#c.drm_helper_hpd_irq_event "drm_helper_hpd_irq_event").

### Connector Functions Reference

enum drm\_connector\_status
:   status for a [`drm_connector`](#c.drm_connector "drm_connector")

**Constants**

`connector_status_connected`
:   The connector is definitely connected to
    a sink device, and can be enabled.

`connector_status_disconnected`
:   The connector isn’t connected to a
    sink device which can be autodetect. For digital outputs like DP or
    HDMI (which can be realiable probed) this means there’s really
    nothing there. It is driver-dependent whether a connector with this
    status can be lit up or not.

`connector_status_unknown`
:   The connector’s status could not be
    reliably detected. This happens when probing would either cause
    flicker (like load-detection when the connector is in use), or when a
    hardware resource isn’t available (like when load-detection needs a
    free CRTC). It should be possible to light up the connector with one
    of the listed fallback modes. For default configuration userspace
    should only try to light up connectors with unknown status when
    there’s not connector with **connector\_status\_connected**.

**Description**

This `enum is` used to track the connector status. There are no separate
#defines for the uapi!

enum drm\_connector\_registration\_state
:   userspace registration status for a [`drm_connector`](#c.drm_connector "drm_connector")

**Constants**

`DRM_CONNECTOR_INITIALIZING`
:   The connector has just been created,
    but has yet to be exposed to userspace. There should be no
    additional restrictions to how the state of this connector may be
    modified.

`DRM_CONNECTOR_REGISTERED`
:   The connector has been fully initialized
    and registered with sysfs, as such it has been exposed to
    userspace. There should be no additional restrictions to how the
    state of this connector may be modified.

`DRM_CONNECTOR_UNREGISTERED`
:   The connector has either been exposed
    to userspace and has since been unregistered and removed from
    userspace, or the connector was unregistered before it had a chance
    to be exposed to userspace (e.g. still in the
    **DRM\_CONNECTOR\_INITIALIZING** state). When a connector is
    unregistered, there are additional restrictions to how its state
    may be modified:

    * An unregistered connector may only have its DPMS changed from
      On->Off. Once DPMS is changed to Off, it may not be switched back
      to On.
    * Modesets are not allowed on unregistered connectors, unless they
      would result in disabling its assigned CRTCs. This means
      disabling a CRTC on an unregistered connector is OK, but enabling
      one is not.
    * Removing a CRTC from an unregistered connector is OK, but new
      CRTCs may never be assigned to an unregistered connector.

**Description**

This `enum is` used to track the status of initializing a connector and
registering it with userspace, so that DRM can prevent bogus modesets on
connectors that no longer exist.

enum drm\_connector\_tv\_mode
:   Analog TV output mode

**Constants**

`DRM_MODE_TV_MODE_NTSC`
:   CCIR System M (aka 525-lines)
    together with the NTSC Color Encoding.

`DRM_MODE_TV_MODE_NTSC_443`
:   Variant of
    **DRM\_MODE\_TV\_MODE\_NTSC**. Uses a color subcarrier frequency
    of 4.43 MHz.

`DRM_MODE_TV_MODE_NTSC_J`
:   Variant of **DRM\_MODE\_TV\_MODE\_NTSC**
    used in Japan. Uses a black level equals to the blanking
    level.

`DRM_MODE_TV_MODE_PAL`
:   CCIR System B together with the PAL
    color system.

`DRM_MODE_TV_MODE_PAL_M`
:   CCIR System M (aka 525-lines)
    together with the PAL color encoding

`DRM_MODE_TV_MODE_PAL_N`
:   CCIR System N together with the PAL
    color encoding. It uses 625 lines, but has a color subcarrier
    frequency of 3.58MHz, the SECAM color space, and narrower
    channels compared to most of the other PAL variants.

`DRM_MODE_TV_MODE_SECAM`
:   CCIR System B together with the
    SECAM color system.

`DRM_MODE_TV_MODE_MONOCHROME`
:   Use timings appropriate to
    the DRM mode, including equalizing pulses for a 525-line
    or 625-line mode, with no pedestal or color encoding.

`DRM_MODE_TV_MODE_MAX`
:   Number of analog TV output modes.

    Internal implementation detail; this is not uABI.

**Description**

This `enum is` used to indicate the TV output mode used on an analog TV
connector.

WARNING: The values of this `enum is` uABI since they’re exposed in the
“TV mode” connector property.

struct drm\_scrambling
:   sink’s scrambling support.

**Definition**:

```
struct drm_scrambling {
    bool supported;
    bool low_rates;
};
```

**Members**

`supported`
:   scrambling supported for rates > 340 Mhz.

`low_rates`
:   scrambling supported for rates <= 340 Mhz.

struct drm\_hdmi\_dsc\_cap
:   DSC capabilities of HDMI sink

**Definition**:

```
struct drm_hdmi_dsc_cap {
    bool v_1p2;
    bool native_420;
    bool all_bpp;
    u8 bpc_supported;
    u8 max_slices;
    int clk_per_slice;
    u8 max_lanes;
    u8 max_frl_rate_per_lane;
    u8 total_chunk_kbytes;
};
```

**Members**

`v_1p2`
:   flag for dsc1.2 version support by sink

`native_420`
:   Does sink support DSC with 4:2:0 compression

`all_bpp`
:   Does sink support all bpp with 4:4:4: or 4:2:2
    compressed formats

`bpc_supported`
:   compressed bpc supported by sink : 10, 12 or 16 bpc

`max_slices`
:   maximum number of Horizontal slices supported by

`clk_per_slice`
:   max pixel clock in MHz supported per slice

`max_lanes`
:   dsc max lanes supported for Fixed rate Link training

`max_frl_rate_per_lane`
:   maximum frl rate with DSC per lane

`total_chunk_kbytes`
:   max size of chunks in KBs supported per line

**Description**

Describes the DSC support provided by HDMI 2.1 sink.
The information is fetched fom additional HFVSDB blocks defined
for HDMI 2.1.

struct drm\_hdmi\_info
:   runtime information about the connected HDMI sink

**Definition**:

```
struct drm_hdmi_info {
    struct drm_scdc scdc;
    unsigned long y420_vdb_modes[BITS_TO_LONGS(256)];
    unsigned long y420_cmdb_modes[BITS_TO_LONGS(256)];
    u8 y420_dc_modes;
    u8 max_frl_rate_per_lane;
    u8 max_lanes;
    struct drm_hdmi_dsc_cap dsc_cap;
};
```

**Members**

`scdc`
:   sink’s scdc support and capabilities

`y420_vdb_modes`
:   bitmap of modes which can support ycbcr420
    output only (not normal RGB/YCBCR444/422 outputs). The max VIC
    defined by the CEA-861-G spec is 219, so the size is 256 bits to map
    up to 256 VICs.

`y420_cmdb_modes`
:   bitmap of modes which can support ycbcr420
    output also, along with normal HDMI outputs. The max VIC defined by
    the CEA-861-G spec is 219, so the size is 256 bits to map up to 256
    VICs.

`y420_dc_modes`
:   bitmap of deep color support index

`max_frl_rate_per_lane`
:   support fixed rate link

`max_lanes`
:   supported by sink

`dsc_cap`
:   DSC capabilities of the sink

**Description**

Describes if a given display supports advanced HDMI 2.0 features.
This information is available in CEA-861-F extension blocks (like HF-VSDB).

enum drm\_link\_status
:   connector’s link\_status property value

**Constants**

`DRM_LINK_STATUS_GOOD`
:   DP Link is Good as a result of successful
    link training

`DRM_LINK_STATUS_BAD`
:   DP Link is BAD as a result of link training
    failure

**Description**

This `enum is` used as the connector’s link status property value.
It is set to the values defined in uapi.

enum drm\_panel\_orientation
:   panel\_orientation info for [`drm_display_info`](#c.drm_display_info "drm_display_info")

**Constants**

`DRM_MODE_PANEL_ORIENTATION_UNKNOWN`
:   The drm driver has not provided any
    panel orientation information (normal
    for non panels) in this case the “panel
    orientation” connector prop will not be
    attached.

`DRM_MODE_PANEL_ORIENTATION_NORMAL`
:   The top side of the panel matches the
    top side of the device’s casing.

`DRM_MODE_PANEL_ORIENTATION_BOTTOM_UP`
:   The top side of the panel matches the
    bottom side of the device’s casing, iow
    the panel is mounted upside-down.

`DRM_MODE_PANEL_ORIENTATION_LEFT_UP`
:   The left side of the panel matches the
    top side of the device’s casing.

`DRM_MODE_PANEL_ORIENTATION_RIGHT_UP`
:   The right side of the panel matches the
    top side of the device’s casing.

**Description**

This `enum is` used to track the (LCD) panel orientation. There are no
separate #defines for the uapi!

enum drm\_hdmi\_broadcast\_rgb
:   Broadcast RGB Selection for an HDMI **drm\_connector**

**Constants**

`DRM_HDMI_BROADCAST_RGB_AUTO`
:   The RGB range is selected
    automatically based on the mode.

`DRM_HDMI_BROADCAST_RGB_FULL`
:   Full range RGB is forced.

`DRM_HDMI_BROADCAST_RGB_LIMITED`
:   Limited range RGB is forced.

struct drm\_monitor\_range\_info
:   Panel’s Monitor range in EDID for [`drm_display_info`](#c.drm_display_info "drm_display_info")

**Definition**:

```
struct drm_monitor_range_info {
    u16 min_vfreq;
    u16 max_vfreq;
};
```

**Members**

`min_vfreq`
:   This is the min supported refresh rate in Hz from
    EDID’s detailed monitor range.

`max_vfreq`
:   This is the max supported refresh rate in Hz from
    EDID’s detailed monitor range

**Description**

This `struct is` used to store a frequency range supported by panel
as parsed from EDID’s detailed monitor range descriptor block.

struct drm\_luminance\_range\_info
:   Panel’s luminance range for [`drm_display_info`](#c.drm_display_info "drm_display_info"). Calculated using data in EDID

**Definition**:

```
struct drm_luminance_range_info {
    u32 min_luminance;
    u32 max_luminance;
};
```

**Members**

`min_luminance`
:   This is the min supported luminance value

`max_luminance`
:   This is the max supported luminance value

**Description**

This `struct is` used to store a luminance range supported by panel
as calculated using data from EDID’s static hdr metadata.

enum drm\_privacy\_screen\_status
:   privacy screen status

**Constants**

`PRIVACY_SCREEN_DISABLED`
:   The privacy-screen on the panel is disabled

`PRIVACY_SCREEN_ENABLED`
:   The privacy-screen on the panel is enabled

`PRIVACY_SCREEN_DISABLED_LOCKED`
:   The privacy-screen on the panel is disabled and locked (cannot be changed)

`PRIVACY_SCREEN_ENABLED_LOCKED`
:   The privacy-screen on the panel is enabled and locked (cannot be changed)

**Description**

This `enum is` used to track and control the state of the integrated privacy
screen present on some display panels, via the “privacy-screen sw-state”
and “privacy-screen hw-state” properties. Note the \_LOCKED `enum values`
are only valid for the “privacy-screen hw-state” property.

enum drm\_colorspace
:   color space

**Constants**

`DRM_MODE_COLORIMETRY_DEFAULT`
:   Driver specific behavior.

`DRM_MODE_COLORIMETRY_NO_DATA`
:   Driver specific behavior.

`DRM_MODE_COLORIMETRY_SMPTE_170M_YCC`
:   (HDMI)
    SMPTE ST 170M colorimetry format

`DRM_MODE_COLORIMETRY_BT709_YCC`
:   (HDMI, DP)
    ITU-R BT.709 colorimetry format

`DRM_MODE_COLORIMETRY_XVYCC_601`
:   (HDMI, DP)
    xvYCC601 colorimetry format

`DRM_MODE_COLORIMETRY_XVYCC_709`
:   (HDMI, DP)
    xvYCC709 colorimetry format

`DRM_MODE_COLORIMETRY_SYCC_601`
:   (HDMI, DP)
    sYCC601 colorimetry format

`DRM_MODE_COLORIMETRY_OPYCC_601`
:   (HDMI, DP)
    opYCC601 colorimetry format

`DRM_MODE_COLORIMETRY_OPRGB`
:   (HDMI, DP)
    opRGB colorimetry format

`DRM_MODE_COLORIMETRY_BT2020_CYCC`
:   (HDMI, DP)
    ITU-R BT.2020 Y’c C’bc C’rc (constant luminance) colorimetry format

`DRM_MODE_COLORIMETRY_BT2020_RGB`
:   (HDMI, DP)
    ITU-R BT.2020 R’ G’ B’ colorimetry format

`DRM_MODE_COLORIMETRY_BT2020_YCC`
:   (HDMI, DP)
    ITU-R BT.2020 Y’ C’b C’r colorimetry format

`DRM_MODE_COLORIMETRY_DCI_P3_RGB_D65`
:   (HDMI)
    SMPTE ST 2113 P3D65 colorimetry format

`DRM_MODE_COLORIMETRY_DCI_P3_RGB_THEATER`
:   (HDMI)
    SMPTE ST 2113 P3DCI colorimetry format

`DRM_MODE_COLORIMETRY_RGB_WIDE_FIXED`
:   (DP)
    RGB wide gamut fixed point colorimetry format

`DRM_MODE_COLORIMETRY_RGB_WIDE_FLOAT`
:   (DP)
    RGB wide gamut floating point
    (scRGB (IEC 61966-2-2)) colorimetry format

`DRM_MODE_COLORIMETRY_BT601_YCC`
:   (DP)
    ITU-R BT.601 colorimetry format
    The DP spec does not say whether this is the 525 or the 625
    line version.

`DRM_MODE_COLORIMETRY_COUNT`
:   Not a valid value; merely used four counting

**Description**

This `enum is` a consolidated colorimetry list supported by HDMI and
DP protocol standard. The respective connectors will register
a property with the subset of this list (supported by that
respective protocol). Userspace will set the colorspace through
a colorspace property which will be created and exposed to
userspace.

DP definitions come from the DP v2.0 spec
HDMI definitions come from the CTA-861-H spec

enum drm\_output\_color\_format
:   Output Color Format

**Constants**

`DRM_OUTPUT_COLOR_FORMAT_RGB444`
:   RGB output format

`DRM_OUTPUT_COLOR_FORMAT_YCBCR444`
:   YCbCr 4:4:4 output format (ie. not subsampled)

`DRM_OUTPUT_COLOR_FORMAT_YCBCR422`
:   YCbCr 4:2:2 output format (ie. with horizontal subsampling)

`DRM_OUTPUT_COLOR_FORMAT_YCBCR420`
:   YCbCr 4:2:0 output format (ie. with horizontal and vertical subsampling)

**Description**

This `enum is` a consolidated color format list supported by
connectors. It’s only ever really been used for HDMI and DP so far,
so it’s not exhaustive and can be extended to represent other formats
in the future.

enum drm\_bus\_flags
:   bus\_flags info for [`drm_display_info`](#c.drm_display_info "drm_display_info")

**Constants**

`DRM_BUS_FLAG_DE_LOW`
:   The Data Enable signal is active low

`DRM_BUS_FLAG_DE_HIGH`
:   The Data Enable signal is active high

`DRM_BUS_FLAG_PIXDATA_DRIVE_POSEDGE`
:   Data is driven on the rising edge of the pixel clock

`DRM_BUS_FLAG_PIXDATA_DRIVE_NEGEDGE`
:   Data is driven on the falling edge of the pixel clock

`DRM_BUS_FLAG_PIXDATA_SAMPLE_POSEDGE`
:   Data is sampled on the rising edge of the pixel clock

`DRM_BUS_FLAG_PIXDATA_SAMPLE_NEGEDGE`
:   Data is sampled on the falling edge of the pixel clock

`DRM_BUS_FLAG_DATA_MSB_TO_LSB`
:   Data is transmitted MSB to LSB on the bus

`DRM_BUS_FLAG_DATA_LSB_TO_MSB`
:   Data is transmitted LSB to MSB on the bus

`DRM_BUS_FLAG_SYNC_DRIVE_POSEDGE`
:   Sync signals are driven on the rising edge of the pixel clock

`DRM_BUS_FLAG_SYNC_DRIVE_NEGEDGE`
:   Sync signals are driven on the falling edge of the pixel clock

`DRM_BUS_FLAG_SYNC_SAMPLE_POSEDGE`
:   Sync signals are sampled on the rising edge of the pixel clock

`DRM_BUS_FLAG_SYNC_SAMPLE_NEGEDGE`
:   Sync signals are sampled on the falling edge of the pixel clock

`DRM_BUS_FLAG_SHARP_SIGNALS`
:   Set if the Sharp-specific signals (SPL, CLS, PS, REV) must be used

**Description**

This `enum defines` signal polarities and clock edge information for signals on
a bus as bitmask flags.

The clock edge information is conveyed by two sets of symbols,
DRM\_BUS\_FLAGS\_\*\_DRIVE\_\* and DRM\_BUS\_FLAGS\_\*\_SAMPLE\_\*. When this `enum is`
used to describe a bus from the point of view of the transmitter, the
\*\_DRIVE\_\* flags should be used. When used from the point of view of the
receiver, the \*\_SAMPLE\_\* flags should be used. The \*\_DRIVE\_\* and
\*\_SAMPLE\_\* flags alias each other, with the \*\_SAMPLE\_POSEDGE and
\*\_SAMPLE\_NEGEDGE flags being equal to \*\_DRIVE\_NEGEDGE and \*\_DRIVE\_POSEDGE
respectively. This simplifies code as signals are usually sampled on the
opposite edge of the driving edge. Transmitters and receivers may however
need to take other signal timings into account to convert between driving
and sample edges.

struct drm\_amd\_vsdb\_info
:   AMD-specific VSDB information

**Definition**:

```
struct drm_amd_vsdb_info {
    u8 version;
    bool replay_mode;
    u8 panel_type;
    struct drm_luminance_range_info luminance_range1;
    struct drm_luminance_range_info luminance_range2;
};
```

**Members**

`version`
:   Version of the Vendor-Specific Data Block (VSDB)

`replay_mode`
:   Panel Replay supported

`panel_type`
:   Panel technology type

`luminance_range1`
:   Luminance for max back light

`luminance_range2`
:   Luminance for min back light

**Description**

This structure holds information parsed from the AMD Vendor-Specific Data
Block (VSDB) version 3.

struct drm\_display\_info
:   runtime data about the connected sink

**Definition**:

```
struct drm_display_info {
    unsigned int width_mm;
    unsigned int height_mm;
    unsigned int bpc;
    enum subpixel_order subpixel_order;
    int panel_orientation;
    u32 color_formats;
    const u32 *bus_formats;
    unsigned int num_bus_formats;
    u32 bus_flags;
    int max_tmds_clock;
    bool dvi_dual;
    bool is_hdmi;
    bool has_audio;
    bool has_hdmi_infoframe;
    bool rgb_quant_range_selectable;
    u8 edid_hdmi_rgb444_dc_modes;
    u8 edid_hdmi_ycbcr444_dc_modes;
    u8 cea_rev;
    struct drm_hdmi_info hdmi;
    struct hdr_sink_metadata hdr_sink_metadata;
    bool non_desktop;
    struct drm_monitor_range_info monitor_range;
    struct drm_luminance_range_info luminance_range;
    u8 mso_stream_count;
    u8 mso_pixel_overlap;
    u32 max_dsc_bpp;
    u8 *vics;
    int vics_len;
    u32 quirks;
    u16 source_physical_address;
    struct drm_amd_vsdb_info amd_vsdb;
};
```

**Members**

`width_mm`
:   Physical width in mm.

`height_mm`
:   Physical height in mm.

`bpc`
:   Maximum bits per color channel. Used by HDMI and DP outputs.

`subpixel_order`
:   Subpixel order of LCD panels.

`panel_orientation`
:   Read only connector property for built-in panels,
    indicating the orientation of the panel vs the device’s casing.
    [`drm_connector_init()`](#c.drm_connector_init "drm_connector_init") sets this to DRM\_MODE\_PANEL\_ORIENTATION\_UNKNOWN.
    When not UNKNOWN this gets used by the drm\_fb\_helpers to rotate the
    fb to compensate and gets exported as prop to userspace.

`color_formats`
:   HDMI Color formats, selects between RGB and
    YCbCr modes. Uses a bitmask of DRM\_OUTPUT\_COLOR\_FORMAT\_
    defines, which are \_not\_ the same ones as used to describe
    the pixel format in framebuffers, and also don’t match the
    formats in **bus\_formats** which are shared with v4l.

`bus_formats`
:   Pixel data format on the wire, somewhat redundant with
    **color\_formats**. Array of size **num\_bus\_formats** encoded using
    MEDIA\_BUS\_FMT\_ defines shared with v4l and media drivers.

`num_bus_formats`
:   Size of **bus\_formats** array.

`bus_flags`
:   Additional information (like pixel signal polarity) for
    the pixel data on the bus, using [`enum drm_bus_flags`](#c.drm_bus_flags "drm_bus_flags") values
    DRM\_BUS\_FLAGS\_.

`max_tmds_clock`
:   Maximum TMDS clock rate supported by the
    sink in kHz. 0 means undefined.

`dvi_dual`
:   Dual-link DVI sink?

`is_hdmi`
:   True if the sink is an HDMI device.

    This field shall be used instead of calling
    [`drm_detect_hdmi_monitor()`](drm-kms-helpers.html#c.drm_detect_hdmi_monitor "drm_detect_hdmi_monitor") when possible.

`has_audio`
:   True if the sink supports audio.

    This field shall be used instead of calling
    [`drm_detect_monitor_audio()`](drm-kms-helpers.html#c.drm_detect_monitor_audio "drm_detect_monitor_audio") when possible.

`has_hdmi_infoframe`
:   Does the sink support the HDMI infoframe?

`rgb_quant_range_selectable`
:   Does the sink support selecting
    the RGB quantization range?

`edid_hdmi_rgb444_dc_modes`
:   Mask of supported hdmi deep color modes
    in RGB 4:4:4. Even more stuff redundant with **bus\_formats**.

`edid_hdmi_ycbcr444_dc_modes`
:   Mask of supported hdmi deep color
    modes in YCbCr 4:4:4. Even more stuff redundant with **bus\_formats**.

`cea_rev`
:   CEA revision of the HDMI sink.

`hdmi`
:   advance features of a HDMI sink.

`hdr_sink_metadata`
:   HDR Metadata Information read from sink

`non_desktop`
:   Non desktop display (HMD).

`monitor_range`
:   Frequency range supported by monitor range descriptor

`luminance_range`
:   Luminance range supported by panel

`mso_stream_count`
:   eDP Multi-SST Operation (MSO) stream count from
    the DisplayID VESA vendor block. 0 for conventional Single-Stream
    Transport (SST), or 2 or 4 MSO streams.

`mso_pixel_overlap`
:   eDP MSO segment pixel overlap, 0-8 pixels.

`max_dsc_bpp`
:   Maximum DSC target bitrate, if it is set to 0 the
    monitor’s default value is used instead.

`vics`
:   Array of vics\_len VICs. Internal to EDID parsing.

`vics_len`
:   Number of elements in vics. Internal to EDID parsing.

`quirks`
:   EDID based quirks. DRM core and drivers can query the
    **drm\_edid\_quirk** quirks using `drm_edid_has_quirk()`, the rest of
    the quirks also tracked here are internal to EDID parsing.

`source_physical_address`
:   Source Physical Address from HDMI
    Vendor-Specific Data Block, for CEC usage.

    Defaults to CEC\_PHYS\_ADDR\_INVALID (0xffff).

`amd_vsdb`
:   AMD-specific VSDB information.

**Description**

Describes a given display (e.g. CRT or flat panel) and its limitations. For
fixed display sinks like built-in panels there’s not much difference between
this and [`struct drm_connector`](#c.drm_connector "drm_connector"). But for sinks with a real cable this
structure is meant to describe all the things at the other end of the cable.

For sinks which provide an EDID this can be filled out by calling
[`drm_add_edid_modes()`](drm-kms-helpers.html#c.drm_add_edid_modes "drm_add_edid_modes").

struct drm\_connector\_tv\_margins
:   TV connector related margins

**Definition**:

```
struct drm_connector_tv_margins {
    unsigned int bottom;
    unsigned int left;
    unsigned int right;
    unsigned int top;
};
```

**Members**

`bottom`
:   Bottom margin in pixels.

`left`
:   Left margin in pixels.

`right`
:   Right margin in pixels.

`top`
:   Top margin in pixels.

**Description**

Describes the margins in pixels to put around the image on TV
connectors to deal with overscan.

struct drm\_tv\_connector\_state
:   TV connector related states

**Definition**:

```
struct drm_tv_connector_state {
    enum drm_mode_subconnector select_subconnector;
    enum drm_mode_subconnector subconnector;
    struct drm_connector_tv_margins margins;
    unsigned int legacy_mode;
    unsigned int mode;
    unsigned int brightness;
    unsigned int contrast;
    unsigned int flicker_reduction;
    unsigned int overscan;
    unsigned int saturation;
    unsigned int hue;
};
```

**Members**

`select_subconnector`
:   selected subconnector

`subconnector`
:   detected subconnector

`margins`
:   TV margins

`legacy_mode`
:   Legacy TV mode, driver specific value

`mode`
:   TV mode

`brightness`
:   brightness in percent

`contrast`
:   contrast in percent

`flicker_reduction`
:   flicker reduction in percent

`overscan`
:   overscan in percent

`saturation`
:   saturation in percent

`hue`
:   hue in percent

struct drm\_connector\_hdmi\_infoframe
:   HDMI Infoframe container

**Definition**:

```
struct drm_connector_hdmi_infoframe {
    union hdmi_infoframe data;
    bool set;
};
```

**Members**

`data`
:   HDMI Infoframe structure

`set`
:   Is the content of **data** valid?

struct drm\_connector\_state
:   mutable connector state

**Definition**:

```
struct drm_connector_state {
    struct drm_connector *connector;
    struct drm_crtc *crtc;
    struct drm_encoder *best_encoder;
    enum drm_link_status link_status;
    struct drm_atomic_state *state;
    struct drm_crtc_commit *commit;
    struct drm_tv_connector_state tv;
    bool self_refresh_aware;
    enum hdmi_picture_aspect picture_aspect_ratio;
    unsigned int content_type;
    unsigned int hdcp_content_type;
    unsigned int scaling_mode;
    unsigned int content_protection;
    enum drm_colorspace colorspace;
    struct drm_writeback_job *writeback_job;
    u8 max_requested_bpc;
    u8 max_bpc;
    enum drm_privacy_screen_status privacy_screen_sw_state;
    struct drm_property_blob *hdr_output_metadata;
    struct drm_connector_hdmi_state hdmi;
};
```

**Members**

`connector`
:   backpointer to the connector

`crtc`
:   CRTC to connect connector to, NULL if disabled.

    Do not change this directly, use [`drm_atomic_set_crtc_for_connector()`](#c.drm_atomic_set_crtc_for_connector "drm_atomic_set_crtc_for_connector")
    instead.

`best_encoder`
:   Used by the atomic helpers to select the encoder, through the
    [`drm_connector_helper_funcs.atomic_best_encoder`](drm-kms-helpers.html#c.drm_connector_helper_funcs "drm_connector_helper_funcs") or
    [`drm_connector_helper_funcs.best_encoder`](drm-kms-helpers.html#c.drm_connector_helper_funcs "drm_connector_helper_funcs") callbacks.

    This is also used in the atomic helpers to map encoders to their
    current and previous connectors, see
    [`drm_atomic_get_old_connector_for_encoder()`](#c.drm_atomic_get_old_connector_for_encoder "drm_atomic_get_old_connector_for_encoder") and
    [`drm_atomic_get_new_connector_for_encoder()`](#c.drm_atomic_get_new_connector_for_encoder "drm_atomic_get_new_connector_for_encoder").

    NOTE: Atomic drivers must fill this out (either themselves or through
    helpers), for otherwise the GETCONNECTOR and GETENCODER IOCTLs will
    not return correct data to userspace.

`link_status`
:   Connector link\_status to keep track of whether link is
    GOOD or BAD to notify userspace if retraining is necessary.

`state`
:   backpointer to global drm\_atomic\_state

`commit`
:   Tracks the pending commit to prevent use-after-free conditions.

    Is only set when **crtc** is NULL.

`tv`
:   TV connector state

`self_refresh_aware`
:   This tracks whether a connector is aware of the self refresh state.
    It should be set to true for those connector implementations which
    understand the self refresh state. This is needed since the crtc
    registers the self refresh helpers and it doesn’t know if the
    connectors downstream have implemented self refresh entry/exit.

    Drivers should set this to true in atomic\_check if they know how to
    handle self\_refresh requests.

`picture_aspect_ratio`
:   Connector property to control the
    HDMI infoframe aspect ratio setting.

    The `DRM_MODE_PICTURE_ASPECT_`\* values much match the
    values for `enum hdmi_picture_aspect`

`content_type`
:   Connector property to control the
    HDMI infoframe content type setting.
    The `DRM_MODE_CONTENT_TYPE_`\* values much
    match the values.

`hdcp_content_type`
:   Connector property to pass the type of
    protected content. This is most commonly used for HDCP.

`scaling_mode`
:   Connector property to control the
    upscaling, mostly used for built-in panels.

`content_protection`
:   Connector property to request content
    protection. This is most commonly used for HDCP.

`colorspace`
:   State variable for Connector property to request
    colorspace change on Sink. This is most commonly used to switch
    to wider color gamuts like BT2020.

`writeback_job`
:   Writeback job for writeback connectors

    Holds the framebuffer and out-fence for a writeback connector. As
    the writeback completion may be asynchronous to the normal commit
    cycle, the writeback job lifetime is managed separately from the
    normal atomic state by this object.

    See also: [`drm_writeback_queue_job()`](#c.drm_writeback_queue_job "drm_writeback_queue_job") and
    [`drm_writeback_signal_completion()`](#c.drm_writeback_signal_completion "drm_writeback_signal_completion")

`max_requested_bpc`
:   Connector property to limit the maximum bit
    depth of the pixels.

`max_bpc`
:   Connector max\_bpc based on the requested max\_bpc property
    and the connector bpc limitations obtained from edid.

`privacy_screen_sw_state`
:   See [Standard Connector
    Properties](#standard-connector-properties)

`hdr_output_metadata`
:   DRM blob property for HDR output metadata

`hdmi`
:   HDMI-related variable and properties. Filled by
    **[`drm_atomic_helper_connector_hdmi_check()`](drm-kms-helpers.html#c.drm_atomic_helper_connector_hdmi_check "drm_atomic_helper_connector_hdmi_check")**.

struct drm\_connector\_cec\_funcs
:   drm\_hdmi\_connector control functions

**Definition**:

```
struct drm_connector_cec_funcs {
    void (*phys_addr_invalidate)(struct drm_connector *connector);
    void (*phys_addr_set)(struct drm_connector *connector, u16 addr);
};
```

**Members**

`phys_addr_invalidate`
:   mark CEC physical address as invalid

    The callback to mark CEC physical address as invalid, abstracting
    the operation.

`phys_addr_set`
:   set CEC physical address

    The callback to set CEC physical address, abstracting the operation.

struct drm\_connector\_infoframe\_funcs
:   InfoFrame-related functions

**Definition**:

```
struct drm_connector_infoframe_funcs {
    int (*clear_infoframe)(struct drm_connector *connector);
    int (*write_infoframe)(struct drm_connector *connector, const u8 *buffer, size_t len);
};
```

**Members**

`clear_infoframe`
:   This callback is invoked through
    **drm\_atomic\_helper\_connector\_hdmi\_update\_infoframes** during a
    commit to clear the infoframes into the hardware. It will be
    called once for each frame type to be disabled.

    The **clear\_infoframe** callback is mandatory for AVI and HDMI-VS
    InfoFrame types.

    Returns:
    0 on success, a negative error code otherwise

`write_infoframe`
:   This callback is invoked through
    **drm\_atomic\_helper\_connector\_hdmi\_update\_infoframes** during a
    commit to program the infoframes into the hardware. It will
    be called for every updated infoframe type.

    The **write\_infoframe** callback is mandatory for AVI and HDMI-VS
    InfoFrame types.

    Returns:
    0 on success, a negative error code otherwise

struct drm\_connector\_hdmi\_funcs
:   drm\_hdmi\_connector control functions

**Definition**:

```
struct drm_connector_hdmi_funcs {
    enum drm_mode_status (*tmds_char_rate_valid)(const struct drm_connector *connector, const struct drm_display_mode *mode, unsigned long long tmds_rate);
    const struct drm_edid *(*read_edid)(struct drm_connector *connector);
    struct drm_connector_infoframe_funcs avi;
    struct drm_connector_infoframe_funcs hdmi;
    struct drm_connector_infoframe_funcs audio;
    struct drm_connector_infoframe_funcs hdr_drm;
    struct drm_connector_infoframe_funcs spd;
};
```

**Members**

`tmds_char_rate_valid`
:   This callback is invoked at atomic\_check time to figure out
    whether a particular TMDS character rate is supported by the
    driver.

    The **tmds\_char\_rate\_valid** callback is optional.

    Returns:

    Either [`drm_mode_status.MODE_OK`](#c.drm_mode_status "drm_mode_status") or one of the failure reasons
    in [`enum drm_mode_status`](#c.drm_mode_status "drm_mode_status").

`read_edid`
:   This callback is used by the framework as a replacement for reading
    the EDID from connector->ddc. It is still recommended to provide
    connector->ddc instead of implementing this callback. Returned EDID
    should be freed via the [`drm_edid_free()`](drm-kms-helpers.html#c.drm_edid_free "drm_edid_free").

    The **read\_edid** callback is optional.

    Returns:
    Valid EDID on success, NULL in case of failure.

`avi`
:   Set of callbacks for handling the AVI InfoFrame. These callbacks are
    mandatory.

`hdmi`
:   Set of callbacks for handling the HDMI Vendor-Specific InfoFrame.
    These callbacks are mandatory.

`audio`
:   Set of callbacks for handling the Audio InfoFrame. These callbacks
    are optional, but they are required for drivers which use
    [`drm_atomic_helper_connector_hdmi_update_audio_infoframe()`](drm-kms-helpers.html#c.drm_atomic_helper_connector_hdmi_update_audio_infoframe "drm_atomic_helper_connector_hdmi_update_audio_infoframe").

`hdr_drm`
:   Set of callbacks for handling the HDR DRM InfoFrame. These callbacks
    are mandatory if HDR output is to be supported.

`spd`
:   Set of callbacks for handling the SPD InfoFrame. These callbacks are
    optional.

struct drm\_connector\_funcs
:   control connectors on a given device

**Definition**:

```
struct drm_connector_funcs {
    int (*dpms)(struct drm_connector *connector, int mode);
    void (*reset)(struct drm_connector *connector);
    enum drm_connector_status (*detect)(struct drm_connector *connector, bool force);
    void (*force)(struct drm_connector *connector);
    int (*fill_modes)(struct drm_connector *connector, uint32_t max_width, uint32_t max_height);
    int (*set_property)(struct drm_connector *connector, struct drm_property *property, uint64_t val);
    int (*late_register)(struct drm_connector *connector);
    void (*early_unregister)(struct drm_connector *connector);
    void (*destroy)(struct drm_connector *connector);
    struct drm_connector_state *(*atomic_duplicate_state)(struct drm_connector *connector);
    void (*atomic_destroy_state)(struct drm_connector *connector, struct drm_connector_state *state);
    int (*atomic_set_property)(struct drm_connector *connector, struct drm_connector_state *state, struct drm_property *property, uint64_t val);
    int (*atomic_get_property)(struct drm_connector *connector, const struct drm_connector_state *state, struct drm_property *property, uint64_t *val);
    void (*atomic_print_state)(struct drm_printer *p, const struct drm_connector_state *state);
    void (*oob_hotplug_event)(struct drm_connector *connector, enum drm_connector_status status);
    void (*debugfs_init)(struct drm_connector *connector, struct dentry *root);
};
```

**Members**

`dpms`
:   Legacy entry point to set the per-connector DPMS state. Legacy DPMS
    is exposed as a standard property on the connector, but diverted to
    this callback in the drm core. Note that atomic drivers don’t
    implement the 4 level DPMS support on the connector any more, but
    instead only have an on/off “ACTIVE” property on the CRTC object.

    This hook is not used by atomic drivers, remapping of the legacy DPMS
    property is entirely handled in the DRM core.

    RETURNS:

    0 on success or a negative error code on failure.

`reset`
:   Reset connector hardware and software state to off. This function isn’t
    called by the core directly, only through [`drm_mode_config_reset()`](#c.drm_mode_config_reset "drm_mode_config_reset").
    It’s not a helper hook only for historical reasons.

    Atomic drivers can use [`drm_atomic_helper_connector_reset()`](drm-kms-helpers.html#c.drm_atomic_helper_connector_reset "drm_atomic_helper_connector_reset") to reset
    atomic state using this hook.

`detect`
:   Check to see if anything is attached to the connector. The parameter
    force is set to false whilst polling, true when checking the
    connector due to a user request. force can be used by the driver to
    avoid expensive, destructive operations during automated probing.

    This callback is optional, if not implemented the connector will be
    considered as always being attached.

    FIXME:

    Note that this hook is only called by the probe helper. It’s not in
    the helper library vtable purely for historical reasons. The only DRM
    core entry point to probe connector state is **fill\_modes**.

    Note that the helper library will already hold
    [`drm_mode_config.connection_mutex`](#c.drm_mode_config "drm_mode_config"). Drivers which need to grab additional
    locks to avoid races with concurrent modeset changes need to use
    [`drm_connector_helper_funcs.detect_ctx`](drm-kms-helpers.html#c.drm_connector_helper_funcs "drm_connector_helper_funcs") instead.

    Also note that this callback can be called no matter the
    state the connector is in. Drivers that need the underlying
    device to be powered to perform the detection will first need
    to make sure it’s been properly enabled.

    RETURNS:

    drm\_connector\_status indicating the connector’s status.

`force`
:   This function is called to update internal encoder state when the
    connector is forced to a certain state by userspace, either through
    the sysfs interfaces or on the kernel cmdline. In that case the
    **detect** callback isn’t called.

    FIXME:

    Note that this hook is only called by the probe helper. It’s not in
    the helper library vtable purely for historical reasons. The only DRM
    core entry point to probe connector state is **fill\_modes**.

`fill_modes`
:   Entry point for output detection and basic mode validation. The
    driver should reprobe the output if needed (e.g. when hotplug
    handling is unreliable), add all detected modes to [`drm_connector.modes`](#c.drm_connector "drm_connector")
    and filter out any the device can’t support in any configuration. It
    also needs to filter out any modes wider or higher than the
    parameters max\_width and max\_height indicate.

    The drivers must also prune any modes no longer valid from
    [`drm_connector.modes`](#c.drm_connector "drm_connector"). Furthermore it must update
    [`drm_connector.status`](#c.drm_connector "drm_connector") and [`drm_connector.edid`](#c.drm_connector "drm_connector"). If no EDID has been
    received for this output connector->edid must be NULL.

    Drivers using the probe helpers should use
    [`drm_helper_probe_single_connector_modes()`](drm-kms-helpers.html#c.drm_helper_probe_single_connector_modes "drm_helper_probe_single_connector_modes") to implement this
    function.

    RETURNS:

    The number of modes detected and filled into [`drm_connector.modes`](#c.drm_connector "drm_connector").

`set_property`
:   This is the legacy entry point to update a property attached to the
    connector.

    This callback is optional if the driver does not support any legacy
    driver-private properties. For atomic drivers it is not used because
    property handling is done entirely in the DRM core.

    RETURNS:

    0 on success or a negative error code on failure.

`late_register`
:   This optional hook can be used to register additional userspace
    interfaces attached to the connector, light backlight control, i2c,
    DP aux or similar interfaces. It is called late in the driver load
    sequence from [`drm_connector_register()`](#c.drm_connector_register "drm_connector_register") when registering all the
    core drm connector interfaces. Everything added from this callback
    should be unregistered in the early\_unregister callback.

    This is called while holding [`drm_connector.mutex`](#c.drm_connector "drm_connector").

    Returns:

    0 on success, or a negative error code on failure.

`early_unregister`
:   This optional hook should be used to unregister the additional
    userspace interfaces attached to the connector from
    `late_register()`. It is called from [`drm_connector_unregister()`](#c.drm_connector_unregister "drm_connector_unregister"),
    early in the driver unload sequence to disable userspace access
    before data structures are torndown.

    This is called while holding [`drm_connector.mutex`](#c.drm_connector "drm_connector").

`destroy`
:   Clean up connector resources. This is called at driver unload time
    through [`drm_mode_config_cleanup()`](#c.drm_mode_config_cleanup "drm_mode_config_cleanup"). It can also be called at runtime
    when a connector is being hot-unplugged for drivers that support
    connector hotplugging (e.g. DisplayPort MST).

`atomic_duplicate_state`
:   Duplicate the current atomic state for this connector and return it.
    The core and helpers guarantee that any atomic state duplicated with
    this hook and still owned by the caller (i.e. not transferred to the
    driver by calling [`drm_mode_config_funcs.atomic_commit`](#c.drm_mode_config_funcs "drm_mode_config_funcs")) will be
    cleaned up by calling the **atomic\_destroy\_state** hook in this
    structure.

    This callback is mandatory for atomic drivers.

    Atomic drivers which don’t subclass [`struct drm_connector_state`](#c.drm_connector_state "drm_connector_state") should use
    [`drm_atomic_helper_connector_duplicate_state()`](drm-kms-helpers.html#c.drm_atomic_helper_connector_duplicate_state "drm_atomic_helper_connector_duplicate_state"). Drivers that subclass the
    state structure to extend it with driver-private state should use
    [`__drm_atomic_helper_connector_duplicate_state()`](drm-kms-helpers.html#c.__drm_atomic_helper_connector_duplicate_state "__drm_atomic_helper_connector_duplicate_state") to make sure shared state is
    duplicated in a consistent fashion across drivers.

    It is an error to call this hook before [`drm_connector.state`](#c.drm_connector "drm_connector") has been
    initialized correctly.

    NOTE:

    If the duplicate state references refcounted resources this hook must
    acquire a reference for each of them. The driver must release these
    references again in **atomic\_destroy\_state**.

    RETURNS:

    Duplicated atomic state or NULL when the allocation failed.

`atomic_destroy_state`
:   Destroy a state duplicated with **atomic\_duplicate\_state** and release
    or unreference all resources it references

    This callback is mandatory for atomic drivers.

`atomic_set_property`
:   Decode a driver-private property value and store the decoded value
    into the passed-in state structure. Since the atomic core decodes all
    standardized properties (even for extensions beyond the core set of
    properties which might not be implemented by all drivers) this
    requires drivers to subclass the state structure.

    Such driver-private properties should really only be implemented for
    truly hardware/vendor specific state. Instead it is preferred to
    standardize atomic extension and decode the properties used to expose
    such an extension in the core.

    Do not call this function directly, use
    `drm_atomic_connector_set_property()` instead.

    This callback is optional if the driver does not support any
    driver-private atomic properties.

    NOTE:

    This function is called in the state assembly phase of atomic
    modesets, which can be aborted for any reason (including on
    userspace’s request to just check whether a configuration would be
    possible). Drivers MUST NOT touch any persistent state (hardware or
    software) or data structures except the passed in **state** parameter.

    Also since userspace controls in which order properties are set this
    function must not do any input validation (since the state update is
    incomplete and hence likely inconsistent). Instead any such input
    validation must be done in the various atomic\_check callbacks.

    RETURNS:

    0 if the property has been found, -EINVAL if the property isn’t
    implemented by the driver (which shouldn’t ever happen, the core only
    asks for properties attached to this connector). No other validation
    is allowed by the driver. The core already checks that the property
    value is within the range (integer, valid `enum value`, ...) the driver
    set when registering the property.

`atomic_get_property`
:   Reads out the decoded driver-private property. This is used to
    implement the GETCONNECTOR IOCTL.

    Do not call this function directly, use
    `drm_atomic_connector_get_property()` instead.

    This callback is optional if the driver does not support any
    driver-private atomic properties.

    RETURNS:

    0 on success, -EINVAL if the property isn’t implemented by the
    driver (which shouldn’t ever happen, the core only asks for
    properties attached to this connector).

`atomic_print_state`
:   If driver subclasses [`struct drm_connector_state`](#c.drm_connector_state "drm_connector_state"), it should implement
    this optional hook for printing additional driver specific state.

    Do not call this directly, use `drm_atomic_connector_print_state()`
    instead.

`oob_hotplug_event`
:   This will get called when a hotplug-event for a drm-connector
    has been received from a source outside the display driver / device.

`debugfs_init`
:   Allows connectors to create connector-specific debugfs files.

**Description**

Each CRTC may have one or more connectors attached to it. The functions
below allow the core DRM code to control connectors, enumerate available modes,
etc.

struct drm\_cmdline\_mode
:   DRM Mode passed through the kernel command-line

**Definition**:

```
struct drm_cmdline_mode {
    char name[DRM_DISPLAY_MODE_LEN];
    bool specified;
    bool refresh_specified;
    bool bpp_specified;
    unsigned int pixel_clock;
    int xres;
    int yres;
    int bpp;
    int refresh;
    bool rb;
    bool interlace;
    bool cvt;
    bool margins;
    enum drm_connector_force force;
    unsigned int rotation_reflection;
    enum drm_panel_orientation panel_orientation;
    struct drm_connector_tv_margins tv_margins;
    enum drm_connector_tv_mode tv_mode;
    bool tv_mode_specified;
};
```

**Members**

`name`
:   Name of the mode.

`specified`
:   Has a mode been read from the command-line?

`refresh_specified`
:   Did the mode have a preferred refresh rate?

`bpp_specified`
:   Did the mode have a preferred BPP?

`pixel_clock`
:   Pixel Clock in kHz. Optional.

`xres`
:   Active resolution on the X axis, in pixels.

`yres`
:   Active resolution on the Y axis, in pixels.

`bpp`
:   Bits per pixels for the mode.

`refresh`
:   Refresh rate, in Hertz.

`rb`
:   Do we need to use reduced blanking?

`interlace`
:   The mode is interlaced.

`cvt`
:   The timings will be calculated using the VESA Coordinated
    Video Timings instead of looking up the mode from a table.

`margins`
:   Add margins to the mode calculation (1.8% of xres rounded
    down to 8 pixels and 1.8% of yres).

`force`
:   Ignore the hotplug state of the connector, and force its
    state to one of the DRM\_FORCE\_\* values.

`rotation_reflection`
:   Initial rotation and reflection of the mode setup from the
    command line. See DRM\_MODE\_ROTATE\_\* and
    DRM\_MODE\_REFLECT\_\*. The only rotations supported are
    DRM\_MODE\_ROTATE\_0 and DRM\_MODE\_ROTATE\_180.

`panel_orientation`
:   drm-connector “panel orientation” property override value,
    DRM\_MODE\_PANEL\_ORIENTATION\_UNKNOWN if not set.

`tv_margins`
:   TV margins to apply to the mode.

`tv_mode`
:   TV mode standard. See DRM\_MODE\_TV\_MODE\_\*.

`tv_mode_specified`
:   Did the mode have a preferred TV mode?

**Description**

Each connector can have an initial mode with additional options
passed through the kernel command line. This structure allows to
express those parameters and will be filled by the command-line
parser.

struct drm\_connector\_hdmi\_audio
:   DRM gemeric HDMI Codec-related structure

**Definition**:

```
struct drm_connector_hdmi_audio {
    const struct drm_connector_hdmi_audio_funcs *funcs;
    struct platform_device *codec_pdev;
    struct mutex lock;
    void (*plugged_cb)(struct device *dev, bool plugged);
    struct device *plugged_cb_dev;
    bool last_state;
    int dai_port;
};
```

**Members**

`funcs`
:   Implementation of the HDMI codec functionality to be used by the DRM
    HDMI Codec framework.

`codec_pdev`
:   Platform device created to hold the HDMI Codec. It will be
    automatically unregistered during [`drm_connector_cleanup()`](#c.drm_connector_cleanup "drm_connector_cleanup").

`lock`
:   Mutex to protect **last\_state**, **plugged\_cb** and **plugged\_cb\_dev**.

`plugged_cb`
:   Callback to be called when the HDMI sink get plugged to or unplugged
    from this connector. This is assigned by the framework when
    requested by the ASoC code.

`plugged_cb_dev`
:   The data for **`plugged_cb()`**. It is being provided by the ASoC.

`last_state`
:   Last plugged state recored by the framework. It is used to correctly
    report the state to **`plugged_cb()`**.

`dai_port`
:   The port in DT that is used for the Codec DAI.

**Description**

HDMI drivers usually incorporate a HDMI Codec. This structure expresses the
generic HDMI Codec as used by the DRM HDMI Codec framework.

struct drm\_connector\_cec
:   DRM Connector CEC-related structure

**Definition**:

```
struct drm_connector_cec {
    struct mutex mutex;
    const struct drm_connector_cec_funcs *funcs;
    void *data;
};
```

**Members**

`mutex`
:   protects all fields in this structure.

`funcs`
:   CEC Control Functions

`data`
:   CEC implementation-specific data

struct drm\_connector
:   central DRM connector control structure

**Definition**:

```
struct drm_connector {
    struct drm_device *dev;
    struct device *kdev;
    struct device_attribute *attr;
    struct fwnode_handle *fwnode;
    struct list_head head;
    struct list_head global_connector_list_entry;
    struct drm_mode_object base;
    char *name;
    struct mutex mutex;
    unsigned index;
    int connector_type;
    int connector_type_id;
    bool interlace_allowed;
    bool doublescan_allowed;
    bool stereo_allowed;
    bool ycbcr_420_allowed;
    enum drm_connector_registration_state registration_state;
    struct list_head modes;
    enum drm_connector_status status;
    struct list_head probed_modes;
    struct drm_display_info display_info;
    const struct drm_connector_funcs *funcs;
    struct drm_property_blob *edid_blob_ptr;
    struct drm_object_properties properties;
    struct drm_property *scaling_mode_property;
    struct drm_property *vrr_capable_property;
    struct drm_property *colorspace_property;
    struct drm_property_blob *path_blob_ptr;
    unsigned int max_bpc;
    struct drm_property *max_bpc_property;
    struct drm_privacy_screen *privacy_screen;
    struct notifier_block privacy_screen_notifier;
    struct drm_property *privacy_screen_sw_state_property;
    struct drm_property *privacy_screen_hw_state_property;
    struct drm_property *broadcast_rgb_property;
#define DRM_CONNECTOR_POLL_HPD (1 << 0);
#define DRM_CONNECTOR_POLL_CONNECT (1 << 1);
#define DRM_CONNECTOR_POLL_DISCONNECT (1 << 2);
    uint8_t polled;
    int dpms;
    const struct drm_connector_helper_funcs *helper_private;
    struct drm_cmdline_mode cmdline_mode;
    enum drm_connector_force force;
    const struct drm_edid *edid_override;
    struct mutex edid_override_mutex;
    u64 epoch_counter;
    u32 possible_encoders;
    struct drm_encoder *encoder;
#define MAX_ELD_BYTES   128;
    uint8_t eld[MAX_ELD_BYTES];
    struct mutex eld_mutex;
    bool latency_present[2];
    int video_latency[2];
    int audio_latency[2];
    struct i2c_adapter *ddc;
    int null_edid_counter;
    unsigned bad_edid_counter;
    bool edid_corrupt;
    u8 real_edid_checksum;
    struct dentry *debugfs_entry;
    struct drm_connector_state *state;
    struct drm_property_blob *tile_blob_ptr;
    bool has_tile;
    struct drm_tile_group *tile_group;
    bool tile_is_single_monitor;
    uint8_t num_h_tile, num_v_tile;
    uint8_t tile_h_loc, tile_v_loc;
    uint16_t tile_h_size, tile_v_size;
    struct llist_node free_node;
    struct drm_connector_hdmi hdmi;
    struct drm_connector_hdmi_audio hdmi_audio;
    struct drm_connector_cec cec;
};
```

**Members**

`dev`
:   parent DRM device

`kdev`
:   kernel device for sysfs attributes

`attr`
:   sysfs attributes

`fwnode`
:   associated fwnode supplied by platform firmware

    Drivers can set this to associate a fwnode with a connector, drivers
    are expected to get a reference on the fwnode when setting this.
    [`drm_connector_cleanup()`](#c.drm_connector_cleanup "drm_connector_cleanup") will call `fwnode_handle_put()` on this.

`head`
:   List of all connectors on a **dev**, linked from
    [`drm_mode_config.connector_list`](#c.drm_mode_config "drm_mode_config"). Protected by
    [`drm_mode_config.connector_list_lock`](#c.drm_mode_config "drm_mode_config"), but please only use
    [`drm_connector_list_iter`](#c.drm_connector_list_iter "drm_connector_list_iter") to walk this list.

`global_connector_list_entry`
:   Connector entry in the global connector-list, used by
    `drm_connector_find_by_fwnode()`.

`base`
:   base KMS object

`name`
:   human readable name, can be overwritten by the driver

`mutex`
:   Lock for general connector state, but currently only protects
    **registered**. Most of the connector state is still protected by
    [`drm_mode_config.mutex`](#c.drm_mode_config "drm_mode_config").

`index`
:   Compacted connector index, which matches the position inside
    the mode\_config.list for drivers not supporting hot-add/removing. Can
    be used as an array index. It is invariant over the lifetime of the
    connector.

`connector_type`
:   one of the DRM\_MODE\_CONNECTOR\_<foo> types from drm\_mode.h

`connector_type_id`
:   index into connector type enum

`interlace_allowed`
:   Can this connector handle interlaced modes? Only used by
    [`drm_helper_probe_single_connector_modes()`](drm-kms-helpers.html#c.drm_helper_probe_single_connector_modes "drm_helper_probe_single_connector_modes") for mode filtering.

`doublescan_allowed`
:   Can this connector handle doublescan? Only used by
    [`drm_helper_probe_single_connector_modes()`](drm-kms-helpers.html#c.drm_helper_probe_single_connector_modes "drm_helper_probe_single_connector_modes") for mode filtering.

`stereo_allowed`
:   Can this connector handle stereo modes? Only used by
    [`drm_helper_probe_single_connector_modes()`](drm-kms-helpers.html#c.drm_helper_probe_single_connector_modes "drm_helper_probe_single_connector_modes") for mode filtering.

`ycbcr_420_allowed`
:   This bool indicates if this connector is
    capable of handling YCBCR 420 output. While parsing the EDID
    blocks it’s very helpful to know if the source is capable of
    handling YCBCR 420 outputs.

`registration_state`
:   Is this connector initializing, exposed
    (registered) with userspace, or unregistered?

    Protected by **mutex**.

`modes`
:   Modes available on this connector (from `fill_modes()` + user).
    Protected by [`drm_mode_config.mutex`](#c.drm_mode_config "drm_mode_config").

`status`
:   One of the drm\_connector\_status enums (connected, not, or unknown).
    Protected by [`drm_mode_config.mutex`](#c.drm_mode_config "drm_mode_config").

`probed_modes`
:   These are modes added by probing with DDC or the BIOS, before
    filtering is applied. Used by the probe helpers. Protected by
    [`drm_mode_config.mutex`](#c.drm_mode_config "drm_mode_config").

`display_info`
:   Display information is filled from EDID information
    when a display is detected. For non hot-pluggable displays such as
    flat panels in embedded systems, the driver should initialize the
    [`drm_display_info.width_mm`](#c.drm_display_info "drm_display_info") and [`drm_display_info.height_mm`](#c.drm_display_info "drm_display_info") fields
    with the physical size of the display.

    Protected by [`drm_mode_config.mutex`](#c.drm_mode_config "drm_mode_config").

`funcs`
:   connector control functions

`edid_blob_ptr`
:   DRM property containing EDID if present. Protected by
    [`drm_mode_config.mutex`](#c.drm_mode_config "drm_mode_config").

    This must be updated only by calling [`drm_edid_connector_update()`](drm-kms-helpers.html#c.drm_edid_connector_update "drm_edid_connector_update") or
    [`drm_connector_update_edid_property()`](drm-kms-helpers.html#c.drm_connector_update_edid_property "drm_connector_update_edid_property").

    This must not be used by drivers directly.

`properties`
:   property tracking for this connector

`scaling_mode_property`
:   Optional atomic property to control the
    upscaling. See [`drm_connector_attach_content_protection_property()`](drm-kms-helpers.html#c.drm_connector_attach_content_protection_property "drm_connector_attach_content_protection_property").

`vrr_capable_property`
:   Optional property to help userspace
    query hardware support for variable refresh rate on a connector.
    connector. Drivers can add the property to a connector by
    calling [`drm_connector_attach_vrr_capable_property()`](#c.drm_connector_attach_vrr_capable_property "drm_connector_attach_vrr_capable_property").

    This should be updated only by calling
    [`drm_connector_set_vrr_capable_property()`](#c.drm_connector_set_vrr_capable_property "drm_connector_set_vrr_capable_property").

`colorspace_property`
:   Connector property to set the suitable
    colorspace supported by the sink.

`path_blob_ptr`
:   DRM blob property data for the DP MST path property. This should only
    be updated by calling [`drm_connector_set_path_property()`](#c.drm_connector_set_path_property "drm_connector_set_path_property").

`max_bpc`
:   Maximum bits per color channel the connector supports.

`max_bpc_property`
:   Default connector property for the max bpc to be
    driven out of the connector.

`privacy_screen`
:   drm\_privacy\_screen for this connector, or NULL.

`privacy_screen_notifier`
:   privacy-screen notifier\_block

`privacy_screen_sw_state_property`
:   Optional atomic property for the
    connector to control the integrated privacy screen.

`privacy_screen_hw_state_property`
:   Optional atomic property for the
    connector to report the actual integrated privacy screen state.

`broadcast_rgb_property`
:   Connector property to set the
    Broadcast RGB selection to output with.

`polled`
:   Connector polling mode, a combination of

    DRM\_CONNECTOR\_POLL\_HPD
    :   The connector generates hotplug events and doesn’t need to be
        periodically polled. The CONNECT and DISCONNECT flags must not
        be set together with the HPD flag.

    DRM\_CONNECTOR\_POLL\_CONNECT
    :   Periodically poll the connector for connection.

    DRM\_CONNECTOR\_POLL\_DISCONNECT
    :   Periodically poll the connector for disconnection, without
        causing flickering even when the connector is in use. DACs should
        rarely do this without a lot of testing.

    Set to 0 for connectors that don’t support connection status
    discovery.

`dpms`
:   Current dpms state. For legacy drivers the
    [`drm_connector_funcs.dpms`](#c.drm_connector_funcs "drm_connector_funcs") callback must update this. For atomic
    drivers, this is handled by the core atomic code, and drivers must
    only take [`drm_crtc_state.active`](#c.drm_crtc_state "drm_crtc_state") into account.

`helper_private`
:   mid-layer private data

`cmdline_mode`
:   mode line parsed from the kernel cmdline for this connector

`force`
:   a DRM\_FORCE\_<foo> state for forced mode sets

`edid_override`
:   Override EDID set via debugfs.

    Do not modify or access outside of the drm\_edid\_override\_\* family of
    functions.

`edid_override_mutex`
:   Protect access to edid\_override.

`epoch_counter`
:   used to detect any other changes in connector, besides status

`possible_encoders`
:   Bit mask of encoders that can drive this
    connector, [`drm_encoder_index()`](#c.drm_encoder_index "drm_encoder_index") determines the index into the bitfield
    and the bits are set with [`drm_connector_attach_encoder()`](#c.drm_connector_attach_encoder "drm_connector_attach_encoder").

`encoder`
:   Currently bound encoder driving this connector, if any.
    Only really meaningful for non-atomic drivers. Atomic drivers should
    instead look at [`drm_connector_state.best_encoder`](#c.drm_connector_state "drm_connector_state"), and in case they
    need the CRTC driving this output, [`drm_connector_state.crtc`](#c.drm_connector_state "drm_connector_state").

`eld`
:   EDID-like data, if present, protected by **eld\_mutex**

`eld_mutex`
:   protection for concurrenct access to **eld**

`latency_present`
:   AV delay info from ELD, if found

`video_latency`
:   Video latency info from ELD, if found.
    [0]: progressive, [1]: interlaced

`audio_latency`
:   audio latency info from ELD, if found
    [0]: progressive, [1]: interlaced

`ddc`
:   associated ddc adapter.
    A connector usually has its associated ddc adapter. If a driver uses
    this field, then an appropriate symbolic link is created in connector
    sysfs directory to make it easy for the user to tell which i2c
    adapter is for a particular display.

    The field should be set by calling [`drm_connector_init_with_ddc()`](#c.drm_connector_init_with_ddc "drm_connector_init_with_ddc").

`null_edid_counter`
:   track sinks that give us all zeros for the EDID.
    Needed to workaround some HW bugs where we get all 0s

`bad_edid_counter`
:   track sinks that give us an EDID with invalid checksum

`edid_corrupt`
:   Indicates whether the last read EDID was corrupt. Used
    in Displayport compliance testing - Displayport Link CTS Core 1.2
    rev1.1 4.2.2.6

`real_edid_checksum`
:   real edid checksum for corrupted edid block.
    Required in Displayport 1.4 compliance testing
    rev1.1 4.2.2.6

`debugfs_entry`
:   debugfs directory for this connector

`state`
:   Current atomic state for this connector.

    This is protected by [`drm_mode_config.connection_mutex`](#c.drm_mode_config "drm_mode_config"). Note that
    nonblocking atomic commits access the current connector state without
    taking locks. Either by going through the [`struct drm_atomic_state`](#c.drm_atomic_state "drm_atomic_state")
    pointers, see [`for_each_oldnew_connector_in_state()`](#c.for_each_oldnew_connector_in_state "for_each_oldnew_connector_in_state"),
    [`for_each_old_connector_in_state()`](#c.for_each_old_connector_in_state "for_each_old_connector_in_state") and
    [`for_each_new_connector_in_state()`](#c.for_each_new_connector_in_state "for_each_new_connector_in_state"). Or through careful ordering of
    atomic commit operations as implemented in the atomic helpers, see
    [`struct drm_crtc_commit`](#c.drm_crtc_commit "drm_crtc_commit").

`tile_blob_ptr`
:   DRM blob property data for the tile property (used mostly by DP MST).
    This is meant for screens which are driven through separate display
    pipelines represented by [`drm_crtc`](#c.drm_crtc "drm_crtc"), which might not be running with
    genlocked clocks. For tiled panels which are genlocked, like
    dual-link LVDS or dual-link DSI, the driver should try to not expose
    the tiling and virtualize both [`drm_crtc`](#c.drm_crtc "drm_crtc") and [`drm_plane`](#c.drm_plane "drm_plane") if needed.

    This should only be updated by calling
    [`drm_connector_set_tile_property()`](#c.drm_connector_set_tile_property "drm_connector_set_tile_property").

`has_tile`
:   is this connector connected to a tiled monitor

`tile_group`
:   tile group for the connected monitor

`tile_is_single_monitor`
:   whether the tile is one monitor housing

`num_h_tile`
:   number of horizontal tiles in the tile group

`num_v_tile`
:   number of vertical tiles in the tile group

`tile_h_loc`
:   horizontal location of this tile

`tile_v_loc`
:   vertical location of this tile

`tile_h_size`
:   horizontal size of this tile.

`tile_v_size`
:   vertical size of this tile.

`free_node`
:   List used only by [`drm_connector_list_iter`](#c.drm_connector_list_iter "drm_connector_list_iter") to be able to clean up a
    connector from any context, in conjunction with
    [`drm_mode_config.connector_free_work`](#c.drm_mode_config "drm_mode_config").

`hdmi`
:   HDMI-related variable and properties.

`hdmi_audio`
:   HDMI codec properties and non-DRM state.

`cec`
:   CEC-related data.

**Description**

Each connector may be connected to one or more CRTCs, or may be clonable by
another connector if they can share a CRTC. Each connector also has a specific
position in the broader display (referred to as a ‘screen’ though it could
span multiple monitors).

struct [drm\_connector](#c.drm_connector "drm_connector") \*drm\_connector\_lookup(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_priv, uint32\_t id)
:   lookup connector object

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_file *file_priv`
:   drm file to check for lease against.

`uint32_t id`
:   connector object id

**Description**

This function looks up the connector object specified by id
add takes a reference to it.

void drm\_connector\_get(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   acquire a connector reference

**Parameters**

`struct drm_connector *connector`
:   DRM connector

**Description**

This function increments the connector’s refcount.

void drm\_connector\_put(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   release a connector reference

**Parameters**

`struct drm_connector *connector`
:   DRM connector

**Description**

This function decrements the connector’s reference count and frees the
object if the reference count drops to zero.

bool drm\_connector\_is\_unregistered(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   has the connector been unregistered from userspace?

**Parameters**

`struct drm_connector *connector`
:   DRM connector

**Description**

Checks whether or not **connector** has been unregistered from userspace.

**Return**

True if the connector was unregistered, false if the connector is
registered or has not yet been registered with userspace.

struct drm\_tile\_group
:   Tile group metadata

**Definition**:

```
struct drm_tile_group {
    struct kref refcount;
    struct drm_device *dev;
    int id;
    u8 group_data[8];
};
```

**Members**

`refcount`
:   reference count

`dev`
:   DRM device

`id`
:   tile group id exposed to userspace

`group_data`
:   Sink-private data identifying this group

**Description**

**group\_data** corresponds to displayid vend/prod/serial for external screens
with an EDID.

struct drm\_connector\_list\_iter
:   connector\_list iterator

**Definition**:

```
struct drm_connector_list_iter {
};
```

**Members**

**Description**

This iterator tracks state needed to be able to walk the connector\_list
within [`struct drm_mode_config`](#c.drm_mode_config "drm_mode_config"). Only use together with
[`drm_connector_list_iter_begin()`](#c.drm_connector_list_iter_begin "drm_connector_list_iter_begin"), [`drm_connector_list_iter_end()`](#c.drm_connector_list_iter_end "drm_connector_list_iter_end") and
[`drm_connector_list_iter_next()`](#c.drm_connector_list_iter_next "drm_connector_list_iter_next") respectively the convenience macro
[`drm_for_each_connector_iter()`](#c.drm_for_each_connector_iter "drm_for_each_connector_iter").

Note that the return value of [`drm_connector_list_iter_next()`](#c.drm_connector_list_iter_next "drm_connector_list_iter_next") is only valid
up to the next [`drm_connector_list_iter_next()`](#c.drm_connector_list_iter_next "drm_connector_list_iter_next") or
[`drm_connector_list_iter_end()`](#c.drm_connector_list_iter_end "drm_connector_list_iter_end") call. If you want to use the connector later,
then you need to grab your own reference first using [`drm_connector_get()`](#c.drm_connector_get "drm_connector_get").

drm\_for\_each\_connector\_iter

`drm_for_each_connector_iter (connector, iter)`

> connector\_list iterator macro

**Parameters**

`connector`
:   [`struct drm_connector`](#c.drm_connector "drm_connector") pointer used as cursor

`iter`
:   [`struct drm_connector_list_iter`](#c.drm_connector_list_iter "drm_connector_list_iter")

**Description**

Note that **connector** is only valid within the list body, if you want to use
**connector** after calling [`drm_connector_list_iter_end()`](#c.drm_connector_list_iter_end "drm_connector_list_iter_end") then you need to grab
your own reference first using [`drm_connector_get()`](#c.drm_connector_get "drm_connector_get").

drm\_connector\_for\_each\_possible\_encoder

`drm_connector_for_each_possible_encoder (connector, encoder)`

> iterate connector’s possible encoders

**Parameters**

`connector`
:   [`struct drm_connector`](#c.drm_connector "drm_connector") pointer

`encoder`
:   [`struct drm_encoder`](#c.drm_encoder "drm_encoder") pointer used as cursor

const char \*drm\_get\_connector\_type\_name(unsigned int type)
:   return a string for connector type

**Parameters**

`unsigned int type`
:   The connector type (DRM\_MODE\_CONNECTOR\_\*)

**Return**

the name of the connector type, or NULL if the type is not valid.

int drm\_connector\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, const struct [drm\_connector\_funcs](#c.drm_connector_funcs "drm_connector_funcs") \*funcs, int connector\_type)
:   Init a preallocated connector

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_connector *connector`
:   the connector to init

`const struct drm_connector_funcs *funcs`
:   callbacks for this connector

`int connector_type`
:   user visible type of the connector

**Description**

Initialises a preallocated connector. Connectors should be
subclassed as part of driver connector objects.

At driver unload time the driver’s [`drm_connector_funcs.destroy`](#c.drm_connector_funcs "drm_connector_funcs") hook
should call [`drm_connector_cleanup()`](#c.drm_connector_cleanup "drm_connector_cleanup") and free the connector structure.
The connector structure should not be allocated with `devm_kzalloc()`.

**Note**

consider using [`drmm_connector_init()`](#c.drmm_connector_init "drmm_connector_init") instead of
[`drm_connector_init()`](#c.drm_connector_init "drm_connector_init") to let the DRM managed resource infrastructure
take care of cleanup and deallocation.

**Return**

Zero on success, error code on failure.

int drm\_connector\_dynamic\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, const struct [drm\_connector\_funcs](#c.drm_connector_funcs "drm_connector_funcs") \*funcs, int connector\_type, struct i2c\_adapter \*ddc)
:   Init a preallocated dynamic connector

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_connector *connector`
:   the connector to init

`const struct drm_connector_funcs *funcs`
:   callbacks for this connector

`int connector_type`
:   user visible type of the connector

`struct i2c_adapter *ddc`
:   pointer to the associated ddc adapter

**Description**

Initialises a preallocated dynamic connector. Connectors should be
subclassed as part of driver connector objects. The connector
structure should not be allocated with `devm_kzalloc()`.

Drivers should call this for dynamic connectors which can be hotplugged
after [`drm_dev_register()`](drm-internals.html#c.drm_dev_register "drm_dev_register") has been called already, e.g. DP MST connectors.
For all other - static - connectors, drivers should call one of the
drm\_connector\_init\*()/drmm\_connector\_init\*() functions.

After calling this function the drivers must call
[`drm_connector_dynamic_register()`](#c.drm_connector_dynamic_register "drm_connector_dynamic_register").

To remove the connector the driver must call [`drm_connector_unregister()`](#c.drm_connector_unregister "drm_connector_unregister")
followed by [`drm_connector_put()`](#c.drm_connector_put "drm_connector_put"). Putting the last reference will call the
driver’s [`drm_connector_funcs.destroy`](#c.drm_connector_funcs "drm_connector_funcs") hook, which in turn must call
[`drm_connector_cleanup()`](#c.drm_connector_cleanup "drm_connector_cleanup") and free the connector structure.

**Return**

Zero on success, error code on failure.

int drm\_connector\_init\_with\_ddc(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, const struct [drm\_connector\_funcs](#c.drm_connector_funcs "drm_connector_funcs") \*funcs, int connector\_type, struct i2c\_adapter \*ddc)
:   Init a preallocated connector

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_connector *connector`
:   the connector to init

`const struct drm_connector_funcs *funcs`
:   callbacks for this connector

`int connector_type`
:   user visible type of the connector

`struct i2c_adapter *ddc`
:   pointer to the associated ddc adapter

**Description**

Initialises a preallocated connector. Connectors should be
subclassed as part of driver connector objects.

At driver unload time the driver’s [`drm_connector_funcs.destroy`](#c.drm_connector_funcs "drm_connector_funcs") hook
should call [`drm_connector_cleanup()`](#c.drm_connector_cleanup "drm_connector_cleanup") and free the connector structure.
The connector structure should not be allocated with `devm_kzalloc()`.

Ensures that the ddc field of the connector is correctly set.

**Note**

consider using [`drmm_connector_init()`](#c.drmm_connector_init "drmm_connector_init") instead of
[`drm_connector_init_with_ddc()`](#c.drm_connector_init_with_ddc "drm_connector_init_with_ddc") to let the DRM managed resource
infrastructure take care of cleanup and deallocation.

**Return**

Zero on success, error code on failure.

int drmm\_connector\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, const struct [drm\_connector\_funcs](#c.drm_connector_funcs "drm_connector_funcs") \*funcs, int connector\_type, struct i2c\_adapter \*ddc)
:   Init a preallocated connector

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_connector *connector`
:   the connector to init

`const struct drm_connector_funcs *funcs`
:   callbacks for this connector

`int connector_type`
:   user visible type of the connector

`struct i2c_adapter *ddc`
:   optional pointer to the associated ddc adapter

**Description**

Initialises a preallocated connector. Connectors should be
subclassed as part of driver connector objects.

Cleanup is automatically handled with a call to
[`drm_connector_cleanup()`](#c.drm_connector_cleanup "drm_connector_cleanup") in a DRM-managed action.

The connector structure should be allocated with [`drmm_kzalloc()`](drm-internals.html#c.drmm_kzalloc "drmm_kzalloc").

The **drm\_connector\_funcs.destroy** hook must be NULL.

**Return**

Zero on success, error code on failure.

int drmm\_connector\_hdmi\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, const char \*vendor, const char \*product, const struct [drm\_connector\_funcs](#c.drm_connector_funcs "drm_connector_funcs") \*funcs, const struct [drm\_connector\_hdmi\_funcs](#c.drm_connector_hdmi_funcs "drm_connector_hdmi_funcs") \*hdmi\_funcs, int connector\_type, struct i2c\_adapter \*ddc, unsigned long supported\_formats, unsigned int max\_bpc)
:   Init a preallocated HDMI connector

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_connector *connector`
:   A pointer to the HDMI connector to init

`const char *vendor`
:   HDMI Controller Vendor name

`const char *product`
:   HDMI Controller Product name

`const struct drm_connector_funcs *funcs`
:   callbacks for this connector

`const struct drm_connector_hdmi_funcs *hdmi_funcs`
:   HDMI-related callbacks for this connector

`int connector_type`
:   user visible type of the connector

`struct i2c_adapter *ddc`
:   optional pointer to the associated ddc adapter

`unsigned long supported_formats`
:   Bitmask of **drm\_output\_color\_format** listing supported output formats

`unsigned int max_bpc`
:   Maximum bits per char the HDMI connector supports

**Description**

Initialises a preallocated HDMI connector. Connectors can be
subclassed as part of driver connector objects.

Cleanup is automatically handled with a call to
[`drm_connector_cleanup()`](#c.drm_connector_cleanup "drm_connector_cleanup") in a DRM-managed action.

The connector structure should be allocated with [`drmm_kzalloc()`](drm-internals.html#c.drmm_kzalloc "drmm_kzalloc").

The **drm\_connector\_funcs.destroy** hook must be NULL.

**Return**

Zero on success, error code on failure.

void drm\_connector\_attach\_edid\_property(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   attach edid property.

**Parameters**

`struct drm_connector *connector`
:   the connector

**Description**

Some connector types like DRM\_MODE\_CONNECTOR\_VIRTUAL do not get a
edid property attached by default. This function can be used to
explicitly enable the edid property in these cases.

int drm\_connector\_attach\_encoder(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, struct [drm\_encoder](#c.drm_encoder "drm_encoder") \*encoder)
:   attach a connector to an encoder

**Parameters**

`struct drm_connector *connector`
:   connector to attach

`struct drm_encoder *encoder`
:   encoder to attach **connector** to

**Description**

This function links up a connector to an encoder. Note that the routing
restrictions between encoders and crtcs are exposed to userspace through the
possible\_clones and possible\_crtcs bitmasks.

**Return**

Zero on success, negative errno on failure.

bool drm\_connector\_has\_possible\_encoder(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, struct [drm\_encoder](#c.drm_encoder "drm_encoder") \*encoder)
:   check if the connector and encoder are associated with each other

**Parameters**

`struct drm_connector *connector`
:   the connector

`struct drm_encoder *encoder`
:   the encoder

**Return**

True if **encoder** is one of the possible encoders for **connector**.

void drm\_connector\_cec\_phys\_addr\_invalidate(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   invalidate CEC physical address

**Parameters**

`struct drm_connector *connector`
:   connector undergoing CEC operation

**Description**

Invalidated CEC physical address set for this DRM connector.

void drm\_connector\_cec\_phys\_addr\_set(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   propagate CEC physical address

**Parameters**

`struct drm_connector *connector`
:   connector undergoing CEC operation

**Description**

Propagate CEC physical address from the display\_info to this DRM connector.

void drm\_connector\_cleanup(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   cleans up an initialised connector

**Parameters**

`struct drm_connector *connector`
:   connector to cleanup

**Description**

Cleans up the connector but doesn’t free the object.

int drm\_connector\_register(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   register a connector

**Parameters**

`struct drm_connector *connector`
:   the connector to register

**Description**

Register userspace interfaces for a connector. Drivers shouldn’t call this
function. Static connectors will be registered automatically by DRM core
from [`drm_dev_register()`](drm-internals.html#c.drm_dev_register "drm_dev_register"), dynamic connectors (MST) should be registered by
drivers calling [`drm_connector_dynamic_register()`](#c.drm_connector_dynamic_register "drm_connector_dynamic_register").

When the connector is no longer available, callers must call
[`drm_connector_unregister()`](#c.drm_connector_unregister "drm_connector_unregister").

**Note**

Existing uses of this function in drivers should be a nop already and
are scheduled to be removed.

**Return**

Zero on success, error code on failure.

int drm\_connector\_dynamic\_register(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   register a dynamic connector

**Parameters**

`struct drm_connector *connector`
:   the connector to register

**Description**

Register userspace interfaces for a connector. Only call this for connectors
initialized by calling [`drm_connector_dynamic_init()`](#c.drm_connector_dynamic_init "drm_connector_dynamic_init"). All other connectors
will be registered automatically when calling [`drm_dev_register()`](drm-internals.html#c.drm_dev_register "drm_dev_register").

When the connector is no longer available the driver must call
[`drm_connector_unregister()`](#c.drm_connector_unregister "drm_connector_unregister").

**Return**

Zero on success, error code on failure.

void drm\_connector\_unregister(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   unregister a connector

**Parameters**

`struct drm_connector *connector`
:   the connector to unregister

**Description**

Unregister userspace interfaces for a connector. Drivers should call this
for dynamic connectors (MST) only, which were registered explicitly by
calling [`drm_connector_dynamic_register()`](#c.drm_connector_dynamic_register "drm_connector_dynamic_register"). All other - static - connectors
will be unregistered automatically by DRM core and drivers shouldn’t call
this function for those.

**Note**

Existing uses of this function in drivers for static connectors
should be a nop already and are scheduled to be removed.

const char \*drm\_get\_connector\_status\_name(enum [drm\_connector\_status](#c.drm_connector_status "drm_connector_status") status)
:   return a string for connector status

**Parameters**

`enum drm_connector_status status`
:   connector status to compute name of

**Description**

In contrast to the other drm\_get\_\*\_name functions this one here returns a
const pointer and hence is threadsafe.

**Return**

connector status string

void drm\_connector\_list\_iter\_begin(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_connector\_list\_iter](#c.drm_connector_list_iter "drm_connector_list_iter") \*iter)
:   initialize a connector\_list iterator

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_connector_list_iter *iter`
:   connector\_list iterator

**Description**

Sets **iter** up to walk the [`drm_mode_config.connector_list`](#c.drm_mode_config "drm_mode_config") of **dev**. **iter**
must always be cleaned up again by calling [`drm_connector_list_iter_end()`](#c.drm_connector_list_iter_end "drm_connector_list_iter_end").
Iteration itself happens using [`drm_connector_list_iter_next()`](#c.drm_connector_list_iter_next "drm_connector_list_iter_next") or
[`drm_for_each_connector_iter()`](#c.drm_for_each_connector_iter "drm_for_each_connector_iter").

struct [drm\_connector](#c.drm_connector "drm_connector") \*drm\_connector\_list\_iter\_next(struct [drm\_connector\_list\_iter](#c.drm_connector_list_iter "drm_connector_list_iter") \*iter)
:   return next connector

**Parameters**

`struct drm_connector_list_iter *iter`
:   connector\_list iterator

**Return**

the next connector for **iter**, or NULL when the list walk has
completed.

void drm\_connector\_list\_iter\_end(struct [drm\_connector\_list\_iter](#c.drm_connector_list_iter "drm_connector_list_iter") \*iter)
:   tear down a connector\_list iterator

**Parameters**

`struct drm_connector_list_iter *iter`
:   connector\_list iterator

**Description**

Tears down **iter** and releases any resources (like [`drm_connector`](#c.drm_connector "drm_connector") references)
acquired while walking the list. This must always be called, both when the
iteration completes fully or when it was aborted without walking the entire
list.

const char \*drm\_get\_subpixel\_order\_name(enum subpixel\_order order)
:   return a string for a given subpixel enum

**Parameters**

`enum subpixel_order order`
:   `enum of` subpixel\_order

**Description**

Note you could abuse this and return something out of bounds, but that
would be a caller error. No unscrubbed user data should make it here.

**Return**

string describing an enumerated subpixel property

int drm\_display\_info\_set\_bus\_formats(struct [drm\_display\_info](#c.drm_display_info "drm_display_info") \*info, const u32 \*formats, unsigned int num\_formats)
:   set the supported bus formats

**Parameters**

`struct drm_display_info *info`
:   display info to store bus formats in

`const u32 *formats`
:   array containing the supported bus formats

`unsigned int num_formats`
:   the number of entries in the fmts array

**Description**

Store the supported bus formats in display info structure.
See MEDIA\_BUS\_FMT\_\* definitions in include/uapi/linux/media-bus-format.h for
a full list of available formats.

**Return**

0 on success or a negative error code on failure.

int drm\_get\_tv\_mode\_from\_name(const char \*name, size\_t len)
:   Translates a TV mode name into its `enum value`

**Parameters**

`const char *name`
:   TV Mode name we want to convert

`size_t len`
:   Length of **name**

**Description**

Translates **name** into an [`enum drm_connector_tv_mode`](#c.drm_connector_tv_mode "drm_connector_tv_mode").

**Return**

the `enum value` on success, a negative errno otherwise.

int drm\_mode\_create\_dvi\_i\_properties(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   create DVI-I specific connector properties

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

Called by a driver the first time a DVI-I connector is made.

**Return**

`0`

void drm\_connector\_attach\_dp\_subconnector\_property(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   create subconnector property for DP

**Parameters**

`struct drm_connector *connector`
:   drm\_connector to attach property

**Description**

Called by a driver when DP connector is created.

int drm\_connector\_attach\_content\_type\_property(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   attach content-type property

**Parameters**

`struct drm_connector *connector`
:   connector to attach content type property on.

**Description**

Called by a driver the first time a HDMI connector is made.

**Return**

`0`

void drm\_connector\_attach\_tv\_margin\_properties(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   attach TV connector margin properties

**Parameters**

`struct drm_connector *connector`
:   DRM connector

**Description**

Called by a driver when it needs to attach TV margin props to a connector.
Typically used on SDTV and HDMI connectors.

int drm\_mode\_create\_tv\_margin\_properties(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   create TV connector margin properties

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

Called by a driver’s HDMI connector initialization routine, this function
creates the TV margin properties for a given device. No need to call this
function for an SDTV connector, it’s already called from
[`drm_mode_create_tv_properties_legacy()`](#c.drm_mode_create_tv_properties_legacy "drm_mode_create_tv_properties_legacy").

**Return**

0 on success or a negative error code on failure.

int drm\_mode\_create\_tv\_properties\_legacy(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, unsigned int num\_modes, const char \*const modes[])
:   create TV specific connector properties

**Parameters**

`struct drm_device *dev`
:   DRM device

`unsigned int num_modes`
:   number of different TV formats (modes) supported

`const char * const modes[]`
:   array of pointers to strings containing name of each format

**Description**

Called by a driver’s TV initialization routine, this function creates
the TV specific connector properties for a given device. Caller is
responsible for allocating a list of format names and passing them to
this routine.

**NOTE**

This functions registers the deprecated “mode” connector
property to select the analog TV mode (ie, NTSC, PAL, etc.). New
drivers must use [`drm_mode_create_tv_properties()`](#c.drm_mode_create_tv_properties "drm_mode_create_tv_properties") instead.

**Return**

0 on success or a negative error code on failure.

int drm\_mode\_create\_tv\_properties(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, unsigned int supported\_tv\_modes)
:   create TV specific connector properties

**Parameters**

`struct drm_device *dev`
:   DRM device

`unsigned int supported_tv_modes`
:   Bitmask of TV modes supported (See DRM\_MODE\_TV\_MODE\_\*)

**Description**

Called by a driver’s TV initialization routine, this function creates
the TV specific connector properties for a given device.

**Return**

0 on success or a negative error code on failure.

int drm\_mode\_create\_scaling\_mode\_property(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   create scaling mode property

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

Called by a driver the first time it’s needed, must be attached to desired
connectors.

Atomic drivers should use [`drm_connector_attach_scaling_mode_property()`](#c.drm_connector_attach_scaling_mode_property "drm_connector_attach_scaling_mode_property")
instead to correctly assign [`drm_connector_state.scaling_mode`](#c.drm_connector_state "drm_connector_state")
in the atomic state.

**Return**

`0`

int drm\_connector\_attach\_vrr\_capable\_property(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   creates the vrr\_capable property

**Parameters**

`struct drm_connector *connector`
:   connector to create the vrr\_capable property on.

**Description**

This is used by atomic drivers to add support for querying
variable refresh rate capability for a connector.

**Return**

Zero on success, negative errno on failure.

int drm\_connector\_attach\_scaling\_mode\_property(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, u32 scaling\_mode\_mask)
:   attach atomic scaling mode property

**Parameters**

`struct drm_connector *connector`
:   connector to attach scaling mode property on.

`u32 scaling_mode_mask`
:   or’ed mask of BIT(`DRM_MODE_SCALE_`\*).

**Description**

This is used to add support for scaling mode to atomic drivers.
The scaling mode will be set to [`drm_connector_state.scaling_mode`](#c.drm_connector_state "drm_connector_state")
and can be used from [`drm_connector_helper_funcs->atomic_check`](drm-kms-helpers.html#c.drm_connector_helper_funcs "drm_connector_helper_funcs") for validation.

This is the atomic version of [`drm_mode_create_scaling_mode_property()`](#c.drm_mode_create_scaling_mode_property "drm_mode_create_scaling_mode_property").

**Return**

Zero on success, negative errno on failure.

int drm\_mode\_create\_aspect\_ratio\_property(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   create aspect ratio property

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

Called by a driver the first time it’s needed, must be attached to desired
connectors.

**Return**

Zero on success, negative errno on failure.

int drm\_mode\_create\_hdmi\_colorspace\_property(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, u32 supported\_colorspaces)
:   create hdmi colorspace property

**Parameters**

`struct drm_connector *connector`
:   connector to create the Colorspace property on.

`u32 supported_colorspaces`
:   bitmap of supported color spaces

**Description**

Called by a driver the first time it’s needed, must be attached to desired
HDMI connectors.

**Return**

Zero on success, negative errno on failure.

int drm\_mode\_create\_dp\_colorspace\_property(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, u32 supported\_colorspaces)
:   create dp colorspace property

**Parameters**

`struct drm_connector *connector`
:   connector to create the Colorspace property on.

`u32 supported_colorspaces`
:   bitmap of supported color spaces

**Description**

Called by a driver the first time it’s needed, must be attached to desired
DP connectors.

**Return**

Zero on success, negative errno on failure.

int drm\_mode\_create\_content\_type\_property(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   create content type property

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

Called by a driver the first time it’s needed, must be attached to desired
connectors.

**Return**

Zero on success, negative errno on failure.

int drm\_mode\_create\_suggested\_offset\_properties(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   create suggests offset properties

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

Create the suggested x/y offset property for connectors.

**Return**

0 on success or a negative error code on failure.

int drm\_connector\_set\_path\_property(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, const char \*path)
:   set tile property on connector

**Parameters**

`struct drm_connector *connector`
:   connector to set property on.

`const char *path`
:   path to use for property; must not be NULL.

**Description**

This creates a property to expose to userspace to specify a
connector path. This is mainly used for DisplayPort MST where
connectors have a topology and we want to allow userspace to give
them more meaningful names.

**Return**

Zero on success, negative errno on failure.

int drm\_connector\_set\_tile\_property(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   set tile property on connector

**Parameters**

`struct drm_connector *connector`
:   connector to set property on.

**Description**

This looks up the tile information for a connector, and creates a
property for userspace to parse if it exists. The property is of
the form of 8 integers using ‘:’ as a separator.
This is used for dual port tiled displays with DisplayPort SST
or DisplayPort MST connectors.

**Return**

Zero on success, errno on failure.

void drm\_connector\_set\_link\_status\_property(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, uint64\_t link\_status)
:   Set link status property of a connector

**Parameters**

`struct drm_connector *connector`
:   drm connector

`uint64_t link_status`
:   new value of link status property (0: Good, 1: Bad)

**Description**

In usual working scenario, this link status property will always be set to
“GOOD”. If something fails during or after a mode set, the kernel driver
may set this link status property to “BAD”. The caller then needs to send a
hotplug uevent for userspace to re-check the valid modes through
GET\_CONNECTOR\_IOCTL and retry modeset.

**Note**

Drivers cannot rely on userspace to support this property and
issue a modeset. As such, they may choose to handle issues (like
re-training a link) without userspace’s intervention.

The reason for adding this property is to handle link training failures, but
it is not limited to DP or link training. For example, if we implement
asynchronous setcrtc, this property can be used to report any failures in that.

int drm\_connector\_attach\_max\_bpc\_property(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, int min, int max)
:   attach “max bpc” property

**Parameters**

`struct drm_connector *connector`
:   connector to attach max bpc property on.

`int min`
:   The minimum bit depth supported by the connector.

`int max`
:   The maximum bit depth supported by the connector.

**Description**

This is used to add support for limiting the bit depth on a connector.

**Return**

Zero on success, negative errno on failure.

int drm\_connector\_attach\_hdr\_output\_metadata\_property(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   attach “HDR\_OUTPUT\_METADA” property

**Parameters**

`struct drm_connector *connector`
:   connector to attach the property on.

**Description**

This is used to allow the userspace to send HDR Metadata to the
driver.

**Return**

Zero on success, negative errno on failure.

int drm\_connector\_attach\_broadcast\_rgb\_property(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   attach “Broadcast RGB” property

**Parameters**

`struct drm_connector *connector`
:   connector to attach the property on.

**Description**

This is used to add support for forcing the RGB range on a connector

**Return**

Zero on success, negative errno on failure.

int drm\_connector\_attach\_colorspace\_property(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   attach “Colorspace” property

**Parameters**

`struct drm_connector *connector`
:   connector to attach the property on.

**Description**

This is used to allow the userspace to signal the output colorspace
to the driver.

**Return**

Zero on success, negative errno on failure.

bool drm\_connector\_atomic\_hdr\_metadata\_equal(struct [drm\_connector\_state](#c.drm_connector_state "drm_connector_state") \*old\_state, struct [drm\_connector\_state](#c.drm_connector_state "drm_connector_state") \*new\_state)
:   checks if the hdr metadata changed

**Parameters**

`struct drm_connector_state *old_state`
:   old connector state to compare

`struct drm_connector_state *new_state`
:   new connector state to compare

**Description**

This is used by HDR-enabled drivers to test whether the HDR metadata
have changed between two different connector state (and thus probably
requires a full blown mode change).

**Return**

True if the metadata are equal, False otherwise

void drm\_connector\_set\_vrr\_capable\_property(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, bool capable)
:   sets the variable refresh rate capable property for a connector

**Parameters**

`struct drm_connector *connector`
:   drm connector

`bool capable`
:   True if the connector is variable refresh rate capable

**Description**

Should be used by atomic drivers to update the indicated support for
variable refresh rate over a connector.

int drm\_connector\_set\_panel\_orientation(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, enum [drm\_panel\_orientation](#c.drm_panel_orientation "drm_panel_orientation") panel\_orientation)
:   sets the connector’s panel\_orientation

**Parameters**

`struct drm_connector *connector`
:   connector for which to set the panel-orientation property.

`enum drm_panel_orientation panel_orientation`
:   drm\_panel\_orientation value to set

**Description**

This function sets the connector’s panel\_orientation and attaches
a “panel orientation” property to the connector.

Calling this function on a connector where the panel\_orientation has
already been set is a no-op (e.g. the orientation has been overridden with
a kernel commandline option).

It is allowed to call this function with a panel\_orientation of
DRM\_MODE\_PANEL\_ORIENTATION\_UNKNOWN, in which case it is a no-op.

The function shouldn’t be called in panel after drm is registered (i.e.
[`drm_dev_register()`](drm-internals.html#c.drm_dev_register "drm_dev_register") is called in drm).

**Return**

Zero on success, negative errno on failure.

int drm\_connector\_set\_panel\_orientation\_with\_quirk(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, enum [drm\_panel\_orientation](#c.drm_panel_orientation "drm_panel_orientation") panel\_orientation, int width, int height)
:   set the connector’s panel\_orientation after checking for quirks

**Parameters**

`struct drm_connector *connector`
:   connector for which to init the panel-orientation property.

`enum drm_panel_orientation panel_orientation`
:   drm\_panel\_orientation value to set

`int width`
:   width in pixels of the panel, used for panel quirk detection

`int height`
:   height in pixels of the panel, used for panel quirk detection

**Description**

Like [`drm_connector_set_panel_orientation()`](#c.drm_connector_set_panel_orientation "drm_connector_set_panel_orientation"), but with a check for platform
specific (e.g. DMI based) quirks overriding the passed in panel\_orientation.

**Return**

Zero on success, negative errno on failure.

int drm\_connector\_set\_orientation\_from\_panel(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, struct [drm\_panel](drm-kms-helpers.html#c.drm_panel "drm_panel") \*panel)
:   set the connector’s panel\_orientation from panel’s callback.

**Parameters**

`struct drm_connector *connector`
:   connector for which to init the panel-orientation property.

`struct drm_panel *panel`
:   panel that can provide orientation information.

**Description**

Drm drivers should call this function before [`drm_dev_register()`](drm-internals.html#c.drm_dev_register "drm_dev_register").
Orientation is obtained from panel’s .`get_orientation()` callback.

**Return**

Zero on success, negative errno on failure.

void drm\_connector\_create\_privacy\_screen\_properties(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   create the drm connecter’s privacy-screen properties.

**Parameters**

`struct drm_connector *connector`
:   connector for which to create the privacy-screen properties

**Description**

This function creates the “privacy-screen sw-state” and “privacy-screen
hw-state” properties for the connector. They are not attached.

void drm\_connector\_attach\_privacy\_screen\_properties(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   attach the drm connecter’s privacy-screen properties.

**Parameters**

`struct drm_connector *connector`
:   connector on which to attach the privacy-screen properties

**Description**

This function attaches the “privacy-screen sw-state” and “privacy-screen
hw-state” properties to the connector. The initial state of both is set
to “Disabled”.

void drm\_connector\_attach\_privacy\_screen\_provider(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector, struct [drm\_privacy\_screen](drm-kms-helpers.html#c.drm_privacy_screen "drm_privacy_screen") \*priv)
:   attach a privacy-screen to the connector

**Parameters**

`struct drm_connector *connector`
:   connector to attach the privacy-screen to

`struct drm_privacy_screen *priv`
:   drm\_privacy\_screen to attach

**Description**

Create and attach the standard privacy-screen properties and register
a generic notifier for generating sysfs-connector-status-events
on external changes to the privacy-screen status.
This function takes ownership of the passed in drm\_privacy\_screen and will
call [`drm_privacy_screen_put()`](drm-kms-helpers.html#c.drm_privacy_screen_put "drm_privacy_screen_put") on it when the connector is destroyed.

void drm\_connector\_update\_privacy\_screen(const struct [drm\_connector\_state](#c.drm_connector_state "drm_connector_state") \*connector\_state)
:   update connector’s privacy-screen sw-state

**Parameters**

`const struct drm_connector_state *connector_state`
:   connector-state to update the privacy-screen for

**Description**

This function calls [`drm_privacy_screen_set_sw_state()`](drm-kms-helpers.html#c.drm_privacy_screen_set_sw_state "drm_privacy_screen_set_sw_state") on the connector’s
privacy-screen.

If the connector has no privacy-screen, then this is a no-op.

void drm\_connector\_oob\_hotplug\_event(struct fwnode\_handle \*connector\_fwnode, enum [drm\_connector\_status](#c.drm_connector_status "drm_connector_status") status)
:   Report out-of-band hotplug event to connector

**Parameters**

`struct fwnode_handle *connector_fwnode`
:   fwnode\_handle to report the event on

`enum drm_connector_status status`
:   hot plug detect logical state

**Description**

On some hardware a hotplug event notification may come from outside the display
driver / device. An example of this is some USB Type-C setups where the hardware
muxes the DisplayPort data and aux-lines but does not pass the altmode HPD
status bit to the GPU’s DP HPD pin.

This function can be used to report these out-of-band events after obtaining
a drm\_connector reference through calling `drm_connector_find_by_fwnode()`.

void drm\_mode\_put\_tile\_group(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_tile\_group](#c.drm_tile_group "drm_tile_group") \*tg)
:   drop a reference to a tile group.

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_tile_group *tg`
:   tile group to drop reference to.

**Description**

drop reference to tile group and free if 0.

struct [drm\_tile\_group](#c.drm_tile_group "drm_tile_group") \*drm\_mode\_get\_tile\_group(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, const char topology[8])
:   get a reference to an existing tile group

**Parameters**

`struct drm_device *dev`
:   DRM device

`const char topology[8]`
:   8-bytes unique per monitor.

**Description**

Use the unique bytes to get a reference to an existing tile group.

**Return**

tile group or NULL if not found.

struct [drm\_tile\_group](#c.drm_tile_group "drm_tile_group") \*drm\_mode\_create\_tile\_group(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, const char topology[8])
:   create a tile group from a displayid description

**Parameters**

`struct drm_device *dev`
:   DRM device

`const char topology[8]`
:   8-bytes unique per monitor.

**Description**

Create a tile group for the unique monitor, and get a unique
identifier for the tile group.

**Return**

new tile group or NULL.

void drm\_connector\_attach\_panel\_type\_property(struct [drm\_connector](#c.drm_connector "drm_connector") \*connector)
:   attaches panel type property

**Parameters**

`struct drm_connector *connector`
:   connector to attach the property on.

**Description**

This is used to add support for panel type detection.

### Writeback Connectors

Writeback connectors are used to expose hardware which can write the output
from a CRTC to a memory buffer. They are used and act similarly to other
types of connectors, with some important differences:

* Writeback connectors don’t provide a way to output visually to the user.
* Writeback connectors are visible to userspace only when the client sets
  DRM\_CLIENT\_CAP\_WRITEBACK\_CONNECTORS.
* Writeback connectors don’t have EDID.

A framebuffer may only be attached to a writeback connector when the
connector is attached to a CRTC. The WRITEBACK\_FB\_ID property which sets the
framebuffer applies only to a single commit (see below). A framebuffer may
not be attached while the CRTC is off.

Unlike with planes, when a writeback framebuffer is removed by userspace DRM
makes no attempt to remove it from active use by the connector. This is
because no method is provided to abort a writeback operation, and in any
case making a new commit whilst a writeback is ongoing is undefined (see
WRITEBACK\_OUT\_FENCE\_PTR below). As soon as the current writeback is finished,
the framebuffer will automatically no longer be in active use. As it will
also have already been removed from the framebuffer list, there will be no
way for any userspace application to retrieve a reference to it in the
intervening period.

Writeback connectors have some additional properties, which userspace
can use to query and control them:

> “WRITEBACK\_FB\_ID”:
> :   Write-only object property storing a DRM\_MODE\_OBJECT\_FB: it stores the
>     framebuffer to be written by the writeback connector. This property is
>     similar to the FB\_ID property on planes, but will always read as zero
>     and is not preserved across commits.
>     Userspace must set this property to an output buffer every time it
>     wishes the buffer to get filled.
>
> “WRITEBACK\_PIXEL\_FORMATS”:
> :   Immutable blob property to store the supported pixel formats table. The
>     data is an array of u32 DRM\_FORMAT\_\* fourcc values.
>     Userspace can use this blob to find out what pixel formats are supported
>     by the connector’s writeback engine.
>
> “WRITEBACK\_OUT\_FENCE\_PTR”:
> :   Userspace can use this property to provide a pointer for the kernel to
>     fill with a sync\_file file descriptor, which will signal once the
>     writeback is finished. The value should be the address of a 32-bit
>     signed integer, cast to a u64.
>     Userspace should wait for this fence to signal before making another
>     commit affecting any of the same CRTCs, Planes or Connectors.
>     **Failure to do so will result in undefined behaviour.**
>     For this reason it is strongly recommended that all userspace
>     applications making use of writeback connectors *always* retrieve an
>     out-fence for the commit and use it appropriately.
>     From userspace, this property will always read as zero.

struct drm\_writeback\_connector
:   DRM writeback connector

**Definition**:

```
struct drm_writeback_connector {
    struct drm_connector base;
    struct drm_encoder encoder;
    struct drm_property_blob *pixel_formats_blob_ptr;
    spinlock_t job_lock;
    struct list_head job_queue;
    unsigned int fence_context;
    spinlock_t fence_lock;
    unsigned long fence_seqno;
    char timeline_name[32];
};
```

**Members**

`base`
:   base drm\_connector object

`encoder`
:   Internal encoder used by the connector to fulfill
    the DRM framework requirements. The users of the
    **drm\_writeback\_connector** control the behaviour of the **encoder**
    by passing the **enc\_funcs** parameter to [`drm_writeback_connector_init()`](#c.drm_writeback_connector_init "drm_writeback_connector_init")
    function.
    For users of [`drm_writeback_connector_init_with_encoder()`](#c.drm_writeback_connector_init_with_encoder "drm_writeback_connector_init_with_encoder"), this field
    is not valid as the encoder is managed within their drivers.

`pixel_formats_blob_ptr`
:   DRM blob property data for the pixel formats list on writeback
    connectors
    See also [`drm_writeback_connector_init()`](#c.drm_writeback_connector_init "drm_writeback_connector_init")

`job_lock`
:   Protects job\_queue

`job_queue`
:   Holds a list of a connector’s writeback jobs; the last item is the
    most recent. The first item may be either waiting for the hardware
    to begin writing, or currently being written.

    See also: [`drm_writeback_queue_job()`](#c.drm_writeback_queue_job "drm_writeback_queue_job") and
    [`drm_writeback_signal_completion()`](#c.drm_writeback_signal_completion "drm_writeback_signal_completion")

`fence_context`
:   timeline context used for fence operations.

`fence_lock`
:   spinlock to protect the fences in the fence\_context.

`fence_seqno`
:   Seqno variable used as monotonic counter for the fences
    created on the connector’s timeline.

`timeline_name`
:   The name of the connector’s fence timeline.

struct drm\_writeback\_job
:   DRM writeback job

**Definition**:

```
struct drm_writeback_job {
    struct drm_writeback_connector *connector;
    bool prepared;
    struct work_struct cleanup_work;
    struct list_head list_entry;
    struct drm_framebuffer *fb;
    struct dma_fence *out_fence;
    void *priv;
};
```

**Members**

`connector`
:   Back-pointer to the writeback connector associated with the job

`prepared`
:   Set when the job has been prepared with `drm_writeback_prepare_job()`

`cleanup_work`
:   Used to allow drm\_writeback\_signal\_completion to defer dropping the
    framebuffer reference to a workqueue

`list_entry`
:   List item for the writeback connector’s **job\_queue**

`fb`
:   Framebuffer to be written to by the writeback connector. Do not set
    directly, use `drm_writeback_set_fb()`

`out_fence`
:   Fence which will signal once the writeback has completed

`priv`
:   Driver-private data

int drm\_writeback\_connector\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_writeback\_connector](#c.drm_writeback_connector "drm_writeback_connector") \*wb\_connector, const struct [drm\_connector\_funcs](#c.drm_connector_funcs "drm_connector_funcs") \*con\_funcs, const struct [drm\_encoder\_helper\_funcs](drm-kms-helpers.html#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") \*enc\_helper\_funcs, const u32 \*formats, int n\_formats, u32 possible\_crtcs)
:   Initialize a writeback connector and its properties

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_writeback_connector *wb_connector`
:   Writeback connector to initialize

`const struct drm_connector_funcs *con_funcs`
:   Connector funcs vtable

`const struct drm_encoder_helper_funcs *enc_helper_funcs`
:   Encoder helper funcs vtable to be used by the internal encoder

`const u32 *formats`
:   Array of supported pixel formats for the writeback engine

`int n_formats`
:   Length of the formats array

`u32 possible_crtcs`
:   possible crtcs for the internal writeback encoder

**Description**

This function creates the writeback-connector-specific properties if they
have not been already created, initializes the connector as
type DRM\_MODE\_CONNECTOR\_WRITEBACK, and correctly initializes the property
values. It will also create an internal encoder associated with the
drm\_writeback\_connector and set it to use the **enc\_helper\_funcs** vtable for
the encoder helper.

Drivers should always use this function instead of [`drm_connector_init()`](#c.drm_connector_init "drm_connector_init") to
set up writeback connectors.

**Return**

0 on success, or a negative error code

int drm\_writeback\_connector\_init\_with\_encoder(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_writeback\_connector](#c.drm_writeback_connector "drm_writeback_connector") \*wb\_connector, struct [drm\_encoder](#c.drm_encoder "drm_encoder") \*enc, const struct [drm\_connector\_funcs](#c.drm_connector_funcs "drm_connector_funcs") \*con\_funcs, const u32 \*formats, int n\_formats)
:   Initialize a writeback connector with a custom encoder

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_writeback_connector *wb_connector`
:   Writeback connector to initialize

`struct drm_encoder *enc`
:   handle to the already initialized drm encoder

`const struct drm_connector_funcs *con_funcs`
:   Connector funcs vtable

`const u32 *formats`
:   Array of supported pixel formats for the writeback engine

`int n_formats`
:   Length of the formats array

**Description**

This function creates the writeback-connector-specific properties if they
have not been already created, initializes the connector as
type DRM\_MODE\_CONNECTOR\_WRITEBACK, and correctly initializes the property
values.

This function assumes that the drm\_writeback\_connector’s encoder has already been
created and initialized before invoking this function.

In addition, this function also assumes that callers of this API will manage
assigning the encoder helper functions, possible\_crtcs and any other encoder
specific operation.

Drivers should always use this function instead of [`drm_connector_init()`](#c.drm_connector_init "drm_connector_init") to
set up writeback connectors if they want to manage themselves the lifetime of the
associated encoder.

**Return**

0 on success, or a negative error code

int drmm\_writeback\_connector\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_writeback\_connector](#c.drm_writeback_connector "drm_writeback_connector") \*wb\_connector, const struct [drm\_connector\_funcs](#c.drm_connector_funcs "drm_connector_funcs") \*con\_funcs, struct [drm\_encoder](#c.drm_encoder "drm_encoder") \*enc, const u32 \*formats, int n\_formats)
:   Initialize a writeback connector with a custom encoder

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_writeback_connector *wb_connector`
:   Writeback connector to initialize

`const struct drm_connector_funcs *con_funcs`
:   Connector funcs vtable

`struct drm_encoder *enc`
:   Encoder to connect this writeback connector

`const u32 *formats`
:   Array of supported pixel formats for the writeback engine

`int n_formats`
:   Length of the formats array

**Description**

This function initialize a writeback connector and register its cleanup.

This function creates the writeback-connector-specific properties if they
have not been already created, initializes the connector as
type DRM\_MODE\_CONNECTOR\_WRITEBACK, and correctly initializes the property
values.

**Return**

0 on success, or a negative error code

void drm\_writeback\_queue\_job(struct [drm\_writeback\_connector](#c.drm_writeback_connector "drm_writeback_connector") \*wb\_connector, struct [drm\_connector\_state](#c.drm_connector_state "drm_connector_state") \*conn\_state)
:   Queue a writeback job for later signalling

**Parameters**

`struct drm_writeback_connector *wb_connector`
:   The writeback connector to queue a job on

`struct drm_connector_state *conn_state`
:   The connector state containing the job to queue

**Description**

This function adds the job contained in **conn\_state** to the job\_queue for a
writeback connector. It takes ownership of the writeback job and sets the
**conn\_state->writeback\_job** to NULL, and so no access to the job may be
performed by the caller after this function returns.

Drivers must ensure that for a given writeback connector, jobs are queued in
exactly the same order as they will be completed by the hardware (and
signaled via drm\_writeback\_signal\_completion).

For every call to [`drm_writeback_queue_job()`](#c.drm_writeback_queue_job "drm_writeback_queue_job") there must be exactly one call to
[`drm_writeback_signal_completion()`](#c.drm_writeback_signal_completion "drm_writeback_signal_completion")

See also: [`drm_writeback_signal_completion()`](#c.drm_writeback_signal_completion "drm_writeback_signal_completion")

void drm\_writeback\_signal\_completion(struct [drm\_writeback\_connector](#c.drm_writeback_connector "drm_writeback_connector") \*wb\_connector, int status)
:   Signal the completion of a writeback job

**Parameters**

`struct drm_writeback_connector *wb_connector`
:   The writeback connector whose job is complete

`int status`
:   Status code to set in the writeback out\_fence (0 for success)

**Description**

Drivers should call this to signal the completion of a previously queued
writeback job. It should be called as soon as possible after the hardware
has finished writing, and may be called from interrupt context.
It is the driver’s responsibility to ensure that for a given connector, the
hardware completes writeback jobs in the same order as they are queued.

Unless the driver is holding its own reference to the framebuffer, it must
not be accessed after calling this function.

See also: [`drm_writeback_queue_job()`](#c.drm_writeback_queue_job "drm_writeback_queue_job")

## Encoder Abstraction

Encoders represent the connecting element between the CRTC (as the overall
pixel pipeline, represented by [`struct drm_crtc`](#c.drm_crtc "drm_crtc")) and the connectors (as the
generic sink entity, represented by [`struct drm_connector`](#c.drm_connector "drm_connector")). An encoder takes
pixel data from a CRTC and converts it to a format suitable for any attached
connector. Encoders are objects exposed to userspace, originally to allow
userspace to infer cloning and connector/CRTC restrictions. Unfortunately
almost all drivers get this wrong, making the uabi pretty much useless. On
top of that the exposed restrictions are too simple for today’s hardware, and
the recommended way to infer restrictions is by using the
DRM\_MODE\_ATOMIC\_TEST\_ONLY flag for the atomic IOCTL.

Otherwise encoders aren’t used in the uapi at all (any modeset request from
userspace directly connects a connector with a CRTC), drivers are therefore
free to use them however they wish. Modeset helper libraries make strong use
of encoders to facilitate code sharing. But for more complex settings it is
usually better to move shared code into a separate [`drm_bridge`](drm-kms-helpers.html#c.drm_bridge "drm_bridge"). Compared to
encoders, bridges also have the benefit of being purely an internal
abstraction since they are not exposed to userspace at all.

Encoders are initialized with [`drm_encoder_init()`](#c.drm_encoder_init "drm_encoder_init") and cleaned up using
[`drm_encoder_cleanup()`](#c.drm_encoder_cleanup "drm_encoder_cleanup").

### Encoder Functions Reference

struct drm\_encoder\_funcs
:   encoder controls

**Definition**:

```
struct drm_encoder_funcs {
    void (*reset)(struct drm_encoder *encoder);
    void (*destroy)(struct drm_encoder *encoder);
    int (*late_register)(struct drm_encoder *encoder);
    void (*early_unregister)(struct drm_encoder *encoder);
    void (*debugfs_init)(struct drm_encoder *encoder, struct dentry *root);
};
```

**Members**

`reset`
:   Reset encoder hardware and software state to off. This function isn’t
    called by the core directly, only through [`drm_mode_config_reset()`](#c.drm_mode_config_reset "drm_mode_config_reset").
    It’s not a helper hook only for historical reasons.

`destroy`
:   Clean up encoder resources. This is only called at driver unload time
    through [`drm_mode_config_cleanup()`](#c.drm_mode_config_cleanup "drm_mode_config_cleanup") since an encoder cannot be
    hotplugged in DRM.

`late_register`
:   This optional hook can be used to register additional userspace
    interfaces attached to the encoder.
    It is called late in the driver load sequence from [`drm_dev_register()`](drm-internals.html#c.drm_dev_register "drm_dev_register").
    Everything added from this callback should be unregistered in
    the early\_unregister callback.

    Returns:

    0 on success, or a negative error code on failure.

`early_unregister`
:   This optional hook should be used to unregister the additional
    userspace interfaces attached to the encoder from
    **late\_register**. It is called from [`drm_dev_unregister()`](drm-internals.html#c.drm_dev_unregister "drm_dev_unregister"),
    early in the driver unload sequence to disable userspace access
    before data structures are torndown.

`debugfs_init`
:   Allows encoders to create encoder-specific debugfs files.

**Description**

Encoders sit between CRTCs and connectors.

struct drm\_encoder
:   central DRM encoder structure

**Definition**:

```
struct drm_encoder {
    struct drm_device *dev;
    struct list_head head;
    struct drm_mode_object base;
    char *name;
    int encoder_type;
    unsigned index;
    uint32_t possible_crtcs;
    uint32_t possible_clones;
    struct drm_crtc *crtc;
    struct list_head bridge_chain;
    const struct drm_encoder_funcs *funcs;
    const struct drm_encoder_helper_funcs *helper_private;
    struct dentry *debugfs_entry;
};
```

**Members**

`dev`
:   parent DRM device

`head`
:   list management

`base`
:   base KMS object

`name`
:   human readable name, can be overwritten by the driver

`encoder_type`
:   One of the DRM\_MODE\_ENCODER\_<foo> types in drm\_mode.h. The following
    encoder types are defined thus far:

    * DRM\_MODE\_ENCODER\_DAC for VGA and analog on DVI-I/DVI-A.
    * DRM\_MODE\_ENCODER\_TMDS for DVI, HDMI and (embedded) DisplayPort.
    * DRM\_MODE\_ENCODER\_LVDS for display panels, or in general any panel
      with a proprietary parallel connector.
    * DRM\_MODE\_ENCODER\_TVDAC for TV output (Composite, S-Video,
      Component, SCART).
    * DRM\_MODE\_ENCODER\_VIRTUAL for virtual machine displays
    * DRM\_MODE\_ENCODER\_DSI for panels connected using the DSI serial bus.
    * DRM\_MODE\_ENCODER\_DPI for panels connected using the DPI parallel
      bus.
    * DRM\_MODE\_ENCODER\_DPMST for special fake encoders used to allow
      mutliple DP MST streams to share one physical encoder.

`index`
:   Position inside the mode\_config.list, can be used as an array
    index. It is invariant over the lifetime of the encoder.

`possible_crtcs`
:   Bitmask of potential CRTC bindings, using
    [`drm_crtc_index()`](#c.drm_crtc_index "drm_crtc_index") as the index into the bitfield. The driver must set
    the bits for all [`drm_crtc`](#c.drm_crtc "drm_crtc") objects this encoder can be connected to
    before calling [`drm_dev_register()`](drm-internals.html#c.drm_dev_register "drm_dev_register").

    You will get a WARN if you get this wrong in the driver.

    Note that since CRTC objects can’t be hotplugged the assigned indices
    are stable and hence known before registering all objects.

`possible_clones`
:   Bitmask of potential sibling encoders for cloning,
    using [`drm_encoder_index()`](#c.drm_encoder_index "drm_encoder_index") as the index into the bitfield. The driver
    must set the bits for all [`drm_encoder`](#c.drm_encoder "drm_encoder") objects which can clone a
    [`drm_crtc`](#c.drm_crtc "drm_crtc") together with this encoder before calling
    [`drm_dev_register()`](drm-internals.html#c.drm_dev_register "drm_dev_register"). Drivers should set the bit representing the
    encoder itself, too. Cloning bits should be set such that when two
    encoders can be used in a cloned configuration, they both should have
    each another bits set.

    As an exception to the above rule if the driver doesn’t implement
    any cloning it can leave **possible\_clones** set to 0. The core will
    automagically fix this up by setting the bit for the encoder itself.

    You will get a WARN if you get this wrong in the driver.

    Note that since encoder objects can’t be hotplugged the assigned indices
    are stable and hence known before registering all objects.

`crtc`
:   Currently bound CRTC, only really meaningful for non-atomic
    drivers. Atomic drivers should instead check
    [`drm_connector_state.crtc`](#c.drm_connector_state "drm_connector_state").

`bridge_chain`
:   Bridges attached to this encoder. Drivers shall not
    access this field directly.

`funcs`
:   control functions, can be NULL for simple managed encoders

`helper_private`
:   mid-layer private data

`debugfs_entry`
:   Debugfs directory for this CRTC.

**Description**

CRTCs drive pixels to encoders, which convert them into signals
appropriate for a given connector or set of connectors.

drmm\_encoder\_alloc

`drmm_encoder_alloc (dev, type, member, funcs, encoder_type, name, ...)`

> Allocate and initialize an encoder

**Parameters**

`dev`
:   drm device

`type`
:   the type of the struct which contains struct [`drm_encoder`](#c.drm_encoder "drm_encoder")

`member`
:   the name of the [`drm_encoder`](#c.drm_encoder "drm_encoder") within **type**

`funcs`
:   callbacks for this encoder (optional)

`encoder_type`
:   user visible type of the encoder

`name`
:   printf style format string for the encoder name, or NULL for default name

`...`
:   variable arguments

**Description**

Allocates and initializes an encoder. Encoder should be subclassed as part of
driver encoder objects. Cleanup is automatically handled through registering
[`drm_encoder_cleanup()`](#c.drm_encoder_cleanup "drm_encoder_cleanup") with [`drmm_add_action()`](drm-internals.html#c.drmm_add_action "drmm_add_action").

The **drm\_encoder\_funcs.destroy** hook must be NULL.

**Return**

Pointer to new encoder, or ERR\_PTR on failure.

drmm\_plain\_encoder\_alloc

`drmm_plain_encoder_alloc (dev, funcs, encoder_type, name, ...)`

> Allocate and initialize an encoder

**Parameters**

`dev`
:   drm device

`funcs`
:   callbacks for this encoder (optional)

`encoder_type`
:   user visible type of the encoder

`name`
:   printf style format string for the encoder name, or NULL for default name

`...`
:   variable arguments

**Description**

This is a simplified version of [`drmm_encoder_alloc()`](#c.drmm_encoder_alloc "drmm_encoder_alloc"), which only allocates
and returns a [`struct drm_encoder`](#c.drm_encoder "drm_encoder") instance, with no subclassing.

**Return**

Pointer to the new drm\_encoder struct, or ERR\_PTR on failure.

unsigned int drm\_encoder\_index(const struct [drm\_encoder](#c.drm_encoder "drm_encoder") \*encoder)
:   find the index of a registered encoder

**Parameters**

`const struct drm_encoder *encoder`
:   encoder to find index for

**Description**

Given a registered encoder, return the index of that encoder within a DRM
device’s list of encoders.

u32 drm\_encoder\_mask(const struct [drm\_encoder](#c.drm_encoder "drm_encoder") \*encoder)
:   find the mask of a registered encoder

**Parameters**

`const struct drm_encoder *encoder`
:   encoder to find mask for

**Description**

Given a registered encoder, return the mask bit of that encoder for an
encoder’s possible\_clones field.

bool drm\_encoder\_crtc\_ok(struct [drm\_encoder](#c.drm_encoder "drm_encoder") \*encoder, struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   can a given crtc drive a given encoder?

**Parameters**

`struct drm_encoder *encoder`
:   encoder to test

`struct drm_crtc *crtc`
:   crtc to test

**Description**

Returns false if **encoder** can’t be driven by **crtc**, true otherwise.

struct [drm\_encoder](#c.drm_encoder "drm_encoder") \*drm\_encoder\_find(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_priv, uint32\_t id)
:   find a [`drm_encoder`](#c.drm_encoder "drm_encoder")

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_file *file_priv`
:   drm file to check for lease against.

`uint32_t id`
:   encoder id

**Description**

Returns the encoder with **id**, NULL if it doesn’t exist. Simple wrapper around
[`drm_mode_object_find()`](#c.drm_mode_object_find "drm_mode_object_find").

drm\_for\_each\_encoder\_mask

`drm_for_each_encoder_mask (encoder, dev, encoder_mask)`

> iterate over encoders specified by bitmask

**Parameters**

`encoder`
:   the loop cursor

`dev`
:   the DRM device

`encoder_mask`
:   bitmask of encoder indices

**Description**

Iterate over all encoders specified by bitmask.

drm\_for\_each\_encoder

`drm_for_each_encoder (encoder, dev)`

> iterate over all encoders

**Parameters**

`encoder`
:   the loop cursor

`dev`
:   the DRM device

**Description**

Iterate over all encoders of **dev**.

int drm\_encoder\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_encoder](#c.drm_encoder "drm_encoder") \*encoder, const struct [drm\_encoder\_funcs](#c.drm_encoder_funcs "drm_encoder_funcs") \*funcs, int encoder\_type, const char \*name, ...)
:   Init a preallocated encoder

**Parameters**

`struct drm_device *dev`
:   drm device

`struct drm_encoder *encoder`
:   the encoder to init

`const struct drm_encoder_funcs *funcs`
:   callbacks for this encoder

`int encoder_type`
:   user visible type of the encoder

`const char *name`
:   printf style format string for the encoder name, or NULL for default name

`...`
:   variable arguments

**Description**

Initializes a preallocated encoder. Encoder should be subclassed as part of
driver encoder objects. At driver unload time the driver’s
[`drm_encoder_funcs.destroy`](#c.drm_encoder_funcs "drm_encoder_funcs") hook should call [`drm_encoder_cleanup()`](#c.drm_encoder_cleanup "drm_encoder_cleanup") and [`kfree()`](../core-api/mm-api.html#c.kfree "kfree")
the encoder structure. The encoder structure should not be allocated with
`devm_kzalloc()`.

**Note**

consider using [`drmm_encoder_alloc()`](#c.drmm_encoder_alloc "drmm_encoder_alloc") or [`drmm_encoder_init()`](#c.drmm_encoder_init "drmm_encoder_init")
instead of [`drm_encoder_init()`](#c.drm_encoder_init "drm_encoder_init") to let the DRM managed resource
infrastructure take care of cleanup and deallocation.

**Return**

Zero on success, error code on failure.

void drm\_encoder\_cleanup(struct [drm\_encoder](#c.drm_encoder "drm_encoder") \*encoder)
:   cleans up an initialised encoder

**Parameters**

`struct drm_encoder *encoder`
:   encoder to cleanup

**Description**

Cleans up the encoder but doesn’t free the object.

int drmm\_encoder\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_encoder](#c.drm_encoder "drm_encoder") \*encoder, const struct [drm\_encoder\_funcs](#c.drm_encoder_funcs "drm_encoder_funcs") \*funcs, int encoder\_type, const char \*name, ...)
:   Initialize a preallocated encoder

**Parameters**

`struct drm_device *dev`
:   drm device

`struct drm_encoder *encoder`
:   the encoder to init

`const struct drm_encoder_funcs *funcs`
:   callbacks for this encoder (optional)

`int encoder_type`
:   user visible type of the encoder

`const char *name`
:   printf style format string for the encoder name, or NULL for default name

`...`
:   variable arguments

**Description**

Initializes a preallocated encoder. Encoder should be subclassed as
part of driver encoder objects. Cleanup is automatically handled
through registering [`drm_encoder_cleanup()`](#c.drm_encoder_cleanup "drm_encoder_cleanup") with [`drmm_add_action()`](drm-internals.html#c.drmm_add_action "drmm_add_action"). The
encoder structure should be allocated with [`drmm_kzalloc()`](drm-internals.html#c.drmm_kzalloc "drmm_kzalloc").

The **drm\_encoder\_funcs.destroy** hook must be NULL.

**Return**

Zero on success, error code on failure.

## KMS Locking

As KMS moves toward more fine grained locking, and atomic ioctl where
userspace can indirectly control locking order, it becomes necessary
to use `ww_mutex` and acquire-contexts to avoid deadlocks. But because
the locking is more distributed around the driver code, we want a bit
of extra utility/tracking out of our acquire-ctx. This is provided
by [`struct drm_modeset_lock`](#c.drm_modeset_lock "drm_modeset_lock") and [`struct drm_modeset_acquire_ctx`](#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx").

For basic principles of `ww_mutex`, see: [Wound/Wait Deadlock-Proof Mutex Design](../locking/ww-mutex-design.html)

The basic usage pattern is to:

```
drm_modeset_acquire_init(ctx, DRM_MODESET_ACQUIRE_INTERRUPTIBLE)
retry:
foreach (lock in random_ordered_set_of_locks) {
    ret = drm_modeset_lock(lock, ctx)
    if (ret == -EDEADLK) {
        ret = drm_modeset_backoff(ctx);
        if (!ret)
            goto retry;
    }
    if (ret)
        goto out;
}
... do stuff ...
out:
drm_modeset_drop_locks(ctx);
drm_modeset_acquire_fini(ctx);
```

For convenience this control flow is implemented in
[`DRM_MODESET_LOCK_ALL_BEGIN()`](#c.DRM_MODESET_LOCK_ALL_BEGIN "DRM_MODESET_LOCK_ALL_BEGIN") and [`DRM_MODESET_LOCK_ALL_END()`](#c.DRM_MODESET_LOCK_ALL_END "DRM_MODESET_LOCK_ALL_END") for the case
where all modeset locks need to be taken through [`drm_modeset_lock_all_ctx()`](#c.drm_modeset_lock_all_ctx "drm_modeset_lock_all_ctx").

If all that is needed is a single modeset lock, then the [`struct
drm_modeset_acquire_ctx`](#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") is not needed and the locking can be simplified
by passing a NULL instead of ctx in the [`drm_modeset_lock()`](#c.drm_modeset_lock "drm_modeset_lock") call or
calling [`drm_modeset_lock_single_interruptible()`](#c.drm_modeset_lock_single_interruptible "drm_modeset_lock_single_interruptible"). To unlock afterwards
call [`drm_modeset_unlock()`](#c.drm_modeset_unlock "drm_modeset_unlock").

On top of these per-object locks using `ww_mutex` there’s also an overall
[`drm_mode_config.mutex`](#c.drm_mode_config "drm_mode_config"), for protecting everything else. Mostly this means
probe state of connectors, and preventing hotplug add/removal of connectors.

Finally there’s a bunch of dedicated locks to protect drm core internal
lists and lookup data structures.

struct drm\_modeset\_acquire\_ctx
:   locking context (see ww\_acquire\_ctx)

**Definition**:

```
struct drm_modeset_acquire_ctx {
    struct ww_acquire_ctx ww_ctx;
    struct drm_modeset_lock *contended;
    depot_stack_handle_t stack_depot;
    struct list_head locked;
    bool trylock_only;
    bool interruptible;
};
```

**Members**

`ww_ctx`
:   base acquire ctx

`contended`
:   used internally for -EDEADLK handling

`stack_depot`
:   used internally for contention debugging

`locked`
:   list of held locks

`trylock_only`
:   trylock mode used in atomic contexts/panic notifiers

`interruptible`
:   whether interruptible locking should be used.

**Description**

Each thread competing for a set of locks must use one acquire
ctx. And if any lock fxn returns -EDEADLK, it must backoff and
retry.

struct drm\_modeset\_lock
:   used for locking modeset resources.

**Definition**:

```
struct drm_modeset_lock {
    struct ww_mutex mutex;
    struct list_head head;
};
```

**Members**

`mutex`
:   resource locking

`head`
:   used to hold its place on `drm_atomi_state.locked` list when
    part of an atomic update

**Description**

Used for locking CRTCs and other modeset resources.

void drm\_modeset\_lock\_fini(struct [drm\_modeset\_lock](#c.drm_modeset_lock "drm_modeset_lock") \*lock)
:   cleanup lock

**Parameters**

`struct drm_modeset_lock *lock`
:   lock to cleanup

bool drm\_modeset\_is\_locked(struct [drm\_modeset\_lock](#c.drm_modeset_lock "drm_modeset_lock") \*lock)
:   equivalent to [`mutex_is_locked()`](../kernel-hacking/locking.html#c.mutex_is_locked "mutex_is_locked")

**Parameters**

`struct drm_modeset_lock *lock`
:   lock to check

void drm\_modeset\_lock\_assert\_held(struct [drm\_modeset\_lock](#c.drm_modeset_lock "drm_modeset_lock") \*lock)
:   equivalent to `lockdep_assert_held()`

**Parameters**

`struct drm_modeset_lock *lock`
:   lock to check

DRM\_MODESET\_LOCK\_ALL\_BEGIN

`DRM_MODESET_LOCK_ALL_BEGIN (dev, ctx, flags, ret)`

> Helper to acquire modeset locks

**Parameters**

`dev`
:   drm device

`ctx`
:   local modeset acquire context, will be dereferenced

`flags`
:   DRM\_MODESET\_ACQUIRE\_\* flags to pass to [`drm_modeset_acquire_init()`](#c.drm_modeset_acquire_init "drm_modeset_acquire_init")

`ret`
:   local ret/err/etc variable to track error status

**Description**

Use these macros to simplify grabbing all modeset locks using a local
context. This has the advantage of reducing boilerplate, but also properly
checking return values where appropriate.

Any code run between BEGIN and END will be holding the modeset locks.

This must be paired with [`DRM_MODESET_LOCK_ALL_END()`](#c.DRM_MODESET_LOCK_ALL_END "DRM_MODESET_LOCK_ALL_END"). We will jump back and
forth between the labels on deadlock and error conditions.

Drivers can acquire additional modeset locks. If any lock acquisition
fails, the control flow needs to jump to [`DRM_MODESET_LOCK_ALL_END()`](#c.DRM_MODESET_LOCK_ALL_END "DRM_MODESET_LOCK_ALL_END") with
the **ret** parameter containing the return value of [`drm_modeset_lock()`](#c.drm_modeset_lock "drm_modeset_lock").

**Return**

The only possible value of ret immediately after [`DRM_MODESET_LOCK_ALL_BEGIN()`](#c.DRM_MODESET_LOCK_ALL_BEGIN "DRM_MODESET_LOCK_ALL_BEGIN")
is 0, so no error checking is necessary

DRM\_MODESET\_LOCK\_ALL\_END

`DRM_MODESET_LOCK_ALL_END (dev, ctx, ret)`

> Helper to release and cleanup modeset locks

**Parameters**

`dev`
:   drm device

`ctx`
:   local modeset acquire context, will be dereferenced

`ret`
:   local ret/err/etc variable to track error status

**Description**

The other side of [`DRM_MODESET_LOCK_ALL_BEGIN()`](#c.DRM_MODESET_LOCK_ALL_BEGIN "DRM_MODESET_LOCK_ALL_BEGIN"). It will bounce back to BEGIN
if ret is -EDEADLK.

It’s important that you use the same ret variable for begin and end so
deadlock conditions are properly handled.

**Return**

ret will be untouched unless it is -EDEADLK on entry. That means that if you
successfully acquire the locks, ret will be whatever your code sets it to. If
there is a deadlock or other failure with acquire or backoff, ret will be set
to that failure. In both of these cases the code between BEGIN/END will not
be run, so the failure will reflect the inability to grab the locks.

void drm\_modeset\_lock\_all(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   take all modeset locks

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

This function takes all modeset locks, suitable where a more fine-grained
scheme isn’t (yet) implemented. Locks must be dropped by calling the
[`drm_modeset_unlock_all()`](#c.drm_modeset_unlock_all "drm_modeset_unlock_all") function.

This function is deprecated. It allocates a lock acquisition context and
stores it in [`drm_device.mode_config`](drm-internals.html#c.drm_device "drm_device"). This facilitate conversion of
existing code because it removes the need to manually deal with the
acquisition context, but it is also brittle because the context is global
and care must be taken not to nest calls. New code should use the
[`drm_modeset_lock_all_ctx()`](#c.drm_modeset_lock_all_ctx "drm_modeset_lock_all_ctx") function and pass in the context explicitly.

void drm\_modeset\_unlock\_all(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   drop all modeset locks

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

This function drops all modeset locks taken by a previous call to the
[`drm_modeset_lock_all()`](#c.drm_modeset_lock_all "drm_modeset_lock_all") function.

This function is deprecated. It uses the lock acquisition context stored
in [`drm_device.mode_config`](drm-internals.html#c.drm_device "drm_device"). This facilitates conversion of existing
code because it removes the need to manually deal with the acquisition
context, but it is also brittle because the context is global and care must
be taken not to nest calls. New code should pass the acquisition context
directly to the [`drm_modeset_drop_locks()`](#c.drm_modeset_drop_locks "drm_modeset_drop_locks") function.

void drm\_warn\_on\_modeset\_not\_all\_locked(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   check that all modeset locks are locked

**Parameters**

`struct drm_device *dev`
:   device

**Description**

Useful as a debug assert.

void drm\_modeset\_acquire\_init(struct [drm\_modeset\_acquire\_ctx](#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx, uint32\_t flags)
:   initialize acquire context

**Parameters**

`struct drm_modeset_acquire_ctx *ctx`
:   the acquire context

`uint32_t flags`
:   0 or `DRM_MODESET_ACQUIRE_INTERRUPTIBLE`

**Description**

When passing `DRM_MODESET_ACQUIRE_INTERRUPTIBLE` to **flags**,
all calls to [`drm_modeset_lock()`](#c.drm_modeset_lock "drm_modeset_lock") will perform an interruptible
wait.

void drm\_modeset\_acquire\_fini(struct [drm\_modeset\_acquire\_ctx](#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx)
:   cleanup acquire context

**Parameters**

`struct drm_modeset_acquire_ctx *ctx`
:   the acquire context

void drm\_modeset\_drop\_locks(struct [drm\_modeset\_acquire\_ctx](#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx)
:   drop all locks

**Parameters**

`struct drm_modeset_acquire_ctx *ctx`
:   the acquire context

**Description**

Drop all locks currently held against this acquire context.

int drm\_modeset\_backoff(struct [drm\_modeset\_acquire\_ctx](#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx)
:   deadlock avoidance backoff

**Parameters**

`struct drm_modeset_acquire_ctx *ctx`
:   the acquire context

**Description**

If deadlock is detected (ie. [`drm_modeset_lock()`](#c.drm_modeset_lock "drm_modeset_lock") returns -EDEADLK),
you must call this function to drop all currently held locks and
block until the contended lock becomes available.

This function returns 0 on success, or -ERESTARTSYS if this context
is initialized with `DRM_MODESET_ACQUIRE_INTERRUPTIBLE` and the
wait has been interrupted.

void drm\_modeset\_lock\_init(struct [drm\_modeset\_lock](#c.drm_modeset_lock "drm_modeset_lock") \*lock)
:   initialize lock

**Parameters**

`struct drm_modeset_lock *lock`
:   lock to init

int drm\_modeset\_lock(struct [drm\_modeset\_lock](#c.drm_modeset_lock "drm_modeset_lock") \*lock, struct [drm\_modeset\_acquire\_ctx](#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx)
:   take modeset lock

**Parameters**

`struct drm_modeset_lock *lock`
:   lock to take

`struct drm_modeset_acquire_ctx *ctx`
:   acquire ctx

**Description**

If **ctx** is not NULL, then its ww acquire context is used and the
lock will be tracked by the context and can be released by calling
[`drm_modeset_drop_locks()`](#c.drm_modeset_drop_locks "drm_modeset_drop_locks"). If -EDEADLK is returned, this means a
deadlock scenario has been detected and it is an error to attempt
to take any more locks without first calling [`drm_modeset_backoff()`](#c.drm_modeset_backoff "drm_modeset_backoff").

If the **ctx** is not NULL and initialized with
`DRM_MODESET_ACQUIRE_INTERRUPTIBLE`, this function will fail with
-ERESTARTSYS when interrupted.

If **ctx** is NULL then the function call behaves like a normal,
uninterruptible non-nesting [`mutex_lock()`](../kernel-hacking/locking.html#c.mutex_lock "mutex_lock") call.

int drm\_modeset\_lock\_single\_interruptible(struct [drm\_modeset\_lock](#c.drm_modeset_lock "drm_modeset_lock") \*lock)
:   take a single modeset lock

**Parameters**

`struct drm_modeset_lock *lock`
:   lock to take

**Description**

This function behaves as [`drm_modeset_lock()`](#c.drm_modeset_lock "drm_modeset_lock") with a NULL context,
but performs interruptible waits.

This function returns 0 on success, or -ERESTARTSYS when interrupted.

void drm\_modeset\_unlock(struct [drm\_modeset\_lock](#c.drm_modeset_lock "drm_modeset_lock") \*lock)
:   drop modeset lock

**Parameters**

`struct drm_modeset_lock *lock`
:   lock to release

int drm\_modeset\_lock\_all\_ctx(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_modeset\_acquire\_ctx](#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx)
:   take all modeset locks

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_modeset_acquire_ctx *ctx`
:   lock acquisition context

**Description**

This function takes all modeset locks, suitable where a more fine-grained
scheme isn’t (yet) implemented.

Unlike [`drm_modeset_lock_all()`](#c.drm_modeset_lock_all "drm_modeset_lock_all"), it doesn’t take the [`drm_mode_config.mutex`](#c.drm_mode_config "drm_mode_config")
since that lock isn’t required for modeset state changes. Callers which
need to grab that lock too need to do so outside of the acquire context
**ctx**.

Locks acquired with this function should be released by calling the
[`drm_modeset_drop_locks()`](#c.drm_modeset_drop_locks "drm_modeset_drop_locks") function on **ctx**.

See also: [`DRM_MODESET_LOCK_ALL_BEGIN()`](#c.DRM_MODESET_LOCK_ALL_BEGIN "DRM_MODESET_LOCK_ALL_BEGIN") and [`DRM_MODESET_LOCK_ALL_END()`](#c.DRM_MODESET_LOCK_ALL_END "DRM_MODESET_LOCK_ALL_END")

**Return**

0 on success or a negative error-code on failure.

## KMS Properties

This section of the documentation is primarily aimed at user-space developers.
For the driver APIs, see the other sections.

### Requirements

KMS drivers might need to add extra properties to support new features. Each
new property introduced in a driver needs to meet a few requirements, in
addition to the one mentioned above:

* It must be standardized, documenting:

  + The full, exact, name string;
  + If the property is an enum, all the valid value name strings;
  + What values are accepted, and what these values mean;
  + What the property does and how it can be used;
  + How the property might interact with other, existing properties.
* It must provide a generic helper in the core code to register that
  property on the object it attaches to.
* Its content must be decoded by the core and provided in the object’s
  associated state structure. That includes anything drivers might want
  to precompute, like `struct drm_clip_rect` for planes.
* Its initial state must match the behavior prior to the property
  introduction. This might be a fixed value matching what the hardware
  does, or it may be inherited from the state the firmware left the
  system in during boot.
* An IGT test must be submitted where reasonable.

For historical reasons, non-standard, driver-specific properties exist. If a KMS
driver wants to add support for one of those properties, the requirements for
new properties apply where possible. Additionally, the documented behavior must
match the de facto semantics of the existing property to ensure compatibility.
Developers of the driver that first added the property should help with those
tasks and must ACK the documented behavior if possible.

### Property Types and Blob Property Support

Properties as represented by [`drm_property`](#c.drm_property "drm_property") are used to extend the modeset
interface exposed to userspace. For the atomic modeset IOCTL properties are
even the only way to transport metadata about the desired new modeset
configuration from userspace to the kernel. Properties have a well-defined
value range, which is enforced by the drm core. See the documentation of the
flags member of [`struct drm_property`](#c.drm_property "drm_property") for an overview of the different
property types and ranges.

Properties don’t store the current value directly, but need to be
instantiated by attaching them to a [`drm_mode_object`](#c.drm_mode_object "drm_mode_object") with
[`drm_object_attach_property()`](#c.drm_object_attach_property "drm_object_attach_property").

Property values are only 64bit. To support bigger piles of data (like gamma
tables, color correction matrices or large structures) a property can instead
point at a [`drm_property_blob`](#c.drm_property_blob "drm_property_blob") with that additional data.

Properties are defined by their symbolic name, userspace must keep a
per-object mapping from those names to the property ID used in the atomic
IOCTL and in the get/set property IOCTL.

struct drm\_property\_enum
:   symbolic values for enumerations

**Definition**:

```
struct drm_property_enum {
    uint64_t value;
    struct list_head head;
    char name[DRM_PROP_NAME_LEN];
};
```

**Members**

`value`
:   numeric property value for this `enum entry`

    If the property has the type `DRM_MODE_PROP_BITMASK`, **value** stores a
    bitshift, not a bitmask. In other words, the `enum entry` is enabled
    if the bit number **value** is set in the property’s value. This `enum
    entry` has the bitmask `1 << value`.

`head`
:   list of `enum values`, linked to [`drm_property.enum_list`](#c.drm_property "drm_property")

`name`
:   symbolic name for the enum

**Description**

For enumeration and bitmask properties this structure stores the symbolic
decoding for each value. This is used for example for the rotation property.

struct drm\_property
:   modeset object property

**Definition**:

```
struct drm_property {
    struct list_head head;
    struct drm_mode_object base;
    uint32_t flags;
    char name[DRM_PROP_NAME_LEN];
    uint32_t num_values;
    uint64_t *values;
    struct drm_device *dev;
    struct list_head enum_list;
};
```

**Members**

`head`
:   per-device list of properties, for cleanup.

`base`
:   base KMS object

`flags`
:   Property flags and type. A property needs to be one of the following
    types:

    DRM\_MODE\_PROP\_RANGE
    :   Range properties report their minimum and maximum admissible unsigned values.
        The KMS core verifies that values set by application fit in that
        range. The range is unsigned. Range properties are created using
        [`drm_property_create_range()`](#c.drm_property_create_range "drm_property_create_range").

    DRM\_MODE\_PROP\_SIGNED\_RANGE
    :   Range properties report their minimum and maximum admissible unsigned values.
        The KMS core verifies that values set by application fit in that
        range. The range is signed. Range properties are created using
        [`drm_property_create_signed_range()`](#c.drm_property_create_signed_range "drm_property_create_signed_range").

    DRM\_MODE\_PROP\_ENUM
    :   Enumerated properties take a numerical value that ranges from 0 to
        the number of enumerated values defined by the property minus one,
        and associate a free-formed string name to each value. Applications
        can retrieve the list of defined value-name pairs and use the
        numerical value to get and set property instance values. Enum
        properties are created using [`drm_property_create_enum()`](#c.drm_property_create_enum "drm_property_create_enum").

    DRM\_MODE\_PROP\_BITMASK
    :   Bitmask properties are enumeration properties that additionally
        restrict all enumerated values to the 0..63 range. Bitmask property
        instance values combine one or more of the enumerated bits defined
        by the property. Bitmask properties are created using
        [`drm_property_create_bitmask()`](#c.drm_property_create_bitmask "drm_property_create_bitmask").

    DRM\_MODE\_PROP\_OBJECT
    :   Object properties are used to link modeset objects. This is used
        extensively in the atomic support to create the display pipeline,
        by linking [`drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer") to [`drm_plane`](#c.drm_plane "drm_plane"), [`drm_plane`](#c.drm_plane "drm_plane") to
        [`drm_crtc`](#c.drm_crtc "drm_crtc") and [`drm_connector`](#c.drm_connector "drm_connector") to [`drm_crtc`](#c.drm_crtc "drm_crtc"). An object property can
        only link to a specific type of [`drm_mode_object`](#c.drm_mode_object "drm_mode_object"), this limit is
        enforced by the core. Object properties are created using
        [`drm_property_create_object()`](#c.drm_property_create_object "drm_property_create_object").

        Object properties work like blob properties, but in a more
        general fashion. They are limited to atomic drivers and must have
        the DRM\_MODE\_PROP\_ATOMIC flag set.

    DRM\_MODE\_PROP\_BLOB
    :   Blob properties store a binary blob without any format restriction.
        The binary blobs are created as KMS standalone objects, and blob
        property instance values store the ID of their associated blob
        object. Blob properties are created by calling
        [`drm_property_create()`](#c.drm_property_create "drm_property_create") with DRM\_MODE\_PROP\_BLOB as the type.

        Actual blob objects to contain blob data are created using
        [`drm_property_create_blob()`](#c.drm_property_create_blob "drm_property_create_blob"), or through the corresponding IOCTL.

        Besides the built-in limit to only accept blob objects blob
        properties work exactly like object properties. The only reasons
        blob properties exist is backwards compatibility with existing
        userspace.

    In addition a property can have any combination of the below flags:

    DRM\_MODE\_PROP\_ATOMIC
    :   Set for properties which encode atomic modeset state. Such
        properties are not exposed to legacy userspace.

    DRM\_MODE\_PROP\_IMMUTABLE
    :   Set for properties whose values cannot be changed by
        userspace. The kernel is allowed to update the value of these
        properties. This is generally used to expose probe state to
        userspace, e.g. the EDID, or the connector path property on DP
        MST sinks. Kernel can update the value of an immutable property
        by calling [`drm_object_property_set_value()`](#c.drm_object_property_set_value "drm_object_property_set_value").

`name`
:   symbolic name of the properties

`num_values`
:   size of the **values** array.

`values`
:   Array with limits and values for the property. The
    interpretation of these limits is dependent upon the type per **flags**.

`dev`
:   DRM device

`enum_list`
:   List of `drm_prop_enum_list` structures with the symbolic names for
    `enum and` bitmask values.

**Description**

This structure represent a modeset object property. It combines both the name
of the property with the set of permissible values. This means that when a
driver wants to use a property with the same name on different objects, but
with different value ranges, then it must create property for each one. An
example would be rotation of [`drm_plane`](#c.drm_plane "drm_plane"), when e.g. the primary plane cannot
be rotated. But if both the name and the value range match, then the same
property structure can be instantiated multiple times for the same object.
Userspace must be able to cope with this and cannot assume that the same
symbolic property will have the same modeset object ID on all modeset
objects.

Properties are created by one of the special functions, as explained in
detail in the **flags** structure member.

To actually expose a property it must be attached to each object using
[`drm_object_attach_property()`](#c.drm_object_attach_property "drm_object_attach_property"). Currently properties can only be attached to
[`drm_connector`](#c.drm_connector "drm_connector"), [`drm_crtc`](#c.drm_crtc "drm_crtc") and [`drm_plane`](#c.drm_plane "drm_plane").

Properties are also used as the generic metadatatransport for the atomic
IOCTL. Everything that was set directly in structures in the legacy modeset
IOCTLs (like the plane source or destination windows, or e.g. the links to
the CRTC) is exposed as a property with the DRM\_MODE\_PROP\_ATOMIC flag set.

struct drm\_property\_blob
:   Blob data for [`drm_property`](#c.drm_property "drm_property")

**Definition**:

```
struct drm_property_blob {
    struct drm_mode_object base;
    struct drm_device *dev;
    struct list_head head_global;
    struct list_head head_file;
    size_t length;
    void *data;
};
```

**Members**

`base`
:   base KMS object

`dev`
:   DRM device

`head_global`
:   entry on the global blob list in
    [`drm_mode_config.property_blob_list`](#c.drm_mode_config "drm_mode_config").

`head_file`
:   entry on the per-file blob list in [`drm_file.blobs`](drm-internals.html#c.drm_file "drm_file") list.

`length`
:   size of the blob in bytes, invariant over the lifetime of the object

`data`
:   actual data, embedded at the end of this structure

**Description**

Blobs are used to store bigger values than what fits directly into the 64
bits available for a [`drm_property`](#c.drm_property "drm_property").

Blobs are reference counted using [`drm_property_blob_get()`](#c.drm_property_blob_get "drm_property_blob_get") and
[`drm_property_blob_put()`](#c.drm_property_blob_put "drm_property_blob_put"). They are created using [`drm_property_create_blob()`](#c.drm_property_create_blob "drm_property_create_blob").

bool drm\_property\_type\_is(struct [drm\_property](#c.drm_property "drm_property") \*property, uint32\_t type)
:   check the type of a property

**Parameters**

`struct drm_property *property`
:   property to check

`uint32_t type`
:   property type to compare with

**Description**

This is a helper function becauase the uapi encoding of property types is
a bit special for historical reasons.

struct [drm\_property](#c.drm_property "drm_property") \*drm\_property\_find(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file\_priv, uint32\_t id)
:   find property object

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_file *file_priv`
:   drm file to check for lease against.

`uint32_t id`
:   property object id

**Description**

This function looks up the property object specified by id and returns it.

struct [drm\_property](#c.drm_property "drm_property") \*drm\_property\_create(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, u32 flags, const char \*name, int num\_values)
:   create a new property type

**Parameters**

`struct drm_device *dev`
:   drm device

`u32 flags`
:   flags specifying the property type

`const char *name`
:   name of the property

`int num_values`
:   number of pre-defined values

**Description**

This creates a new generic drm property which can then be attached to a drm
object with [`drm_object_attach_property()`](#c.drm_object_attach_property "drm_object_attach_property"). The returned property object must
be freed with [`drm_property_destroy()`](#c.drm_property_destroy "drm_property_destroy"), which is done automatically when
calling [`drm_mode_config_cleanup()`](#c.drm_mode_config_cleanup "drm_mode_config_cleanup").

**Return**

A pointer to the newly created property on success, NULL on failure.

struct [drm\_property](#c.drm_property "drm_property") \*drm\_property\_create\_enum(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, u32 flags, const char \*name, const struct drm\_prop\_enum\_list \*props, int num\_values)
:   create a new enumeration property type

**Parameters**

`struct drm_device *dev`
:   drm device

`u32 flags`
:   flags specifying the property type

`const char *name`
:   name of the property

`const struct drm_prop_enum_list *props`
:   enumeration lists with property values

`int num_values`
:   number of pre-defined values

**Description**

This creates a new generic drm property which can then be attached to a drm
object with [`drm_object_attach_property()`](#c.drm_object_attach_property "drm_object_attach_property"). The returned property object must
be freed with [`drm_property_destroy()`](#c.drm_property_destroy "drm_property_destroy"), which is done automatically when
calling [`drm_mode_config_cleanup()`](#c.drm_mode_config_cleanup "drm_mode_config_cleanup").

Userspace is only allowed to set one of the predefined values for enumeration
properties.

**Return**

A pointer to the newly created property on success, NULL on failure.

struct [drm\_property](#c.drm_property "drm_property") \*drm\_property\_create\_bitmask(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, u32 flags, const char \*name, const struct drm\_prop\_enum\_list \*props, int num\_props, uint64\_t supported\_bits)
:   create a new bitmask property type

**Parameters**

`struct drm_device *dev`
:   drm device

`u32 flags`
:   flags specifying the property type

`const char *name`
:   name of the property

`const struct drm_prop_enum_list *props`
:   enumeration lists with property bitflags

`int num_props`
:   size of the **props** array

`uint64_t supported_bits`
:   bitmask of all supported enumeration values

**Description**

This creates a new bitmask drm property which can then be attached to a drm
object with [`drm_object_attach_property()`](#c.drm_object_attach_property "drm_object_attach_property"). The returned property object must
be freed with [`drm_property_destroy()`](#c.drm_property_destroy "drm_property_destroy"), which is done automatically when
calling [`drm_mode_config_cleanup()`](#c.drm_mode_config_cleanup "drm_mode_config_cleanup").

Compared to plain enumeration properties userspace is allowed to set any
or’ed together combination of the predefined property bitflag values

**Return**

A pointer to the newly created property on success, NULL on failure.

struct [drm\_property](#c.drm_property "drm_property") \*drm\_property\_create\_range(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, u32 flags, const char \*name, uint64\_t min, uint64\_t max)
:   create a new unsigned ranged property type

**Parameters**

`struct drm_device *dev`
:   drm device

`u32 flags`
:   flags specifying the property type

`const char *name`
:   name of the property

`uint64_t min`
:   minimum value of the property

`uint64_t max`
:   maximum value of the property

**Description**

This creates a new generic drm property which can then be attached to a drm
object with [`drm_object_attach_property()`](#c.drm_object_attach_property "drm_object_attach_property"). The returned property object must
be freed with [`drm_property_destroy()`](#c.drm_property_destroy "drm_property_destroy"), which is done automatically when
calling [`drm_mode_config_cleanup()`](#c.drm_mode_config_cleanup "drm_mode_config_cleanup").

Userspace is allowed to set any unsigned integer value in the (min, max)
range inclusive.

**Return**

A pointer to the newly created property on success, NULL on failure.

struct [drm\_property](#c.drm_property "drm_property") \*drm\_property\_create\_signed\_range(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, u32 flags, const char \*name, int64\_t min, int64\_t max)
:   create a new signed ranged property type

**Parameters**

`struct drm_device *dev`
:   drm device

`u32 flags`
:   flags specifying the property type

`const char *name`
:   name of the property

`int64_t min`
:   minimum value of the property

`int64_t max`
:   maximum value of the property

**Description**

This creates a new generic drm property which can then be attached to a drm
object with [`drm_object_attach_property()`](#c.drm_object_attach_property "drm_object_attach_property"). The returned property object must
be freed with [`drm_property_destroy()`](#c.drm_property_destroy "drm_property_destroy"), which is done automatically when
calling [`drm_mode_config_cleanup()`](#c.drm_mode_config_cleanup "drm_mode_config_cleanup").

Userspace is allowed to set any signed integer value in the (min, max)
range inclusive.

**Return**

A pointer to the newly created property on success, NULL on failure.

struct [drm\_property](#c.drm_property "drm_property") \*drm\_property\_create\_object(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, u32 flags, const char \*name, uint32\_t type)
:   create a new object property type

**Parameters**

`struct drm_device *dev`
:   drm device

`u32 flags`
:   flags specifying the property type

`const char *name`
:   name of the property

`uint32_t type`
:   object type from DRM\_MODE\_OBJECT\_\* defines

**Description**

This creates a new generic drm property which can then be attached to a drm
object with [`drm_object_attach_property()`](#c.drm_object_attach_property "drm_object_attach_property"). The returned property object must
be freed with [`drm_property_destroy()`](#c.drm_property_destroy "drm_property_destroy"), which is done automatically when
calling [`drm_mode_config_cleanup()`](#c.drm_mode_config_cleanup "drm_mode_config_cleanup").

Userspace is only allowed to set this to any property value of the given
**type**. Only useful for atomic properties, which is enforced.

**Return**

A pointer to the newly created property on success, NULL on failure.

struct [drm\_property](#c.drm_property "drm_property") \*drm\_property\_create\_bool(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, u32 flags, const char \*name)
:   create a new boolean property type

**Parameters**

`struct drm_device *dev`
:   drm device

`u32 flags`
:   flags specifying the property type

`const char *name`
:   name of the property

**Description**

This creates a new generic drm property which can then be attached to a drm
object with [`drm_object_attach_property()`](#c.drm_object_attach_property "drm_object_attach_property"). The returned property object must
be freed with [`drm_property_destroy()`](#c.drm_property_destroy "drm_property_destroy"), which is done automatically when
calling [`drm_mode_config_cleanup()`](#c.drm_mode_config_cleanup "drm_mode_config_cleanup").

This is implemented as a ranged property with only {0, 1} as valid values.

**Return**

A pointer to the newly created property on success, NULL on failure.

int drm\_property\_add\_enum(struct [drm\_property](#c.drm_property "drm_property") \*property, uint64\_t value, const char \*name)
:   add a possible value to an enumeration property

**Parameters**

`struct drm_property *property`
:   enumeration property to change

`uint64_t value`
:   value of the new enumeration

`const char *name`
:   symbolic name of the new enumeration

**Description**

This functions adds enumerations to a property.

It’s use is deprecated, drivers should use one of the more specific helpers
to directly create the property with all enumerations already attached.

**Return**

Zero on success, error code on failure.

void drm\_property\_destroy(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_property](#c.drm_property "drm_property") \*property)
:   destroy a drm property

**Parameters**

`struct drm_device *dev`
:   drm device

`struct drm_property *property`
:   property to destroy

**Description**

This function frees a property including any attached resources like
enumeration values.

struct [drm\_property\_blob](#c.drm_property_blob "drm_property_blob") \*drm\_property\_create\_blob(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, size\_t length, const void \*data)
:   Create new blob property

**Parameters**

`struct drm_device *dev`
:   DRM device to create property for

`size_t length`
:   Length to allocate for blob data

`const void *data`
:   If specified, copies data into blob

**Description**

Creates a new blob property for a specified DRM device, optionally
copying data. Note that blob properties are meant to be invariant, hence the
data must be filled out before the blob is used as the value of any property.

**Return**

New blob property with a single reference on success, or an ERR\_PTR
value on failure.

void drm\_property\_blob\_put(struct [drm\_property\_blob](#c.drm_property_blob "drm_property_blob") \*blob)
:   release a blob property reference

**Parameters**

`struct drm_property_blob *blob`
:   DRM blob property

**Description**

Releases a reference to a blob property. May free the object.

struct [drm\_property\_blob](#c.drm_property_blob "drm_property_blob") \*drm\_property\_blob\_get(struct [drm\_property\_blob](#c.drm_property_blob "drm_property_blob") \*blob)
:   acquire blob property reference

**Parameters**

`struct drm_property_blob *blob`
:   DRM blob property

**Description**

Acquires a reference to an existing blob property. Returns **blob**, which
allows this to be used as a shorthand in assignments.

struct [drm\_property\_blob](#c.drm_property_blob "drm_property_blob") \*drm\_property\_lookup\_blob(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, uint32\_t id)
:   look up a blob property and take a reference

**Parameters**

`struct drm_device *dev`
:   drm device

`uint32_t id`
:   id of the blob property

**Description**

If successful, this takes an additional reference to the blob property.
callers need to make sure to eventually unreferenced the returned property
again, using [`drm_property_blob_put()`](#c.drm_property_blob_put "drm_property_blob_put").

**Return**

NULL on failure, pointer to the blob on success.

int drm\_property\_replace\_global\_blob(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_property\_blob](#c.drm_property_blob "drm_property_blob") \*\*replace, size\_t length, const void \*data, struct [drm\_mode\_object](#c.drm_mode_object "drm_mode_object") \*obj\_holds\_id, struct [drm\_property](#c.drm_property "drm_property") \*prop\_holds\_id)
:   replace existing blob property

**Parameters**

`struct drm_device *dev`
:   drm device

`struct drm_property_blob **replace`
:   location of blob property pointer to be replaced

`size_t length`
:   length of data for new blob, or 0 for no data

`const void *data`
:   content for new blob, or NULL for no data

`struct drm_mode_object *obj_holds_id`
:   optional object for property holding blob ID

`struct drm_property *prop_holds_id`
:   optional property holding blob ID
    **return** 0 on success or error on failure

**Description**

This function will replace a global property in the blob list, optionally
updating a property which holds the ID of that property.

If length is 0 or data is NULL, no new blob will be created, and the holding
property, if specified, will be set to 0.

Access to the replace pointer is assumed to be protected by the caller, e.g.
by holding the relevant modesetting object lock for its parent.

For example, a drm\_connector has a ‘PATH’ property, which contains the ID
of a blob property with the value of the MST path information. Calling this
function with replace pointing to the connector’s path\_blob\_ptr, length and
data set for the new path information, obj\_holds\_id set to the connector’s
base object, and prop\_holds\_id set to the path property name, will perform
a completely atomic update. The access to path\_blob\_ptr is protected by the
caller holding a lock on the connector.

bool drm\_property\_replace\_blob(struct [drm\_property\_blob](#c.drm_property_blob "drm_property_blob") \*\*blob, struct [drm\_property\_blob](#c.drm_property_blob "drm_property_blob") \*new\_blob)
:   replace a blob property

**Parameters**

`struct drm_property_blob **blob`
:   a pointer to the member blob to be replaced

`struct drm_property_blob *new_blob`
:   the new blob to replace with

**Return**

true if the blob was in fact replaced.

int drm\_property\_replace\_blob\_from\_id(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_property\_blob](#c.drm_property_blob "drm_property_blob") \*\*blob, uint64\_t blob\_id, ssize\_t max\_size, ssize\_t expected\_size, ssize\_t expected\_elem\_size, bool \*replaced)
:   replace a blob property taking a reference

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_property_blob **blob`
:   a pointer to the member blob to be replaced

`uint64_t blob_id`
:   the id of the new blob to replace with

`ssize_t max_size`
:   the maximum size of the blob property for variable-size blobs

`ssize_t expected_size`
:   expected size of the blob property

`ssize_t expected_elem_size`
:   expected size of an element in the blob property

`bool *replaced`
:   if the blob was in fact replaced

**Description**

Look up the new blob from id, take its reference, check expected sizes of
the blob and its element and replace the old blob by the new one. Advertise
if the replacement operation was successful.

**Return**

true if the blob was in fact replaced. -EINVAL if the new blob was
not found or sizes don’t match.

### Standard Connector Properties

DRM connectors have a few standardized properties:

EDID:
:   Blob property which contains the current EDID read from the sink. This
    is useful to parse sink identification information like vendor, model
    and serial. Drivers should update this property by calling
    [`drm_connector_update_edid_property()`](drm-kms-helpers.html#c.drm_connector_update_edid_property "drm_connector_update_edid_property"), usually after having parsed
    the EDID using [`drm_add_edid_modes()`](drm-kms-helpers.html#c.drm_add_edid_modes "drm_add_edid_modes"). Userspace cannot change this
    property.

    User-space should not parse the EDID to obtain information exposed via
    other KMS properties (because the kernel might apply limits, quirks or
    fixups to the EDID). For instance, user-space should not try to parse
    mode lists from the EDID.

DPMS:
:   Legacy property for setting the power state of the connector. For atomic
    drivers this is only provided for backwards compatibility with existing
    drivers, it remaps to controlling the “ACTIVE” property on the CRTC the
    connector is linked to. Drivers should never set this property directly,
    it is handled by the DRM core by calling the [`drm_connector_funcs.dpms`](#c.drm_connector_funcs "drm_connector_funcs")
    callback. For atomic drivers the remapping to the “ACTIVE” property is
    implemented in the DRM core.

    On atomic drivers any DPMS setproperty ioctl where the value does not
    change is completely skipped, otherwise a full atomic commit will occur.
    On legacy drivers the exact behavior is driver specific.

    Note that this property cannot be set through the MODE\_ATOMIC ioctl,
    userspace must use “ACTIVE” on the CRTC instead.

    WARNING:

    For userspace also running on legacy drivers the “DPMS” semantics are a
    lot more complicated. First, userspace cannot rely on the “DPMS” value
    returned by the GETCONNECTOR actually reflecting reality, because many
    drivers fail to update it. For atomic drivers this is taken care of in
    [`drm_atomic_helper_update_legacy_modeset_state()`](drm-kms-helpers.html#c.drm_atomic_helper_update_legacy_modeset_state "drm_atomic_helper_update_legacy_modeset_state").

    The second issue is that the DPMS state is only well-defined when the
    connector is connected to a CRTC. In atomic the DRM core enforces that
    “ACTIVE” is off in such a case, no such checks exists for “DPMS”.

    Finally, when enabling an output using the legacy SETCONFIG ioctl then
    “DPMS” is forced to ON. But see above, that might not be reflected in
    the software value on legacy drivers.

    Summarizing: Only set “DPMS” when the connector is known to be enabled,
    assume that a successful SETCONFIG call also sets “DPMS” to on, and
    never read back the value of “DPMS” because it can be incorrect.

panel\_type:
:   Immutable `enum property` to indicate the type of connected panel.
    Possible values are “unknown” (default) and “OLED”.

PATH:
:   Connector path property to identify how this sink is physically
    connected. Used by DP MST. This should be set by calling
    [`drm_connector_set_path_property()`](#c.drm_connector_set_path_property "drm_connector_set_path_property"), in the case of DP MST with the
    path property the MST manager created. Userspace cannot change this
    property.

    In the case of DP MST, the property has the format
    `mst:<parent>-<ports>` where `<parent>` is the KMS object ID of the
    parent connector and `<ports>` is a hyphen-separated list of DP MST
    port numbers. Note, KMS object IDs are not guaranteed to be stable
    across reboots.

TILE:
:   Connector tile group property to indicate how a set of DRM connector
    compose together into one logical screen. This is used by both high-res
    external screens (often only using a single cable, but exposing multiple
    DP MST sinks), or high-res integrated panels (like dual-link DSI) which
    are not gen-locked. Note that for tiled panels which are genlocked, like
    dual-link LVDS or dual-link DSI, the driver should try to not expose the
    tiling and virtualise both [`drm_crtc`](#c.drm_crtc "drm_crtc") and [`drm_plane`](#c.drm_plane "drm_plane") if needed. Drivers
    should update this value using [`drm_connector_set_tile_property()`](#c.drm_connector_set_tile_property "drm_connector_set_tile_property").
    Userspace cannot change this property.

link-status:
:   Connector link-status property to indicate the status of link. The
    default value of link-status is “GOOD”. If something fails during or
    after modeset, the kernel driver may set this to “BAD” and issue a
    hotplug uevent. Drivers should update this value using
    [`drm_connector_set_link_status_property()`](#c.drm_connector_set_link_status_property "drm_connector_set_link_status_property").

    When user-space receives the hotplug uevent and detects a “BAD”
    link-status, the sink doesn’t receive pixels anymore (e.g. the screen
    becomes completely black). The list of available modes may have
    changed. User-space is expected to pick a new mode if the current one
    has disappeared and perform a new modeset with link-status set to
    “GOOD” to re-enable the connector.

    If multiple connectors share the same CRTC and one of them gets a “BAD”
    link-status, the other are unaffected (ie. the sinks still continue to
    receive pixels).

    When user-space performs an atomic commit on a connector with a “BAD”
    link-status without resetting the property to “GOOD”, the sink may
    still not receive pixels. When user-space performs an atomic commit
    which resets the link-status property to “GOOD” without the
    ALLOW\_MODESET flag set, it might fail because a modeset is required.

    User-space can only change link-status to “GOOD”, changing it to “BAD”
    is a no-op.

    For backwards compatibility with non-atomic userspace the kernel
    tries to automatically set the link-status back to “GOOD” in the
    SETCRTC IOCTL. This might fail if the mode is no longer valid, similar
    to how it might fail if a different screen has been connected in the
    interim.

non\_desktop:
:   Indicates the output should be ignored for purposes of displaying a
    standard desktop environment or console. This is most likely because
    the output device is not rectilinear.

Content Protection:
:   This property is used by userspace to request the kernel protect future
    content communicated over the link. When requested, kernel will apply
    the appropriate means of protection (most often HDCP), and use the
    property to tell userspace the protection is active.

    Drivers can set this up by calling
    [`drm_connector_attach_content_protection_property()`](drm-kms-helpers.html#c.drm_connector_attach_content_protection_property "drm_connector_attach_content_protection_property") on initialization.

    The value of this property can be one of the following:

    DRM\_MODE\_CONTENT\_PROTECTION\_UNDESIRED = 0
    :   The link is not protected, content is transmitted in the clear.

    DRM\_MODE\_CONTENT\_PROTECTION\_DESIRED = 1
    :   Userspace has requested content protection, but the link is not
        currently protected. When in this state, kernel should enable
        Content Protection as soon as possible.

    DRM\_MODE\_CONTENT\_PROTECTION\_ENABLED = 2
    :   Userspace has requested content protection, and the link is
        protected. Only the driver can set the property to this value.
        If userspace attempts to set to ENABLED, kernel will return
        -EINVAL.

    A few guidelines:

    * DESIRED state should be preserved until userspace de-asserts it by
      setting the property to UNDESIRED. This means ENABLED should only
      transition to UNDESIRED when the user explicitly requests it.
    * If the state is DESIRED, kernel should attempt to re-authenticate the
      link whenever possible. This includes across disable/enable, dpms,
      hotplug, downstream device changes, link status failures, etc..
    * Kernel sends uevent with the connector id and property id through
      **drm\_hdcp\_update\_content\_protection**, upon below kernel triggered
      scenarios:

      > + DESIRED -> ENABLED (authentication success)
      > + ENABLED -> DESIRED (termination of authentication)
    * Please note no uevents for userspace triggered property state changes,
      which can’t fail such as

      > + DESIRED/ENABLED -> UNDESIRED
      > + UNDESIRED -> DESIRED
    * Userspace is responsible for polling the property or listen to uevents
      to determine when the value transitions from ENABLED to DESIRED.
      This signifies the link is no longer protected and userspace should
      take appropriate action (whatever that might be).

HDCP Content Type:
:   This Enum property is used by the userspace to declare the content type
    of the display stream, to kernel. Here display stream stands for any
    display content that userspace intended to display through HDCP
    encryption.

    Content Type of a stream is decided by the owner of the stream, as
    “HDCP Type0” or “HDCP Type1”.

    The value of the property can be one of the below:
    :   * “HDCP Type0”: DRM\_MODE\_HDCP\_CONTENT\_TYPE0 = 0
        * “HDCP Type1”: DRM\_MODE\_HDCP\_CONTENT\_TYPE1 = 1

    When kernel starts the HDCP authentication (see “Content Protection”
    for details), it uses the content type in “HDCP Content Type”
    for performing the HDCP authentication with the display sink.

    Please note in HDCP spec versions, a link can be authenticated with
    HDCP 2.2 for Content Type 0/Content Type 1. Where as a link can be
    authenticated with HDCP1.4 only for Content Type 0(though it is implicit
    in nature. As there is no reference for Content Type in HDCP1.4).

    HDCP2.2 authentication protocol itself takes the “Content Type” as a
    parameter, which is a input for the DP HDCP2.2 encryption algo.

    In case of Type 0 content protection request, kernel driver can choose
    either of HDCP spec versions 1.4 and 2.2. When HDCP2.2 is used for
    “HDCP Type 0”, a HDCP 2.2 capable repeater in the downstream can send
    that content to a HDCP 1.4 authenticated HDCP sink (Type0 link).
    But if the content is classified as “HDCP Type 1”, above mentioned
    HDCP 2.2 repeater wont send the content to the HDCP sink as it can’t
    authenticate the HDCP1.4 capable sink for “HDCP Type 1”.

    Please note userspace can be ignorant of the HDCP versions used by the
    kernel driver to achieve the “HDCP Content Type”.

    At current scenario, classifying a content as Type 1 ensures that the
    content will be displayed only through the HDCP2.2 encrypted link.

    Note that the HDCP Content Type property is introduced at HDCP 2.2, and
    defaults to type 0. It is only exposed by drivers supporting HDCP 2.2
    (hence supporting Type 0 and Type 1). Based on how next versions of
    HDCP specs are defined content Type could be used for higher versions
    too.

    If content type is changed when “Content Protection” is not UNDESIRED,
    then kernel will disable the HDCP and re-enable with new type in the
    same atomic commit. And when “Content Protection” is ENABLED, it means
    that link is HDCP authenticated and encrypted, for the transmission of
    the Type of stream mentioned at “HDCP Content Type”.

HDR\_OUTPUT\_METADATA:
:   Connector property to enable userspace to send HDR Metadata to
    driver. This metadata is based on the composition and blending
    policies decided by user, taking into account the hardware and
    sink capabilities. The driver gets this metadata and creates a
    Dynamic Range and Mastering Infoframe (DRM) in case of HDMI,
    SDP packet (Non-audio INFOFRAME SDP v1.3) for DP. This is then
    sent to sink. This notifies the sink of the upcoming frame’s Color
    Encoding and Luminance parameters.

    Userspace first need to detect the HDR capabilities of sink by
    reading and parsing the EDID. Details of HDR metadata for HDMI
    are added in CTA 861.G spec. For DP , its defined in VESA DP
    Standard v1.4. It needs to then get the metadata information
    of the video/game/app content which are encoded in HDR (basically
    using HDR transfer functions). With this information it needs to
    decide on a blending policy and compose the relevant
    layers/overlays into a common format. Once this blending is done,
    userspace will be aware of the metadata of the composed frame to
    be send to sink. It then uses this property to communicate this
    metadata to driver which then make a Infoframe packet and sends
    to sink based on the type of encoder connected.

    Userspace will be responsible to do Tone mapping operation in case:
    :   * Some layers are HDR and others are SDR
        * HDR layers luminance is not same as sink

    It will even need to do colorspace conversion and get all layers
    to one common colorspace for blending. It can use either GL, Media
    or display engine to get this done based on the capabilities of the
    associated hardware.

    Driver expects metadata to be put in [`struct hdr_output_metadata`](drm-uapi.html#c.hdr_output_metadata "hdr_output_metadata")
    structure from userspace. This is received as blob and stored in
    [`drm_connector_state.hdr_output_metadata`](#c.drm_connector_state "drm_connector_state"). It parses EDID and saves the
    sink metadata in [`struct hdr_sink_metadata`](drm-kms-helpers.html#c.hdr_sink_metadata "hdr_sink_metadata"), as
    [`drm_connector.display_info`](#c.drm_connector "drm_connector").hdr\_sink\_metadata. Driver uses
    `drm_hdmi_infoframe_set_hdr_metadata()` helper to set the HDR metadata,
    [`hdmi_drm_infoframe_pack()`](drm-kms-helpers.html#c.hdmi_drm_infoframe_pack "hdmi_drm_infoframe_pack") to pack the infoframe as per spec, in case of
    HDMI encoder.

max bpc:
:   This range property is used by userspace to limit the bit depth. When
    used the driver would limit the bpc in accordance with the valid range
    supported by the hardware and sink. Drivers to use the function
    [`drm_connector_attach_max_bpc_property()`](#c.drm_connector_attach_max_bpc_property "drm_connector_attach_max_bpc_property") to create and attach the
    property to the connector during initialization.

Connectors also have one standardized atomic property:

CRTC\_ID:
:   Mode object ID of the [`drm_crtc`](#c.drm_crtc "drm_crtc") this connector should be connected to.

Connectors for LCD panels may also have one standardized property:

panel orientation:
:   On some devices the LCD panel is mounted in the casing in such a way
    that the up/top side of the panel does not match with the top side of
    the device. Userspace can use this property to check for this.
    Note that input coordinates from touchscreens (input devices with
    INPUT\_PROP\_DIRECT) will still map 1:1 to the actual LCD panel
    coordinates, so if userspace rotates the picture to adjust for
    the orientation it must also apply the same transformation to the
    touchscreen input coordinates. This property is initialized by calling
    [`drm_connector_set_panel_orientation()`](#c.drm_connector_set_panel_orientation "drm_connector_set_panel_orientation") or
    [`drm_connector_set_panel_orientation_with_quirk()`](#c.drm_connector_set_panel_orientation_with_quirk "drm_connector_set_panel_orientation_with_quirk")

scaling mode:
:   This property defines how a non-native mode is upscaled to the native
    mode of an LCD panel:

    None:
    :   No upscaling happens, scaling is left to the panel. Not all
        drivers expose this mode.

    Full:
    :   The output is upscaled to the full resolution of the panel,
        ignoring the aspect ratio.

    Center:
    :   No upscaling happens, the output is centered within the native
        resolution the panel.

    Full aspect:
    :   The output is upscaled to maximize either the width or height
        while retaining the aspect ratio.

    This property should be set up by calling
    [`drm_connector_attach_scaling_mode_property()`](#c.drm_connector_attach_scaling_mode_property "drm_connector_attach_scaling_mode_property"). Note that drivers
    can also expose this property to external outputs, in which case they
    must support “None”, which should be the default (since external screens
    have a built-in scaler).

subconnector:
:   This property is used by DVI-I, TVout and DisplayPort to indicate different
    connector subtypes. Enum values more or less match with those from main
    connector types.
    For DVI-I and TVout there is also a matching property “select subconnector”
    allowing to switch between signal types.
    DP subconnector corresponds to a downstream port.

privacy-screen sw-state, privacy-screen hw-state:
:   These 2 optional properties can be used to query the state of the
    electronic privacy screen that is available on some displays; and in
    some cases also control the state. If a driver implements these
    properties then both properties must be present.

    “privacy-screen hw-state” is read-only and reflects the actual state
    of the privacy-screen, possible values: “Enabled”, “Disabled,
    “Enabled-locked”, “Disabled-locked”. The locked states indicate
    that the state cannot be changed through the DRM API. E.g. there
    might be devices where the firmware-setup options, or a hardware
    slider-switch, offer always on / off modes.

    “privacy-screen sw-state” can be set to change the privacy-screen state
    when not locked. In this case the driver must update the hw-state
    property to reflect the new state on completion of the commit of the
    sw-state property. Setting the sw-state property when the hw-state is
    locked must be interpreted by the driver as a request to change the
    state to the set state when the hw-state becomes unlocked. E.g. if
    “privacy-screen hw-state” is “Enabled-locked” and the sw-state
    gets set to “Disabled” followed by the user unlocking the state by
    changing the slider-switch position, then the driver must set the
    state to “Disabled” upon receiving the unlock event.

    In some cases the privacy-screen’s actual state might change outside of
    control of the DRM code. E.g. there might be a firmware handled hotkey
    which toggles the actual state, or the actual state might be changed
    through another userspace API such as writing /proc/acpi/ibm/lcdshadow.
    In this case the driver must update both the hw-state and the sw-state
    to reflect the new value, overwriting any pending state requests in the
    sw-state. Any pending sw-state requests are thus discarded.

    Note that the ability for the state to change outside of control of
    the DRM master process means that userspace must not cache the value
    of the sw-state. Caching the sw-state value and including it in later
    atomic commits may lead to overriding a state change done through e.g.
    a firmware handled hotkey. Therefor userspace must not include the
    privacy-screen sw-state in an atomic commit unless it wants to change
    its value.

left margin, right margin, top margin, bottom margin:
:   Add margins to the connector’s viewport. This is typically used to
    mitigate overscan on TVs.

    The value is the size in pixels of the black border which will be
    added. The attached CRTC’s content will be scaled to fill the whole
    area inside the margin.

    The margins configuration might be sent to the sink, e.g. via HDMI AVI
    InfoFrames.

    Drivers can set up these properties by calling
    [`drm_mode_create_tv_margin_properties()`](#c.drm_mode_create_tv_margin_properties "drm_mode_create_tv_margin_properties").

Colorspace:
:   This property is used to inform the driver about the color encoding
    user space configured the pixel operation properties to produce.
    The variants set the colorimetry, transfer characteristics, and which
    YCbCr conversion should be used when necessary.
    The transfer characteristics from HDR\_OUTPUT\_METADATA takes precedence
    over this property.
    User space always configures the pixel operation properties to produce
    full quantization range data (see the Broadcast RGB property).

    Drivers inform the sink about what colorimetry, transfer
    characteristics, YCbCr conversion, and quantization range to expect
    (this can depend on the output mode, output format and other
    properties). Drivers also convert the user space provided data to what
    the sink expects.

    User space has to check if the sink supports all of the possible
    colorimetries that the driver is allowed to pick by parsing the EDID.

    For historical reasons this property exposes a number of variants which
    result in undefined behavior.

    Default:
    :   The behavior is driver-specific.

    BT2020\_RGB:

    BT2020\_YCC:
    :   User space configures the pixel operation properties to produce
        RGB content with Rec. ITU-R BT.2020 colorimetry, Rec.
        ITU-R BT.2020 (Table 4, RGB) transfer characteristics and full
        quantization range.
        User space can use the HDR\_OUTPUT\_METADATA property to set the
        transfer characteristics to PQ (Rec. ITU-R BT.2100 Table 4) or
        HLG (Rec. ITU-R BT.2100 Table 5) in which case, user space
        configures pixel operation properties to produce content with
        the respective transfer characteristics.
        User space has to make sure the sink supports Rec.
        ITU-R BT.2020 R’G’B’ and Rec. ITU-R BT.2020 Y’C’BC’R
        colorimetry.
        Drivers can configure the sink to use an RGB format, tell the
        sink to expect Rec. ITU-R BT.2020 R’G’B’ colorimetry and convert
        to the appropriate quantization range.
        Drivers can configure the sink to use a YCbCr format, tell the
        sink to expect Rec. ITU-R BT.2020 Y’C’BC’R colorimetry, convert
        to YCbCr using the Rec. ITU-R BT.2020 non-constant luminance
        conversion matrix and convert to the appropriate quantization
        range.
        The variants BT2020\_RGB and BT2020\_YCC are equivalent and the
        driver chooses between RGB and YCbCr on its own.

    SMPTE\_170M\_YCC:
    BT709\_YCC:
    XVYCC\_601:
    XVYCC\_709:
    SYCC\_601:
    opYCC\_601:
    opRGB:
    BT2020\_CYCC:
    DCI-P3\_RGB\_D65:
    DCI-P3\_RGB\_Theater:
    RGB\_WIDE\_FIXED:
    RGB\_WIDE\_FLOAT:

    BT601\_YCC:
    :   The behavior is undefined.

Because between HDMI and DP have different colorspaces,
[`drm_mode_create_hdmi_colorspace_property()`](#c.drm_mode_create_hdmi_colorspace_property "drm_mode_create_hdmi_colorspace_property") is used for HDMI connector and
[`drm_mode_create_dp_colorspace_property()`](#c.drm_mode_create_dp_colorspace_property "drm_mode_create_dp_colorspace_property") is used for DP connector.

### HDMI Specific Connector Properties

Broadcast RGB (HDMI specific)
:   Indicates the Quantization Range (Full vs Limited) used. The color
    processing pipeline will be adjusted to match the value of the
    property, and the Infoframes will be generated and sent accordingly.

    This property is only relevant if the HDMI output format is RGB. If
    it’s one of the YCbCr variant, it will be ignored.

    The CRTC attached to the connector must be configured by user-space to
    always produce full-range pixels.

    The value of this property can be one of the following:

    Automatic:
    :   The quantization range is selected automatically based on the
        mode according to the HDMI specifications (HDMI 1.4b - Section
        6.6 - Video Quantization Ranges).

    Full:
    :   Full quantization range is forced.

    Limited 16:235:
    :   Limited quantization range is forced. Unlike the name suggests,
        this works for any number of bits-per-component.

    Property values other than Automatic can result in colors being off (if
    limited is selected but the display expects full), or a black screen
    (if full is selected but the display expects limited).

    Drivers can set up this property by calling
    [`drm_connector_attach_broadcast_rgb_property()`](#c.drm_connector_attach_broadcast_rgb_property "drm_connector_attach_broadcast_rgb_property").

content type (HDMI specific):
:   Indicates content type setting to be used in HDMI infoframes to indicate
    content type for the external device, so that it adjusts its display
    settings accordingly.

    The value of this property can be one of the following:

    No Data:
    :   Content type is unknown

    Graphics:
    :   Content type is graphics

    Photo:
    :   Content type is photo

    Cinema:
    :   Content type is cinema

    Game:
    :   Content type is game

    The meaning of each content type is defined in CTA-861-G table 15.

    Drivers can set up this property by calling
    [`drm_connector_attach_content_type_property()`](#c.drm_connector_attach_content_type_property "drm_connector_attach_content_type_property"). Decoding to
    infoframe values is done through `drm_hdmi_avi_infoframe_content_type()`.

### Analog TV Specific Connector Properties

TV Mode:
:   Indicates the TV Mode used on an analog TV connector. The value
    of this property can be one of the following:

    NTSC:
    :   TV Mode is CCIR System M (aka 525-lines) together with
        the NTSC Color Encoding.

    NTSC-443:

    > TV Mode is CCIR System M (aka 525-lines) together with
    > the NTSC Color Encoding, but with a color subcarrier
    > frequency of 4.43MHz

    NTSC-J:

    > TV Mode is CCIR System M (aka 525-lines) together with
    > the NTSC Color Encoding, but with a black level equal to
    > the blanking level.

    PAL:

    > TV Mode is CCIR System B (aka 625-lines) together with
    > the PAL Color Encoding.

    PAL-M:

    > TV Mode is CCIR System M (aka 525-lines) together with
    > the PAL Color Encoding.

    PAL-N:

    > TV Mode is CCIR System N together with the PAL Color
    > Encoding, a color subcarrier frequency of 3.58MHz, the
    > SECAM color space, and narrower channels than other PAL
    > variants.

    SECAM:

    > TV Mode is CCIR System B (aka 625-lines) together with
    > the SECAM Color Encoding.

    Mono:

    > Use timings appropriate to the DRM mode, including
    > equalizing pulses for a 525-line or 625-line mode,
    > with no pedestal or color encoding.

    Drivers can set up this property by calling
    [`drm_mode_create_tv_properties()`](#c.drm_mode_create_tv_properties "drm_mode_create_tv_properties").

### Standard CRTC Properties

DRM CRTCs have a few standardized properties:

ACTIVE:
:   Atomic property for setting the power state of the CRTC. When set to 1
    the CRTC will actively display content. When set to 0 the CRTC will be
    powered off. There is no expectation that user-space will reset CRTC
    resources like the mode and planes when setting ACTIVE to 0.

    User-space can rely on an ACTIVE change to 1 to never fail an atomic
    test as long as no other property has changed. If a change to ACTIVE
    fails an atomic test, this is a driver bug. For this reason setting
    ACTIVE to 0 must not release internal resources (like reserved memory
    bandwidth or clock generators).

    Note that the legacy DPMS property on connectors is internally routed
    to control this property for atomic drivers.

MODE\_ID:
:   Atomic property for setting the CRTC display timings. The value is the
    ID of a blob containing the DRM mode info. To disable the CRTC,
    user-space must set this property to 0.

    Setting MODE\_ID to 0 will release reserved resources for the CRTC.

SCALING\_FILTER:
:   Atomic property for setting the scaling filter for CRTC scaler

    The value of this property can be one of the following:

    Default:
    :   Driver’s default scaling filter

    Nearest Neighbor:
    :   Nearest Neighbor scaling filter

SHARPNESS\_STRENGTH:
:   Atomic property for setting the sharpness strength/intensity by userspace.

    The value of this property is set as an integer value ranging
    from 0 - 255 where:

    0: Sharpness feature is disabled(default value).

    1: Minimum sharpness.

    255: Maximum sharpness.

    User can gradually increase or decrease the sharpness level and can
    set the optimum value depending on content.
    This value will be passed to kernel through the UAPI.
    The setting of this property does not require modeset.
    The sharpness effect takes place post blending on the final composed output.
    If the feature is disabled, the content remains same without any sharpening effect
    and when this feature is applied, it enhances the clarity of the content.

### Standard Plane Properties

DRM planes have a few standardized properties:

type:
:   Immutable property describing the type of the plane.

    For user-space which has enabled the [`DRM_CLIENT_CAP_ATOMIC`](drm-uapi.html#c.DRM_CLIENT_CAP_ATOMIC "DRM_CLIENT_CAP_ATOMIC") capability,
    the plane type is just a hint and is mostly superseded by atomic
    test-only commits. The type hint can still be used to come up more
    easily with a plane configuration accepted by the driver.

    The value of this property can be one of the following:

    “Primary”:
    :   To light up a CRTC, attaching a primary plane is the most likely to
        work if it covers the whole CRTC and doesn’t have scaling or
        cropping set up.

        Drivers may support more features for the primary plane, user-space
        can find out with test-only atomic commits.

        Some primary planes are implicitly used by the kernel in the legacy
        IOCTLs `DRM_IOCTL_MODE_SETCRTC` and `DRM_IOCTL_MODE_PAGE_FLIP`.
        Therefore user-space must not mix explicit usage of any primary
        plane (e.g. through an atomic commit) with these legacy IOCTLs.

    “Cursor”:
    :   To enable this plane, using a framebuffer configured without scaling
        or cropping and with the following properties is the most likely to
        work:

        * If the driver provides the capabilities [`DRM_CAP_CURSOR_WIDTH`](drm-uapi.html#c.DRM_CAP_CURSOR_WIDTH "DRM_CAP_CURSOR_WIDTH") and
          [`DRM_CAP_CURSOR_HEIGHT`](drm-uapi.html#c.DRM_CAP_CURSOR_HEIGHT "DRM_CAP_CURSOR_HEIGHT"), create the framebuffer with this size.
          Otherwise, create a framebuffer with the size 64x64.
        * If the driver doesn’t support modifiers, create a framebuffer with
          a linear layout. Otherwise, use the IN\_FORMATS plane property.

        Drivers may support more features for the cursor plane, user-space
        can find out with test-only atomic commits.

        Some cursor planes are implicitly used by the kernel in the legacy
        IOCTLs `DRM_IOCTL_MODE_CURSOR` and `DRM_IOCTL_MODE_CURSOR2`.
        Therefore user-space must not mix explicit usage of any cursor
        plane (e.g. through an atomic commit) with these legacy IOCTLs.

        Some drivers may support cursors even if no cursor plane is exposed.
        In this case, the legacy cursor IOCTLs can be used to configure the
        cursor.

    “Overlay”:
    :   Neither primary nor cursor.

        Overlay planes are the only planes exposed when the
        [`DRM_CLIENT_CAP_UNIVERSAL_PLANES`](drm-uapi.html#c.DRM_CLIENT_CAP_UNIVERSAL_PLANES "DRM_CLIENT_CAP_UNIVERSAL_PLANES") capability is disabled.

IN\_FORMATS:
:   Blob property which contains the set of buffer format and modifier
    pairs supported by this plane. The blob is a `struct
    drm_format_modifier_blob`. Without this property the plane doesn’t
    support buffers with modifiers. Userspace cannot change this property.

    Note that userspace can check the [`DRM_CAP_ADDFB2_MODIFIERS`](drm-uapi.html#c.DRM_CAP_ADDFB2_MODIFIERS "DRM_CAP_ADDFB2_MODIFIERS") driver
    capability for general modifier support. If this flag is set then every
    plane will have the IN\_FORMATS property, even when it only supports
    DRM\_FORMAT\_MOD\_LINEAR. Before linux kernel release v5.1 there have been
    various bugs in this area with inconsistencies between the capability
    flag and per-plane properties.

IN\_FORMATS\_ASYNC:
:   Blob property which contains the set of buffer format and modifier
    pairs supported by this plane for asynchronous flips. The blob is a `struct
    drm_format_modifier_blob`. Userspace cannot change this property. This is an
    optional property and if not present then user should expect a failure in
    atomic ioctl when the modifier/format is not supported by that plane under
    asynchronous flip.

SIZE\_HINTS:
:   Blob property which contains the set of recommended plane size
    which can used for simple “cursor like” use cases (eg. no scaling).
    Using these hints frees userspace from extensive probing of
    supported plane sizes through atomic/setcursor ioctls.

    The blob contains an array of [`struct drm_plane_size_hint`](drm-uapi.html#c.drm_plane_size_hint "drm_plane_size_hint"), in
    order of preference. For optimal usage userspace should pick
    the first size that satisfies its own requirements.

    Drivers should only attach this property to planes that
    support a very limited set of sizes.

    Note that property value 0 (ie. no blob) is reserved for potential
    future use. Current userspace is expected to ignore the property
    if the value is 0, and fall back to some other means (eg.
    [`DRM_CAP_CURSOR_WIDTH`](drm-uapi.html#c.DRM_CAP_CURSOR_WIDTH "DRM_CAP_CURSOR_WIDTH") and [`DRM_CAP_CURSOR_HEIGHT`](drm-uapi.html#c.DRM_CAP_CURSOR_HEIGHT "DRM_CAP_CURSOR_HEIGHT")) to determine
    the appropriate plane size to use.

### Plane Composition Properties

The basic plane composition model supported by standard plane properties only
has a source rectangle (in logical pixels within the [`drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer")), with
sub-pixel accuracy, which is scaled up to a pixel-aligned destination
rectangle in the visible area of a [`drm_crtc`](#c.drm_crtc "drm_crtc"). The visible area of a CRTC is
defined by the horizontal and vertical visible pixels (stored in **hdisplay**
and **vdisplay**) of the requested mode (stored in [`drm_crtc_state.mode`](#c.drm_crtc_state "drm_crtc_state")). These
two rectangles are both stored in the [`drm_plane_state`](#c.drm_plane_state "drm_plane_state").

For the atomic ioctl the following standard (atomic) properties on the plane object
encode the basic plane composition model:

SRC\_X:
:   X coordinate offset for the source rectangle within the
    [`drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer"), in 16.16 fixed point. Must be positive.

SRC\_Y:
:   Y coordinate offset for the source rectangle within the
    [`drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer"), in 16.16 fixed point. Must be positive.

SRC\_W:
:   Width for the source rectangle within the [`drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer"), in 16.16
    fixed point. SRC\_X plus SRC\_W must be within the width of the source
    framebuffer. Must be positive.

SRC\_H:
:   Height for the source rectangle within the [`drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer"), in 16.16
    fixed point. SRC\_Y plus SRC\_H must be within the height of the source
    framebuffer. Must be positive.

CRTC\_X:
:   X coordinate offset for the destination rectangle. Can be negative.

CRTC\_Y:
:   Y coordinate offset for the destination rectangle. Can be negative.

CRTC\_W:
:   Width for the destination rectangle. CRTC\_X plus CRTC\_W can extend past
    the currently visible horizontal area of the [`drm_crtc`](#c.drm_crtc "drm_crtc").

CRTC\_H:
:   Height for the destination rectangle. CRTC\_Y plus CRTC\_H can extend past
    the currently visible vertical area of the [`drm_crtc`](#c.drm_crtc "drm_crtc").

FB\_ID:
:   Mode object ID of the [`drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer") this plane should scan out.

    When a KMS client is performing front-buffer rendering, it should set
    FB\_ID to the same front-buffer FB on each atomic commit. This implies
    to the driver that it needs to re-read the same FB again. Otherwise
    drivers which do not employ continuously repeated scanout cycles might
    not update the screen.

CRTC\_ID:
:   Mode object ID of the [`drm_crtc`](#c.drm_crtc "drm_crtc") this plane should be connected to.

Note that the source rectangle must fully lie within the bounds of the
[`drm_framebuffer`](#c.drm_framebuffer "drm_framebuffer"). The destination rectangle can lie outside of the visible
area of the current mode of the CRTC. It must be appropriately clipped by the
driver, which can be done by calling `drm_plane_helper_check_update()`. Drivers
are also allowed to round the subpixel sampling positions appropriately, but
only to the next full pixel. No pixel outside of the source rectangle may
ever be sampled, which is important when applying more sophisticated
filtering than just a bilinear one when scaling. The filtering mode when
scaling is unspecified.

On top of this basic transformation additional properties can be exposed by
the driver:

alpha:
:   Alpha is setup with [`drm_plane_create_alpha_property()`](#c.drm_plane_create_alpha_property "drm_plane_create_alpha_property"). It controls the
    plane-wide opacity, from transparent (0) to opaque (0xffff). It can be
    combined with pixel alpha.
    The pixel values in the framebuffers are expected to not be
    pre-multiplied by the global alpha associated to the plane.

rotation:
:   Rotation is set up with [`drm_plane_create_rotation_property()`](#c.drm_plane_create_rotation_property "drm_plane_create_rotation_property"). It adds a
    rotation and reflection step between the source and destination rectangles.
    Without this property the rectangle is only scaled, but not rotated or
    reflected.

    Possbile values:

    “rotate-<degrees>”:
    :   Signals that a drm plane is rotated <degrees> degrees in counter
        clockwise direction.

    “reflect-<axis>”:
    :   Signals that the contents of a drm plane is reflected along the
        <axis> axis, in the same way as mirroring.

    reflect-x:

    ```
    |o |    | o|
    |  | -> |  |
    | v|    |v |
    ```

    reflect-y:

    ```
    |o |    | ^|
    |  | -> |  |
    | v|    |o |
    ```

zpos:
:   Z position is set up with [`drm_plane_create_zpos_immutable_property()`](#c.drm_plane_create_zpos_immutable_property "drm_plane_create_zpos_immutable_property") and
    [`drm_plane_create_zpos_property()`](#c.drm_plane_create_zpos_property "drm_plane_create_zpos_property"). It controls the visibility of overlapping
    planes. Without this property the primary plane is always below the cursor
    plane, and ordering between all other planes is undefined. The positive
    Z axis points towards the user, i.e. planes with lower Z position values
    are underneath planes with higher Z position values. Two planes with the
    same Z position value have undefined ordering. Note that the Z position
    value can also be immutable, to inform userspace about the hard-coded
    stacking of planes, see [`drm_plane_create_zpos_immutable_property()`](#c.drm_plane_create_zpos_immutable_property "drm_plane_create_zpos_immutable_property"). If
    any plane has a zpos property (either mutable or immutable), then all
    planes shall have a zpos property.

pixel blend mode:
:   Pixel blend mode is set up with [`drm_plane_create_blend_mode_property()`](#c.drm_plane_create_blend_mode_property "drm_plane_create_blend_mode_property").
    It adds a blend mode for alpha blending equation selection, describing
    how the pixels from the current plane are composited with the
    background.

    > Three alpha blending equations are defined:
    >
    > “None”:
    > :   Blend formula that ignores the pixel alpha:
    >
    >     ```
    >     out.rgb = plane_alpha * fg.rgb +
    >             (1 - plane_alpha) * bg.rgb
    >     ```
    >
    > “Pre-multiplied”:
    > :   Blend formula that assumes the pixel color values
    >     have been already pre-multiplied with the alpha
    >     channel values:
    >
    >     ```
    >     out.rgb = plane_alpha * fg.rgb +
    >             (1 - (plane_alpha * fg.alpha)) * bg.rgb
    >     ```
    >
    > “Coverage”:
    > :   Blend formula that assumes the pixel color values have not
    >     been pre-multiplied and will do so when blending them to the
    >     background color values:
    >
    >     ```
    >     out.rgb = plane_alpha * fg.alpha * fg.rgb +
    >             (1 - (plane_alpha * fg.alpha)) * bg.rgb
    >     ```
    >
    > Using the following symbols:
    >
    > “fg.rgb”:
    > :   Each of the RGB component values from the plane’s pixel
    >
    > “fg.alpha”:
    > :   Alpha component value from the plane’s pixel. If the plane’s
    >     pixel format has no alpha component, then this is assumed to be
    >     1.0. In these cases, this property has no effect, as all three
    >     equations become equivalent.
    >
    > “bg.rgb”:
    > :   Each of the RGB component values from the background
    >
    > “plane\_alpha”:
    > :   Plane alpha value set by the plane “alpha” property. If the
    >     plane does not expose the “alpha” property, then this is
    >     assumed to be 1.0

SCALING\_FILTER:
:   Indicates scaling filter to be used for plane scaler

    The value of this property can be one of the following:

    Default:
    :   Driver’s default scaling filter

    Nearest Neighbor:
    :   Nearest Neighbor scaling filter

Drivers can set up this property for a plane by calling
drm\_plane\_create\_scaling\_filter\_property

The property extensions described above all apply to the plane. Drivers
may also expose the following crtc property extension:

BACKGROUND\_COLOR:
:   Background color is set up with [`drm_crtc_attach_background_color_property()`](#c.drm_crtc_attach_background_color_property "drm_crtc_attach_background_color_property"),
    and expects a 64-bit ARGB value following DRM\_FORMAT\_ARGB16161616, as
    generated by the DRM\_ARGB64\_PREP\*() helpers. It controls the color of a
    full-screen layer that exists below all planes. This color will be used
    for pixels not covered by any plane and may also be blended with plane
    contents as allowed by a plane’s alpha values.
    The background color defaults to black, and is assumed to be black for
    drivers that do not expose this property. Although background color
    isn’t a plane, it is assumed that the color provided here undergoes the
    CRTC degamma/CSC/gamma transformations applied after the planes blending.
    Note that the color value includes an alpha channel, hence non-opaque
    background color values are allowed, but since physically transparent
    monitors do not (yet) exists, the final alpha value may not reach the
    video sink or it may simply ignore it.

### Damage Tracking Properties

FB\_DAMAGE\_CLIPS is an optional plane property which provides a means to
specify a list of damage rectangles on a plane in framebuffer coordinates of
the framebuffer attached to the plane. In current context damage is the area
of plane framebuffer that has changed since last plane update (also called
page-flip), irrespective of whether currently attached framebuffer is same as
framebuffer attached during last plane update or not.

FB\_DAMAGE\_CLIPS is a hint to kernel which could be helpful for some drivers
to optimize internally especially for virtual devices where each framebuffer
change needs to be transmitted over network, usb, etc.

Since FB\_DAMAGE\_CLIPS is a hint so it is an optional property. User-space can
ignore damage clips property and in that case driver will do a full plane
update. In case damage clips are provided then it is guaranteed that the area
inside damage clips will be updated to plane. For efficiency driver can do
full update or can update more than specified in damage clips. Since driver
is free to read more, user-space must always render the entire visible
framebuffer. Otherwise there can be corruptions. Also, if a user-space
provides damage clips which doesn’t encompass the actual damage to
framebuffer (since last plane update) can result in incorrect rendering.

FB\_DAMAGE\_CLIPS is a blob property with the layout of blob data is simply an
array of [`drm_mode_rect`](drm-uapi.html#c.drm_mode_rect "drm_mode_rect"). Unlike plane [`drm_plane_state.src`](#c.drm_plane_state "drm_plane_state") coordinates,
damage clips are not in 16.16 fixed point. Similar to plane src in
framebuffer, damage clips cannot be negative. In damage clip, x1/y1 are
inclusive and x2/y2 are exclusive. While kernel does not error for overlapped
damage clips, it is strongly discouraged.

Drivers that are interested in damage interface for plane should enable
FB\_DAMAGE\_CLIPS property by calling [`drm_plane_enable_fb_damage_clips()`](#c.drm_plane_enable_fb_damage_clips "drm_plane_enable_fb_damage_clips").
Drivers implementing damage can use [`drm_atomic_helper_damage_iter_init()`](#c.drm_atomic_helper_damage_iter_init "drm_atomic_helper_damage_iter_init") and
[`drm_atomic_helper_damage_iter_next()`](#c.drm_atomic_helper_damage_iter_next "drm_atomic_helper_damage_iter_next") helper iterator function to get damage
rectangles clipped to [`drm_plane_state.src`](#c.drm_plane_state "drm_plane_state").

Note that there are two types of damage handling: frame damage and buffer
damage, the type of damage handling implemented depends on a driver’s upload
target. Drivers implementing a per-plane or per-CRTC upload target need to
handle frame damage, while drivers implementing a per-buffer upload target
need to handle buffer damage.

The existing damage helpers only support the frame damage type, there is no
buffer age support or similar damage accumulation algorithm implemented yet.

Only drivers handling frame damage can use the mentioned damage helpers to
iterate over the damaged regions. Drivers that handle buffer damage, must set
[`drm_plane_state.ignore_damage_clips`](#c.drm_plane_state "drm_plane_state") for [`drm_atomic_helper_damage_iter_init()`](#c.drm_atomic_helper_damage_iter_init "drm_atomic_helper_damage_iter_init")
to know that damage clips should be ignored and return [`drm_plane_state.src`](#c.drm_plane_state "drm_plane_state")
as the damage rectangle, to force a full plane update.

Drivers with a per-buffer upload target could compare the [`drm_plane_state.fb`](#c.drm_plane_state "drm_plane_state")
of the old and new plane states to determine if the framebuffer attached to a
plane has changed or not since the last plane update. If [`drm_plane_state.fb`](#c.drm_plane_state "drm_plane_state")
has changed, then [`drm_plane_state.ignore_damage_clips`](#c.drm_plane_state "drm_plane_state") must be set to true.

That is because drivers with a per-plane upload target, expect the backing
storage buffer to not change for a given plane. If the upload buffer changes
between page flips, the new upload buffer has to be updated as a whole. This
can be improved in the future if support for frame damage is added to the DRM
damage helpers, similarly to how user-space already handle this case as it is
explained in the following documents:

> <https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_swap_buffers_with_damage.txt>
> <https://emersion.fr/blog/2019/intro-to-damage-tracking/>

### Color Management Properties

Color management or color space adjustments is supported through a set of 5
properties on the [`drm_crtc`](#c.drm_crtc "drm_crtc") object. They are set up by calling
[`drm_crtc_enable_color_mgmt()`](#c.drm_crtc_enable_color_mgmt "drm_crtc_enable_color_mgmt").

“DEGAMMA\_LUT”:
:   Blob property to set the degamma lookup table (LUT) mapping pixel data
    from the framebuffer before it is given to the transformation matrix.
    The data is interpreted as an array of `struct drm_color_lut` elements.
    Hardware might choose not to use the full precision of the LUT elements
    nor use all the elements of the LUT (for example the hardware might
    choose to interpolate between LUT[0] and LUT[4]).

    Setting this to NULL (blob property value set to 0) means a
    linear/pass-thru gamma table should be used. This is generally the
    driver boot-up state too. Drivers can access this blob through
    [`drm_crtc_state.degamma_lut`](#c.drm_crtc_state "drm_crtc_state").

“DEGAMMA\_LUT\_SIZE”:
:   Unsinged range property to give the size of the lookup table to be set
    on the DEGAMMA\_LUT property (the size depends on the underlying
    hardware). If drivers support multiple LUT sizes then they should
    publish the largest size, and sub-sample smaller sized LUTs (e.g. for
    split-gamma modes) appropriately.

“CTM”:
:   Blob property to set the current transformation matrix (CTM) apply to
    pixel data after the lookup through the degamma LUT and before the
    lookup through the gamma LUT. The data is interpreted as a struct
    `drm_color_ctm`.

    Setting this to NULL (blob property value set to 0) means a
    unit/pass-thru matrix should be used. This is generally the driver
    boot-up state too. Drivers can access the blob for the color conversion
    matrix through [`drm_crtc_state.ctm`](#c.drm_crtc_state "drm_crtc_state").

“GAMMA\_LUT”:
:   Blob property to set the gamma lookup table (LUT) mapping pixel data
    after the transformation matrix to data sent to the connector. The
    data is interpreted as an array of `struct drm_color_lut` elements.
    Hardware might choose not to use the full precision of the LUT elements
    nor use all the elements of the LUT (for example the hardware might
    choose to interpolate between LUT[0] and LUT[4]).

    Setting this to NULL (blob property value set to 0) means a
    linear/pass-thru gamma table should be used. This is generally the
    driver boot-up state too. Drivers can access this blob through
    [`drm_crtc_state.gamma_lut`](#c.drm_crtc_state "drm_crtc_state").

    Note that for mostly historical reasons stemming from Xorg heritage,
    this is also used to store the color map (also sometimes color lut, CLUT
    or color palette) for indexed formats like DRM\_FORMAT\_C8.

“GAMMA\_LUT\_SIZE”:
:   Unsigned range property to give the size of the lookup table to be set
    on the GAMMA\_LUT property (the size depends on the underlying hardware).
    If drivers support multiple LUT sizes then they should publish the
    largest size, and sub-sample smaller sized LUTs (e.g. for split-gamma
    modes) appropriately.

There is also support for a legacy gamma table, which is set up by calling
[`drm_mode_crtc_set_gamma_size()`](#c.drm_mode_crtc_set_gamma_size "drm_mode_crtc_set_gamma_size"). The DRM core will then alias the legacy gamma
ramp with “GAMMA\_LUT” or, if that is unavailable, “DEGAMMA\_LUT”.

Support for different non RGB color encodings is controlled through
[`drm_plane`](#c.drm_plane "drm_plane") specific COLOR\_ENCODING and COLOR\_RANGE properties. They
are set up by calling [`drm_plane_create_color_properties()`](#c.drm_plane_create_color_properties "drm_plane_create_color_properties").

“COLOR\_ENCODING”:
:   Optional plane `enum property` to support different non RGB
    color encodings. The driver can provide a subset of standard
    `enum values` supported by the DRM plane.

“COLOR\_RANGE”:
:   Optional plane `enum property` to support different non RGB
    color parameter ranges. The driver can provide a subset of
    standard `enum values` supported by the DRM plane.

### Tile Group Property

Tile groups are used to represent tiled monitors with a unique integer
identifier. Tiled monitors using DisplayID v1.3 have a unique 8-byte handle,
we store this in a tile group, so we have a common identifier for all tiles
in a monitor group. The property is called “TILE”. Drivers can manage tile
groups using [`drm_mode_create_tile_group()`](#c.drm_mode_create_tile_group "drm_mode_create_tile_group"), [`drm_mode_put_tile_group()`](#c.drm_mode_put_tile_group "drm_mode_put_tile_group") and
[`drm_mode_get_tile_group()`](#c.drm_mode_get_tile_group "drm_mode_get_tile_group"). But this is only needed for internal panels where
the tile group information is exposed through a non-standard way.

### Explicit Fencing Properties

Explicit fencing allows userspace to control the buffer synchronization
between devices. A Fence or a group of fences are transferred to/from
userspace using Sync File fds and there are two DRM properties for that.
IN\_FENCE\_FD on each DRM Plane to send fences to the kernel and
OUT\_FENCE\_PTR on each DRM CRTC to receive fences from the kernel.

As a contrast, with implicit fencing the kernel keeps track of any
ongoing rendering, and automatically ensures that the atomic update waits
for any pending rendering to complete. This is usually tracked in [`struct
dma_resv`](../driver-api/dma-buf.html#c.dma_resv "dma_resv") which can also contain mandatory kernel fences. Implicit syncing
is how Linux traditionally worked (e.g. DRI2/3 on X.org), whereas explicit
fencing is what Android wants.

“IN\_FENCE\_FD”:
:   Use this property to pass a fence that DRM should wait on before
    proceeding with the Atomic Commit request and show the framebuffer for
    the plane on the screen. The fence can be either a normal fence or a
    merged one, the sync\_file framework will handle both cases and use a
    fence\_array if a merged fence is received. Passing -1 here means no
    fences to wait on.

    If the Atomic Commit request has the DRM\_MODE\_ATOMIC\_TEST\_ONLY flag
    it will only check if the Sync File is a valid one.

    On the driver side the fence is stored on the **fence** parameter of
    [`struct drm_plane_state`](#c.drm_plane_state "drm_plane_state"). Drivers which also support implicit fencing
    should extract the implicit fence using [`drm_gem_plane_helper_prepare_fb()`](drm-kms-helpers.html#c.drm_gem_plane_helper_prepare_fb "drm_gem_plane_helper_prepare_fb"),
    to make sure there’s consistent behaviour between drivers in precedence
    of implicit vs. explicit fencing.

“OUT\_FENCE\_PTR”:
:   Use this property to pass a file descriptor pointer to DRM. Once the
    Atomic Commit request call returns OUT\_FENCE\_PTR will be filled with
    the file descriptor number of a Sync File. This Sync File contains the
    CRTC fence that will be signaled when all framebuffers present on the
    Atomic Commit \* request for that given CRTC are scanned out on the
    screen.

    The Atomic Commit request fails if a invalid pointer is passed. If the
    Atomic Commit request fails for any other reason the out fence fd
    returned will be -1. On a Atomic Commit with the
    DRM\_MODE\_ATOMIC\_TEST\_ONLY flag the out fence will also be set to -1.

    Note that out-fences don’t have a special interface to drivers and are
    internally represented by a [`struct drm_pending_vblank_event`](#c.drm_pending_vblank_event "drm_pending_vblank_event") in struct
    [`drm_crtc_state`](#c.drm_crtc_state "drm_crtc_state"), which is also used by the nonblocking atomic commit
    helpers and for the DRM event handling for existing userspace.

### Variable Refresh Properties

Variable refresh rate capable displays can dynamically adjust their
refresh rate by extending the duration of their vertical front porch
until page flip or timeout occurs. This can reduce or remove stuttering
and latency in scenarios where the page flip does not align with the
vblank interval.

An example scenario would be an application flipping at a constant rate
of 48Hz on a 60Hz display. The page flip will frequently miss the vblank
interval and the same contents will be displayed twice. This can be
observed as stuttering for content with motion.

If variable refresh rate was active on a display that supported a
variable refresh range from 35Hz to 60Hz no stuttering would be observable
for the example scenario. The minimum supported variable refresh rate of
35Hz is below the page flip frequency and the vertical front porch can
be extended until the page flip occurs. The vblank interval will be
directly aligned to the page flip rate.

Not all userspace content is suitable for use with variable refresh rate.
Large and frequent changes in vertical front porch duration may worsen
perceived stuttering for input sensitive applications.

Panel brightness will also vary with vertical front porch duration. Some
panels may have noticeable differences in brightness between the minimum
vertical front porch duration and the maximum vertical front porch duration.
Large and frequent changes in vertical front porch duration may produce
observable flickering for such panels.

Userspace control for variable refresh rate is supported via properties
on the [`drm_connector`](#c.drm_connector "drm_connector") and [`drm_crtc`](#c.drm_crtc "drm_crtc") objects.

“vrr\_capable”:
:   Optional [`drm_connector`](#c.drm_connector "drm_connector") boolean property that drivers should attach
    with [`drm_connector_attach_vrr_capable_property()`](#c.drm_connector_attach_vrr_capable_property "drm_connector_attach_vrr_capable_property") on connectors that
    could support variable refresh rates. Drivers should update the
    property value by calling [`drm_connector_set_vrr_capable_property()`](#c.drm_connector_set_vrr_capable_property "drm_connector_set_vrr_capable_property").

    Absence of the property should indicate absence of support.

“VRR\_ENABLED”:
:   Default [`drm_crtc`](#c.drm_crtc "drm_crtc") boolean property that notifies the driver that the
    content on the CRTC is suitable for variable refresh rate presentation.
    The driver will take this property as a hint to enable variable
    refresh rate support if the receiver supports it, ie. if the
    “vrr\_capable” property is true on the [`drm_connector`](#c.drm_connector "drm_connector") object. The
    vertical front porch duration will be extended until page-flip or
    timeout when enabled.

    The minimum vertical front porch duration is defined as the vertical
    front porch duration for the current mode.

    The maximum vertical front porch duration is greater than or equal to
    the minimum vertical front porch duration. The duration is derived
    from the minimum supported variable refresh rate for the connector.

    The driver may place further restrictions within these minimum
    and maximum bounds.

### Cursor Hotspot Properties

HOTSPOT\_X: property to set mouse hotspot x offset.
HOTSPOT\_Y: property to set mouse hotspot y offset.

When the plane is being used as a cursor image to display a mouse pointer,
the “hotspot” is the offset within the cursor image where mouse events
are expected to go.

Positive values move the hotspot from the top-left corner of the cursor
plane towards the right and bottom.

Most display drivers do not need this information because the
hotspot is not actually connected to anything visible on screen.
However, this is necessary for display drivers like the para-virtualized
drivers (eg qxl, vbox, virtio, vmwgfx), that are attached to a user console
with a mouse pointer. Since these consoles are often being remoted over a
network, they would otherwise have to wait to display the pointer movement to
the user until a full network round-trip has occurred. New mouse events have
to be sent from the user’s console, over the network to the virtual input
devices, forwarded to the desktop for processing, and then the cursor plane’s
position can be updated and sent back to the user’s console over the network.
Instead, with the hotspot information, the console can anticipate the new
location, and draw the mouse cursor there before the confirmation comes in.
To do that correctly, the user’s console must be able predict how the
desktop will process mouse events, which normally requires the desktop’s
mouse topology information, ie where each CRTC sits in the mouse coordinate
space. This is typically sent to the para-virtualized drivers using some
driver-specific method, and the driver then forwards it to the console by
way of the virtual display device or hypervisor.

The assumption is generally made that there is only one cursor plane being
used this way at a time, and that the desktop is feeding all mouse devices
into the same global pointer. Para-virtualized drivers that require this
should only be exposing a single cursor plane, or find some other way
to coordinate with a userspace desktop that supports multiple pointers.
If the hotspot properties are set, the cursor plane is therefore assumed to be
used only for displaying a mouse cursor image, and the position of the combined
cursor plane + offset can therefore be used for coordinating with input from a
mouse device.

The cursor will then be drawn either at the location of the plane in the CRTC
console, or as a free-floating cursor plane on the user’s console
corresponding to their desktop mouse position.

DRM clients which would like to work correctly on drivers which expose
hotspot properties should advertise DRM\_CLIENT\_CAP\_CURSOR\_PLANE\_HOTSPOT.
Setting this property on drivers which do not special case
cursor planes will return EOPNOTSUPP, which can be used by userspace to
gauge requirements of the hardware/drivers they’re running on. Advertising
DRM\_CLIENT\_CAP\_CURSOR\_PLANE\_HOTSPOT implies that the userspace client will be
correctly setting the hotspot properties.

### Existing KMS Properties

The following table gives description of drm properties exposed by various
modules/drivers. Because this table is very unwieldy, do not add any new
properties here. Instead document them in a section above.

| Owner Module/Drivers | Group | Property Name | Type | Property Values | Object attached | Description/Restrictions |
| --- | --- | --- | --- | --- | --- | --- |
|  | DVI-I | “subconnector” | ENUM | { “Unknown”, “DVI-D”, “DVI-A” } | Connector | TBD |
|  |  | “select subconnector” | ENUM | { “Automatic”, “DVI-D”, “DVI-A” } | Connector | TBD |
|  | TV | “subconnector” | ENUM | { “Unknown”, “Composite”, “SVIDEO”, “Component”, “SCART” } | Connector | TBD |
|  |  | “select subconnector” | ENUM | { “Automatic”, “Composite”, “SVIDEO”, “Component”, “SCART” } | Connector | TBD |
|  |  | “mode” | ENUM | { “NTSC\_M”, “NTSC\_J”, “NTSC\_443”, “PAL\_B” } etc. | Connector | TBD |
|  |  | “left margin” | RANGE | Min=0, Max=100 | Connector | TBD |
|  |  | “right margin” | RANGE | Min=0, Max=100 | Connector | TBD |
|  |  | “top margin” | RANGE | Min=0, Max=100 | Connector | TBD |
|  |  | “bottom margin” | RANGE | Min=0, Max=100 | Connector | TBD |
|  |  | “brightness” | RANGE | Min=0, Max=100 | Connector | TBD |
|  |  | “contrast” | RANGE | Min=0, Max=100 | Connector | TBD |
|  |  | “flicker reduction” | RANGE | Min=0, Max=100 | Connector | TBD |
|  |  | “overscan” | RANGE | Min=0, Max=100 | Connector | TBD |
|  |  | “saturation” | RANGE | Min=0, Max=100 | Connector | TBD |
|  |  | “hue” | RANGE | Min=0, Max=100 | Connector | TBD |
|  | Virtual GPU | “suggested X” | RANGE | Min=0, Max=0xffffffff | Connector | property to suggest an X offset for a connector |
|  |  | “suggested Y” | RANGE | Min=0, Max=0xffffffff | Connector | property to suggest an Y offset for a connector |
|  | Optional | “aspect ratio” | ENUM | { “None”, “4:3”, “16:9” } | Connector | TDB |
|  |  | “audio” | ENUM | { “force-dvi”, “off”, “auto”, “on” } | Connector | TBD |
|  | SDVO-TV | “mode” | ENUM | { “NTSC\_M”, “NTSC\_J”, “NTSC\_443”, “PAL\_B” } etc. | Connector | TBD |
|  |  | “left\_margin” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “right\_margin” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “top\_margin” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “bottom\_margin” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “hpos” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “vpos” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “contrast” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “saturation” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “hue” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “sharpness” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “flicker\_filter” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “flicker\_filter\_adaptive” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “flicker\_filter\_2d” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “tv\_chroma\_filter” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “tv\_luma\_filter” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “dot\_crawl” | RANGE | Min=0, Max=1 | Connector | TBD |
|  | SDVO-TV/LVDS | “brightness” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
| CDV gma-500 | Generic | “Broadcast RGB” | ENUM | { “Full”, “Limited 16:235” } | Connector | TBD |
| Poulsbo | Generic | “backlight” | RANGE | Min=0, Max=100 | Connector | TBD |
|  | SDVO-TV | “mode” | ENUM | { “NTSC\_M”, “NTSC\_J”, “NTSC\_443”, “PAL\_B” } etc. | Connector | TBD |
|  |  | “left\_margin” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “right\_margin” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “top\_margin” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “bottom\_margin” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “hpos” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “vpos” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “contrast” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “saturation” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “hue” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “sharpness” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “flicker\_filter” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “flicker\_filter\_adaptive” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “flicker\_filter\_2d” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “tv\_chroma\_filter” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “tv\_luma\_filter” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
|  |  | “dot\_crawl” | RANGE | Min=0, Max=1 | Connector | TBD |
|  | SDVO-TV/LVDS | “brightness” | RANGE | Min=0, Max= SDVO dependent | Connector | TBD |
| armada | CRTC | “CSC\_YUV” | ENUM | { “Auto” , “CCIR601”, “CCIR709” } | CRTC | TBD |
|  |  | “CSC\_RGB” | ENUM | { “Auto”, “Computer system”, “Studio” } | CRTC | TBD |
|  | Overlay | “colorkey” | RANGE | Min=0, Max=0xffffff | Plane | TBD |
|  |  | “colorkey\_min” | RANGE | Min=0, Max=0xffffff | Plane | TBD |
|  |  | “colorkey\_max” | RANGE | Min=0, Max=0xffffff | Plane | TBD |
|  |  | “colorkey\_val” | RANGE | Min=0, Max=0xffffff | Plane | TBD |
|  |  | “colorkey\_alpha” | RANGE | Min=0, Max=0xffffff | Plane | TBD |
|  |  | “colorkey\_mode” | ENUM | { “disabled”, “Y component”, “U component” , “V component”, “RGB”, “R component”, “G component”, “B component” } | Plane | TBD |
|  |  | “brightness” | RANGE | Min=0, Max=256 + 255 | Plane | TBD |
|  |  | “contrast” | RANGE | Min=0, Max=0x7fff | Plane | TBD |
|  |  | “saturation” | RANGE | Min=0, Max=0x7fff | Plane | TBD |
| exynos | CRTC | “mode” | ENUM | { “normal”, “blank” } | CRTC | TBD |
| i2c/ch7006\_drv | Generic | “scale” | RANGE | Min=0, Max=2 | Connector | TBD |
|  | TV | “mode” | ENUM | { “PAL”, “PAL-M”,”PAL-N”}, ”PAL-Nc” , “PAL-60”, “NTSC-M”, “NTSC-J” } | Connector | TBD |
| nouveau | NV10 Overlay | “colorkey” | RANGE | Min=0, Max=0x01ffffff | Plane | TBD |
|  |  | “contrast” | RANGE | Min=0, Max=8192-1 | Plane | TBD |
|  |  | “brightness” | RANGE | Min=0, Max=1024 | Plane | TBD |
|  |  | “hue” | RANGE | Min=0, Max=359 | Plane | TBD |
|  |  | “saturation” | RANGE | Min=0, Max=8192-1 | Plane | TBD |
|  |  | “iturbt\_709” | RANGE | Min=0, Max=1 | Plane | TBD |
|  | Nv04 Overlay | “colorkey” | RANGE | Min=0, Max=0x01ffffff | Plane | TBD |
|  |  | “brightness” | RANGE | Min=0, Max=1024 | Plane | TBD |
|  | Display | “dithering mode” | ENUM | { “auto”, “off”, “on” } | Connector | TBD |
|  |  | “dithering depth” | ENUM | { “auto”, “off”, “on”, “static 2x2”, “dynamic 2x2”, “temporal” } | Connector | TBD |
|  |  | “underscan” | ENUM | { “auto”, “6 bpc”, “8 bpc” } | Connector | TBD |
|  |  | “underscan hborder” | RANGE | Min=0, Max=128 | Connector | TBD |
|  |  | “underscan vborder” | RANGE | Min=0, Max=128 | Connector | TBD |
|  |  | “vibrant hue” | RANGE | Min=0, Max=180 | Connector | TBD |
|  |  | “color vibrance” | RANGE | Min=0, Max=200 | Connector | TBD |
| omap | Generic | “zorder” | RANGE | Min=0, Max=3 | CRTC, Plane | TBD |
| qxl | Generic | “hotplug\_mode\_update” | RANGE | Min=0, Max=1 | Connector | TBD |
| radeon | DVI-I | “coherent” | RANGE | Min=0, Max=1 | Connector | TBD |
|  | DAC enable load detect | “load detection” | RANGE | Min=0, Max=1 | Connector | TBD |
|  | TV Standard | “tv standard” | ENUM | { “ntsc”, “pal”, “pal-m”, “pal-60”, “ntsc-j” , “scart-pal”, “pal-cn”, “secam” } | Connector | TBD |
|  | legacy TMDS PLL detect | “tmds\_pll” | ENUM | { “driver”, “bios” } |  | TBD |
|  | Underscan | “underscan” | ENUM | { “off”, “on”, “auto” } | Connector | TBD |
|  |  | “underscan hborder” | RANGE | Min=0, Max=128 | Connector | TBD |
|  |  | “underscan vborder” | RANGE | Min=0, Max=128 | Connector | TBD |
|  | Audio | “audio” | ENUM | { “off”, “on”, “auto” } | Connector | TBD |
|  | FMT Dithering | “dither” | ENUM | { “off”, “on” } | Connector | TBD |
|  |  | “colorkey” | RANGE | Min=0, Max=0x01ffffff | Plane | TBD |

## Vertical Blanking

From the computer’s perspective, every time the monitor displays
a new frame the scanout engine has “scanned out” the display image
from top to bottom, one row of pixels at a time. The current row
of pixels is referred to as the current scanline.

In addition to the display’s visible area, there’s usually a couple of
extra scanlines which aren’t actually displayed on the screen.
These extra scanlines don’t contain image data and are occasionally used
for features like audio and infoframes. The region made up of these
scanlines is referred to as the vertical blanking region, or vblank for
short.

For historical reference, the vertical blanking period was designed to
give the electron gun (on CRTs) enough time to move back to the top of
the screen to start scanning out the next frame. Similar for horizontal
blanking periods. They were designed to give the electron gun enough
time to move back to the other side of the screen to start scanning the
next scanline.

```
physical →   ⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽
top of      |                                        |
display     |                                        |
            |               New frame                |
            |                                        |
            |↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓|
            |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~| ← Scanline,
            |↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓|   updates the
            |                                        |   frame as it
            |                                        |   travels down
            |                                        |   ("scan out")
            |               Old frame                |
            |                                        |
            |                                        |
            |                                        |
            |                                        |   physical
            |                                        |   bottom of
vertical    |⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽| ← display
blanking    ┆xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx┆
region   →  ┆xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx┆
            ┆xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx┆
start of →   ⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽
new frame
```

“Physical top of display” is the reference point for the high-precision/
corrected timestamp.

On a lot of display hardware, programming needs to take effect during the
vertical blanking period so that settings like gamma, the image buffer
buffer to be scanned out, etc. can safely be changed without showing
any visual artifacts on the screen. In some unforgiving hardware, some of
this programming has to both start and end in the same vblank. To help
with the timing of the hardware programming, an interrupt is usually
available to notify the driver when it can start the updating of registers.
The interrupt is in this context named the vblank interrupt.

The vblank interrupt may be fired at different points depending on the
hardware. Some hardware implementations will fire the interrupt when the
new frame start, other implementations will fire the interrupt at different
points in time.

Vertical blanking plays a major role in graphics rendering. To achieve
tear-free display, users must synchronize page flips and/or rendering to
vertical blanking. The DRM API offers ioctls to perform page flips
synchronized to vertical blanking and wait for vertical blanking.

The DRM core handles most of the vertical blanking management logic, which
involves filtering out spurious interrupts, keeping race-free blanking
counters, coping with counter wrap-around and resets and keeping use counts.
It relies on the driver to generate vertical blanking interrupts and
optionally provide a hardware vertical blanking counter.

Drivers must initialize the vertical blanking handling core with a call to
[`drm_vblank_init()`](#c.drm_vblank_init "drm_vblank_init"). Minimally, a driver needs to implement
[`drm_crtc_funcs.enable_vblank`](#c.drm_crtc_funcs "drm_crtc_funcs") and [`drm_crtc_funcs.disable_vblank`](#c.drm_crtc_funcs "drm_crtc_funcs") plus call
[`drm_crtc_handle_vblank()`](#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank") in its vblank interrupt handler for working vblank
support.

Vertical blanking interrupts can be enabled by the DRM core or by drivers
themselves (for instance to handle page flipping operations). The DRM core
maintains a vertical blanking use count to ensure that the interrupts are not
disabled while a user still needs them. To increment the use count, drivers
call [`drm_crtc_vblank_get()`](#c.drm_crtc_vblank_get "drm_crtc_vblank_get") and release the vblank reference again with
[`drm_crtc_vblank_put()`](#c.drm_crtc_vblank_put "drm_crtc_vblank_put"). In between these two calls vblank interrupts are
guaranteed to be enabled.

On many hardware disabling the vblank interrupt cannot be done in a race-free
manner, see [`drm_vblank_crtc_config.disable_immediate`](#c.drm_vblank_crtc_config "drm_vblank_crtc_config") and
[`drm_driver.max_vblank_count`](drm-internals.html#c.drm_driver "drm_driver"). In that case the vblank core only disables the
vblanks after a timer has expired, which can be configured through the
`vblankoffdelay` module parameter.

Drivers for hardware without support for vertical-blanking interrupts can
use DRM vblank timers to send vblank events at the rate of the current
display mode’s refresh. While not synchronized to the hardware’s
vertical-blanking regions, the timer helps DRM clients and compositors to
adapt their update cycle to the display output. Drivers should set up
vblanking as usual, but call [`drm_crtc_vblank_start_timer()`](#c.drm_crtc_vblank_start_timer "drm_crtc_vblank_start_timer") and
[`drm_crtc_vblank_cancel_timer()`](#c.drm_crtc_vblank_cancel_timer "drm_crtc_vblank_cancel_timer") as part of their atomic mode setting.
See also DRM vblank helpers for more information.

Drivers without support for vertical-blanking interrupts nor timers must
not call [`drm_vblank_init()`](#c.drm_vblank_init "drm_vblank_init"). For these drivers, atomic helpers will
automatically generate fake vblank events as part of the display update.
This functionality also can be controlled by the driver by enabling and
disabling [`struct drm_crtc_state`](#c.drm_crtc_state "drm_crtc_state").no\_vblank.

### Vertical Blanking and Interrupt Handling Functions Reference

struct drm\_pending\_vblank\_event
:   pending vblank event tracking

**Definition**:

```
struct drm_pending_vblank_event {
    struct drm_pending_event base;
    unsigned int pipe;
    u64 sequence;
    union {
        struct drm_event base;
        struct drm_event_vblank vbl;
        struct drm_event_crtc_sequence seq;
    } event;
};
```

**Members**

`base`
:   Base structure for tracking pending DRM events.

`pipe`
:   [`drm_crtc_index()`](#c.drm_crtc_index "drm_crtc_index") of the [`drm_crtc`](#c.drm_crtc "drm_crtc") this event is for.

`sequence`
:   frame event should be triggered at

`event`
:   Actual event which will be sent to userspace.

`event.base`
:   DRM event base class.

`event.vbl`
:   Event payload for vblank events, requested through
    either the MODE\_PAGE\_FLIP or MODE\_ATOMIC IOCTL. Also
    generated by the legacy WAIT\_VBLANK IOCTL, but new userspace
    should use MODE\_QUEUE\_SEQUENCE and `event.seq` instead.

`event.seq`
:   Event payload for the MODE\_QUEUEU\_SEQUENCE IOCTL.

struct drm\_vblank\_crtc\_config
:   vblank configuration for a CRTC

**Definition**:

```
struct drm_vblank_crtc_config {
    int offdelay_ms;
    bool disable_immediate;
};
```

**Members**

`offdelay_ms`
:   Vblank off delay in ms, used to determine how long
    [`drm_vblank_crtc.disable_timer`](#c.drm_vblank_crtc "drm_vblank_crtc") waits before disabling.

    Defaults to the value of drm\_vblank\_offdelay in [`drm_crtc_vblank_on()`](#c.drm_crtc_vblank_on "drm_crtc_vblank_on").

`disable_immediate`
:   See [`drm_device.vblank_disable_immediate`](drm-internals.html#c.drm_device "drm_device")
    for the exact semantics of immediate vblank disabling.

    Additionally, this tracks the disable immediate value per crtc, just
    in case it needs to differ from the default value for a given device.

    Defaults to the value of [`drm_device.vblank_disable_immediate`](drm-internals.html#c.drm_device "drm_device") in
    [`drm_crtc_vblank_on()`](#c.drm_crtc_vblank_on "drm_crtc_vblank_on").

struct drm\_vblank\_crtc\_timer
:   vblank timer for a CRTC

**Definition**:

```
struct drm_vblank_crtc_timer {
    struct hrtimer timer;
    spinlock_t interval_lock;
    ktime_t interval;
    struct drm_crtc *crtc;
};
```

**Members**

`timer`
:   The vblank’s high-resolution timer

`interval_lock`
:   Protects **interval**

`interval`
:   Duration between two vblanks

`crtc`
:   The timer’s CRTC

struct drm\_vblank\_crtc
:   vblank tracking for a CRTC

**Definition**:

```
struct drm_vblank_crtc {
    struct drm_device *dev;
    wait_queue_head_t queue;
    struct timer_list disable_timer;
    seqlock_t seqlock;
    atomic64_t count;
    ktime_t time;
    atomic_t refcount;
    u32 last;
    u32 max_vblank_count;
    unsigned int inmodeset;
    unsigned int pipe;
    int framedur_ns;
    int linedur_ns;
    struct drm_display_mode hwmode;
    struct drm_vblank_crtc_config config;
    bool enabled;
    struct kthread_worker *worker;
    struct list_head pending_work;
    wait_queue_head_t work_wait_queue;
    struct drm_vblank_crtc_timer vblank_timer;
};
```

**Members**

`dev`
:   Pointer to the [`drm_device`](drm-internals.html#c.drm_device "drm_device").

`queue`
:   Wait queue for vblank waiters.

`disable_timer`
:   Disable timer for the delayed vblank disabling
    hysteresis logic. Vblank disabling is controlled through
    [`drm_vblank_crtc_config.offdelay_ms`](#c.drm_vblank_crtc_config "drm_vblank_crtc_config") and the setting of the
    [`drm_device.max_vblank_count`](drm-internals.html#c.drm_device "drm_device") value.

`seqlock`
:   Protect vblank count and time.

`count`
:   Current software vblank counter.

    Note that for a given vblank counter value [`drm_crtc_handle_vblank()`](#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank")
    and [`drm_crtc_vblank_count()`](#c.drm_crtc_vblank_count "drm_crtc_vblank_count") or [`drm_crtc_vblank_count_and_time()`](#c.drm_crtc_vblank_count_and_time "drm_crtc_vblank_count_and_time")
    provide a barrier: Any writes done before calling
    [`drm_crtc_handle_vblank()`](#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank") will be visible to callers of the later
    functions, iff the vblank count is the same or a later one.

    IMPORTANT: This guarantee requires barriers, therefor never access
    this field directly. Use [`drm_crtc_vblank_count()`](#c.drm_crtc_vblank_count "drm_crtc_vblank_count") instead.

`time`
:   Vblank timestamp corresponding to **count**.

`refcount`
:   Number of users/waiters of the vblank interrupt. Only when
    this refcount reaches 0 can the hardware interrupt be disabled using
    **disable\_timer**.

`last`
:   Protected by [`drm_device.vbl_lock`](drm-internals.html#c.drm_device "drm_device"), used for wraparound handling.

`max_vblank_count`
:   Maximum value of the vblank registers for this crtc. This value +1
    will result in a wrap-around of the vblank register. It is used
    by the vblank core to handle wrap-arounds.

    If set to zero the vblank core will try to guess the elapsed vblanks
    between times when the vblank interrupt is disabled through
    high-precision timestamps. That approach is suffering from small
    races and imprecision over longer time periods, hence exposing a
    hardware vblank counter is always recommended.

    This is the runtime configurable per-crtc maximum set through
    [`drm_crtc_set_max_vblank_count()`](#c.drm_crtc_set_max_vblank_count "drm_crtc_set_max_vblank_count"). If this is used the driver
    must leave the device wide [`drm_device.max_vblank_count`](drm-internals.html#c.drm_device "drm_device") at zero.

    If non-zero, [`drm_crtc_funcs.get_vblank_counter`](#c.drm_crtc_funcs "drm_crtc_funcs") must be set.

`inmodeset`
:   Tracks whether the vblank is disabled due to a modeset.
    For legacy driver bit 2 additionally tracks whether an additional
    temporary vblank reference has been acquired to paper over the
    hardware counter resetting/jumping. KMS drivers should instead just
    call [`drm_crtc_vblank_off()`](#c.drm_crtc_vblank_off "drm_crtc_vblank_off") and [`drm_crtc_vblank_on()`](#c.drm_crtc_vblank_on "drm_crtc_vblank_on"), which explicitly
    save and restore the vblank count.

`pipe`
:   [`drm_crtc_index()`](#c.drm_crtc_index "drm_crtc_index") of the [`drm_crtc`](#c.drm_crtc "drm_crtc") corresponding to this
    structure.

`framedur_ns`
:   Frame/Field duration in ns, used by
    [`drm_crtc_vblank_helper_get_vblank_timestamp()`](#c.drm_crtc_vblank_helper_get_vblank_timestamp "drm_crtc_vblank_helper_get_vblank_timestamp") and computed by
    [`drm_calc_timestamping_constants()`](#c.drm_calc_timestamping_constants "drm_calc_timestamping_constants").

`linedur_ns`
:   Line duration in ns, used by
    [`drm_crtc_vblank_helper_get_vblank_timestamp()`](#c.drm_crtc_vblank_helper_get_vblank_timestamp "drm_crtc_vblank_helper_get_vblank_timestamp") and computed by
    [`drm_calc_timestamping_constants()`](#c.drm_calc_timestamping_constants "drm_calc_timestamping_constants").

`hwmode`
:   Cache of the current hardware display mode. Only valid when **enabled**
    is set. This is used by helpers like
    [`drm_crtc_vblank_helper_get_vblank_timestamp()`](#c.drm_crtc_vblank_helper_get_vblank_timestamp "drm_crtc_vblank_helper_get_vblank_timestamp"). We can’t just access
    the hardware mode by e.g. looking at [`drm_crtc_state.adjusted_mode`](#c.drm_crtc_state "drm_crtc_state"),
    because that one is really hard to get from interrupt context.

`config`
:   Stores vblank configuration values for a given CRTC.
    Also, see [`drm_crtc_vblank_on_config()`](#c.drm_crtc_vblank_on_config "drm_crtc_vblank_on_config").

`enabled`
:   Tracks the enabling state of the corresponding [`drm_crtc`](#c.drm_crtc "drm_crtc") to
    avoid double-disabling and hence corrupting saved state. Needed by
    drivers not using atomic KMS, since those might go through their CRTC
    disabling functions multiple times.

`worker`
:   The `kthread_worker` used for executing vblank works.

`pending_work`
:   A list of scheduled [`drm_vblank_work`](#c.drm_vblank_work "drm_vblank_work") items that are
    waiting for a future vblank.

`work_wait_queue`
:   The wait queue used for signaling that a
    [`drm_vblank_work`](#c.drm_vblank_work "drm_vblank_work") item has either finished executing, or was
    cancelled.

`vblank_timer`
:   Holds the state of the vblank timer

**Description**

This structure tracks the vblank state for one CRTC.

Note that for historical reasons - the vblank handling code is still shared
with legacy/non-kms drivers - this is a free-standing structure not directly
connected to [`struct drm_crtc`](#c.drm_crtc "drm_crtc"). But all public interface functions are taking
a [`struct drm_crtc`](#c.drm_crtc "drm_crtc") to hide this implementation detail.

u64 drm\_crtc\_accurate\_vblank\_count(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   retrieve the master vblank counter

**Parameters**

`struct drm_crtc *crtc`
:   which counter to retrieve

**Description**

This function is similar to [`drm_crtc_vblank_count()`](#c.drm_crtc_vblank_count "drm_crtc_vblank_count") but this function
interpolates to handle a race with vblank interrupts using the high precision
timestamping support.

This is mostly useful for hardware that can obtain the scanout position, but
doesn’t have a hardware frame counter.

int drm\_vblank\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, unsigned int num\_crtcs)
:   initialize vblank support

**Parameters**

`struct drm_device *dev`
:   DRM device

`unsigned int num_crtcs`
:   number of CRTCs supported by **dev**

**Description**

This function initializes vblank support for **num\_crtcs** display pipelines.
Cleanup is handled automatically through a cleanup function added with
[`drmm_add_action_or_reset()`](drm-internals.html#c.drmm_add_action_or_reset "drmm_add_action_or_reset").

**Return**

Zero on success or a negative error code on failure.

bool drm\_dev\_has\_vblank(const struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   test if vblanking has been initialized for a device

**Parameters**

`const struct drm_device *dev`
:   the device

**Description**

Drivers may call this function to test if vblank support is
initialized for a device. For most hardware this means that vblanking
can also be enabled.

Atomic helpers use this function to initialize
[`drm_crtc_state.no_vblank`](#c.drm_crtc_state "drm_crtc_state"). See also [`drm_atomic_helper_check_modeset()`](drm-kms-helpers.html#c.drm_atomic_helper_check_modeset "drm_atomic_helper_check_modeset").

**Return**

True if vblanking has been initialized for the given device, false
otherwise.

wait\_queue\_head\_t \*drm\_crtc\_vblank\_waitqueue(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   get vblank waitqueue for the CRTC

**Parameters**

`struct drm_crtc *crtc`
:   which CRTC’s vblank waitqueue to retrieve

**Description**

This function returns a pointer to the vblank waitqueue for the CRTC.
Drivers can use this to implement vblank waits using [`wait_event()`](../driver-api/basics.html#c.wait_event "wait_event") and related
functions.

void drm\_calc\_timestamping\_constants(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, const struct [drm\_display\_mode](#c.drm_display_mode "drm_display_mode") \*mode)
:   calculate vblank timestamp constants

**Parameters**

`struct drm_crtc *crtc`
:   drm\_crtc whose timestamp constants should be updated.

`const struct drm_display_mode *mode`
:   display mode containing the scanout timings

**Description**

Calculate and store various constants which are later needed by vblank and
swap-completion timestamping, e.g, by
[`drm_crtc_vblank_helper_get_vblank_timestamp()`](#c.drm_crtc_vblank_helper_get_vblank_timestamp "drm_crtc_vblank_helper_get_vblank_timestamp"). They are derived from
CRTC’s true scanout timing, so they take things like panel scaling or
other adjustments into account.

bool drm\_crtc\_vblank\_helper\_get\_vblank\_timestamp\_internal(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, int \*max\_error, ktime\_t \*vblank\_time, bool in\_vblank\_irq, drm\_vblank\_get\_scanout\_position\_func get\_scanout\_position)
:   precise vblank timestamp helper

**Parameters**

`struct drm_crtc *crtc`
:   CRTC whose vblank timestamp to retrieve

`int *max_error`
:   Desired maximum allowable error in timestamps (nanosecs)
    On return contains true maximum error of timestamp

`ktime_t *vblank_time`
:   Pointer to time which should receive the timestamp

`bool in_vblank_irq`
:   True when called from [`drm_crtc_handle_vblank()`](#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank"). Some drivers
    need to apply some workarounds for gpu-specific vblank irq quirks
    if flag is set.

`drm_vblank_get_scanout_position_func get_scanout_position`
:   Callback function to retrieve the scanout position. See
    **struct** drm\_crtc\_helper\_funcs.get\_scanout\_position.

**Description**

Implements calculation of exact vblank timestamps from given drm\_display\_mode
timings and current video scanout position of a CRTC.

The current implementation only handles standard video modes. For double scan
and interlaced modes the driver is supposed to adjust the hardware mode
(taken from [`drm_crtc_state.adjusted`](#c.drm_crtc_state "drm_crtc_state") mode for atomic modeset drivers) to
match the scanout position reported.

Note that atomic drivers must call [`drm_calc_timestamping_constants()`](#c.drm_calc_timestamping_constants "drm_calc_timestamping_constants") before
enabling a CRTC. The atomic helpers already take care of that in
[`drm_atomic_helper_calc_timestamping_constants()`](drm-kms-helpers.html#c.drm_atomic_helper_calc_timestamping_constants "drm_atomic_helper_calc_timestamping_constants").

**Return**

Returns true on success, and false on failure, i.e. when no accurate
timestamp could be acquired.

bool drm\_crtc\_vblank\_helper\_get\_vblank\_timestamp(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, int \*max\_error, ktime\_t \*vblank\_time, bool in\_vblank\_irq)
:   precise vblank timestamp helper

**Parameters**

`struct drm_crtc *crtc`
:   CRTC whose vblank timestamp to retrieve

`int *max_error`
:   Desired maximum allowable error in timestamps (nanosecs)
    On return contains true maximum error of timestamp

`ktime_t *vblank_time`
:   Pointer to time which should receive the timestamp

`bool in_vblank_irq`
:   True when called from [`drm_crtc_handle_vblank()`](#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank"). Some drivers
    need to apply some workarounds for gpu-specific vblank irq quirks
    if flag is set.

**Description**

Implements calculation of exact vblank timestamps from given drm\_display\_mode
timings and current video scanout position of a CRTC. This can be directly
used as the [`drm_crtc_funcs.get_vblank_timestamp`](#c.drm_crtc_funcs "drm_crtc_funcs") implementation of a kms
driver if [`drm_crtc_helper_funcs.get_scanout_position`](drm-kms-helpers.html#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") is implemented.

The current implementation only handles standard video modes. For double scan
and interlaced modes the driver is supposed to adjust the hardware mode
(taken from [`drm_crtc_state.adjusted`](#c.drm_crtc_state "drm_crtc_state") mode for atomic modeset drivers) to
match the scanout position reported.

Note that atomic drivers must call [`drm_calc_timestamping_constants()`](#c.drm_calc_timestamping_constants "drm_calc_timestamping_constants") before
enabling a CRTC. The atomic helpers already take care of that in
[`drm_atomic_helper_calc_timestamping_constants()`](drm-kms-helpers.html#c.drm_atomic_helper_calc_timestamping_constants "drm_atomic_helper_calc_timestamping_constants").

**Return**

Returns true on success, and false on failure, i.e. when no accurate
timestamp could be acquired.

u64 drm\_crtc\_vblank\_count(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   retrieve “cooked” vblank counter value

**Parameters**

`struct drm_crtc *crtc`
:   which counter to retrieve

**Description**

Fetches the “cooked” vblank count value that represents the number of
vblank events since the system was booted, including lost events due to
modesetting activity. Note that this timer isn’t correct against a racing
vblank interrupt (since it only reports the software vblank counter), see
[`drm_crtc_accurate_vblank_count()`](#c.drm_crtc_accurate_vblank_count "drm_crtc_accurate_vblank_count") for such use-cases.

Note that for a given vblank counter value [`drm_crtc_handle_vblank()`](#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank")
and [`drm_crtc_vblank_count()`](#c.drm_crtc_vblank_count "drm_crtc_vblank_count") or [`drm_crtc_vblank_count_and_time()`](#c.drm_crtc_vblank_count_and_time "drm_crtc_vblank_count_and_time")
provide a barrier: Any writes done before calling
[`drm_crtc_handle_vblank()`](#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank") will be visible to callers of the later
functions, if the vblank count is the same or a later one.

See also [`drm_vblank_crtc.count`](#c.drm_vblank_crtc "drm_vblank_crtc").

**Return**

The software vblank counter.

u64 drm\_crtc\_vblank\_count\_and\_time(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, ktime\_t \*vblanktime)
:   retrieve “cooked” vblank counter value and the system timestamp corresponding to that vblank counter value

**Parameters**

`struct drm_crtc *crtc`
:   which counter to retrieve

`ktime_t *vblanktime`
:   Pointer to time to receive the vblank timestamp.

**Description**

Fetches the “cooked” vblank count value that represents the number of
vblank events since the system was booted, including lost events due to
modesetting activity. Returns corresponding system timestamp of the time
of the vblank interval that corresponds to the current vblank counter value.

Note that for a given vblank counter value [`drm_crtc_handle_vblank()`](#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank")
and [`drm_crtc_vblank_count()`](#c.drm_crtc_vblank_count "drm_crtc_vblank_count") or [`drm_crtc_vblank_count_and_time()`](#c.drm_crtc_vblank_count_and_time "drm_crtc_vblank_count_and_time")
provide a barrier: Any writes done before calling
[`drm_crtc_handle_vblank()`](#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank") will be visible to callers of the later
functions, if the vblank count is the same or a later one.

See also [`drm_vblank_crtc.count`](#c.drm_vblank_crtc "drm_vblank_crtc").

int drm\_crtc\_next\_vblank\_start(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, ktime\_t \*vblanktime)
:   calculate the time of the next vblank

**Parameters**

`struct drm_crtc *crtc`
:   the crtc for which to calculate next vblank time

`ktime_t *vblanktime`
:   pointer to time to receive the next vblank timestamp.

**Description**

Calculate the expected time of the start of the next vblank period,
based on time of previous vblank and frame duration

void drm\_crtc\_arm\_vblank\_event(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_pending\_vblank\_event](#c.drm_pending_vblank_event "drm_pending_vblank_event") \*e)
:   arm vblank event after pageflip

**Parameters**

`struct drm_crtc *crtc`
:   the source CRTC of the vblank event

`struct drm_pending_vblank_event *e`
:   the event to send

**Description**

A lot of drivers need to generate vblank events for the very next vblank
interrupt. For example when the page flip interrupt happens when the page
flip gets armed, but not when it actually executes within the next vblank
period. This helper function implements exactly the required vblank arming
behaviour.

**NOTE**

Drivers using this to send out the [`drm_crtc_state.event`](#c.drm_crtc_state "drm_crtc_state") as part of an
atomic commit must ensure that the next vblank happens at exactly the same
time as the atomic commit is committed to the hardware. This function itself
does **not** protect against the next vblank interrupt racing with either this
function call or the atomic commit operation. A possible sequence could be:

1. Driver commits new hardware state into vblank-synchronized registers.
2. A vblank happens, committing the hardware state. Also the corresponding
   vblank interrupt is fired off and fully processed by the interrupt
   handler.
3. The atomic commit operation proceeds to call [`drm_crtc_arm_vblank_event()`](#c.drm_crtc_arm_vblank_event "drm_crtc_arm_vblank_event").
4. The event is only send out for the next vblank, which is wrong.

An equivalent race can happen when the driver calls
[`drm_crtc_arm_vblank_event()`](#c.drm_crtc_arm_vblank_event "drm_crtc_arm_vblank_event") before writing out the new hardware state.

The only way to make this work safely is to prevent the vblank from firing
(and the hardware from committing anything else) until the entire atomic
commit sequence has run to completion. If the hardware does not have such a
feature (e.g. using a “go” bit), then it is unsafe to use this functions.
Instead drivers need to manually send out the event from their interrupt
handler by calling [`drm_crtc_send_vblank_event()`](#c.drm_crtc_send_vblank_event "drm_crtc_send_vblank_event") and make sure that there’s no
possible race with the hardware committing the atomic update.

Caller must hold a vblank reference for the event **e** acquired by a
[`drm_crtc_vblank_get()`](#c.drm_crtc_vblank_get "drm_crtc_vblank_get"), which will be dropped when the next vblank arrives.

void drm\_crtc\_send\_vblank\_event(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_pending\_vblank\_event](#c.drm_pending_vblank_event "drm_pending_vblank_event") \*e)
:   helper to send vblank event after pageflip

**Parameters**

`struct drm_crtc *crtc`
:   the source CRTC of the vblank event

`struct drm_pending_vblank_event *e`
:   the event to send

**Description**

Updates sequence # and timestamp on event for the most recently processed
vblank, and sends it to userspace. Caller must hold event lock.

See [`drm_crtc_arm_vblank_event()`](#c.drm_crtc_arm_vblank_event "drm_crtc_arm_vblank_event") for a helper which can be used in certain
situation, especially to send out events for atomic commit operations.

int drm\_crtc\_vblank\_get(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   get a reference count on vblank events

**Parameters**

`struct drm_crtc *crtc`
:   which CRTC to own

**Description**

Acquire a reference count on vblank events to avoid having them disabled
while in use.

**Return**

Zero on success or a negative error code on failure.

void drm\_crtc\_vblank\_put(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   give up ownership of vblank events

**Parameters**

`struct drm_crtc *crtc`
:   which counter to give up

**Description**

Release ownership of a given vblank counter, turning off interrupts
if possible. Disable interrupts after [`drm_vblank_crtc_config.offdelay_ms`](#c.drm_vblank_crtc_config "drm_vblank_crtc_config")
milliseconds.

int drm\_crtc\_wait\_one\_vblank(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   wait for one vblank

**Parameters**

`struct drm_crtc *crtc`
:   DRM crtc

**Description**

This waits for one vblank to pass on **crtc**, using the irq driver interfaces.
It is a failure to call this when the vblank irq for **crtc** is disabled, e.g.
due to lack of driver support or because the crtc is off.

**Return**

0 on success, negative error on failures.

void drm\_crtc\_vblank\_off(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   disable vblank events on a CRTC

**Parameters**

`struct drm_crtc *crtc`
:   CRTC in question

**Description**

Drivers can use this function to shut down the vblank interrupt handling when
disabling a crtc. This function ensures that the latest vblank frame count is
stored so that drm\_vblank\_on can restore it again.

Drivers must use this function when the hardware vblank counter can get
reset, e.g. when suspending or disabling the **crtc** in general.

void drm\_crtc\_vblank\_reset(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   reset vblank state to off on a CRTC

**Parameters**

`struct drm_crtc *crtc`
:   CRTC in question

**Description**

Drivers can use this function to reset the vblank state to off at load time.
Drivers should use this together with the [`drm_crtc_vblank_off()`](#c.drm_crtc_vblank_off "drm_crtc_vblank_off") and
[`drm_crtc_vblank_on()`](#c.drm_crtc_vblank_on "drm_crtc_vblank_on") functions. The difference compared to
[`drm_crtc_vblank_off()`](#c.drm_crtc_vblank_off "drm_crtc_vblank_off") is that this function doesn’t save the vblank counter
and hence doesn’t need to call any driver hooks.

This is useful for recovering driver state e.g. on driver load, or on resume.

void drm\_crtc\_set\_max\_vblank\_count(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, u32 max\_vblank\_count)
:   configure the hw max vblank counter value

**Parameters**

`struct drm_crtc *crtc`
:   CRTC in question

`u32 max_vblank_count`
:   max hardware vblank counter value

**Description**

Update the maximum hardware vblank counter value for **crtc**
at runtime. Useful for hardware where the operation of the
hardware vblank counter depends on the currently active
display configuration.

For example, if the hardware vblank counter does not work
when a specific connector is active the maximum can be set
to zero. And when that specific connector isn’t active the
maximum can again be set to the appropriate non-zero value.

If used, must be called before `drm_vblank_on()`.

void drm\_crtc\_vblank\_on\_config(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, const struct [drm\_vblank\_crtc\_config](#c.drm_vblank_crtc_config "drm_vblank_crtc_config") \*config)
:   enable vblank events on a CRTC with custom configuration options

**Parameters**

`struct drm_crtc *crtc`
:   CRTC in question

`const struct drm_vblank_crtc_config *config`
:   Vblank configuration value

**Description**

See [`drm_crtc_vblank_on()`](#c.drm_crtc_vblank_on "drm_crtc_vblank_on"). In addition, this function allows you to provide a
custom vblank configuration for a given CRTC.

Note that **config** is copied, the pointer does not need to stay valid beyond
this function call. For details of the parameters see
[`struct drm_vblank_crtc_config`](#c.drm_vblank_crtc_config "drm_vblank_crtc_config").

void drm\_crtc\_vblank\_on(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   enable vblank events on a CRTC

**Parameters**

`struct drm_crtc *crtc`
:   CRTC in question

**Description**

This functions restores the vblank interrupt state captured with
[`drm_crtc_vblank_off()`](#c.drm_crtc_vblank_off "drm_crtc_vblank_off") again and is generally called when enabling **crtc**. Note
that calls to [`drm_crtc_vblank_on()`](#c.drm_crtc_vblank_on "drm_crtc_vblank_on") and [`drm_crtc_vblank_off()`](#c.drm_crtc_vblank_off "drm_crtc_vblank_off") can be
unbalanced and so can also be unconditionally called in driver load code to
reflect the current hardware state of the crtc.

Note that unlike in [`drm_crtc_vblank_on_config()`](#c.drm_crtc_vblank_on_config "drm_crtc_vblank_on_config"), default values are used.

void drm\_crtc\_vblank\_restore(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   estimate missed vblanks and update vblank count.

**Parameters**

`struct drm_crtc *crtc`
:   CRTC in question

**Description**

Power manamement features can cause frame counter resets between vblank
disable and enable. Drivers can use this function in their
[`drm_crtc_funcs.enable_vblank`](#c.drm_crtc_funcs "drm_crtc_funcs") implementation to estimate missed vblanks since
the last [`drm_crtc_funcs.disable_vblank`](#c.drm_crtc_funcs "drm_crtc_funcs") using timestamps and update the
vblank counter.

Note that drivers must have race-free high-precision timestamping support,
i.e. [`drm_crtc_funcs.get_vblank_timestamp`](#c.drm_crtc_funcs "drm_crtc_funcs") must be hooked up and
[`drm_vblank_crtc_config.disable_immediate`](#c.drm_vblank_crtc_config "drm_vblank_crtc_config") must be set to indicate the
time-stamping functions are race-free against vblank hardware counter
increments.

bool drm\_handle\_vblank(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, unsigned int pipe)
:   handle a vblank event

**Parameters**

`struct drm_device *dev`
:   DRM device

`unsigned int pipe`
:   index of CRTC where this event occurred

**Description**

Drivers should call this routine in their vblank interrupt handlers to
update the vblank counter and send any signals that may be pending.

This is the legacy version of [`drm_crtc_handle_vblank()`](#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank").

bool drm\_crtc\_handle\_vblank(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   handle a vblank event

**Parameters**

`struct drm_crtc *crtc`
:   where this event occurred

**Description**

Drivers should call this routine in their vblank interrupt handlers to
update the vblank counter and send any signals that may be pending.

This is the native KMS version of [`drm_handle_vblank()`](#c.drm_handle_vblank "drm_handle_vblank").

Note that for a given vblank counter value [`drm_crtc_handle_vblank()`](#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank")
and [`drm_crtc_vblank_count()`](#c.drm_crtc_vblank_count "drm_crtc_vblank_count") or [`drm_crtc_vblank_count_and_time()`](#c.drm_crtc_vblank_count_and_time "drm_crtc_vblank_count_and_time")
provide a barrier: Any writes done before calling
[`drm_crtc_handle_vblank()`](#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank") will be visible to callers of the later
functions, if the vblank count is the same or a later one.

See also [`drm_vblank_crtc.count`](#c.drm_vblank_crtc "drm_vblank_crtc").

**Return**

True if the event was successfully handled, false on failure.

int drm\_crtc\_vblank\_start\_timer(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   Starts the vblank timer on the given CRTC

**Parameters**

`struct drm_crtc *crtc`
:   the CRTC

**Description**

Drivers should call this function from their CRTC’s enable\_vblank
function to start a vblank timer. The timer will fire after the duration
of a full frame. [`drm_crtc_vblank_cancel_timer()`](#c.drm_crtc_vblank_cancel_timer "drm_crtc_vblank_cancel_timer") disables a running timer.

**Return**

0 on success, or a negative errno code otherwise.

void drm\_crtc\_vblank\_cancel\_timer(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   Cancels the given CRTC’s vblank timer

**Parameters**

`struct drm_crtc *crtc`
:   the CRTC

**Description**

Drivers should call this function from their CRTC’s disable\_vblank
function to stop a vblank timer.

void drm\_crtc\_vblank\_get\_vblank\_timeout(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, ktime\_t \*vblank\_time)
:   Returns the vblank timeout

**Parameters**

`struct drm_crtc *crtc`
:   The CRTC

`ktime_t *vblank_time`
:   Returns the next vblank timestamp

**Description**

The helper [`drm_crtc_vblank_get_vblank_timeout()`](#c.drm_crtc_vblank_get_vblank_timeout "drm_crtc_vblank_get_vblank_timeout") returns the next vblank
timestamp of the CRTC’s vblank timer according to the timer’s expiry
time.

## Vertical Blank Work

Many DRM drivers need to program hardware in a time-sensitive manner, many
times with a deadline of starting and finishing within a certain region of
the scanout. Most of the time the safest way to accomplish this is to
simply do said time-sensitive programming in the driver’s IRQ handler,
which allows drivers to avoid being preempted during these critical
regions. Or even better, the hardware may even handle applying such
time-critical programming independently of the CPU.

While there’s a decent amount of hardware that’s designed so that the CPU
doesn’t need to be concerned with extremely time-sensitive programming,
there’s a few situations where it can’t be helped. Some unforgiving
hardware may require that certain time-sensitive programming be handled
completely by the CPU, and said programming may even take too long to
handle in an IRQ handler. Another such situation would be where the driver
needs to perform a task that needs to complete within a specific scanout
period, but might possibly block and thus cannot be handled in an IRQ
context. Both of these situations can’t be solved perfectly in Linux since
we’re not a realtime kernel, and thus the scheduler may cause us to miss
our deadline if it decides to preempt us. But for some drivers, it’s good
enough if we can lower our chance of being preempted to an absolute
minimum.

This is where [`drm_vblank_work`](#c.drm_vblank_work "drm_vblank_work") comes in. [`drm_vblank_work`](#c.drm_vblank_work "drm_vblank_work") provides a simple
generic delayed work implementation which delays work execution until a
particular vblank has passed, and then executes the work at realtime
priority. This provides the best possible chance at performing
time-sensitive hardware programming on time, even when the system is under
heavy load. [`drm_vblank_work`](#c.drm_vblank_work "drm_vblank_work") also supports rescheduling, so that self
re-arming work items can be easily implemented.

### Vertical Blank Work Functions Reference

struct drm\_vblank\_work
:   A delayed work item which delays until a target vblank passes, and then executes at realtime priority outside of IRQ context.

**Definition**:

```
struct drm_vblank_work {
    struct kthread_work base;
    struct drm_vblank_crtc *vblank;
    u64 count;
    int cancelling;
    struct list_head node;
};
```

**Members**

`base`
:   The base `kthread_work` item which will be executed by
    [`drm_vblank_crtc.worker`](#c.drm_vblank_crtc "drm_vblank_crtc"). Drivers should not interact with this
    directly, and instead rely on [`drm_vblank_work_init()`](#c.drm_vblank_work_init "drm_vblank_work_init") to initialize
    this.

`vblank`
:   A pointer to [`drm_vblank_crtc`](#c.drm_vblank_crtc "drm_vblank_crtc") this work item belongs to.

`count`
:   The target vblank this work will execute on. Drivers should
    not modify this value directly, and instead use
    [`drm_vblank_work_schedule()`](#c.drm_vblank_work_schedule "drm_vblank_work_schedule")

`cancelling`
:   The number of [`drm_vblank_work_cancel_sync()`](#c.drm_vblank_work_cancel_sync "drm_vblank_work_cancel_sync") calls that
    are currently running. A work item cannot be rescheduled until all
    calls have finished.

`node`
:   The position of this work item in
    [`drm_vblank_crtc.pending_work`](#c.drm_vblank_crtc "drm_vblank_crtc").

**Description**

See also:
[`drm_vblank_work_schedule()`](#c.drm_vblank_work_schedule "drm_vblank_work_schedule")
[`drm_vblank_work_init()`](#c.drm_vblank_work_init "drm_vblank_work_init")
[`drm_vblank_work_cancel_sync()`](#c.drm_vblank_work_cancel_sync "drm_vblank_work_cancel_sync")
[`drm_vblank_work_flush()`](#c.drm_vblank_work_flush "drm_vblank_work_flush")
[`drm_vblank_work_flush_all()`](#c.drm_vblank_work_flush_all "drm_vblank_work_flush_all")

to\_drm\_vblank\_work

`to_drm_vblank_work (_work)`

> Retrieve the respective [`drm_vblank_work`](#c.drm_vblank_work "drm_vblank_work") item from a `kthread_work`

**Parameters**

`_work`
:   The `kthread_work` embedded inside a [`drm_vblank_work`](#c.drm_vblank_work "drm_vblank_work")

int drm\_vblank\_work\_schedule(struct [drm\_vblank\_work](#c.drm_vblank_work "drm_vblank_work") \*work, u64 count, bool nextonmiss)
:   schedule a vblank work

**Parameters**

`struct drm_vblank_work *work`
:   vblank work to schedule

`u64 count`
:   target vblank count

`bool nextonmiss`
:   defer until the next vblank if target vblank was missed

**Description**

Schedule **work** for execution once the crtc vblank count reaches **count**.

If the crtc vblank count has already reached **count** and **nextonmiss** is
`false` the work starts to execute immediately.

If the crtc vblank count has already reached **count** and **nextonmiss** is
`true` the work is deferred until the next vblank (as if **count** has been
specified as crtc vblank count + 1).

If **work** is already scheduled, this function will reschedule said work
using the new **count**. This can be used for self-rearming work items.

**Return**

`1` if **work** was successfully (re)scheduled, `0` if it was either already
scheduled or cancelled, or a negative error code on failure.

bool drm\_vblank\_work\_cancel\_sync(struct [drm\_vblank\_work](#c.drm_vblank_work "drm_vblank_work") \*work)
:   cancel a vblank work and wait for it to finish executing

**Parameters**

`struct drm_vblank_work *work`
:   vblank work to cancel

**Description**

Cancel an already scheduled vblank work and wait for its
execution to finish.

On return, **work** is guaranteed to no longer be scheduled or running, even
if it’s self-arming.

**Return**

`True` if the work was cancelled before it started to execute, `false`
otherwise.

void drm\_vblank\_work\_flush(struct [drm\_vblank\_work](#c.drm_vblank_work "drm_vblank_work") \*work)
:   wait for a scheduled vblank work to finish executing

**Parameters**

`struct drm_vblank_work *work`
:   vblank work to flush

**Description**

Wait until **work** has finished executing once.

void drm\_vblank\_work\_flush\_all(struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc)
:   flush all currently pending vblank work on crtc.

**Parameters**

`struct drm_crtc *crtc`
:   crtc for which vblank work to flush

**Description**

Wait until all currently queued vblank work on **crtc**
has finished executing once.

void drm\_vblank\_work\_init(struct [drm\_vblank\_work](#c.drm_vblank_work "drm_vblank_work") \*work, struct [drm\_crtc](#c.drm_crtc "drm_crtc") \*crtc, void (\*func)(struct kthread\_work \*work))
:   initialize a vblank work item

**Parameters**

`struct drm_vblank_work *work`
:   vblank work item

`struct drm_crtc *crtc`
:   CRTC whose vblank will trigger the work execution

`void (*func)(struct kthread_work *work)`
:   work function to be executed

**Description**

Initialize a vblank work item for a specific crtc.
