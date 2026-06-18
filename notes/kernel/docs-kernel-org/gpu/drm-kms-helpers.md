# Mode Setting Helper Functions

> 출처(원문): https://docs.kernel.org/gpu/drm-kms-helpers.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Mode Setting Helper Functions

The DRM subsystem aims for a strong separation between core code and helper
libraries. Core code takes care of general setup and teardown and decoding
userspace requests to kernel internal objects. Everything else is handled by a
large set of helper libraries, which can be combined freely to pick and choose
for each driver what fits, and avoid shared code where special behaviour is
needed.

This distinction between core code and helpers is especially strong in the
modesetting code, where there’s a shared userspace ABI for all drivers. This is
in contrast to the render side, where pretty much everything (with very few
exceptions) can be considered optional helper code.

There are a few areas these helpers can grouped into:

* Helpers to implement modesetting. The important ones here are the atomic
  helpers. Old drivers still often use the legacy CRTC helpers. They both share
  the same set of common helper vtables. For really simple drivers (anything
  that would have been a great fit in the deprecated fbdev subsystem) there’s
  also the simple display pipe helpers.
* There’s a big pile of helpers for handling outputs. First the generic bridge
  helpers for handling encoder and transcoder IP blocks. Second the panel helpers
  for handling panel-related information and logic. Plus then a big set of
  helpers for the various sink standards (DisplayPort, HDMI, MIPI DSI). Finally
  there’s also generic helpers for handling output probing, and for dealing with
  EDIDs.
* The last group of helpers concerns itself with the frontend side of a display
  pipeline: Planes, handling rectangles for visibility checking and scissoring,
  flip queues and assorted bits.

## Modeset Helper Reference for Common Vtables

The DRM mode setting helper functions are common code for drivers to use if
they wish. Drivers are not forced to use this code in their
implementations but it would be useful if the code they do use at least
provides a consistent interface and operation to userspace. Therefore it is
highly recommended to use the provided helpers as much as possible.

Because there is only one pointer per modeset object to hold a vfunc table
for helper libraries they are by necessity shared among the different
helpers.

To make this clear all the helper vtables are pulled together in this location here.

struct drm\_crtc\_helper\_funcs
:   helper operations for CRTCs

**Definition**:

```
struct drm_crtc_helper_funcs {
    void (*dpms)(struct drm_crtc *crtc, int mode);
    void (*prepare)(struct drm_crtc *crtc);
    void (*commit)(struct drm_crtc *crtc);
    enum drm_mode_status (*mode_valid)(struct drm_crtc *crtc, const struct drm_display_mode *mode);
    bool (*mode_fixup)(struct drm_crtc *crtc, const struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode);
    int (*mode_set)(struct drm_crtc *crtc, struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode, int x, int y, struct drm_framebuffer *old_fb);
    void (*mode_set_nofb)(struct drm_crtc *crtc);
    int (*mode_set_base)(struct drm_crtc *crtc, int x, int y, struct drm_framebuffer *old_fb);
    void (*disable)(struct drm_crtc *crtc);
    int (*atomic_check)(struct drm_crtc *crtc, struct drm_atomic_state *state);
    void (*atomic_begin)(struct drm_crtc *crtc, struct drm_atomic_state *state);
    void (*atomic_flush)(struct drm_crtc *crtc, struct drm_atomic_state *state);
    void (*atomic_enable)(struct drm_crtc *crtc, struct drm_atomic_state *state);
    void (*atomic_disable)(struct drm_crtc *crtc, struct drm_atomic_state *state);
    bool (*get_scanout_position)(struct drm_crtc *crtc, bool in_vblank_irq, int *vpos, int *hpos, ktime_t *stime, ktime_t *etime, const struct drm_display_mode *mode);
    bool (*handle_vblank_timeout)(struct drm_crtc *crtc);
};
```

**Members**

`dpms`
:   Callback to control power levels on the CRTC. If the mode passed in
    is unsupported, the provider must use the next lowest power level.
    This is used by the legacy CRTC helpers to implement DPMS
    functionality in [`drm_helper_connector_dpms()`](#c.drm_helper_connector_dpms "drm_helper_connector_dpms").

    This callback is also used to disable a CRTC by calling it with
    DRM\_MODE\_DPMS\_OFF if the **disable** hook isn’t used.

    This callback is used by the legacy CRTC helpers. Atomic helpers
    also support using this hook for enabling and disabling a CRTC to
    facilitate transitions to atomic, but it is deprecated. Instead
    **atomic\_enable** and **atomic\_disable** should be used.

`prepare`
:   This callback should prepare the CRTC for a subsequent modeset, which
    in practice means the driver should disable the CRTC if it is
    running. Most drivers ended up implementing this by calling their
    **dpms** hook with DRM\_MODE\_DPMS\_OFF.

    This callback is used by the legacy CRTC helpers. Atomic helpers
    also support using this hook for disabling a CRTC to facilitate
    transitions to atomic, but it is deprecated. Instead **atomic\_disable**
    should be used.

`commit`
:   This callback should commit the new mode on the CRTC after a modeset,
    which in practice means the driver should enable the CRTC. Most
    drivers ended up implementing this by calling their **dpms** hook with
    DRM\_MODE\_DPMS\_ON.

    This callback is used by the legacy CRTC helpers. Atomic helpers
    also support using this hook for enabling a CRTC to facilitate
    transitions to atomic, but it is deprecated. Instead **atomic\_enable**
    should be used.

`mode_valid`
:   This callback is used to check if a specific mode is valid in this
    crtc. This should be implemented if the crtc has some sort of
    restriction in the modes it can display. For example, a given crtc
    may be responsible to set a clock value. If the clock can not
    produce all the values for the available modes then this callback
    can be used to restrict the number of modes to only the ones that
    can be displayed.

    This hook is used by the probe helpers to filter the mode list in
    [`drm_helper_probe_single_connector_modes()`](#c.drm_helper_probe_single_connector_modes "drm_helper_probe_single_connector_modes"), and it is used by the
    atomic helpers to validate modes supplied by userspace in
    [`drm_atomic_helper_check_modeset()`](#c.drm_atomic_helper_check_modeset "drm_atomic_helper_check_modeset").

    This function is optional.

    NOTE:

    Since this function is both called from the check phase of an atomic
    commit, and the mode validation in the probe paths it is not allowed
    to look at anything else but the passed-in mode, and validate it
    against configuration-invariant hardware constraints. Any further
    limits which depend upon the configuration can only be checked in
    **mode\_fixup** or **atomic\_check**.

    RETURNS:

    drm\_mode\_status Enum

`mode_fixup`
:   This callback is used to validate a mode. The parameter mode is the
    display mode that userspace requested, adjusted\_mode is the mode the
    encoders need to be fed with. Note that this is the inverse semantics
    of the meaning for the [`drm_encoder`](drm-kms.html#c.drm_encoder "drm_encoder") and [`drm_bridge_funcs.mode_fixup`](#c.drm_bridge_funcs "drm_bridge_funcs")
    vfunc. If the CRTC cannot support the requested conversion from mode
    to adjusted\_mode it should reject the modeset. See also
    [`drm_crtc_state.adjusted_mode`](drm-kms.html#c.drm_crtc_state "drm_crtc_state") for more details.

    This function is used by both legacy CRTC helpers and atomic helpers.
    With atomic helpers it is optional.

    NOTE:

    This function is called in the check phase of atomic modesets, which
    can be aborted for any reason (including on userspace’s request to
    just check whether a configuration would be possible). Atomic drivers
    MUST NOT touch any persistent state (hardware or software) or data
    structures except the passed in adjusted\_mode parameter.

    This is in contrast to the legacy CRTC helpers where this was
    allowed.

    Atomic drivers which need to inspect and adjust more state should
    instead use the **atomic\_check** callback, but note that they’re not
    perfectly equivalent: **mode\_valid** is called from
    [`drm_atomic_helper_check_modeset()`](#c.drm_atomic_helper_check_modeset "drm_atomic_helper_check_modeset"), but **atomic\_check** is called from
    [`drm_atomic_helper_check_planes()`](#c.drm_atomic_helper_check_planes "drm_atomic_helper_check_planes"), because originally it was meant for
    plane update checks only.

    Also beware that userspace can request its own custom modes, neither
    core nor helpers filter modes to the list of probe modes reported by
    the GETCONNECTOR IOCTL and stored in [`drm_connector.modes`](drm-kms.html#c.drm_connector "drm_connector"). To ensure
    that modes are filtered consistently put any CRTC constraints and
    limits checks into **mode\_valid**.

    RETURNS:

    True if an acceptable configuration is possible, false if the modeset
    operation should be rejected.

`mode_set`
:   This callback is used by the legacy CRTC helpers to set a new mode,
    position and framebuffer. Since it ties the primary plane to every
    mode change it is incompatible with universal plane support. And
    since it can’t update other planes it’s incompatible with atomic
    modeset support.

    This callback is only used by CRTC helpers and deprecated.

    RETURNS:

    0 on success or a negative error code on failure.

`mode_set_nofb`
:   This callback is used to update the display mode of a CRTC without
    changing anything of the primary plane configuration. This fits the
    requirement of atomic and hence is used by the atomic helpers.

    Note that the display pipe is completely off when this function is
    called. Atomic drivers which need hardware to be running before they
    program the new display mode (e.g. because they implement runtime PM)
    should not use this hook. This is because the helper library calls
    this hook only once per mode change and not every time the display
    pipeline is suspended using either DPMS or the new “ACTIVE” property.
    Which means register values set in this callback might get reset when
    the CRTC is suspended, but not restored. Such drivers should instead
    move all their CRTC setup into the **atomic\_enable** callback.

    This callback is optional.

`mode_set_base`
:   This callback is used by the legacy CRTC helpers to set a new
    framebuffer and scanout position. It is optional and used as an
    optimized fast-path instead of a full mode set operation with all the
    resulting flickering. If it is not present
    [`drm_crtc_helper_set_config()`](#c.drm_crtc_helper_set_config "drm_crtc_helper_set_config") will fall back to a full modeset, using
    the **mode\_set** callback. Since it can’t update other planes it’s
    incompatible with atomic modeset support.

    This callback is only used by the CRTC helpers and deprecated.

    RETURNS:

    0 on success or a negative error code on failure.

`disable`
:   This callback should be used to disable the CRTC. With the atomic
    drivers it is called after all encoders connected to this CRTC have
    been shut off already using their own
    [`drm_encoder_helper_funcs.disable`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") hook. If that sequence is too
    simple drivers can just add their own hooks and call it from this
    CRTC callback here by looping over all encoders connected to it using
    `for_each_encoder_on_crtc()`.

    This hook is used both by legacy CRTC helpers and atomic helpers.
    Atomic drivers don’t need to implement it if there’s no need to
    disable anything at the CRTC level. To ensure that runtime PM
    handling (using either DPMS or the new “ACTIVE” property) works
    **disable** must be the inverse of **atomic\_enable** for atomic drivers.
    Atomic drivers should consider to use **atomic\_disable** instead of
    this one.

    NOTE:

    With legacy CRTC helpers there’s a big semantic difference between
    **disable** and other hooks (like **prepare** or **dpms**) used to shut down a
    CRTC: **disable** is only called when also logically disabling the
    display pipeline and needs to release any resources acquired in
    **mode\_set** (like shared PLLs, or again release pinned framebuffers).

    Therefore **disable** must be the inverse of **mode\_set** plus **commit** for
    drivers still using legacy CRTC helpers, which is different from the
    rules under atomic.

`atomic_check`
:   Drivers should check plane-update related CRTC constraints in this
    hook. They can also check mode related limitations but need to be
    aware of the calling order, since this hook is used by
    [`drm_atomic_helper_check_planes()`](#c.drm_atomic_helper_check_planes "drm_atomic_helper_check_planes") whereas the preparations needed to
    check output routing and the display mode is done in
    [`drm_atomic_helper_check_modeset()`](#c.drm_atomic_helper_check_modeset "drm_atomic_helper_check_modeset"). Therefore drivers that want to
    check output routing and display mode constraints in this callback
    must ensure that [`drm_atomic_helper_check_modeset()`](#c.drm_atomic_helper_check_modeset "drm_atomic_helper_check_modeset") has been called
    beforehand. This is calling order used by the default helper
    implementation in [`drm_atomic_helper_check()`](#c.drm_atomic_helper_check "drm_atomic_helper_check").

    When using [`drm_atomic_helper_check_planes()`](#c.drm_atomic_helper_check_planes "drm_atomic_helper_check_planes") this hook is called
    after the [`drm_plane_helper_funcs.atomic_check`](#c.drm_plane_helper_funcs "drm_plane_helper_funcs") hook for planes, which
    allows drivers to assign shared resources requested by planes in this
    callback here. For more complicated dependencies the driver can call
    the provided check helpers multiple times until the computed state
    has a final configuration and everything has been checked.

    This function is also allowed to inspect any other object’s state and
    can add more state objects to the atomic commit if needed. Care must
    be taken though to ensure that state check and compute functions for
    these added states are all called, and derived state in other objects
    all updated. Again the recommendation is to just call check helpers
    until a maximal configuration is reached.

    This callback is used by the atomic modeset helpers, but it is
    optional.

    NOTE:

    This function is called in the check phase of an atomic update. The
    driver is not allowed to change anything outside of the free-standing
    state object passed-in.

    Also beware that userspace can request its own custom modes, neither
    core nor helpers filter modes to the list of probe modes reported by
    the GETCONNECTOR IOCTL and stored in [`drm_connector.modes`](drm-kms.html#c.drm_connector "drm_connector"). To ensure
    that modes are filtered consistently put any CRTC constraints and
    limits checks into **mode\_valid**.

    RETURNS:

    0 on success, -EINVAL if the state or the transition can’t be
    supported, -ENOMEM on memory allocation failure and -EDEADLK if an
    attempt to obtain another state object ran into a [`drm_modeset_lock`](drm-kms.html#c.drm_modeset_lock "drm_modeset_lock")
    deadlock.

`atomic_begin`
:   Drivers should prepare for an atomic update of multiple planes on
    a CRTC in this hook. Depending upon hardware this might be vblank
    evasion, blocking updates by setting bits or doing preparatory work
    for e.g. manual update display.

    This hook is called before any plane commit functions are called.

    Note that the power state of the display pipe when this function is
    called depends upon the exact helpers and calling sequence the driver
    has picked. See [`drm_atomic_helper_commit_planes()`](#c.drm_atomic_helper_commit_planes "drm_atomic_helper_commit_planes") for a discussion of
    the tradeoffs and variants of plane commit helpers.

    This callback is used by the atomic modeset helpers, but it is
    optional.

`atomic_flush`
:   Drivers should finalize an atomic update of multiple planes on
    a CRTC in this hook. Depending upon hardware this might include
    checking that vblank evasion was successful, unblocking updates by
    setting bits or setting the GO bit to flush out all updates.

    Simple hardware or hardware with special requirements can commit and
    flush out all updates for all planes from this hook and forgo all the
    other commit hooks for plane updates.

    This hook is called after any plane commit functions are called.

    Note that the power state of the display pipe when this function is
    called depends upon the exact helpers and calling sequence the driver
    has picked. See [`drm_atomic_helper_commit_planes()`](#c.drm_atomic_helper_commit_planes "drm_atomic_helper_commit_planes") for a discussion of
    the tradeoffs and variants of plane commit helpers.

    This callback is used by the atomic modeset helpers, but it is
    optional.

`atomic_enable`
:   This callback should be used to enable the CRTC. With the atomic
    drivers it is called before all encoders connected to this CRTC are
    enabled through the encoder’s own [`drm_encoder_helper_funcs.enable`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs")
    hook. If that sequence is too simple drivers can just add their own
    hooks and call it from this CRTC callback here by looping over all
    encoders connected to it using `for_each_encoder_on_crtc()`.

    This hook is used only by atomic helpers, for symmetry with
    **atomic\_disable**. Atomic drivers don’t need to implement it if there’s
    no need to enable anything at the CRTC level. To ensure that runtime
    PM handling (using either DPMS or the new “ACTIVE” property) works
    **atomic\_enable** must be the inverse of **atomic\_disable** for atomic
    drivers.

    This function is optional.

`atomic_disable`
:   This callback should be used to disable the CRTC. With the atomic
    drivers it is called after all encoders connected to this CRTC have
    been shut off already using their own
    [`drm_encoder_helper_funcs.disable`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") hook. If that sequence is too
    simple drivers can just add their own hooks and call it from this
    CRTC callback here by looping over all encoders connected to it using
    `for_each_encoder_on_crtc()`.

    This hook is used only by atomic helpers. Atomic drivers don’t
    need to implement it if there’s no need to disable anything at the
    CRTC level.

    This function is optional.

`get_scanout_position`
:   Called by vblank timestamping code.

    Returns the current display scanout position from a CRTC and an
    optional accurate [`ktime_get()`](../core-api/timekeeping.html#c.ktime_get "ktime_get") timestamp of when the position was
    measured. Note that this is a helper callback which is only used
    if a driver uses [`drm_crtc_vblank_helper_get_vblank_timestamp()`](drm-kms.html#c.drm_crtc_vblank_helper_get_vblank_timestamp "drm_crtc_vblank_helper_get_vblank_timestamp")
    for the **drm\_crtc\_funcs.get\_vblank\_timestamp** callback.

    Parameters:

    crtc:
    :   The CRTC.

    in\_vblank\_irq:
    :   True when called from [`drm_crtc_handle_vblank()`](drm-kms.html#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank"). Some drivers
        need to apply some workarounds for gpu-specific vblank irq
        quirks if the flag is set.

    vpos:
    :   Target location for current vertical scanout position.

    hpos:
    :   Target location for current horizontal scanout position.

    stime:
    :   Target location for timestamp taken immediately before
        scanout position query. Can be NULL to skip timestamp.

    etime:
    :   Target location for timestamp taken immediately after
        scanout position query. Can be NULL to skip timestamp.

    mode:
    :   Current display timings.

    Returns vpos as a positive number while in active scanout area.
    Returns vpos as a negative number inside vblank, counting the number
    of scanlines to go until end of vblank, e.g., -1 means “one scanline
    until start of active scanout / end of vblank.”

    Returns:

    True on success, false if a reliable scanout position counter could
    not be read out.

`handle_vblank_timeout`
:   Handles timeouts of the vblank timer.

    Called by CRTC’s the vblank timer on each timeout. Semantics is
    equivalient to [`drm_crtc_handle_vblank()`](drm-kms.html#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank"). Implementations should
    invoke [`drm_crtc_handle_vblank()`](drm-kms.html#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank") as part of processing the timeout.

    This callback is optional. If unset, the vblank timer invokes
    [`drm_crtc_handle_vblank()`](drm-kms.html#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank") directly.

**Description**

These hooks are used by the legacy CRTC helpers and the new atomic
modesetting helpers.

void drm\_crtc\_helper\_add(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, const struct [drm\_crtc\_helper\_funcs](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") \*funcs)
:   sets the helper vtable for a crtc

**Parameters**

`struct drm_crtc *crtc`
:   DRM CRTC

`const struct drm_crtc_helper_funcs *funcs`
:   helper vtable to set for **crtc**

struct drm\_encoder\_helper\_funcs
:   helper operations for encoders

**Definition**:

```
struct drm_encoder_helper_funcs {
    void (*dpms)(struct drm_encoder *encoder, int mode);
    enum drm_mode_status (*mode_valid)(struct drm_encoder *crtc, const struct drm_display_mode *mode);
    bool (*mode_fixup)(struct drm_encoder *encoder, const struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode);
    void (*prepare)(struct drm_encoder *encoder);
    void (*commit)(struct drm_encoder *encoder);
    void (*mode_set)(struct drm_encoder *encoder, struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode);
    void (*atomic_mode_set)(struct drm_encoder *encoder, struct drm_crtc_state *crtc_state, struct drm_connector_state *conn_state);
    enum drm_connector_status (*detect)(struct drm_encoder *encoder, struct drm_connector *connector);
    void (*atomic_disable)(struct drm_encoder *encoder, struct drm_atomic_state *state);
    void (*atomic_enable)(struct drm_encoder *encoder, struct drm_atomic_state *state);
    void (*disable)(struct drm_encoder *encoder);
    void (*enable)(struct drm_encoder *encoder);
    int (*atomic_check)(struct drm_encoder *encoder, struct drm_crtc_state *crtc_state, struct drm_connector_state *conn_state);
};
```

**Members**

`dpms`
:   Callback to control power levels on the encoder. If the mode passed in
    is unsupported, the provider must use the next lowest power level.
    This is used by the legacy encoder helpers to implement DPMS
    functionality in [`drm_helper_connector_dpms()`](#c.drm_helper_connector_dpms "drm_helper_connector_dpms").

    This callback is also used to disable an encoder by calling it with
    DRM\_MODE\_DPMS\_OFF if the **disable** hook isn’t used.

    This callback is used by the legacy CRTC helpers. Atomic helpers
    also support using this hook for enabling and disabling an encoder to
    facilitate transitions to atomic, but it is deprecated. Instead
    **enable** and **disable** should be used.

`mode_valid`
:   This callback is used to check if a specific mode is valid in this
    encoder. This should be implemented if the encoder has some sort
    of restriction in the modes it can display. For example, a given
    encoder may be responsible to set a clock value. If the clock can
    not produce all the values for the available modes then this callback
    can be used to restrict the number of modes to only the ones that
    can be displayed.

    This hook is used by the probe helpers to filter the mode list in
    [`drm_helper_probe_single_connector_modes()`](#c.drm_helper_probe_single_connector_modes "drm_helper_probe_single_connector_modes"), and it is used by the
    atomic helpers to validate modes supplied by userspace in
    [`drm_atomic_helper_check_modeset()`](#c.drm_atomic_helper_check_modeset "drm_atomic_helper_check_modeset").

    This function is optional.

    NOTE:

    Since this function is both called from the check phase of an atomic
    commit, and the mode validation in the probe paths it is not allowed
    to look at anything else but the passed-in mode, and validate it
    against configuration-invariant hardware constraints. Any further
    limits which depend upon the configuration can only be checked in
    **mode\_fixup** or **atomic\_check**.

    RETURNS:

    drm\_mode\_status Enum

`mode_fixup`
:   This callback is used to validate and adjust a mode. The parameter
    mode is the display mode that should be fed to the next element in
    the display chain, either the final [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector") or a [`drm_bridge`](#c.drm_bridge "drm_bridge").
    The parameter adjusted\_mode is the input mode the encoder requires. It
    can be modified by this callback and does not need to match mode. See
    also [`drm_crtc_state.adjusted_mode`](drm-kms.html#c.drm_crtc_state "drm_crtc_state") for more details.

    This function is used by both legacy CRTC helpers and atomic helpers.
    This hook is optional.

    NOTE:

    This function is called in the check phase of atomic modesets, which
    can be aborted for any reason (including on userspace’s request to
    just check whether a configuration would be possible). Atomic drivers
    MUST NOT touch any persistent state (hardware or software) or data
    structures except the passed in adjusted\_mode parameter.

    This is in contrast to the legacy CRTC helpers where this was
    allowed.

    Atomic drivers which need to inspect and adjust more state should
    instead use the **atomic\_check** callback. If **atomic\_check** is used,
    this hook isn’t called since **atomic\_check** allows a strict superset
    of the functionality of **mode\_fixup**.

    Also beware that userspace can request its own custom modes, neither
    core nor helpers filter modes to the list of probe modes reported by
    the GETCONNECTOR IOCTL and stored in [`drm_connector.modes`](drm-kms.html#c.drm_connector "drm_connector"). To ensure
    that modes are filtered consistently put any encoder constraints and
    limits checks into **mode\_valid**.

    RETURNS:

    True if an acceptable configuration is possible, false if the modeset
    operation should be rejected.

`prepare`
:   This callback should prepare the encoder for a subsequent modeset,
    which in practice means the driver should disable the encoder if it
    is running. Most drivers ended up implementing this by calling their
    **dpms** hook with DRM\_MODE\_DPMS\_OFF.

    This callback is used by the legacy CRTC helpers. Atomic helpers
    also support using this hook for disabling an encoder to facilitate
    transitions to atomic, but it is deprecated. Instead **disable** should
    be used.

`commit`
:   This callback should commit the new mode on the encoder after a modeset,
    which in practice means the driver should enable the encoder. Most
    drivers ended up implementing this by calling their **dpms** hook with
    DRM\_MODE\_DPMS\_ON.

    This callback is used by the legacy CRTC helpers. Atomic helpers
    also support using this hook for enabling an encoder to facilitate
    transitions to atomic, but it is deprecated. Instead **enable** should
    be used.

`mode_set`
:   This callback is used to update the display mode of an encoder.

    Note that the display pipe is completely off when this function is
    called. Drivers which need hardware to be running before they program
    the new display mode (because they implement runtime PM) should not
    use this hook, because the helper library calls it only once and not
    every time the display pipeline is suspend using either DPMS or the
    new “ACTIVE” property. Such drivers should instead move all their
    encoder setup into the **enable** callback.

    This callback is used both by the legacy CRTC helpers and the atomic
    modeset helpers. It is optional in the atomic helpers.

    NOTE:

    If the driver uses the atomic modeset helpers and needs to inspect
    the connector state or connector display info during mode setting,
    **atomic\_mode\_set** can be used instead.

`atomic_mode_set`
:   This callback is used to update the display mode of an encoder.

    Note that the display pipe is completely off when this function is
    called. Drivers which need hardware to be running before they program
    the new display mode (because they implement runtime PM) should not
    use this hook, because the helper library calls it only once and not
    every time the display pipeline is suspended using either DPMS or the
    new “ACTIVE” property. Such drivers should instead move all their
    encoder setup into the **enable** callback.

    This callback is used by the atomic modeset helpers in place of the
    **mode\_set** callback, if set by the driver. It is optional and should
    be used instead of **mode\_set** if the driver needs to inspect the
    connector state or display info, since there is no direct way to
    go from the encoder to the current connector.

`detect`
:   This callback can be used by drivers who want to do detection on the
    encoder object instead of in connector functions.

    It is not used by any helper and therefore has purely driver-specific
    semantics. New drivers shouldn’t use this and instead just implement
    their own private callbacks.

    FIXME:

    This should just be converted into a pile of driver vfuncs.
    Currently radeon, amdgpu and nouveau are using it.

`atomic_disable`
:   This callback should be used to disable the encoder. With the atomic
    drivers it is called before this encoder’s CRTC has been shut off
    using their own [`drm_crtc_helper_funcs.atomic_disable`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") hook. If that
    sequence is too simple drivers can just add their own driver private
    encoder hooks and call them from CRTC’s callback by looping over all
    encoders connected to it using `for_each_encoder_on_crtc()`.

    This callback is a variant of **disable** that provides the atomic state
    to the driver. If **atomic\_disable** is implemented, **disable** is not
    called by the helpers.

    This hook is only used by atomic helpers. Atomic drivers don’t need
    to implement it if there’s no need to disable anything at the encoder
    level. To ensure that runtime PM handling (using either DPMS or the
    new “ACTIVE” property) works **atomic\_disable** must be the inverse of
    **atomic\_enable**.

`atomic_enable`
:   This callback should be used to enable the encoder. It is called
    after this encoder’s CRTC has been enabled using their own
    [`drm_crtc_helper_funcs.atomic_enable`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") hook. If that sequence is
    too simple drivers can just add their own driver private encoder
    hooks and call them from CRTC’s callback by looping over all encoders
    connected to it using `for_each_encoder_on_crtc()`.

    This callback is a variant of **enable** that provides the atomic state
    to the driver. If **atomic\_enable** is implemented, **enable** is not
    called by the helpers.

    This hook is only used by atomic helpers, it is the opposite of
    **atomic\_disable**. Atomic drivers don’t need to implement it if there’s
    no need to enable anything at the encoder level. To ensure that
    runtime PM handling works **atomic\_enable** must be the inverse of
    **atomic\_disable**.

`disable`
:   This callback should be used to disable the encoder. With the atomic
    drivers it is called before this encoder’s CRTC has been shut off
    using their own [`drm_crtc_helper_funcs.disable`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") hook. If that
    sequence is too simple drivers can just add their own driver private
    encoder hooks and call them from CRTC’s callback by looping over all
    encoders connected to it using `for_each_encoder_on_crtc()`.

    This hook is used both by legacy CRTC helpers and atomic helpers.
    Atomic drivers don’t need to implement it if there’s no need to
    disable anything at the encoder level. To ensure that runtime PM
    handling (using either DPMS or the new “ACTIVE” property) works
    **disable** must be the inverse of **enable** for atomic drivers.

    For atomic drivers also consider **atomic\_disable** and save yourself
    from having to read the NOTE below!

    NOTE:

    With legacy CRTC helpers there’s a big semantic difference between
    **disable** and other hooks (like **prepare** or **dpms**) used to shut down a
    encoder: **disable** is only called when also logically disabling the
    display pipeline and needs to release any resources acquired in
    **mode\_set** (like shared PLLs, or again release pinned framebuffers).

    Therefore **disable** must be the inverse of **mode\_set** plus **commit** for
    drivers still using legacy CRTC helpers, which is different from the
    rules under atomic.

`enable`
:   This callback should be used to enable the encoder. With the atomic
    drivers it is called after this encoder’s CRTC has been enabled using
    their own [`drm_crtc_helper_funcs.enable`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") hook. If that sequence is
    too simple drivers can just add their own driver private encoder
    hooks and call them from CRTC’s callback by looping over all encoders
    connected to it using `for_each_encoder_on_crtc()`.

    This hook is only used by atomic helpers, it is the opposite of
    **disable**. Atomic drivers don’t need to implement it if there’s no
    need to enable anything at the encoder level. To ensure that
    runtime PM handling (using either DPMS or the new “ACTIVE” property)
    works **enable** must be the inverse of **disable** for atomic drivers.

`atomic_check`
:   This callback is used to validate encoder state for atomic drivers.
    Since the encoder is the object connecting the CRTC and connector it
    gets passed both states, to be able to validate interactions and
    update the CRTC to match what the encoder needs for the requested
    connector.

    Since this provides a strict superset of the functionality of
    **mode\_fixup** (the requested and adjusted modes are both available
    through the passed in [`struct drm_crtc_state`](drm-kms.html#c.drm_crtc_state "drm_crtc_state")) **mode\_fixup** is not
    called when **atomic\_check** is implemented.

    This function is used by the atomic helpers, but it is optional.

    NOTE:

    This function is called in the check phase of an atomic update. The
    driver is not allowed to change anything outside of the free-standing
    state objects passed-in or assembled in the overall [`drm_atomic_state`](drm-kms.html#c.drm_atomic_state "drm_atomic_state")
    update tracking structure.

    Also beware that userspace can request its own custom modes, neither
    core nor helpers filter modes to the list of probe modes reported by
    the GETCONNECTOR IOCTL and stored in [`drm_connector.modes`](drm-kms.html#c.drm_connector "drm_connector"). To ensure
    that modes are filtered consistently put any encoder constraints and
    limits checks into **mode\_valid**.

    RETURNS:

    0 on success, -EINVAL if the state or the transition can’t be
    supported, -ENOMEM on memory allocation failure and -EDEADLK if an
    attempt to obtain another state object ran into a [`drm_modeset_lock`](drm-kms.html#c.drm_modeset_lock "drm_modeset_lock")
    deadlock.

**Description**

These hooks are used by the legacy CRTC helpers and the new atomic
modesetting helpers.

void drm\_encoder\_helper\_add(struct [drm\_encoder](drm-kms.html#c.drm_encoder "drm_encoder") \*encoder, const struct [drm\_encoder\_helper\_funcs](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") \*funcs)
:   sets the helper vtable for an encoder

**Parameters**

`struct drm_encoder *encoder`
:   DRM encoder

`const struct drm_encoder_helper_funcs *funcs`
:   helper vtable to set for **encoder**

struct drm\_connector\_helper\_funcs
:   helper operations for connectors

**Definition**:

```
struct drm_connector_helper_funcs {
    int (*get_modes)(struct drm_connector *connector);
    int (*detect_ctx)(struct drm_connector *connector, struct drm_modeset_acquire_ctx *ctx, bool force);
    enum drm_mode_status (*mode_valid)(struct drm_connector *connector, const struct drm_display_mode *mode);
    int (*mode_valid_ctx)(struct drm_connector *connector, const struct drm_display_mode *mode, struct drm_modeset_acquire_ctx *ctx, enum drm_mode_status *status);
    struct drm_encoder *(*best_encoder)(struct drm_connector *connector);
    struct drm_encoder *(*atomic_best_encoder)(struct drm_connector *connector, struct drm_atomic_state *state);
    int (*atomic_check)(struct drm_connector *connector, struct drm_atomic_state *state);
    void (*atomic_commit)(struct drm_connector *connector, struct drm_atomic_state *state);
    int (*prepare_writeback_job)(struct drm_writeback_connector *connector, struct drm_writeback_job *job);
    void (*cleanup_writeback_job)(struct drm_writeback_connector *connector, struct drm_writeback_job *job);
    void (*enable_hpd)(struct drm_connector *connector);
    void (*disable_hpd)(struct drm_connector *connector);
};
```

**Members**

`get_modes`
:   This function should fill in all modes currently valid for the sink
    into the [`drm_connector.probed_modes`](drm-kms.html#c.drm_connector "drm_connector") list. It should also update the
    EDID property by calling [`drm_connector_update_edid_property()`](#c.drm_connector_update_edid_property "drm_connector_update_edid_property").

    The usual way to implement this is to cache the EDID retrieved in the
    probe callback somewhere in the driver-private connector structure.
    In this function drivers then parse the modes in the EDID and add
    them by calling [`drm_add_edid_modes()`](#c.drm_add_edid_modes "drm_add_edid_modes"). But connectors that drive a
    fixed panel can also manually add specific modes using
    [`drm_mode_probed_add()`](drm-kms.html#c.drm_mode_probed_add "drm_mode_probed_add"). Drivers which manually add modes should also
    make sure that the [`drm_connector.display_info`](drm-kms.html#c.drm_connector "drm_connector"),
    [`drm_connector.width_mm`](drm-kms.html#c.drm_connector "drm_connector") and [`drm_connector.height_mm`](drm-kms.html#c.drm_connector "drm_connector") fields are
    filled in.

    Note that the caller function will automatically add standard VESA
    DMT modes up to 1024x768 if the .`get_modes()` helper operation returns
    no mode and if the connector status is connector\_status\_connected or
    connector\_status\_unknown. There is no need to call
    [`drm_add_modes_noedid()`](#c.drm_add_modes_noedid "drm_add_modes_noedid") manually in that case.

    Virtual drivers that just want some standard VESA mode with a given
    resolution can call [`drm_add_modes_noedid()`](#c.drm_add_modes_noedid "drm_add_modes_noedid"), and mark the preferred
    one using [`drm_set_preferred_mode()`](drm-kms.html#c.drm_set_preferred_mode "drm_set_preferred_mode").

    This function is only called after the **detect** hook has indicated
    that a sink is connected and when the EDID isn’t overridden through
    sysfs or the kernel commandline.

    This callback is used by the probe helpers in e.g.
    [`drm_helper_probe_single_connector_modes()`](#c.drm_helper_probe_single_connector_modes "drm_helper_probe_single_connector_modes").

    To avoid races with concurrent connector state updates, the helper
    libraries always call this with the [`drm_mode_config.connection_mutex`](drm-kms.html#c.drm_mode_config "drm_mode_config")
    held. Because of this it’s safe to inspect [`drm_connector->state`](drm-kms.html#c.drm_connector "drm_connector").

    RETURNS:

    The number of modes added by calling [`drm_mode_probed_add()`](drm-kms.html#c.drm_mode_probed_add "drm_mode_probed_add"). Return 0
    on failures (no modes) instead of negative error codes.

`detect_ctx`
:   Check to see if anything is attached to the connector. The parameter
    force is set to false whilst polling, true when checking the
    connector due to a user request. force can be used by the driver to
    avoid expensive, destructive operations during automated probing.

    This callback is optional, if not implemented the connector will be
    considered as always being attached.

    This is the atomic version of [`drm_connector_funcs.detect`](drm-kms.html#c.drm_connector_funcs "drm_connector_funcs").

    To avoid races against concurrent connector state updates, the
    helper libraries always call this with ctx set to a valid context,
    and [`drm_mode_config.connection_mutex`](drm-kms.html#c.drm_mode_config "drm_mode_config") will always be locked with
    the ctx parameter set to this ctx. This allows taking additional
    locks as required.

    RETURNS:

    [`drm_connector_status`](drm-kms.html#c.drm_connector_status "drm_connector_status") indicating the connector’s status,
    or the error code returned by [`drm_modeset_lock()`](drm-kms.html#c.drm_modeset_lock "drm_modeset_lock"), -EDEADLK.

`mode_valid`
:   Callback to validate a mode for a connector, irrespective of the
    specific display configuration.

    This callback is used by the probe helpers to filter the mode list
    (which is usually derived from the EDID data block from the sink).
    See e.g. [`drm_helper_probe_single_connector_modes()`](#c.drm_helper_probe_single_connector_modes "drm_helper_probe_single_connector_modes").

    This function is optional.

    NOTE:

    This only filters the mode list supplied to userspace in the
    GETCONNECTOR IOCTL. Compared to [`drm_encoder_helper_funcs.mode_valid`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs"),
    [`drm_crtc_helper_funcs.mode_valid`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") and [`drm_bridge_funcs.mode_valid`](#c.drm_bridge_funcs "drm_bridge_funcs"),
    which are also called by the atomic helpers from
    [`drm_atomic_helper_check_modeset()`](#c.drm_atomic_helper_check_modeset "drm_atomic_helper_check_modeset"). This allows userspace to force and
    ignore sink constraint (like the pixel clock limits in the screen’s
    EDID), which is useful for e.g. testing, or working around a broken
    EDID. Any source hardware constraint (which always need to be
    enforced) therefore should be checked in one of the above callbacks,
    and not this one here.

    To avoid races with concurrent connector state updates, the helper
    libraries always call this with the [`drm_mode_config.connection_mutex`](drm-kms.html#c.drm_mode_config "drm_mode_config")
    held. Because of this it’s safe to inspect [`drm_connector->state`](drm-kms.html#c.drm_connector "drm_connector").

    RETURNS:

    Either [`drm_mode_status.MODE_OK`](drm-kms.html#c.drm_mode_status "drm_mode_status") or one of the failure reasons in [`enum
    drm_mode_status`](drm-kms.html#c.drm_mode_status "drm_mode_status").

`mode_valid_ctx`
:   Callback to validate a mode for a connector, irrespective of the
    specific display configuration.

    This callback is used by the probe helpers to filter the mode list
    (which is usually derived from the EDID data block from the sink).
    See e.g. [`drm_helper_probe_single_connector_modes()`](#c.drm_helper_probe_single_connector_modes "drm_helper_probe_single_connector_modes").

    This function is optional, and is the atomic version of
    [`drm_connector_helper_funcs.mode_valid`](#c.drm_connector_helper_funcs "drm_connector_helper_funcs").

    To allow for accessing the atomic state of modesetting objects, the
    helper libraries always call this with ctx set to a valid context,
    and [`drm_mode_config.connection_mutex`](drm-kms.html#c.drm_mode_config "drm_mode_config") will always be locked with
    the ctx parameter set to **ctx**. This allows for taking additional
    locks as required.

    Even though additional locks may be acquired, this callback is
    still expected not to take any constraints into account which would
    be influenced by the currently set display state - such constraints
    should be handled in the driver’s atomic check. For example, if a
    connector shares display bandwidth with other connectors then it
    would be ok to validate the minimum bandwidth requirement of a mode
    against the maximum possible bandwidth of the connector. But it
    wouldn’t be ok to take the current bandwidth usage of other
    connectors into account, as this would change depending on the
    display state.

    Returns:
    0 if [`drm_connector_helper_funcs.mode_valid_ctx`](#c.drm_connector_helper_funcs "drm_connector_helper_funcs") succeeded and wrote
    the [`enum drm_mode_status`](drm-kms.html#c.drm_mode_status "drm_mode_status") value to **status**, or a negative error
    code otherwise.

`best_encoder`
:   This function should select the best encoder for the given connector.

    This function is used by both the atomic helpers (in the
    [`drm_atomic_helper_check_modeset()`](#c.drm_atomic_helper_check_modeset "drm_atomic_helper_check_modeset") function) and in the legacy CRTC
    helpers.

    NOTE:

    In atomic drivers this function is called in the check phase of an
    atomic update. The driver is not allowed to change or inspect
    anything outside of arguments passed-in. Atomic drivers which need to
    inspect dynamic configuration state should instead use
    **atomic\_best\_encoder**.

    You can leave this function to NULL if the connector is only
    attached to a single encoder. In this case, the core will call
    `drm_connector_get_single_encoder()` for you.

    RETURNS:

    Encoder that should be used for the given connector and connector
    state, or NULL if no suitable encoder exists. Note that the helpers
    will ensure that encoders aren’t used twice, drivers should not check
    for this.

`atomic_best_encoder`
:   This is the atomic version of **best\_encoder** for atomic drivers which
    need to select the best encoder depending upon the desired
    configuration and can’t select it statically.

    This function is used by [`drm_atomic_helper_check_modeset()`](#c.drm_atomic_helper_check_modeset "drm_atomic_helper_check_modeset").
    If it is not implemented, the core will fallback to **best\_encoder**
    (or `drm_connector_get_single_encoder()` if **best\_encoder** is NULL).

    NOTE:

    This function is called in the check phase of an atomic update. The
    driver is not allowed to change anything outside of the
    [`drm_atomic_state`](drm-kms.html#c.drm_atomic_state "drm_atomic_state") update tracking structure passed in.

    RETURNS:

    Encoder that should be used for the given connector and connector
    state, or NULL if no suitable encoder exists. Note that the helpers
    will ensure that encoders aren’t used twice, drivers should not check
    for this.

`atomic_check`
:   This hook is used to validate connector state. This function is
    called from [`drm_atomic_helper_check_modeset`](#c.drm_atomic_helper_check_modeset "drm_atomic_helper_check_modeset"), and is called when
    a connector property is set, or a modeset on the crtc is forced.

    Because [`drm_atomic_helper_check_modeset`](#c.drm_atomic_helper_check_modeset "drm_atomic_helper_check_modeset") may be called multiple times,
    this function should handle being called multiple times as well.

    This function is also allowed to inspect any other object’s state and
    can add more state objects to the atomic commit if needed. Care must
    be taken though to ensure that state check and compute functions for
    these added states are all called, and derived state in other objects
    all updated. Again the recommendation is to just call check helpers
    until a maximal configuration is reached.

    NOTE:

    This function is called in the check phase of an atomic update. The
    driver is not allowed to change anything outside of the free-standing
    state objects passed-in or assembled in the overall [`drm_atomic_state`](drm-kms.html#c.drm_atomic_state "drm_atomic_state")
    update tracking structure.

    RETURNS:

    0 on success, -EINVAL if the state or the transition can’t be
    supported, -ENOMEM on memory allocation failure and -EDEADLK if an
    attempt to obtain another state object ran into a [`drm_modeset_lock`](drm-kms.html#c.drm_modeset_lock "drm_modeset_lock")
    deadlock.

`atomic_commit`
:   This hook is to be used by drivers implementing writeback connectors
    that need a point when to commit the writeback job to the hardware.
    The writeback\_job to commit is available in the new connector state,
    in [`drm_connector_state.writeback_job`](drm-kms.html#c.drm_connector_state "drm_connector_state").

    This hook is optional.

    This callback is used by the atomic modeset helpers.

`prepare_writeback_job`
:   As writeback jobs contain a framebuffer, drivers may need to
    prepare and clean them up the same way they can prepare and
    clean up framebuffers for planes. This optional connector operation
    is used to support the preparation of writeback jobs. The job
    prepare operation is called from [`drm_atomic_helper_prepare_planes()`](#c.drm_atomic_helper_prepare_planes "drm_atomic_helper_prepare_planes")
    for struct [`drm_writeback_connector`](drm-kms.html#c.drm_writeback_connector "drm_writeback_connector") connectors only.

    This operation is optional.

    This callback is used by the atomic modeset helpers.

`cleanup_writeback_job`
:   This optional connector operation is used to support the
    cleanup of writeback jobs. The job cleanup operation is called
    from the existing `drm_writeback_cleanup_job()` function, invoked
    both when destroying the job as part of an aborted commit, or when
    the job completes.

    This operation is optional.

    This callback is used by the atomic modeset helpers.

`enable_hpd`
:   Enable hot-plug detection for the connector.

    This operation is optional.

    This callback is used by the [`drm_kms_helper_poll_enable()`](#c.drm_kms_helper_poll_enable "drm_kms_helper_poll_enable") helpers.

    This operation does not need to perform any hpd state tracking as
    the DRM core handles that maintenance and ensures the calls to enable
    and disable hpd are balanced.

`disable_hpd`
:   Disable hot-plug detection for the connector.

    This operation is optional.

    This callback is used by the [`drm_kms_helper_poll_disable()`](#c.drm_kms_helper_poll_disable "drm_kms_helper_poll_disable") helpers.

    This operation does not need to perform any hpd state tracking as
    the DRM core handles that maintenance and ensures the calls to enable
    and disable hpd are balanced.

**Description**

These functions are used by the atomic and legacy modeset helpers and by the
probe helpers.

void drm\_connector\_helper\_add(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, const struct [drm\_connector\_helper\_funcs](#c.drm_connector_helper_funcs "drm_connector_helper_funcs") \*funcs)
:   sets the helper vtable for a connector

**Parameters**

`struct drm_connector *connector`
:   DRM connector

`const struct drm_connector_helper_funcs *funcs`
:   helper vtable to set for **connector**

struct drm\_plane\_helper\_funcs
:   helper operations for planes

**Definition**:

```
struct drm_plane_helper_funcs {
    int (*prepare_fb)(struct drm_plane *plane, struct drm_plane_state *new_state);
    void (*cleanup_fb)(struct drm_plane *plane, struct drm_plane_state *old_state);
    int (*begin_fb_access)(struct drm_plane *plane, struct drm_plane_state *new_plane_state);
    void (*end_fb_access)(struct drm_plane *plane, struct drm_plane_state *new_plane_state);
    int (*atomic_check)(struct drm_plane *plane, struct drm_atomic_state *state);
    void (*atomic_update)(struct drm_plane *plane, struct drm_atomic_state *state);
    void (*atomic_enable)(struct drm_plane *plane, struct drm_atomic_state *state);
    void (*atomic_disable)(struct drm_plane *plane, struct drm_atomic_state *state);
    int (*atomic_async_check)(struct drm_plane *plane, struct drm_atomic_state *state, bool flip);
    void (*atomic_async_update)(struct drm_plane *plane, struct drm_atomic_state *state);
    int (*get_scanout_buffer)(struct drm_plane *plane, struct drm_scanout_buffer *sb);
    void (*panic_flush)(struct drm_plane *plane);
};
```

**Members**

`prepare_fb`
:   This hook is to prepare a framebuffer for scanout by e.g. pinning
    its backing storage or relocating it into a contiguous block of
    VRAM. Other possible preparatory work includes flushing caches.

    This function must not block for outstanding rendering, since it is
    called in the context of the atomic IOCTL even for async commits to
    be able to return any errors to userspace. Instead the recommended
    way is to fill out the [`drm_plane_state.fence`](drm-kms.html#c.drm_plane_state "drm_plane_state") of the passed-in
    [`drm_plane_state`](drm-kms.html#c.drm_plane_state "drm_plane_state"). If the driver doesn’t support native fences then
    equivalent functionality should be implemented through private
    members in the plane structure.

    For GEM drivers who neither have a **prepare\_fb** nor **cleanup\_fb** hook
    set [`drm_gem_plane_helper_prepare_fb()`](#c.drm_gem_plane_helper_prepare_fb "drm_gem_plane_helper_prepare_fb") is called automatically to
    implement this. Other drivers which need additional plane processing
    can call [`drm_gem_plane_helper_prepare_fb()`](#c.drm_gem_plane_helper_prepare_fb "drm_gem_plane_helper_prepare_fb") from their **prepare\_fb**
    hook.

    The resources acquired in **prepare\_fb** persist after the end of
    the atomic commit. Resources that can be release at the commit’s end
    should be acquired in **begin\_fb\_access** and released in **end\_fb\_access**.
    For example, a GEM buffer’s pin operation belongs into **prepare\_fb** to
    keep the buffer pinned after the commit. But a vmap operation for
    shadow-plane helpers belongs into **begin\_fb\_access**, so that atomic
    helpers remove the mapping at the end of the commit.

    The helpers will call **cleanup\_fb** with matching arguments for every
    successful call to this hook.

    This callback is used by the atomic modeset helpers, but it is
    optional. See **begin\_fb\_access** for preparing per-commit resources.

    RETURNS:

    0 on success or one of the following negative error codes allowed by
    the [`drm_mode_config_funcs.atomic_commit`](drm-kms.html#c.drm_mode_config_funcs "drm_mode_config_funcs") vfunc. When using helpers
    this callback is the only one which can fail an atomic commit,
    everything else must complete successfully.

`cleanup_fb`
:   This hook is called to clean up any resources allocated for the given
    framebuffer and plane configuration in **prepare\_fb**.

    This callback is used by the atomic modeset helpers, but it is
    optional.

`begin_fb_access`
:   This hook prepares the plane for access during an atomic commit.
    In contrast to **prepare\_fb**, resources acquired in **begin\_fb\_access**,
    are released at the end of the atomic commit in **end\_fb\_access**.

    For example, with shadow-plane helpers, the GEM buffer’s vmap
    operation belongs into **begin\_fb\_access**, so that the buffer’s
    memory will be unmapped at the end of the commit in **end\_fb\_access**.
    But a GEM buffer’s pin operation belongs into **prepare\_fb**
    to keep the buffer pinned after the commit.

    The callback is used by the atomic modeset helpers, but it is optional.
    See **end\_fb\_cleanup** for undoing the effects of **begin\_fb\_access** and
    **prepare\_fb** for acquiring resources until the next pageflip.

    Returns:
    0 on success, or a negative errno code otherwise.

`end_fb_access`
:   This hook cleans up resources allocated by **begin\_fb\_access**. It it called
    at the end of a commit for the new plane state.

`atomic_check`
:   Drivers should check plane specific constraints in this hook.

    When using [`drm_atomic_helper_check_planes()`](#c.drm_atomic_helper_check_planes "drm_atomic_helper_check_planes") plane’s **atomic\_check**
    hooks are called before the ones for CRTCs, which allows drivers to
    request shared resources that the CRTC controls here. For more
    complicated dependencies the driver can call the provided check helpers
    multiple times until the computed state has a final configuration and
    everything has been checked.

    This function is also allowed to inspect any other object’s state and
    can add more state objects to the atomic commit if needed. Care must
    be taken though to ensure that state check and compute functions for
    these added states are all called, and derived state in other objects
    all updated. Again the recommendation is to just call check helpers
    until a maximal configuration is reached.

    This callback is used by the atomic modeset helpers, but it is
    optional.

    NOTE:

    This function is called in the check phase of an atomic update. The
    driver is not allowed to change anything outside of the
    [`drm_atomic_state`](drm-kms.html#c.drm_atomic_state "drm_atomic_state") update tracking structure.

    RETURNS:

    0 on success, -EINVAL if the state or the transition can’t be
    supported, -ENOMEM on memory allocation failure and -EDEADLK if an
    attempt to obtain another state object ran into a [`drm_modeset_lock`](drm-kms.html#c.drm_modeset_lock "drm_modeset_lock")
    deadlock.

`atomic_update`
:   Drivers should use this function to update the plane state. This
    hook is called in-between the [`drm_crtc_helper_funcs.atomic_begin`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") and
    drm\_crtc\_helper\_funcs.atomic\_flush callbacks.

    Note that the power state of the display pipe when this function is
    called depends upon the exact helpers and calling sequence the driver
    has picked. See [`drm_atomic_helper_commit_planes()`](#c.drm_atomic_helper_commit_planes "drm_atomic_helper_commit_planes") for a discussion of
    the tradeoffs and variants of plane commit helpers.

    This callback is used by the atomic modeset helpers, but it is optional.

`atomic_enable`
:   Drivers should use this function to unconditionally enable a plane.
    This hook is called in-between the [`drm_crtc_helper_funcs.atomic_begin`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs")
    and drm\_crtc\_helper\_funcs.atomic\_flush callbacks. It is called after
    **atomic\_update**, which will be called for all enabled planes. Drivers
    that use **atomic\_enable** should set up a plane in **atomic\_update** and
    afterwards enable the plane in **atomic\_enable**. If a plane needs to be
    enabled before installing the scanout buffer, drivers can still do
    so in **atomic\_update**.

    Note that the power state of the display pipe when this function is
    called depends upon the exact helpers and calling sequence the driver
    has picked. See [`drm_atomic_helper_commit_planes()`](#c.drm_atomic_helper_commit_planes "drm_atomic_helper_commit_planes") for a discussion of
    the tradeoffs and variants of plane commit helpers.

    This callback is used by the atomic modeset helpers, but it is
    optional. If implemented, **atomic\_enable** should be the inverse of
    **atomic\_disable**. Drivers that don’t want to use either can still
    implement the complete plane update in **atomic\_update**.

`atomic_disable`
:   Drivers should use this function to unconditionally disable a plane.
    This hook is called in-between the
    [`drm_crtc_helper_funcs.atomic_begin`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") and
    drm\_crtc\_helper\_funcs.atomic\_flush callbacks. It is an alternative to
    **atomic\_update**, which will be called for disabling planes, too, if
    the **atomic\_disable** hook isn’t implemented.

    This hook is also useful to disable planes in preparation of a modeset,
    by calling [`drm_atomic_helper_disable_planes_on_crtc()`](#c.drm_atomic_helper_disable_planes_on_crtc "drm_atomic_helper_disable_planes_on_crtc") from the
    [`drm_crtc_helper_funcs.disable`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") hook.

    Note that the power state of the display pipe when this function is
    called depends upon the exact helpers and calling sequence the driver
    has picked. See [`drm_atomic_helper_commit_planes()`](#c.drm_atomic_helper_commit_planes "drm_atomic_helper_commit_planes") for a discussion of
    the tradeoffs and variants of plane commit helpers.

    This callback is used by the atomic modeset helpers, but it is
    optional. It’s intended to reverse the effects of **atomic\_enable**.

`atomic_async_check`
:   Drivers should set this function pointer to check if the plane’s
    atomic state can be updated in a async fashion. Here async means
    “not vblank synchronized”.

    This hook is called by `drm_atomic_async_check()` to establish if a
    given update can be committed asynchronously, that is, if it can
    jump ahead of the state currently queued for update.

    This function is also used by `drm_atomic_set_property()` to determine
    if the plane can be flipped in async. The flip flag is used to
    distinguish if the function is used for just the plane state or for a
    flip.

    RETURNS:

    Return 0 on success and any error returned indicates that the update
    can not be applied in asynchronous manner.

`atomic_async_update`
:   Drivers should set this function pointer to perform asynchronous
    updates of planes, that is, jump ahead of the currently queued
    state and update the plane. Here async means “not vblank
    synchronized”.

    This hook is called by [`drm_atomic_helper_async_commit()`](#c.drm_atomic_helper_async_commit "drm_atomic_helper_async_commit").

    An async update will happen on legacy cursor updates. An async
    update won’t happen if there is an outstanding commit modifying
    the same plane.

    When doing async\_update drivers shouldn’t replace the
    [`drm_plane_state`](drm-kms.html#c.drm_plane_state "drm_plane_state") but update the current one with the new plane
    configurations in the new plane\_state.

    Drivers should also swap the framebuffers between current plane
    state ([`drm_plane.state`](drm-kms.html#c.drm_plane "drm_plane")) and new\_state.
    This is required since cleanup for async commits is performed on
    the new state, rather than old state like for traditional commits.
    Since we want to give up the reference on the current (old) fb
    instead of our brand new one, swap them in the driver during the
    async commit.

    FIXME:
    :   * It only works for single plane updates
        * Async Pageflips are not supported yet
        * Some hw might still scan out the old buffer until the next
          vblank, however we let go of the fb references as soon as
          we run this hook. For now drivers must implement their own workers
          for deferring if needed, until a common solution is created.

`get_scanout_buffer`
:   Get the current scanout buffer, to display a message with drm\_panic.
    The driver should do the minimum changes to provide a buffer,
    that can be used to display the panic screen. Currently only linear
    buffers are supported. Non-linear buffer support is on the TODO list.
    The device `dev.mode_config`.panic\_lock is taken before calling this
    function, so you can safely access the `plane.state`
    It is called from a panic callback, and must follow its restrictions.
    Please look the documentation at [`drm_panic_trylock()`](drm-kms.html#c.drm_panic_trylock "drm_panic_trylock") for an in-depth
    discussions of what’s safe and what is not allowed.
    It’s a best effort mode, so it’s expected that in some complex cases
    the panic screen won’t be displayed.
    The returned [`drm_scanout_buffer.map`](drm-kms.html#c.drm_scanout_buffer "drm_scanout_buffer") must be valid if no error code is
    returned.

    Return:
    `0` on success, negative errno on failure.

`panic_flush`
:   It is used by drm\_panic, and is called after the panic screen is
    drawn to the scanout buffer. In this function, the driver
    can send additional commands to the hardware, to make the scanout
    buffer visible.
    It is only called if `get_scanout_buffer()` returned successfully, and
    the `dev.mode_config`.panic\_lock is held during the entire sequence.
    It is called from a panic callback, and must follow its restrictions.
    Please look the documentation at [`drm_panic_trylock()`](drm-kms.html#c.drm_panic_trylock "drm_panic_trylock") for an in-depth
    discussions of what’s safe and what is not allowed.

**Description**

These functions are used by the atomic helpers.

void drm\_plane\_helper\_add(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, const struct [drm\_plane\_helper\_funcs](#c.drm_plane_helper_funcs "drm_plane_helper_funcs") \*funcs)
:   sets the helper vtable for a plane

**Parameters**

`struct drm_plane *plane`
:   DRM plane

`const struct drm_plane_helper_funcs *funcs`
:   helper vtable to set for **plane**

struct drm\_mode\_config\_helper\_funcs
:   global modeset helper operations

**Definition**:

```
struct drm_mode_config_helper_funcs {
    void (*atomic_commit_tail)(struct drm_atomic_state *state);
    int (*atomic_commit_setup)(struct drm_atomic_state *state);
};
```

**Members**

`atomic_commit_tail`
:   This hook is used by the default `atomic_commit()` hook implemented in
    [`drm_atomic_helper_commit()`](#c.drm_atomic_helper_commit "drm_atomic_helper_commit") together with the nonblocking commit
    helpers (see [`drm_atomic_helper_setup_commit()`](#c.drm_atomic_helper_setup_commit "drm_atomic_helper_setup_commit") for a starting point)
    to implement blocking and nonblocking commits easily. It is not used
    by the atomic helpers

    This function is called when the new atomic state has already been
    swapped into the various state pointers. The passed in state
    therefore contains copies of the old/previous state. This hook should
    commit the new state into hardware. Note that the helpers have
    already waited for preceding atomic commits and fences, but drivers
    can add more waiting calls at the start of their implementation, e.g.
    to wait for driver-internal request for implicit syncing, before
    starting to commit the update to the hardware.

    After the atomic update is committed to the hardware this hook needs
    to call [`drm_atomic_helper_commit_hw_done()`](#c.drm_atomic_helper_commit_hw_done "drm_atomic_helper_commit_hw_done"). Then wait for the update
    to be executed by the hardware, for example using
    [`drm_atomic_helper_wait_for_vblanks()`](#c.drm_atomic_helper_wait_for_vblanks "drm_atomic_helper_wait_for_vblanks") or
    [`drm_atomic_helper_wait_for_flip_done()`](#c.drm_atomic_helper_wait_for_flip_done "drm_atomic_helper_wait_for_flip_done"), and then clean up the old
    framebuffers using [`drm_atomic_helper_cleanup_planes()`](#c.drm_atomic_helper_cleanup_planes "drm_atomic_helper_cleanup_planes").

    When disabling a CRTC this hook \_must\_ stall for the commit to
    complete. Vblank waits don’t work on disabled CRTC, hence the core
    can’t take care of this. And it also can’t rely on the vblank event,
    since that can be signalled already when the screen shows black,
    which can happen much earlier than the last hardware access needed to
    shut off the display pipeline completely.

    This hook is optional, the default implementation is
    [`drm_atomic_helper_commit_tail()`](#c.drm_atomic_helper_commit_tail "drm_atomic_helper_commit_tail").

`atomic_commit_setup`
:   This hook is used by the default `atomic_commit()` hook implemented in
    [`drm_atomic_helper_commit()`](#c.drm_atomic_helper_commit "drm_atomic_helper_commit") together with the nonblocking helpers (see
    [`drm_atomic_helper_setup_commit()`](#c.drm_atomic_helper_setup_commit "drm_atomic_helper_setup_commit")) to extend the DRM commit setup. It
    is not used by the atomic helpers.

    This function is called at the end of
    [`drm_atomic_helper_setup_commit()`](#c.drm_atomic_helper_setup_commit "drm_atomic_helper_setup_commit"), so once the commit has been
    properly setup across the generic DRM object states. It allows
    drivers to do some additional commit tracking that isn’t related to a
    CRTC, plane or connector, tracked in a [`drm_private_obj`](drm-kms.html#c.drm_private_obj "drm_private_obj") structure.

    Note that the documentation of [`drm_private_obj`](drm-kms.html#c.drm_private_obj "drm_private_obj") has more details on
    how one should implement this.

    This hook is optional.

**Description**

These helper functions are used by the atomic helpers.

## Atomic Modeset Helper Functions Reference

### Overview

This helper library provides implementations of check and commit functions on
top of the CRTC modeset helper callbacks and the plane helper callbacks. It
also provides convenience implementations for the atomic state handling
callbacks for drivers which don’t need to subclass the drm core structures to
add their own additional internal state.

This library also provides default implementations for the check callback in
[`drm_atomic_helper_check()`](#c.drm_atomic_helper_check "drm_atomic_helper_check") and for the commit callback with
[`drm_atomic_helper_commit()`](#c.drm_atomic_helper_commit "drm_atomic_helper_commit"). But the individual stages and callbacks are
exposed to allow drivers to mix and match and e.g. use the plane helpers only
together with a driver private modeset implementation.

This library also provides implementations for all the legacy driver
interfaces on top of the atomic interface. See [`drm_atomic_helper_set_config()`](#c.drm_atomic_helper_set_config "drm_atomic_helper_set_config"),
[`drm_atomic_helper_disable_plane()`](#c.drm_atomic_helper_disable_plane "drm_atomic_helper_disable_plane"), and the various functions to implement
set\_property callbacks. New drivers must not implement these functions
themselves but must use the provided helpers.

The atomic helper uses the same function table structures as all other
modesetting helpers. See the documentation for [`struct drm_crtc_helper_funcs`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs"),
struct [`drm_encoder_helper_funcs`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") and [`struct drm_connector_helper_funcs`](#c.drm_connector_helper_funcs "drm_connector_helper_funcs"). It
also shares the [`struct drm_plane_helper_funcs`](#c.drm_plane_helper_funcs "drm_plane_helper_funcs") function table with the plane
helpers.

### Implementing Asynchronous Atomic Commit

Nonblocking atomic commits should use struct [`drm_crtc_commit`](drm-kms.html#c.drm_crtc_commit "drm_crtc_commit") to sequence
different operations against each another. Locks, especially struct
[`drm_modeset_lock`](drm-kms.html#c.drm_modeset_lock "drm_modeset_lock"), should not be held in worker threads or any other
asynchronous context used to commit the hardware state.

[`drm_atomic_helper_commit()`](#c.drm_atomic_helper_commit "drm_atomic_helper_commit") implements the recommended sequence for
nonblocking commits, using [`drm_atomic_helper_setup_commit()`](#c.drm_atomic_helper_setup_commit "drm_atomic_helper_setup_commit") internally:

1. Run [`drm_atomic_helper_prepare_planes()`](#c.drm_atomic_helper_prepare_planes "drm_atomic_helper_prepare_planes"). Since this can fail and we
need to propagate out of memory/VRAM errors to userspace, it must be called
synchronously.

2. Synchronize with any outstanding nonblocking commit worker threads which
might be affected by the new state update. This is handled by
[`drm_atomic_helper_setup_commit()`](#c.drm_atomic_helper_setup_commit "drm_atomic_helper_setup_commit").

Asynchronous workers need to have sufficient parallelism to be able to run
different atomic commits on different CRTCs in parallel. The simplest way to
achieve this is by running them on the `system_dfl_wq` work queue. Note
that drivers are not required to split up atomic commits and run an
individual commit in parallel - userspace is supposed to do that if it cares.
But it might be beneficial to do that for modesets, since those necessarily
must be done as one global operation, and enabling or disabling a CRTC can
take a long time. But even that is not required.

IMPORTANT: A [`drm_atomic_state`](drm-kms.html#c.drm_atomic_state "drm_atomic_state") update for multiple CRTCs is sequenced
against all CRTCs therein. Therefore for atomic state updates which only flip
planes the driver must not get the struct [`drm_crtc_state`](drm-kms.html#c.drm_crtc_state "drm_crtc_state") of unrelated CRTCs
in its atomic check code: This would prevent committing of atomic updates to
multiple CRTCs in parallel. In general, adding additional state structures
should be avoided as much as possible, because this reduces parallelism in
(nonblocking) commits, both due to locking and due to commit sequencing
requirements.

3. The software state is updated synchronously with
[`drm_atomic_helper_swap_state()`](#c.drm_atomic_helper_swap_state "drm_atomic_helper_swap_state"). Doing this under the protection of all modeset
locks means concurrent callers never see inconsistent state. Note that commit
workers do not hold any locks; their access is only coordinated through
ordering. If workers would access state only through the pointers in the
free-standing state objects (currently not the case for any driver) then even
multiple pending commits could be in-flight at the same time.

4. Schedule a work item to do all subsequent steps, using the split-out
commit helpers: a) pre-plane commit b) plane commit c) post-plane commit and
then cleaning up the framebuffers after the old framebuffer is no longer
being displayed. The scheduled work should synchronize against other workers
using the [`drm_crtc_commit`](drm-kms.html#c.drm_crtc_commit "drm_crtc_commit") infrastructure as needed. See
[`drm_atomic_helper_setup_commit()`](#c.drm_atomic_helper_setup_commit "drm_atomic_helper_setup_commit") for more details.

### Helper Functions Reference

drm\_atomic\_crtc\_for\_each\_plane

`drm_atomic_crtc_for_each_plane (plane, crtc)`

> iterate over planes currently attached to CRTC

**Parameters**

`plane`
:   the loop cursor

`crtc`
:   the CRTC whose planes are iterated

**Description**

This iterates over the current state, useful (for example) when applying
atomic state after it has been checked and swapped. To iterate over the
planes which *will* be attached (more useful in code called from
[`drm_mode_config_funcs.atomic_check`](drm-kms.html#c.drm_mode_config_funcs "drm_mode_config_funcs")) see
[`drm_atomic_crtc_state_for_each_plane()`](#c.drm_atomic_crtc_state_for_each_plane "drm_atomic_crtc_state_for_each_plane").

drm\_atomic\_crtc\_state\_for\_each\_plane

`drm_atomic_crtc_state_for_each_plane (plane, crtc_state)`

> iterate over attached planes in new state

**Parameters**

`plane`
:   the loop cursor

`crtc_state`
:   the incoming CRTC state

**Description**

Similar to `drm_crtc_for_each_plane()`, but iterates the planes that will be
attached if the specified state is applied. Useful during for example
in code called from [`drm_mode_config_funcs.atomic_check`](drm-kms.html#c.drm_mode_config_funcs "drm_mode_config_funcs") operations, to
validate the incoming state.

drm\_atomic\_crtc\_state\_for\_each\_plane\_state

`drm_atomic_crtc_state_for_each_plane_state (plane, plane_state, crtc_state)`

> iterate over attached planes in new state

**Parameters**

`plane`
:   the loop cursor

`plane_state`
:   loop cursor for the plane’s state, must be const

`crtc_state`
:   the incoming CRTC state

**Description**

Similar to `drm_crtc_for_each_plane()`, but iterates the planes that will be
attached if the specified state is applied. Useful during for example
in code called from [`drm_mode_config_funcs.atomic_check`](drm-kms.html#c.drm_mode_config_funcs "drm_mode_config_funcs") operations, to
validate the incoming state.

Compared to just [`drm_atomic_crtc_state_for_each_plane()`](#c.drm_atomic_crtc_state_for_each_plane "drm_atomic_crtc_state_for_each_plane") this also fills in a
const plane\_state. This is useful when a driver just wants to peek at other
active planes on this CRTC, but does not need to change it.

bool drm\_atomic\_plane\_enabling(struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*old\_plane\_state, struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*new\_plane\_state)
:   check whether a plane is being enabled

**Parameters**

`struct drm_plane_state *old_plane_state`
:   old atomic plane state

`struct drm_plane_state *new_plane_state`
:   new atomic plane state

**Description**

Checks the atomic state of a plane to determine whether it’s being enabled
or not. This also WARNs if it detects an invalid state (both CRTC and FB
need to either both be NULL or both be non-NULL).

**Return**

True if the plane is being enabled, false otherwise.

bool drm\_atomic\_plane\_disabling(struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*old\_plane\_state, struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*new\_plane\_state)
:   check whether a plane is being disabled

**Parameters**

`struct drm_plane_state *old_plane_state`
:   old atomic plane state

`struct drm_plane_state *new_plane_state`
:   new atomic plane state

**Description**

Checks the atomic state of a plane to determine whether it’s being disabled
or not. This also WARNs if it detects an invalid state (both CRTC and FB
need to either both be NULL or both be non-NULL).

**Return**

True if the plane is being disabled, false otherwise.

int drm\_atomic\_helper\_check\_modeset(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   validate state object for modeset changes

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   the driver state object

**Description**

Check the state object to see if the requested state is physically possible.
This does all the CRTC and connector related computations for an atomic
update and adds any additional connectors needed for full modesets. It calls
the various per-object callbacks in the follow order:

1. [`drm_connector_helper_funcs.atomic_best_encoder`](#c.drm_connector_helper_funcs "drm_connector_helper_funcs") for determining the new encoder.
2. [`drm_connector_helper_funcs.atomic_check`](#c.drm_connector_helper_funcs "drm_connector_helper_funcs") to validate the connector state.
3. If it’s determined a modeset is needed then all connectors on the affected
   CRTC are added and [`drm_connector_helper_funcs.atomic_check`](#c.drm_connector_helper_funcs "drm_connector_helper_funcs") is run on them.
4. [`drm_encoder_helper_funcs.mode_valid`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs"), [`drm_bridge_funcs.mode_valid`](#c.drm_bridge_funcs "drm_bridge_funcs") and
   [`drm_crtc_helper_funcs.mode_valid`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") are called on the affected components.
5. [`drm_bridge_funcs.mode_fixup`](#c.drm_bridge_funcs "drm_bridge_funcs") is called on all encoder bridges.
6. [`drm_encoder_helper_funcs.atomic_check`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") is called to validate any encoder state.
   This function is only called when the encoder will be part of a configured CRTC,
   it must not be used for implementing connector property validation.
   If this function is NULL, `drm_atomic_encoder_helper_funcs.mode_fixup` is called
   instead.
7. [`drm_crtc_helper_funcs.mode_fixup`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") is called last, to fix up the mode with CRTC constraints.

[`drm_crtc_state.mode_changed`](drm-kms.html#c.drm_crtc_state "drm_crtc_state") is set when the input mode is changed.
[`drm_crtc_state.connectors_changed`](drm-kms.html#c.drm_crtc_state "drm_crtc_state") is set when a connector is added or
removed from the CRTC. [`drm_crtc_state.active_changed`](drm-kms.html#c.drm_crtc_state "drm_crtc_state") is set when
[`drm_crtc_state.active`](drm-kms.html#c.drm_crtc_state "drm_crtc_state") changes, which is used for DPMS.
[`drm_crtc_state.no_vblank`](drm-kms.html#c.drm_crtc_state "drm_crtc_state") is set from the result of [`drm_dev_has_vblank()`](drm-kms.html#c.drm_dev_has_vblank "drm_dev_has_vblank").
See also: [`drm_atomic_crtc_needs_modeset()`](drm-kms.html#c.drm_atomic_crtc_needs_modeset "drm_atomic_crtc_needs_modeset")

IMPORTANT:

Drivers which set [`drm_crtc_state.mode_changed`](drm-kms.html#c.drm_crtc_state "drm_crtc_state") (e.g. in their
[`drm_plane_helper_funcs.atomic_check`](#c.drm_plane_helper_funcs "drm_plane_helper_funcs") hooks if a plane update can’t be done
without a full modeset) \_must\_ call this function after that change. It is
permitted to call this function multiple times for the same update, e.g.
when the [`drm_crtc_helper_funcs.atomic_check`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") functions depend upon the
adjusted dotclock for fifo space allocation and watermark computation.

**Return**

Zero for success or -errno

int drm\_atomic\_helper\_check\_wb\_connector\_state(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   Check writeback connector state

**Parameters**

`struct drm_connector *connector`
:   corresponding connector

`struct drm_atomic_state *state`
:   the driver state object

**Description**

Checks if the writeback connector state is valid, and returns an error if it
isn’t.

**Return**

Zero for success or -errno

int drm\_atomic\_helper\_check\_plane\_state(struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*plane\_state, const struct [drm\_crtc\_state](drm-kms.html#c.drm_crtc_state "drm_crtc_state") \*crtc\_state, int min\_scale, int max\_scale, bool can\_position, bool can\_update\_disabled)
:   Check plane state for validity

**Parameters**

`struct drm_plane_state *plane_state`
:   plane state to check

`const struct drm_crtc_state *crtc_state`
:   CRTC state to check

`int min_scale`
:   minimum **src**:**dest** scaling factor in 16.16 fixed point

`int max_scale`
:   maximum **src**:**dest** scaling factor in 16.16 fixed point

`bool can_position`
:   is it legal to position the plane such that it
    doesn’t cover the entire CRTC? This will generally
    only be false for primary planes.

`bool can_update_disabled`
:   can the plane be updated while the CRTC
    is disabled?

**Description**

Checks that a desired plane update is valid, and updates various
bits of derived state (clipped coordinates etc.). Drivers that provide
their own plane handling rather than helper-provided implementations may
still wish to call this function to avoid duplication of error checking
code.

**Return**

Zero if update appears valid, error code on failure

int drm\_atomic\_helper\_check\_crtc\_primary\_plane(struct [drm\_crtc\_state](drm-kms.html#c.drm_crtc_state "drm_crtc_state") \*crtc\_state)
:   Check CRTC state for primary plane

**Parameters**

`struct drm_crtc_state *crtc_state`
:   CRTC state to check

**Description**

Checks that a CRTC has at least one primary plane attached to it, which is
a requirement on some hardware. Note that this only involves the CRTC side
of the test. To test if the primary plane is visible or if it can be updated
without the CRTC being enabled, use [`drm_atomic_helper_check_plane_state()`](#c.drm_atomic_helper_check_plane_state "drm_atomic_helper_check_plane_state") in
the plane’s atomic check.

**Return**

0 if a primary plane is attached to the CRTC, or an error code otherwise

int drm\_atomic\_helper\_check\_planes(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   validate state object for planes changes

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   the driver state object

**Description**

Check the state object to see if the requested state is physically possible.
This does all the plane update related checks using by calling into the
[`drm_crtc_helper_funcs.atomic_check`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") and [`drm_plane_helper_funcs.atomic_check`](#c.drm_plane_helper_funcs "drm_plane_helper_funcs")
hooks provided by the driver.

It also sets [`drm_crtc_state.planes_changed`](drm-kms.html#c.drm_crtc_state "drm_crtc_state") to indicate that a CRTC has
updated planes.

**Return**

Zero for success or -errno

int drm\_atomic\_helper\_check(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   validate state object

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   the driver state object

**Description**

Check the state object to see if the requested state is physically possible.
Only CRTCs and planes have check callbacks, so for any additional (global)
checking that a driver needs it can simply wrap that around this function.
Drivers without such needs can directly use this as their
[`drm_mode_config_funcs.atomic_check`](drm-kms.html#c.drm_mode_config_funcs "drm_mode_config_funcs") callback.

This just wraps the two parts of the state checking for planes and modeset
state in the default order: First it calls [`drm_atomic_helper_check_modeset()`](#c.drm_atomic_helper_check_modeset "drm_atomic_helper_check_modeset")
and then [`drm_atomic_helper_check_planes()`](#c.drm_atomic_helper_check_planes "drm_atomic_helper_check_planes"). The assumption is that the
**drm\_plane\_helper\_funcs.atomic\_check** and **drm\_crtc\_helper\_funcs.atomic\_check**
functions depend upon an updated adjusted\_mode.clock to e.g. properly compute
watermarks.

Note that zpos normalization will add all enable planes to the state which
might not desired for some drivers.
For example enable/disable of a cursor plane which have fixed zpos value
would trigger all other enabled planes to be forced to the state change.

IMPORTANT:

As this function calls [`drm_atomic_helper_check_modeset()`](#c.drm_atomic_helper_check_modeset "drm_atomic_helper_check_modeset") internally, its
restrictions also apply:
Drivers which set [`drm_crtc_state.mode_changed`](drm-kms.html#c.drm_crtc_state "drm_crtc_state") (e.g. in their
[`drm_plane_helper_funcs.atomic_check`](#c.drm_plane_helper_funcs "drm_plane_helper_funcs") hooks if a plane update can’t be done
without a full modeset) \_must\_ call [`drm_atomic_helper_check_modeset()`](#c.drm_atomic_helper_check_modeset "drm_atomic_helper_check_modeset")
function again after that change.

**Return**

Zero for success or -errno

void drm\_atomic\_helper\_commit\_encoder\_bridge\_disable(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   disable bridges and encoder

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   the driver state object

**Description**

Loops over all connectors in the current state and if the CRTC needs
it, disables the bridge chain all the way, then disables the encoder
afterwards.

void drm\_atomic\_helper\_commit\_crtc\_disable(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   disable CRTSs

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   the driver state object

**Description**

Loops over all CRTCs in the current state and if the CRTC needs
it, disables it.

void drm\_atomic\_helper\_commit\_encoder\_bridge\_post\_disable(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   post-disable encoder bridges

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   the driver state object

**Description**

Loops over all connectors in the current state and if the CRTC needs
it, post-disables all encoder bridges.

void drm\_atomic\_helper\_update\_legacy\_modeset\_state(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   update legacy modeset state

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   atomic state object being committed

**Description**

This function updates all the various legacy modeset state pointers in
connectors, encoders and CRTCs.

Drivers can use this for building their own atomic commit if they don’t have
a pure helper-based modeset implementation.

Since these updates are not synchronized with lockings, only code paths
called from [`drm_mode_config_helper_funcs.atomic_commit_tail`](#c.drm_mode_config_helper_funcs "drm_mode_config_helper_funcs") can look at the
legacy state filled out by this helper. Defacto this means this helper and
the legacy state pointers are only really useful for transitioning an
existing driver to the atomic world.

void drm\_atomic\_helper\_calc\_timestamping\_constants(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   update vblank timestamping constants

**Parameters**

`struct drm_atomic_state *state`
:   atomic state object

**Description**

Updates the timestamping constants used for precise vblank timestamps
by calling [`drm_calc_timestamping_constants()`](drm-kms.html#c.drm_calc_timestamping_constants "drm_calc_timestamping_constants") for all enabled crtcs in **state**.

void drm\_atomic\_helper\_commit\_crtc\_set\_mode(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   set the new mode

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   the driver state object

**Description**

Loops over all connectors in the current state and if the mode has
changed, change the mode of the CRTC, then call down the bridge
chain and change the mode in all bridges as well.

void drm\_atomic\_helper\_commit\_modeset\_disables(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   modeset commit to disable outputs

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   atomic state object being committed

**Description**

This function shuts down all the outputs that need to be shut down and
prepares them (if required) with the new mode.

For compatibility with legacy CRTC helpers this should be called before
[`drm_atomic_helper_commit_planes()`](#c.drm_atomic_helper_commit_planes "drm_atomic_helper_commit_planes"), which is what the default commit function
does. But drivers with different needs can group the modeset commits together
and do the plane commits at the end. This is useful for drivers doing runtime
PM since planes updates then only happen when the CRTC is actually enabled.

void drm\_atomic\_helper\_commit\_writebacks(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   issue writebacks

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   atomic state object being committed

**Description**

This loops over the connectors, checks if the new state requires
a writeback job to be issued and in that case issues an atomic
commit on each connector.

void drm\_atomic\_helper\_commit\_encoder\_bridge\_pre\_enable(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   pre-enable bridges

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   atomic state object being committed

**Description**

This loops over the connectors and if the CRTC needs it, pre-enables
the entire bridge chain.

void drm\_atomic\_helper\_commit\_crtc\_enable(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   enables the CRTCs

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   atomic state object being committed

**Description**

This loops over CRTCs in the new state, and of the CRTC needs
it, enables it.

void drm\_atomic\_helper\_commit\_encoder\_bridge\_enable(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   enables the bridges

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   atomic state object being committed

**Description**

This loops over all connectors in the new state, and of the CRTC needs
it, enables the entire bridge chain.

void drm\_atomic\_helper\_commit\_modeset\_enables(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   modeset commit to enable outputs

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   atomic state object being committed

**Description**

This function enables all the outputs with the new configuration which had to
be turned off for the update.

For compatibility with legacy CRTC helpers this should be called after
[`drm_atomic_helper_commit_planes()`](#c.drm_atomic_helper_commit_planes "drm_atomic_helper_commit_planes"), which is what the default commit function
does. But drivers with different needs can group the modeset commits together
and do the plane commits at the end. This is useful for drivers doing runtime
PM since planes updates then only happen when the CRTC is actually enabled.

int drm\_atomic\_helper\_wait\_for\_fences(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state, bool pre\_swap)
:   wait for fences stashed in plane state

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   atomic state object with old state structures

`bool pre_swap`
:   If true, do an interruptible wait, and **state** is the new state.
    Otherwise **state** is the old state.

**Description**

For implicit sync, driver should fish the exclusive fence out from the
incoming fb’s and stash it in the drm\_plane\_state. This is called after
[`drm_atomic_helper_swap_state()`](#c.drm_atomic_helper_swap_state "drm_atomic_helper_swap_state") so it uses the current plane state (and
just uses the atomic state to find the changed planes)

Note that **pre\_swap** is needed since the point where we block for fences moves
around depending upon whether an atomic commit is blocking or
non-blocking. For non-blocking commit all waiting needs to happen after
[`drm_atomic_helper_swap_state()`](#c.drm_atomic_helper_swap_state "drm_atomic_helper_swap_state") is called, but for blocking commits we want
to wait **before** we do anything that can’t be easily rolled back. That is
before we call [`drm_atomic_helper_swap_state()`](#c.drm_atomic_helper_swap_state "drm_atomic_helper_swap_state").

Returns zero if success or < 0 if [`dma_fence_wait()`](../driver-api/dma-buf.html#c.dma_fence_wait "dma_fence_wait") fails.

void drm\_atomic\_helper\_wait\_for\_vblanks(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   wait for vblank on CRTCs

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   atomic state object being committed

**Description**

Helper to, after atomic commit, wait for vblanks on all affected
CRTCs (ie. before cleaning up old framebuffers using
[`drm_atomic_helper_cleanup_planes()`](#c.drm_atomic_helper_cleanup_planes "drm_atomic_helper_cleanup_planes")). It will only wait on CRTCs where the
framebuffers have actually changed to optimize for the legacy cursor and
plane update use-case.

Drivers using the nonblocking commit tracking support initialized by calling
[`drm_atomic_helper_setup_commit()`](#c.drm_atomic_helper_setup_commit "drm_atomic_helper_setup_commit") should look at
[`drm_atomic_helper_wait_for_flip_done()`](#c.drm_atomic_helper_wait_for_flip_done "drm_atomic_helper_wait_for_flip_done") as an alternative.

void drm\_atomic\_helper\_wait\_for\_flip\_done(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   wait for all page flips to be done

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   atomic state object being committed

**Description**

Helper to, after atomic commit, wait for page flips on all affected
crtcs (ie. before cleaning up old framebuffers using
[`drm_atomic_helper_cleanup_planes()`](#c.drm_atomic_helper_cleanup_planes "drm_atomic_helper_cleanup_planes")). Compared to
[`drm_atomic_helper_wait_for_vblanks()`](#c.drm_atomic_helper_wait_for_vblanks "drm_atomic_helper_wait_for_vblanks") this waits for the completion on all
CRTCs, assuming that cursors-only updates are signalling their completion
immediately (or using a different path).

This requires that drivers use the nonblocking commit tracking support
initialized using [`drm_atomic_helper_setup_commit()`](#c.drm_atomic_helper_setup_commit "drm_atomic_helper_setup_commit").

void drm\_atomic\_helper\_commit\_tail(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   commit atomic update to hardware

**Parameters**

`struct drm_atomic_state *state`
:   atomic state object being committed

**Description**

This is the default implementation for the
[`drm_mode_config_helper_funcs.atomic_commit_tail`](#c.drm_mode_config_helper_funcs "drm_mode_config_helper_funcs") hook, for drivers
that do not support runtime\_pm or do not need the CRTC to be
enabled to perform a commit. Otherwise, see
[`drm_atomic_helper_commit_tail_rpm()`](#c.drm_atomic_helper_commit_tail_rpm "drm_atomic_helper_commit_tail_rpm").

Note that the default ordering of how the various stages are called is to
match the legacy modeset helper library closest.

void drm\_atomic\_helper\_commit\_tail\_rpm(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   commit atomic update to hardware

**Parameters**

`struct drm_atomic_state *state`
:   new modeset state to be committed

**Description**

This is an alternative implementation for the
[`drm_mode_config_helper_funcs.atomic_commit_tail`](#c.drm_mode_config_helper_funcs "drm_mode_config_helper_funcs") hook, for drivers
that support runtime\_pm or need the CRTC to be enabled to perform a
commit. Otherwise, one should use the default implementation
[`drm_atomic_helper_commit_tail()`](#c.drm_atomic_helper_commit_tail "drm_atomic_helper_commit_tail").

int drm\_atomic\_helper\_async\_check(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   check if state can be committed asynchronously

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   the driver state object

**Description**

This helper will check if it is possible to commit the state asynchronously.
Async commits are not supposed to swap the states like normal sync commits
but just do in-place changes on the current state.

It will return 0 if the commit can happen in an asynchronous fashion or error
if not. Note that error just mean it can’t be committed asynchronously, if it
fails the commit should be treated like a normal synchronous commit.

void drm\_atomic\_helper\_async\_commit(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   commit state asynchronously

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   the driver state object

**Description**

This function commits a state asynchronously, i.e., not vblank
synchronized. It should be used on a state only when
`drm_atomic_async_check()` succeeds. Async commits are not supposed to swap
the states like normal sync commits, but just do in-place changes on the
current state.

TODO: Implement full swap instead of doing in-place changes.

int drm\_atomic\_helper\_commit(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state, bool nonblock)
:   commit validated state object

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   the driver state object

`bool nonblock`
:   whether nonblocking behavior is requested.

**Description**

This function commits a with [`drm_atomic_helper_check()`](#c.drm_atomic_helper_check "drm_atomic_helper_check") pre-validated state
object. This can still fail when e.g. the framebuffer reservation fails. This
function implements nonblocking commits, using
[`drm_atomic_helper_setup_commit()`](#c.drm_atomic_helper_setup_commit "drm_atomic_helper_setup_commit") and related functions.

Committing the actual hardware state is done through the
[`drm_mode_config_helper_funcs.atomic_commit_tail`](#c.drm_mode_config_helper_funcs "drm_mode_config_helper_funcs") callback, or its default
implementation [`drm_atomic_helper_commit_tail()`](#c.drm_atomic_helper_commit_tail "drm_atomic_helper_commit_tail").

**Return**

Zero for success or -errno.

int drm\_atomic\_helper\_setup\_commit(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state, bool nonblock)
:   setup possibly nonblocking commit

**Parameters**

`struct drm_atomic_state *state`
:   new modeset state to be committed

`bool nonblock`
:   whether nonblocking behavior is requested.

**Description**

This function prepares **state** to be used by the atomic helper’s support for
nonblocking commits. Drivers using the nonblocking commit infrastructure
should always call this function from their
[`drm_mode_config_funcs.atomic_commit`](drm-kms.html#c.drm_mode_config_funcs "drm_mode_config_funcs") hook.

Drivers that need to extend the commit setup to private objects can use the
[`drm_mode_config_helper_funcs.atomic_commit_setup`](#c.drm_mode_config_helper_funcs "drm_mode_config_helper_funcs") hook.

To be able to use this support drivers need to use a few more helper
functions. [`drm_atomic_helper_wait_for_dependencies()`](#c.drm_atomic_helper_wait_for_dependencies "drm_atomic_helper_wait_for_dependencies") must be called before
actually committing the hardware state, and for nonblocking commits this call
must be placed in the async worker. See also [`drm_atomic_helper_swap_state()`](#c.drm_atomic_helper_swap_state "drm_atomic_helper_swap_state")
and its stall parameter, for when a driver’s commit hooks look at the
[`drm_crtc.state`](drm-kms.html#c.drm_crtc "drm_crtc"), [`drm_plane.state`](drm-kms.html#c.drm_plane "drm_plane") or [`drm_connector.state`](drm-kms.html#c.drm_connector "drm_connector") pointer directly.

Completion of the hardware commit step must be signalled using
[`drm_atomic_helper_commit_hw_done()`](#c.drm_atomic_helper_commit_hw_done "drm_atomic_helper_commit_hw_done"). After this step the driver is not allowed
to read or change any permanent software or hardware modeset state. The only
exception is state protected by other means than [`drm_modeset_lock`](drm-kms.html#c.drm_modeset_lock "drm_modeset_lock") locks.
Only the free standing **state** with pointers to the old state structures can
be inspected, e.g. to clean up old buffers using
[`drm_atomic_helper_cleanup_planes()`](#c.drm_atomic_helper_cleanup_planes "drm_atomic_helper_cleanup_planes").

At the very end, before cleaning up **state** drivers must call
[`drm_atomic_helper_commit_cleanup_done()`](#c.drm_atomic_helper_commit_cleanup_done "drm_atomic_helper_commit_cleanup_done").

This is all implemented by in [`drm_atomic_helper_commit()`](#c.drm_atomic_helper_commit "drm_atomic_helper_commit"), giving drivers a
complete and easy-to-use default implementation of the `atomic_commit()` hook.

The tracking of asynchronously executed and still pending commits is done
using the core structure [`drm_crtc_commit`](drm-kms.html#c.drm_crtc_commit "drm_crtc_commit").

By default there’s no need to clean up resources allocated by this function
explicitly: [`drm_atomic_state_default_clear()`](drm-kms.html#c.drm_atomic_state_default_clear "drm_atomic_state_default_clear") will take care of that
automatically.

**Return**

0 on success. -EBUSY when userspace schedules nonblocking commits too fast,
-ENOMEM on allocation failures and -EINTR when a signal is pending.

void drm\_atomic\_helper\_wait\_for\_dependencies(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   wait for required preceding commits

**Parameters**

`struct drm_atomic_state *state`
:   atomic state object being committed

**Description**

This function waits for all preceding commits that touch the same CRTC as
**state** to both be committed to the hardware (as signalled by
[`drm_atomic_helper_commit_hw_done()`](#c.drm_atomic_helper_commit_hw_done "drm_atomic_helper_commit_hw_done")) and executed by the hardware (as signalled
by calling [`drm_crtc_send_vblank_event()`](drm-kms.html#c.drm_crtc_send_vblank_event "drm_crtc_send_vblank_event") on the [`drm_crtc_state.event`](drm-kms.html#c.drm_crtc_state "drm_crtc_state")).

This is part of the atomic helper support for nonblocking commits, see
[`drm_atomic_helper_setup_commit()`](#c.drm_atomic_helper_setup_commit "drm_atomic_helper_setup_commit") for an overview.

void drm\_atomic\_helper\_fake\_vblank(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   fake VBLANK events if needed

**Parameters**

`struct drm_atomic_state *state`
:   atomic state object being committed

**Description**

This function walks all CRTCs and fakes VBLANK events on those with
[`drm_crtc_state.no_vblank`](drm-kms.html#c.drm_crtc_state "drm_crtc_state") set to true and [`drm_crtc_state.event`](drm-kms.html#c.drm_crtc_state "drm_crtc_state") != NULL.
The primary use of this function is writeback connectors working in oneshot
mode and faking VBLANK events. In this case they only fake the VBLANK event
when a job is queued, and any change to the pipeline that does not touch the
connector is leading to timeouts when calling
[`drm_atomic_helper_wait_for_vblanks()`](#c.drm_atomic_helper_wait_for_vblanks "drm_atomic_helper_wait_for_vblanks") or
[`drm_atomic_helper_wait_for_flip_done()`](#c.drm_atomic_helper_wait_for_flip_done "drm_atomic_helper_wait_for_flip_done"). In addition to writeback
connectors, this function can also fake VBLANK events for CRTCs without
VBLANK interrupt.

This is part of the atomic helper support for nonblocking commits, see
[`drm_atomic_helper_setup_commit()`](#c.drm_atomic_helper_setup_commit "drm_atomic_helper_setup_commit") for an overview.

void drm\_atomic\_helper\_commit\_hw\_done(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   setup possible nonblocking commit

**Parameters**

`struct drm_atomic_state *state`
:   atomic state object being committed

**Description**

This function is used to signal completion of the hardware commit step. After
this step the driver is not allowed to read or change any permanent software
or hardware modeset state. The only exception is state protected by other
means than [`drm_modeset_lock`](drm-kms.html#c.drm_modeset_lock "drm_modeset_lock") locks.

Drivers should try to postpone any expensive or delayed cleanup work after
this function is called.

This is part of the atomic helper support for nonblocking commits, see
[`drm_atomic_helper_setup_commit()`](#c.drm_atomic_helper_setup_commit "drm_atomic_helper_setup_commit") for an overview.

void drm\_atomic\_helper\_commit\_cleanup\_done(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   signal completion of commit

**Parameters**

`struct drm_atomic_state *state`
:   atomic state object being committed

**Description**

This signals completion of the atomic update **state**, including any
cleanup work. If used, it must be called right before calling
[`drm_atomic_state_put()`](drm-kms.html#c.drm_atomic_state_put "drm_atomic_state_put").

This is part of the atomic helper support for nonblocking commits, see
[`drm_atomic_helper_setup_commit()`](#c.drm_atomic_helper_setup_commit "drm_atomic_helper_setup_commit") for an overview.

int drm\_atomic\_helper\_prepare\_planes(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   prepare plane resources before commit

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   atomic state object with new state structures

**Description**

This function prepares plane state, specifically framebuffers, for the new
configuration, by calling [`drm_plane_helper_funcs.prepare_fb`](#c.drm_plane_helper_funcs "drm_plane_helper_funcs"). If any failure
is encountered this function will call [`drm_plane_helper_funcs.cleanup_fb`](#c.drm_plane_helper_funcs "drm_plane_helper_funcs") on
any already successfully prepared framebuffer.

**Return**

0 on success, negative error code on failure.

void drm\_atomic\_helper\_unprepare\_planes(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   release plane resources on aborts

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   atomic state object with old state structures

**Description**

This function cleans up plane state, specifically framebuffers, from the
atomic state. It undoes the effects of [`drm_atomic_helper_prepare_planes()`](#c.drm_atomic_helper_prepare_planes "drm_atomic_helper_prepare_planes")
when aborting an atomic commit. For cleaning up after a successful commit
use [`drm_atomic_helper_cleanup_planes()`](#c.drm_atomic_helper_cleanup_planes "drm_atomic_helper_cleanup_planes").

void drm\_atomic\_helper\_commit\_planes(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state, uint32\_t flags)
:   commit plane state

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   atomic state object being committed

`uint32_t flags`
:   flags for committing plane state

**Description**

This function commits the new plane state using the plane and atomic helper
functions for planes and CRTCs. It assumes that the atomic state has already
been pushed into the relevant object state pointers, since this step can no
longer fail.

It still requires the global state object **state** to know which planes and
crtcs need to be updated though.

Note that this function does all plane updates across all CRTCs in one step.
If the hardware can’t support this approach look at
[`drm_atomic_helper_commit_planes_on_crtc()`](#c.drm_atomic_helper_commit_planes_on_crtc "drm_atomic_helper_commit_planes_on_crtc") instead.

Plane parameters can be updated by applications while the associated CRTC is
disabled. The DRM/KMS core will store the parameters in the plane state,
which will be available to the driver when the CRTC is turned on. As a result
most drivers don’t need to be immediately notified of plane updates for a
disabled CRTC.

Unless otherwise needed, drivers are advised to set the ACTIVE\_ONLY flag in
**flags** in order not to receive plane update notifications related to a
disabled CRTC. This avoids the need to manually ignore plane updates in
driver code when the driver and/or hardware can’t or just don’t need to deal
with updates on disabled CRTCs, for example when supporting runtime PM.

Drivers may set the NO\_DISABLE\_AFTER\_MODESET flag in **flags** if the relevant
display controllers require to disable a CRTC’s planes when the CRTC is
disabled. This function would skip the [`drm_plane_helper_funcs.atomic_disable`](#c.drm_plane_helper_funcs "drm_plane_helper_funcs")
call for a plane if the CRTC of the old plane state needs a modesetting
operation. Of course, the drivers need to disable the planes in their CRTC
disable callbacks since no one else would do that.

The [`drm_atomic_helper_commit()`](#c.drm_atomic_helper_commit "drm_atomic_helper_commit") default implementation doesn’t set the
ACTIVE\_ONLY flag to most closely match the behaviour of the legacy helpers.
This should not be copied blindly by drivers.

void drm\_atomic\_helper\_commit\_planes\_on\_crtc(struct [drm\_crtc\_state](drm-kms.html#c.drm_crtc_state "drm_crtc_state") \*old\_crtc\_state)
:   commit plane state for a CRTC

**Parameters**

`struct drm_crtc_state *old_crtc_state`
:   atomic state object with the old CRTC state

**Description**

This function commits the new plane state using the plane and atomic helper
functions for planes on the specific CRTC. It assumes that the atomic state
has already been pushed into the relevant object state pointers, since this
step can no longer fail.

This function is useful when plane updates should be done CRTC-by-CRTC
instead of one global step like [`drm_atomic_helper_commit_planes()`](#c.drm_atomic_helper_commit_planes "drm_atomic_helper_commit_planes") does.

This function can only be savely used when planes are not allowed to move
between different CRTCs because this function doesn’t handle inter-CRTC
dependencies. Callers need to ensure that either no such dependencies exist,
resolve them through ordering of commit calls or through some other means.

void drm\_atomic\_helper\_disable\_planes\_on\_crtc(struct [drm\_crtc\_state](drm-kms.html#c.drm_crtc_state "drm_crtc_state") \*old\_crtc\_state, bool atomic)
:   helper to disable CRTC’s planes

**Parameters**

`struct drm_crtc_state *old_crtc_state`
:   atomic state object with the old CRTC state

`bool atomic`
:   if set, synchronize with CRTC’s atomic\_begin/flush hooks

**Description**

Disables all planes associated with the given CRTC. This can be
used for instance in the CRTC helper atomic\_disable callback to disable
all planes.

If the atomic-parameter is set the function calls the CRTC’s
atomic\_begin hook before and atomic\_flush hook after disabling the
planes.

It is a bug to call this function without having implemented the
[`drm_plane_helper_funcs.atomic_disable`](#c.drm_plane_helper_funcs "drm_plane_helper_funcs") plane hook.

void drm\_atomic\_helper\_cleanup\_planes(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   cleanup plane resources after commit

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   atomic state object being committed

**Description**

This function cleans up plane state, specifically framebuffers, from the old
configuration. Hence the old configuration must be perserved in **state** to
be able to call this function.

This function may not be called on the new state when the atomic update
fails at any point after calling [`drm_atomic_helper_prepare_planes()`](#c.drm_atomic_helper_prepare_planes "drm_atomic_helper_prepare_planes"). Use
[`drm_atomic_helper_unprepare_planes()`](#c.drm_atomic_helper_unprepare_planes "drm_atomic_helper_unprepare_planes") in this case.

int drm\_atomic\_helper\_swap\_state(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state, bool stall)
:   store atomic state into current sw state

**Parameters**

`struct drm_atomic_state *state`
:   atomic state

`bool stall`
:   stall for preceding commits

**Description**

This function stores the atomic state into the current state pointers in all
driver objects. It should be called after all failing steps have been done
and succeeded, but before the actual hardware state is committed.

For cleanup and error recovery the current state for all changed objects will
be swapped into **state**.

With that sequence it fits perfectly into the plane prepare/cleanup sequence:

1. Call [`drm_atomic_helper_prepare_planes()`](#c.drm_atomic_helper_prepare_planes "drm_atomic_helper_prepare_planes") with the staged atomic state.
2. Do any other steps that might fail.
3. Put the staged state into the current state pointers with this function.
4. Actually commit the hardware state.

5. Call [`drm_atomic_helper_cleanup_planes()`](#c.drm_atomic_helper_cleanup_planes "drm_atomic_helper_cleanup_planes") with **state**, which since step 3
contains the old state. Also do any other cleanup required with that state.

**stall** must be set when nonblocking commits for this driver directly access
the [`drm_plane.state`](drm-kms.html#c.drm_plane "drm_plane"), [`drm_crtc.state`](drm-kms.html#c.drm_crtc "drm_crtc") or [`drm_connector.state`](drm-kms.html#c.drm_connector "drm_connector") pointer. With
the current atomic helpers this is almost always the case, since the helpers
don’t pass the right state structures to the callbacks.

**Return**

Returns 0 on success. Can return -ERESTARTSYS when **stall** is true and the
waiting for the previous commits has been interrupted.

int drm\_atomic\_helper\_update\_plane(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, int crtc\_x, int crtc\_y, unsigned int crtc\_w, unsigned int crtc\_h, uint32\_t src\_x, uint32\_t src\_y, uint32\_t src\_w, uint32\_t src\_h, struct [drm\_modeset\_acquire\_ctx](drm-kms.html#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx)
:   Helper for primary plane update using atomic

**Parameters**

`struct drm_plane *plane`
:   plane object to update

`struct drm_crtc *crtc`
:   owning CRTC of owning plane

`struct drm_framebuffer *fb`
:   framebuffer to flip onto plane

`int crtc_x`
:   x offset of primary plane on **crtc**

`int crtc_y`
:   y offset of primary plane on **crtc**

`unsigned int crtc_w`
:   width of primary plane rectangle on **crtc**

`unsigned int crtc_h`
:   height of primary plane rectangle on **crtc**

`uint32_t src_x`
:   x offset of **fb** for panning

`uint32_t src_y`
:   y offset of **fb** for panning

`uint32_t src_w`
:   width of source rectangle in **fb**

`uint32_t src_h`
:   height of source rectangle in **fb**

`struct drm_modeset_acquire_ctx *ctx`
:   lock acquire context

**Description**

Provides a default plane update handler using the atomic driver interface.

**Return**

Zero on success, error code on failure

int drm\_atomic\_helper\_disable\_plane(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_modeset\_acquire\_ctx](drm-kms.html#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx)
:   Helper for primary plane disable using atomic

**Parameters**

`struct drm_plane *plane`
:   plane to disable

`struct drm_modeset_acquire_ctx *ctx`
:   lock acquire context

**Description**

Provides a default plane disable handler using the atomic driver interface.

**Return**

Zero on success, error code on failure

int drm\_atomic\_helper\_set\_config(struct [drm\_mode\_set](drm-kms.html#c.drm_mode_set "drm_mode_set") \*set, struct [drm\_modeset\_acquire\_ctx](drm-kms.html#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx)
:   set a new config from userspace

**Parameters**

`struct drm_mode_set *set`
:   mode set configuration

`struct drm_modeset_acquire_ctx *ctx`
:   lock acquisition context

**Description**

Provides a default CRTC set\_config handler using the atomic driver interface.

**NOTE**

For backwards compatibility with old userspace this automatically
resets the “link-status” property to GOOD, to force any link
re-training. The SETCRTC ioctl does not define whether an update does
need a full modeset or just a plane update, hence we’re allowed to do
that. See also [`drm_connector_set_link_status_property()`](drm-kms.html#c.drm_connector_set_link_status_property "drm_connector_set_link_status_property").

**Return**

Returns 0 on success, negative errno numbers on failure.

int drm\_atomic\_helper\_disable\_all(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_modeset\_acquire\_ctx](drm-kms.html#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx)
:   disable all currently active outputs

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_modeset_acquire_ctx *ctx`
:   lock acquisition context

**Description**

Loops through all connectors, finding those that aren’t turned off and then
turns them off by setting their DPMS mode to OFF and deactivating the CRTC
that they are connected to.

This is used for example in suspend/resume to disable all currently active
functions when suspending. If you just want to shut down everything at e.g.
driver unload, look at [`drm_atomic_helper_shutdown()`](#c.drm_atomic_helper_shutdown "drm_atomic_helper_shutdown").

Note that if callers haven’t already acquired all modeset locks this might
return -EDEADLK, which must be handled by calling [`drm_modeset_backoff()`](drm-kms.html#c.drm_modeset_backoff "drm_modeset_backoff").

See also:
[`drm_atomic_helper_suspend()`](#c.drm_atomic_helper_suspend "drm_atomic_helper_suspend"), [`drm_atomic_helper_resume()`](#c.drm_atomic_helper_resume "drm_atomic_helper_resume") and
[`drm_atomic_helper_shutdown()`](#c.drm_atomic_helper_shutdown "drm_atomic_helper_shutdown").

**Return**

0 on success or a negative error code on failure.

int drm\_atomic\_helper\_reset\_crtc(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_modeset\_acquire\_ctx](drm-kms.html#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx)
:   reset the active outputs of a CRTC

**Parameters**

`struct drm_crtc *crtc`
:   DRM CRTC

`struct drm_modeset_acquire_ctx *ctx`
:   lock acquisition context

**Description**

Reset the active outputs by indicating that connectors have changed.
This implies a reset of all active components available between the CRTC and
connectors.

A variant of this function exists with
`drm_bridge_helper_reset_crtc()`, dedicated to bridges.

**NOTE**

This relies on resetting [`drm_crtc_state.connectors_changed`](drm-kms.html#c.drm_crtc_state "drm_crtc_state").
For drivers which optimize out unnecessary modesets this will result in
a no-op commit, achieving nothing.

**Return**

0 on success or a negative error code on failure.

void drm\_atomic\_helper\_shutdown(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   shutdown all CRTC

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

This shuts down all CRTC, which is useful for driver unloading. Shutdown on
suspend should instead be handled with [`drm_atomic_helper_suspend()`](#c.drm_atomic_helper_suspend "drm_atomic_helper_suspend"), since
that also takes a snapshot of the modeset state to be restored on resume.

This is just a convenience wrapper around [`drm_atomic_helper_disable_all()`](#c.drm_atomic_helper_disable_all "drm_atomic_helper_disable_all"),
and it is the atomic version of [`drm_helper_force_disable_all()`](#c.drm_helper_force_disable_all "drm_helper_force_disable_all").

struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*drm\_atomic\_helper\_duplicate\_state(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_modeset\_acquire\_ctx](drm-kms.html#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx)
:   duplicate an atomic state object

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_modeset_acquire_ctx *ctx`
:   lock acquisition context

**Description**

Makes a copy of the current atomic state by looping over all objects and
duplicating their respective states. This is used for example by suspend/
resume support code to save the state prior to suspend such that it can
be restored upon resume.

Note that this treats atomic state as persistent between save and restore.
Drivers must make sure that this is possible and won’t result in confusion
or erroneous behaviour.

Note that if callers haven’t already acquired all modeset locks this might
return -EDEADLK, which must be handled by calling [`drm_modeset_backoff()`](drm-kms.html#c.drm_modeset_backoff "drm_modeset_backoff").

See also:
[`drm_atomic_helper_suspend()`](#c.drm_atomic_helper_suspend "drm_atomic_helper_suspend"), [`drm_atomic_helper_resume()`](#c.drm_atomic_helper_resume "drm_atomic_helper_resume")

**Return**

A pointer to the copy of the atomic state object on success or an
[`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR")-encoded error code on failure.

struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*drm\_atomic\_helper\_suspend(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   subsystem-level suspend helper

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

Duplicates the current atomic state, disables all active outputs and then
returns a pointer to the original atomic state to the caller. Drivers can
pass this pointer to the [`drm_atomic_helper_resume()`](#c.drm_atomic_helper_resume "drm_atomic_helper_resume") helper upon resume to
restore the output configuration that was active at the time the system
entered suspend.

Note that it is potentially unsafe to use this. The atomic state object
returned by this function is assumed to be persistent. Drivers must ensure
that this holds true. Before calling this function, drivers must make sure
to suspend fbdev emulation so that nothing can be using the device.

See also:
[`drm_atomic_helper_duplicate_state()`](#c.drm_atomic_helper_duplicate_state "drm_atomic_helper_duplicate_state"), [`drm_atomic_helper_disable_all()`](#c.drm_atomic_helper_disable_all "drm_atomic_helper_disable_all"),
[`drm_atomic_helper_resume()`](#c.drm_atomic_helper_resume "drm_atomic_helper_resume"), [`drm_atomic_helper_commit_duplicated_state()`](#c.drm_atomic_helper_commit_duplicated_state "drm_atomic_helper_commit_duplicated_state")

**Return**

A pointer to a copy of the state before suspend on success or an [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR")-
encoded error code on failure. Drivers should store the returned atomic
state object and pass it to the [`drm_atomic_helper_resume()`](#c.drm_atomic_helper_resume "drm_atomic_helper_resume") helper upon
resume.

int drm\_atomic\_helper\_commit\_duplicated\_state(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_modeset\_acquire\_ctx](drm-kms.html#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx)
:   commit duplicated state

**Parameters**

`struct drm_atomic_state *state`
:   duplicated atomic state to commit

`struct drm_modeset_acquire_ctx *ctx`
:   pointer to acquire\_ctx to use for commit.

**Description**

The state returned by [`drm_atomic_helper_duplicate_state()`](#c.drm_atomic_helper_duplicate_state "drm_atomic_helper_duplicate_state") and
[`drm_atomic_helper_suspend()`](#c.drm_atomic_helper_suspend "drm_atomic_helper_suspend") is partially invalid, and needs to
be fixed up before commit.

See also:
[`drm_atomic_helper_suspend()`](#c.drm_atomic_helper_suspend "drm_atomic_helper_suspend")

**Return**

0 on success or a negative error code on failure.

int drm\_atomic\_helper\_resume(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   subsystem-level resume helper

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_atomic_state *state`
:   atomic state to resume to

**Description**

Calls [`drm_mode_config_reset()`](drm-kms.html#c.drm_mode_config_reset "drm_mode_config_reset") to synchronize hardware and software states,
grabs all modeset locks and commits the atomic state object. This can be
used in conjunction with the [`drm_atomic_helper_suspend()`](#c.drm_atomic_helper_suspend "drm_atomic_helper_suspend") helper to
implement suspend/resume for drivers that support atomic mode-setting.

See also:
[`drm_atomic_helper_suspend()`](#c.drm_atomic_helper_suspend "drm_atomic_helper_suspend")

**Return**

0 on success or a negative error code on failure.

int drm\_atomic\_helper\_page\_flip(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, struct [drm\_pending\_vblank\_event](drm-kms.html#c.drm_pending_vblank_event "drm_pending_vblank_event") \*event, uint32\_t flags, struct [drm\_modeset\_acquire\_ctx](drm-kms.html#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx)
:   execute a legacy page flip

**Parameters**

`struct drm_crtc *crtc`
:   DRM CRTC

`struct drm_framebuffer *fb`
:   DRM framebuffer

`struct drm_pending_vblank_event *event`
:   optional DRM event to signal upon completion

`uint32_t flags`
:   flip flags for non-vblank sync’ed updates

`struct drm_modeset_acquire_ctx *ctx`
:   lock acquisition context

**Description**

Provides a default [`drm_crtc_funcs.page_flip`](drm-kms.html#c.drm_crtc_funcs "drm_crtc_funcs") implementation
using the atomic driver interface.

See also:
[`drm_atomic_helper_page_flip_target()`](#c.drm_atomic_helper_page_flip_target "drm_atomic_helper_page_flip_target")

**Return**

Returns 0 on success, negative errno numbers on failure.

int drm\_atomic\_helper\_page\_flip\_target(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, struct [drm\_pending\_vblank\_event](drm-kms.html#c.drm_pending_vblank_event "drm_pending_vblank_event") \*event, uint32\_t flags, uint32\_t target, struct [drm\_modeset\_acquire\_ctx](drm-kms.html#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx)
:   do page flip on target vblank period.

**Parameters**

`struct drm_crtc *crtc`
:   DRM CRTC

`struct drm_framebuffer *fb`
:   DRM framebuffer

`struct drm_pending_vblank_event *event`
:   optional DRM event to signal upon completion

`uint32_t flags`
:   flip flags for non-vblank sync’ed updates

`uint32_t target`
:   specifying the target vblank period when the flip to take effect

`struct drm_modeset_acquire_ctx *ctx`
:   lock acquisition context

**Description**

Provides a default [`drm_crtc_funcs.page_flip_target`](drm-kms.html#c.drm_crtc_funcs "drm_crtc_funcs") implementation.
Similar to [`drm_atomic_helper_page_flip()`](#c.drm_atomic_helper_page_flip "drm_atomic_helper_page_flip") with extra parameter to specify
target vblank period to flip.

**Return**

Returns 0 on success, negative errno numbers on failure.

u32 \*drm\_atomic\_helper\_bridge\_propagate\_bus\_fmt(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge, struct [drm\_bridge\_state](drm-kms.html#c.drm_bridge_state "drm_bridge_state") \*bridge\_state, struct [drm\_crtc\_state](drm-kms.html#c.drm_crtc_state "drm_crtc_state") \*crtc\_state, struct [drm\_connector\_state](drm-kms.html#c.drm_connector_state "drm_connector_state") \*conn\_state, u32 output\_fmt, unsigned int \*num\_input\_fmts)
:   Propagate output format to the input end of a bridge

**Parameters**

`struct drm_bridge *bridge`
:   bridge control structure

`struct drm_bridge_state *bridge_state`
:   new bridge state

`struct drm_crtc_state *crtc_state`
:   new CRTC state

`struct drm_connector_state *conn_state`
:   new connector state

`u32 output_fmt`
:   tested output bus format

`unsigned int *num_input_fmts`
:   will contain the size of the returned array

**Description**

This helper is a pluggable implementation of the
[`drm_bridge_funcs.atomic_get_input_bus_fmts`](#c.drm_bridge_funcs "drm_bridge_funcs") operation for bridges that don’t
modify the bus configuration between their input and their output. It
returns an array of input formats with a single element set to **output\_fmt**.

**Return**

a valid format array of size **num\_input\_fmts**, or NULL if the allocation
failed

### Atomic State Reset and Initialization

Both the drm core and the atomic helpers assume that there is always the full
and correct atomic software state for all connectors, CRTCs and planes
available. Which is a bit a problem on driver load and also after system
suspend. One way to solve this is to have a hardware state read-out
infrastructure which reconstructs the full software state (e.g. the i915
driver).

The simpler solution is to just reset the software state to everything off,
which is easiest to do by calling [`drm_mode_config_reset()`](drm-kms.html#c.drm_mode_config_reset "drm_mode_config_reset"). To facilitate this
the atomic helpers provide default reset implementations for all hooks.

On the upside the precise state tracking of atomic simplifies system suspend
and resume a lot. For drivers using [`drm_mode_config_reset()`](drm-kms.html#c.drm_mode_config_reset "drm_mode_config_reset") a complete recipe
is implemented in [`drm_atomic_helper_suspend()`](#c.drm_atomic_helper_suspend "drm_atomic_helper_suspend") and [`drm_atomic_helper_resume()`](#c.drm_atomic_helper_resume "drm_atomic_helper_resume").
For other drivers the building blocks are split out, see the documentation
for these functions.

### Atomic State Helper Reference

void \_\_drm\_atomic\_helper\_crtc\_state\_reset(struct [drm\_crtc\_state](drm-kms.html#c.drm_crtc_state "drm_crtc_state") \*crtc\_state, struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc)
:   reset the CRTC state

**Parameters**

`struct drm_crtc_state *crtc_state`
:   atomic CRTC state, must not be NULL

`struct drm_crtc *crtc`
:   CRTC object, must not be NULL

**Description**

Initializes the newly allocated **crtc\_state** with default
values. This is useful for drivers that subclass the CRTC state.

void \_\_drm\_atomic\_helper\_crtc\_reset(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_crtc\_state](drm-kms.html#c.drm_crtc_state "drm_crtc_state") \*crtc\_state)
:   reset state on CRTC

**Parameters**

`struct drm_crtc *crtc`
:   drm CRTC

`struct drm_crtc_state *crtc_state`
:   CRTC state to assign

**Description**

Initializes the newly allocated **crtc\_state** and assigns it to
the [`drm_crtc->state`](drm-kms.html#c.drm_crtc "drm_crtc") pointer of **crtc**, usually required when
initializing the drivers or when called from the [`drm_crtc_funcs.reset`](drm-kms.html#c.drm_crtc_funcs "drm_crtc_funcs")
hook.

This is useful for drivers that subclass the CRTC state.

void drm\_atomic\_helper\_crtc\_reset(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc)
:   default [`drm_crtc_funcs.reset`](drm-kms.html#c.drm_crtc_funcs "drm_crtc_funcs") hook for CRTCs

**Parameters**

`struct drm_crtc *crtc`
:   drm CRTC

**Description**

Resets the atomic state for **crtc** by freeing the state pointer (which might
be NULL, e.g. at driver load time) and allocating a new empty state object.

void \_\_drm\_atomic\_helper\_crtc\_duplicate\_state(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_crtc\_state](drm-kms.html#c.drm_crtc_state "drm_crtc_state") \*state)
:   copy atomic CRTC state

**Parameters**

`struct drm_crtc *crtc`
:   CRTC object

`struct drm_crtc_state *state`
:   atomic CRTC state

**Description**

Copies atomic state from a CRTC’s current state and resets inferred values.
This is useful for drivers that subclass the CRTC state.

struct [drm\_crtc\_state](drm-kms.html#c.drm_crtc_state "drm_crtc_state") \*drm\_atomic\_helper\_crtc\_duplicate\_state(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc)
:   default state duplicate hook

**Parameters**

`struct drm_crtc *crtc`
:   drm CRTC

**Description**

Default CRTC state duplicate hook for drivers which don’t have their own
subclassed CRTC state structure.

void \_\_drm\_atomic\_helper\_crtc\_destroy\_state(struct [drm\_crtc\_state](drm-kms.html#c.drm_crtc_state "drm_crtc_state") \*state)
:   release CRTC state

**Parameters**

`struct drm_crtc_state *state`
:   CRTC state object to release

**Description**

Releases all resources stored in the CRTC state without actually freeing
the memory of the CRTC state. This is useful for drivers that subclass the
CRTC state.

void drm\_atomic\_helper\_crtc\_destroy\_state(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_crtc\_state](drm-kms.html#c.drm_crtc_state "drm_crtc_state") \*state)
:   default state destroy hook

**Parameters**

`struct drm_crtc *crtc`
:   drm CRTC

`struct drm_crtc_state *state`
:   CRTC state object to release

**Description**

Default CRTC state destroy hook for drivers which don’t have their own
subclassed CRTC state structure.

void \_\_drm\_atomic\_helper\_plane\_state\_reset(struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*plane\_state, struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane)
:   resets plane state to default values

**Parameters**

`struct drm_plane_state *plane_state`
:   atomic plane state, must not be NULL

`struct drm_plane *plane`
:   plane object, must not be NULL

**Description**

Initializes the newly allocated **plane\_state** with default
values. This is useful for drivers that subclass the CRTC state.

void \_\_drm\_atomic\_helper\_plane\_reset(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*plane\_state)
:   reset state on plane

**Parameters**

`struct drm_plane *plane`
:   drm plane

`struct drm_plane_state *plane_state`
:   plane state to assign

**Description**

Initializes the newly allocated **plane\_state** and assigns it to
the [`drm_crtc->state`](drm-kms.html#c.drm_crtc "drm_crtc") pointer of **plane**, usually required when
initializing the drivers or when called from the [`drm_plane_funcs.reset`](drm-kms.html#c.drm_plane_funcs "drm_plane_funcs")
hook.

This is useful for drivers that subclass the plane state.

void drm\_atomic\_helper\_plane\_reset(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane)
:   default [`drm_plane_funcs.reset`](drm-kms.html#c.drm_plane_funcs "drm_plane_funcs") hook for planes

**Parameters**

`struct drm_plane *plane`
:   drm plane

**Description**

Resets the atomic state for **plane** by freeing the state pointer (which might
be NULL, e.g. at driver load time) and allocating a new empty state object.

void \_\_drm\_atomic\_helper\_plane\_duplicate\_state(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*state)
:   copy atomic plane state

**Parameters**

`struct drm_plane *plane`
:   plane object

`struct drm_plane_state *state`
:   atomic plane state

**Description**

Copies atomic state from a plane’s current state. This is useful for
drivers that subclass the plane state.

struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*drm\_atomic\_helper\_plane\_duplicate\_state(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane)
:   default state duplicate hook

**Parameters**

`struct drm_plane *plane`
:   drm plane

**Description**

Default plane state duplicate hook for drivers which don’t have their own
subclassed plane state structure.

void \_\_drm\_atomic\_helper\_plane\_destroy\_state(struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*state)
:   release plane state

**Parameters**

`struct drm_plane_state *state`
:   plane state object to release

**Description**

Releases all resources stored in the plane state without actually freeing
the memory of the plane state. This is useful for drivers that subclass the
plane state.

void drm\_atomic\_helper\_plane\_destroy\_state(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*state)
:   default state destroy hook

**Parameters**

`struct drm_plane *plane`
:   drm plane

`struct drm_plane_state *state`
:   plane state object to release

**Description**

Default plane state destroy hook for drivers which don’t have their own
subclassed plane state structure.

void \_\_drm\_atomic\_helper\_connector\_state\_reset(struct [drm\_connector\_state](drm-kms.html#c.drm_connector_state "drm_connector_state") \*conn\_state, struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   reset the connector state

**Parameters**

`struct drm_connector_state *conn_state`
:   atomic connector state, must not be NULL

`struct drm_connector *connector`
:   connectotr object, must not be NULL

**Description**

Initializes the newly allocated **conn\_state** with default
values. This is useful for drivers that subclass the connector state.

void \_\_drm\_atomic\_helper\_connector\_reset(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct [drm\_connector\_state](drm-kms.html#c.drm_connector_state "drm_connector_state") \*conn\_state)
:   reset state on connector

**Parameters**

`struct drm_connector *connector`
:   drm connector

`struct drm_connector_state *conn_state`
:   connector state to assign

**Description**

Initializes the newly allocated **conn\_state** and assigns it to
the [`drm_connector->state`](drm-kms.html#c.drm_connector "drm_connector") pointer of **connector**, usually required when
initializing the drivers or when called from the [`drm_connector_funcs.reset`](drm-kms.html#c.drm_connector_funcs "drm_connector_funcs")
hook.

This is useful for drivers that subclass the connector state.

void drm\_atomic\_helper\_connector\_reset(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   default [`drm_connector_funcs.reset`](drm-kms.html#c.drm_connector_funcs "drm_connector_funcs") hook for connectors

**Parameters**

`struct drm_connector *connector`
:   drm connector

**Description**

Resets the atomic state for **connector** by freeing the state pointer (which
might be NULL, e.g. at driver load time) and allocating a new empty state
object.

void drm\_atomic\_helper\_connector\_tv\_margins\_reset(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   Resets TV connector properties

**Parameters**

`struct drm_connector *connector`
:   DRM connector

**Description**

Resets the TV-related properties attached to a connector.

void drm\_atomic\_helper\_connector\_tv\_reset(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   Resets Analog TV connector properties

**Parameters**

`struct drm_connector *connector`
:   DRM connector

**Description**

Resets the analog TV properties attached to a connector

int drm\_atomic\_helper\_connector\_tv\_check(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   Validate an analog TV connector state

**Parameters**

`struct drm_connector *connector`
:   DRM Connector

`struct drm_atomic_state *state`
:   the DRM State object

**Description**

Checks the state object to see if the requested state is valid for an
analog TV connector.

**Return**

`0` for success, a negative error code on error.

void \_\_drm\_atomic\_helper\_connector\_duplicate\_state(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct [drm\_connector\_state](drm-kms.html#c.drm_connector_state "drm_connector_state") \*state)
:   copy atomic connector state

**Parameters**

`struct drm_connector *connector`
:   connector object

`struct drm_connector_state *state`
:   atomic connector state

**Description**

Copies atomic state from a connector’s current state. This is useful for
drivers that subclass the connector state.

struct [drm\_connector\_state](drm-kms.html#c.drm_connector_state "drm_connector_state") \*drm\_atomic\_helper\_connector\_duplicate\_state(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   default state duplicate hook

**Parameters**

`struct drm_connector *connector`
:   drm connector

**Description**

Default connector state duplicate hook for drivers which don’t have their own
subclassed connector state structure.

void \_\_drm\_atomic\_helper\_connector\_destroy\_state(struct [drm\_connector\_state](drm-kms.html#c.drm_connector_state "drm_connector_state") \*state)
:   release connector state

**Parameters**

`struct drm_connector_state *state`
:   connector state object to release

**Description**

Releases all resources stored in the connector state without actually
freeing the memory of the connector state. This is useful for drivers that
subclass the connector state.

void drm\_atomic\_helper\_connector\_destroy\_state(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct [drm\_connector\_state](drm-kms.html#c.drm_connector_state "drm_connector_state") \*state)
:   default state destroy hook

**Parameters**

`struct drm_connector *connector`
:   drm connector

`struct drm_connector_state *state`
:   connector state object to release

**Description**

Default connector state destroy hook for drivers which don’t have their own
subclassed connector state structure.

void \_\_drm\_atomic\_helper\_private\_obj\_create\_state(struct [drm\_private\_obj](drm-kms.html#c.drm_private_obj "drm_private_obj") \*obj, struct [drm\_private\_state](drm-kms.html#c.drm_private_state "drm_private_state") \*state)
:   initializes private object state

**Parameters**

`struct drm_private_obj *obj`
:   private object

`struct drm_private_state *state`
:   new state to initialize

**Description**

Initializes the newly allocated **state**, usually required when
initializing the drivers.

**obj** is assumed to be zeroed.

This is useful for drivers that use private states.

void \_\_drm\_atomic\_helper\_private\_obj\_duplicate\_state(struct [drm\_private\_obj](drm-kms.html#c.drm_private_obj "drm_private_obj") \*obj, struct [drm\_private\_state](drm-kms.html#c.drm_private_state "drm_private_state") \*state)
:   copy atomic private state

**Parameters**

`struct drm_private_obj *obj`
:   CRTC object

`struct drm_private_state *state`
:   new private object state

**Description**

Copies atomic state from a private objects’s current state and resets inferred values.
This is useful for drivers that subclass the private state.

void \_\_drm\_atomic\_helper\_bridge\_duplicate\_state(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge, struct [drm\_bridge\_state](drm-kms.html#c.drm_bridge_state "drm_bridge_state") \*state)
:   Copy atomic bridge state

**Parameters**

`struct drm_bridge *bridge`
:   bridge object

`struct drm_bridge_state *state`
:   atomic bridge state

**Description**

Copies atomic state from a bridge’s current state and resets inferred values.
This is useful for drivers that subclass the bridge state.

struct [drm\_bridge\_state](drm-kms.html#c.drm_bridge_state "drm_bridge_state") \*drm\_atomic\_helper\_bridge\_duplicate\_state(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge)
:   Duplicate a bridge state object

**Parameters**

`struct drm_bridge *bridge`
:   bridge object

**Description**

Allocates a new bridge state and initializes it with the current bridge
state values. This helper is meant to be used as a bridge
[`drm_bridge_funcs.atomic_duplicate_state`](#c.drm_bridge_funcs "drm_bridge_funcs") hook for bridges that don’t
subclass the bridge state.

void drm\_atomic\_helper\_bridge\_destroy\_state(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge, struct [drm\_bridge\_state](drm-kms.html#c.drm_bridge_state "drm_bridge_state") \*state)
:   Destroy a bridge state object

**Parameters**

`struct drm_bridge *bridge`
:   the bridge this state refers to

`struct drm_bridge_state *state`
:   bridge state to destroy

**Description**

Destroys a bridge state previously created by
[`` drm_atomic_helper_bridge_reset`() or
:c:type:`drm_atomic_helper_bridge_duplicate_state`(). This helper is meant to be
used as a bridge :c:type:`drm_bridge_funcs.atomic_destroy_state ``](#c.drm_bridge_funcs "drm_bridge_funcs") hook for bridges
that don’t subclass the bridge state.

void \_\_drm\_atomic\_helper\_bridge\_reset(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge, struct [drm\_bridge\_state](drm-kms.html#c.drm_bridge_state "drm_bridge_state") \*state)
:   Initialize a bridge state to its default

**Parameters**

`struct drm_bridge *bridge`
:   the bridge this state refers to

`struct drm_bridge_state *state`
:   bridge state to initialize

**Description**

Initializes the bridge state to default values. This is meant to be called
by the bridge [`drm_bridge_funcs.atomic_reset`](#c.drm_bridge_funcs "drm_bridge_funcs") hook for bridges that subclass
the bridge state.

struct [drm\_bridge\_state](drm-kms.html#c.drm_bridge_state "drm_bridge_state") \*drm\_atomic\_helper\_bridge\_reset(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge)
:   Allocate and initialize a bridge state to its default

**Parameters**

`struct drm_bridge *bridge`
:   the bridge this state refers to

**Description**

Allocates the bridge state and initializes it to default values. This helper
is meant to be used as a bridge [`drm_bridge_funcs.atomic_reset`](#c.drm_bridge_funcs "drm_bridge_funcs") hook for
bridges that don’t subclass the bridge state.

### GEM Atomic Helper Reference

The GEM atomic helpers library implements generic atomic-commit
functions for drivers that use GEM objects. Currently, it provides
synchronization helpers, and plane state and framebuffer BO mappings
for planes with shadow buffers.

Before scanout, a plane’s framebuffer needs to be synchronized with
possible writers that draw into the framebuffer. All drivers should
call [`drm_gem_plane_helper_prepare_fb()`](#c.drm_gem_plane_helper_prepare_fb "drm_gem_plane_helper_prepare_fb") from their implementation of
struct `drm_plane_helper.prepare_fb` . It sets the plane’s fence from
the framebuffer so that the DRM core can synchronize access automatically.
[`drm_gem_plane_helper_prepare_fb()`](#c.drm_gem_plane_helper_prepare_fb "drm_gem_plane_helper_prepare_fb") can also be used directly as
implementation of prepare\_fb.

```
#include <drm/drm_gem_atomic_helper.h>

struct drm_plane_helper_funcs driver_plane_helper_funcs = {
        ...,
        . prepare_fb = drm_gem_plane_helper_prepare_fb,
};
```

A driver using a shadow buffer copies the content of the shadow buffers
into the HW’s framebuffer memory during an atomic update. This requires
a mapping of the shadow buffer into kernel address space. The mappings
cannot be established by commit-tail functions, such as atomic\_update,
as this would violate locking rules around [`dma_buf_vmap()`](../driver-api/dma-buf.html#c.dma_buf_vmap "dma_buf_vmap").

The helpers for shadow-buffered planes establish and release mappings,
and provide [`struct drm_shadow_plane_state`](#c.drm_shadow_plane_state "drm_shadow_plane_state"), which stores the plane’s mapping
for commit-tail functions.

Shadow-buffered planes can easily be enabled by using the provided macros
`DRM_GEM_SHADOW_PLANE_FUNCS` and `DRM_GEM_SHADOW_PLANE_HELPER_FUNCS`.
These macros set up the plane and plane-helper callbacks to point to the
shadow-buffer helpers.

```
#include <drm/drm_gem_atomic_helper.h>

struct drm_plane_funcs driver_plane_funcs = {
        ...,
        DRM_GEM_SHADOW_PLANE_FUNCS,
};

struct drm_plane_helper_funcs driver_plane_helper_funcs = {
        ...,
        DRM_GEM_SHADOW_PLANE_HELPER_FUNCS,
};
```

In the driver’s atomic-update function, shadow-buffer mappings are available
from the plane state. Use [`to_drm_shadow_plane_state()`](#c.to_drm_shadow_plane_state "to_drm_shadow_plane_state") to upcast from
[`struct drm_plane_state`](drm-kms.html#c.drm_plane_state "drm_plane_state").

```
void driver_plane_atomic_update(struct drm_plane *plane,
                                struct drm_plane_state *old_plane_state)
{
        struct drm_plane_state *plane_state = plane->state;
        struct drm_shadow_plane_state *shadow_plane_state =
                to_drm_shadow_plane_state(plane_state);

        // access shadow buffer via shadow_plane_state->map
}
```

A mapping address for each of the framebuffer’s buffer object is stored in
struct [`drm_shadow_plane_state.map`](#c.drm_shadow_plane_state "drm_shadow_plane_state"). The mappings are valid while the state
is being used.

DRM\_SHADOW\_PLANE\_MAX\_WIDTH

`DRM_SHADOW_PLANE_MAX_WIDTH`

> > Maximum width of a plane’s shadow buffer in pixels
>
> **Description**
>
> For drivers with shadow planes, the maximum width of the framebuffer is
> usually independent from hardware limitations. Drivers can initialize [`struct
> drm_mode_config`](drm-kms.html#c.drm_mode_config "drm_mode_config").max\_width from DRM\_SHADOW\_PLANE\_MAX\_WIDTH.

DRM\_SHADOW\_PLANE\_MAX\_HEIGHT

`DRM_SHADOW_PLANE_MAX_HEIGHT`

> > Maximum height of a plane’s shadow buffer in scanlines
>
> **Description**
>
> For drivers with shadow planes, the maximum height of the framebuffer is
> usually independent from hardware limitations. Drivers can initialize [`struct
> drm_mode_config`](drm-kms.html#c.drm_mode_config "drm_mode_config").max\_height from DRM\_SHADOW\_PLANE\_MAX\_HEIGHT.

struct drm\_shadow\_plane\_state
:   plane state for planes with shadow buffers

**Definition**:

```
struct drm_shadow_plane_state {
    struct drm_plane_state base;
    struct drm_format_conv_state fmtcnv_state;
    struct iosys_map map[DRM_FORMAT_MAX_PLANES];
    struct iosys_map data[DRM_FORMAT_MAX_PLANES];
};
```

**Members**

`base`
:   plane state

`fmtcnv_state`
:   Format-conversion state

    Per-plane state for format conversion.
    Flags for copying shadow buffers into backend storage. Also holds
    temporary storage for format conversion.

`map`
:   Mappings of the plane’s framebuffer BOs in to kernel address space

    The memory mappings stored in map should be established in the plane’s
    prepare\_fb callback and removed in the cleanup\_fb callback.

`data`
:   Address of each framebuffer BO’s data

    The address of the data stored in each mapping. This is different
    for framebuffers with non-zero offset fields.

**Description**

For planes that use a shadow buffer, [`struct drm_shadow_plane_state`](#c.drm_shadow_plane_state "drm_shadow_plane_state")
provides the regular plane state plus mappings of the shadow buffer
into kernel address space.

struct [drm\_shadow\_plane\_state](#c.drm_shadow_plane_state "drm_shadow_plane_state") \*to\_drm\_shadow\_plane\_state(struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*state)
:   upcasts from [`struct drm_plane_state`](drm-kms.html#c.drm_plane_state "drm_plane_state")

**Parameters**

`struct drm_plane_state *state`
:   the plane state

DRM\_GEM\_SHADOW\_PLANE\_FUNCS

`DRM_GEM_SHADOW_PLANE_FUNCS`

> > Initializes [`struct drm_plane_funcs`](drm-kms.html#c.drm_plane_funcs "drm_plane_funcs") for shadow-buffered planes
>
> **Description**
>
> Drivers may use GEM BOs as shadow buffers over the framebuffer memory. This
> macro initializes [`struct drm_plane_funcs`](drm-kms.html#c.drm_plane_funcs "drm_plane_funcs") to use the rsp helper functions.

DRM\_GEM\_SHADOW\_PLANE\_HELPER\_FUNCS

`DRM_GEM_SHADOW_PLANE_HELPER_FUNCS`

> > Initializes [`struct drm_plane_helper_funcs`](#c.drm_plane_helper_funcs "drm_plane_helper_funcs") for shadow-buffered planes
>
> **Description**
>
> Drivers may use GEM BOs as shadow buffers over the framebuffer memory. This
> macro initializes [`struct drm_plane_helper_funcs`](#c.drm_plane_helper_funcs "drm_plane_helper_funcs") to use the rsp helper
> functions.

DRM\_GEM\_SIMPLE\_DISPLAY\_PIPE\_SHADOW\_PLANE\_FUNCS

`DRM_GEM_SIMPLE_DISPLAY_PIPE_SHADOW_PLANE_FUNCS`

> > Initializes `struct drm_simple_display_pipe_funcs` for shadow-buffered planes
>
> **Description**
>
> Drivers may use GEM BOs as shadow buffers over the framebuffer memory. This
> macro initializes `struct drm_simple_display_pipe_funcs` to use the rsp helper
> functions.

int drm\_gem\_plane\_helper\_prepare\_fb(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*state)
:   Prepare a GEM backed framebuffer

**Parameters**

`struct drm_plane *plane`
:   Plane

`struct drm_plane_state *state`
:   Plane state the fence will be attached to

**Description**

This function extracts the exclusive fence from [`drm_gem_object.resv`](drm-mm.html#c.drm_gem_object "drm_gem_object") and
attaches it to plane state for the atomic helper to wait on. This is
necessary to correctly implement implicit synchronization for any buffers
shared as a struct [`dma_buf`](../driver-api/dma-buf.html#c.dma_buf "dma_buf"). This function can be used as the
[`drm_plane_helper_funcs.prepare_fb`](#c.drm_plane_helper_funcs "drm_plane_helper_funcs") callback.

There is no need for [`drm_plane_helper_funcs.cleanup_fb`](#c.drm_plane_helper_funcs "drm_plane_helper_funcs") hook for simple
GEM based framebuffer drivers which have their buffers always pinned in
memory.

This function is the default implementation for GEM drivers of
[`drm_plane_helper_funcs.prepare_fb`](#c.drm_plane_helper_funcs "drm_plane_helper_funcs") if no callback is provided.

void \_\_drm\_gem\_duplicate\_shadow\_plane\_state(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_shadow\_plane\_state](#c.drm_shadow_plane_state "drm_shadow_plane_state") \*new\_shadow\_plane\_state)
:   duplicates shadow-buffered plane state

**Parameters**

`struct drm_plane *plane`
:   the plane

`struct drm_shadow_plane_state *new_shadow_plane_state`
:   the new shadow-buffered plane state

**Description**

This function duplicates shadow-buffered plane state. This is helpful for drivers
that subclass [`struct drm_shadow_plane_state`](#c.drm_shadow_plane_state "drm_shadow_plane_state").

The function does not duplicate existing mappings of the shadow buffers.
Mappings are maintained during the atomic commit by the plane’s prepare\_fb
and cleanup\_fb helpers. See `drm_gem_prepare_shadow_fb()` and `drm_gem_cleanup_shadow_fb()`
for corresponding helpers.

struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*drm\_gem\_duplicate\_shadow\_plane\_state(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane)
:   duplicates shadow-buffered plane state

**Parameters**

`struct drm_plane *plane`
:   the plane

**Description**

This function implements struct [`drm_plane_funcs.atomic_duplicate_state`](drm-kms.html#c.drm_plane_funcs "drm_plane_funcs") for
shadow-buffered planes. It assumes the existing state to be of type
[`struct drm_shadow_plane_state`](#c.drm_shadow_plane_state "drm_shadow_plane_state") and it allocates the new state to be of this
type.

The function does not duplicate existing mappings of the shadow buffers.
Mappings are maintained during the atomic commit by the plane’s prepare\_fb
and cleanup\_fb helpers. See `drm_gem_prepare_shadow_fb()` and `drm_gem_cleanup_shadow_fb()`
for corresponding helpers.

**Return**

A pointer to a new plane state on success, or NULL otherwise.

void \_\_drm\_gem\_destroy\_shadow\_plane\_state(struct [drm\_shadow\_plane\_state](#c.drm_shadow_plane_state "drm_shadow_plane_state") \*shadow\_plane\_state)
:   cleans up shadow-buffered plane state

**Parameters**

`struct drm_shadow_plane_state *shadow_plane_state`
:   the shadow-buffered plane state

**Description**

This function cleans up shadow-buffered plane state. Helpful for drivers that
subclass [`struct drm_shadow_plane_state`](#c.drm_shadow_plane_state "drm_shadow_plane_state").

void drm\_gem\_destroy\_shadow\_plane\_state(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*plane\_state)
:   deletes shadow-buffered plane state

**Parameters**

`struct drm_plane *plane`
:   the plane

`struct drm_plane_state *plane_state`
:   the plane state of type [`struct drm_shadow_plane_state`](#c.drm_shadow_plane_state "drm_shadow_plane_state")

**Description**

This function implements struct [`drm_plane_funcs.atomic_destroy_state`](drm-kms.html#c.drm_plane_funcs "drm_plane_funcs")
for shadow-buffered planes. It expects that mappings of shadow buffers
have been released already.

void \_\_drm\_gem\_reset\_shadow\_plane(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_shadow\_plane\_state](#c.drm_shadow_plane_state "drm_shadow_plane_state") \*shadow\_plane\_state)
:   resets a shadow-buffered plane

**Parameters**

`struct drm_plane *plane`
:   the plane

`struct drm_shadow_plane_state *shadow_plane_state`
:   the shadow-buffered plane state

**Description**

This function resets state for shadow-buffered planes. Helpful
for drivers that subclass [`struct drm_shadow_plane_state`](#c.drm_shadow_plane_state "drm_shadow_plane_state").

void drm\_gem\_reset\_shadow\_plane(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane)
:   resets a shadow-buffered plane

**Parameters**

`struct drm_plane *plane`
:   the plane

**Description**

This function implements struct [`drm_plane_funcs.reset_plane`](drm-kms.html#c.drm_plane_funcs "drm_plane_funcs") for
shadow-buffered planes. It assumes the current plane state to be
of type `struct drm_shadow_plane` and it allocates the new state of
this type.

int drm\_gem\_begin\_shadow\_fb\_access(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*plane\_state)
:   prepares shadow framebuffers for CPU access

**Parameters**

`struct drm_plane *plane`
:   the plane

`struct drm_plane_state *plane_state`
:   the plane state of type [`struct drm_shadow_plane_state`](#c.drm_shadow_plane_state "drm_shadow_plane_state")

**Description**

This function implements struct [`drm_plane_helper_funcs.begin_fb_access`](#c.drm_plane_helper_funcs "drm_plane_helper_funcs"). It
maps all buffer objects of the plane’s framebuffer into kernel address
space and stores them in struct [`drm_shadow_plane_state.map`](#c.drm_shadow_plane_state "drm_shadow_plane_state"). The first data
bytes are available in struct [`drm_shadow_plane_state.data`](#c.drm_shadow_plane_state "drm_shadow_plane_state").

See [`drm_gem_end_shadow_fb_access()`](#c.drm_gem_end_shadow_fb_access "drm_gem_end_shadow_fb_access") for cleanup.

**Return**

0 on success, or a negative errno code otherwise.

void drm\_gem\_end\_shadow\_fb\_access(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*plane\_state)
:   releases shadow framebuffers from CPU access

**Parameters**

`struct drm_plane *plane`
:   the plane

`struct drm_plane_state *plane_state`
:   the plane state of type [`struct drm_shadow_plane_state`](#c.drm_shadow_plane_state "drm_shadow_plane_state")

**Description**

This function implements struct [`drm_plane_helper_funcs.end_fb_access`](#c.drm_plane_helper_funcs "drm_plane_helper_funcs"). It
undoes all effects of [`drm_gem_begin_shadow_fb_access()`](#c.drm_gem_begin_shadow_fb_access "drm_gem_begin_shadow_fb_access") in reverse order.

See [`drm_gem_begin_shadow_fb_access()`](#c.drm_gem_begin_shadow_fb_access "drm_gem_begin_shadow_fb_access") for more information.

int drm\_gem\_simple\_kms\_begin\_shadow\_fb\_access(struct drm\_simple\_display\_pipe \*pipe, struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*plane\_state)
:   prepares shadow framebuffers for CPU access

**Parameters**

`struct drm_simple_display_pipe *pipe`
:   the simple display pipe

`struct drm_plane_state *plane_state`
:   the plane state of type [`struct drm_shadow_plane_state`](#c.drm_shadow_plane_state "drm_shadow_plane_state")

**Description**

This function implements `struct drm_simple_display_funcs`.begin\_fb\_access.

See [`drm_gem_begin_shadow_fb_access()`](#c.drm_gem_begin_shadow_fb_access "drm_gem_begin_shadow_fb_access") for details and
`drm_gem_simple_kms_cleanup_shadow_fb()` for cleanup.

**Return**

0 on success, or a negative errno code otherwise.

void drm\_gem\_simple\_kms\_end\_shadow\_fb\_access(struct drm\_simple\_display\_pipe \*pipe, struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*plane\_state)
:   releases shadow framebuffers from CPU access

**Parameters**

`struct drm_simple_display_pipe *pipe`
:   the simple display pipe

`struct drm_plane_state *plane_state`
:   the plane state of type [`struct drm_shadow_plane_state`](#c.drm_shadow_plane_state "drm_shadow_plane_state")

**Description**

This function implements `struct drm_simple_display_funcs`.end\_fb\_access.
It undoes all effects of [`drm_gem_simple_kms_begin_shadow_fb_access()`](#c.drm_gem_simple_kms_begin_shadow_fb_access "drm_gem_simple_kms_begin_shadow_fb_access") in
reverse order.

See [`drm_gem_simple_kms_begin_shadow_fb_access()`](#c.drm_gem_simple_kms_begin_shadow_fb_access "drm_gem_simple_kms_begin_shadow_fb_access").

void drm\_gem\_simple\_kms\_reset\_shadow\_plane(struct drm\_simple\_display\_pipe \*pipe)
:   resets a shadow-buffered plane

**Parameters**

`struct drm_simple_display_pipe *pipe`
:   the simple display pipe

**Description**

This function implements `struct drm_simple_display_funcs`.reset\_plane
for shadow-buffered planes.

struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*drm\_gem\_simple\_kms\_duplicate\_shadow\_plane\_state(struct drm\_simple\_display\_pipe \*pipe)
:   duplicates shadow-buffered plane state

**Parameters**

`struct drm_simple_display_pipe *pipe`
:   the simple display pipe

**Description**

This function implements `struct drm_simple_display_funcs`.duplicate\_plane\_state
for shadow-buffered planes. It does not duplicate existing mappings of the shadow
buffers. Mappings are maintained during the atomic commit by the plane’s prepare\_fb
and cleanup\_fb helpers.

**Return**

A pointer to a new plane state on success, or NULL otherwise.

void drm\_gem\_simple\_kms\_destroy\_shadow\_plane\_state(struct drm\_simple\_display\_pipe \*pipe, struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*plane\_state)
:   resets shadow-buffered plane state

**Parameters**

`struct drm_simple_display_pipe *pipe`
:   the simple display pipe

`struct drm_plane_state *plane_state`
:   the plane state of type [`struct drm_shadow_plane_state`](#c.drm_shadow_plane_state "drm_shadow_plane_state")

**Description**

This function implements `struct drm_simple_display_funcs`.destroy\_plane\_state
for shadow-buffered planes. It expects that mappings of shadow buffers
have been released already.

### VBLANK Helper Reference

The vblank helper library provides functions for supporting vertical
blanking in DRM drivers.

For vblank timers, several callback implementations are available.
Drivers enable support for vblank timers by setting the vblank callbacks
in struct [`drm_crtc_funcs`](drm-kms.html#c.drm_crtc_funcs "drm_crtc_funcs") to the helpers provided by this library. The
initializer macro DRM\_CRTC\_VBLANK\_TIMER\_FUNCS does this conveniently.
The driver further has to send the VBLANK event from its atomic\_flush
callback and control vblank from the CRTC’s atomic\_enable and atomic\_disable
callbacks. The callbacks are located in struct [`drm_crtc_helper_funcs`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs").
The vblank helper library provides implementations of these callbacks
for drivers without further requirements. The initializer macro
DRM\_CRTC\_HELPER\_VBLANK\_FUNCS sets them coveniently.

Once the driver enables vblank support with [`drm_vblank_init()`](drm-kms.html#c.drm_vblank_init "drm_vblank_init"), each
CRTC’s vblank timer fires according to the programmed display mode. By
default, the vblank timer invokes [`drm_crtc_handle_vblank()`](drm-kms.html#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank"). Drivers with
more specific requirements can set their own handler function in
struct [`drm_crtc_helper_funcs.handle_vblank_timeout`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs").

DRM\_CRTC\_HELPER\_VBLANK\_FUNCS

`DRM_CRTC_HELPER_VBLANK_FUNCS`

> > Default implementation for VBLANK helpers
>
> **Description**
>
> This macro initializes struct [`drm_crtc_helper_funcs`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") to default helpers
> for VBLANK handling.

DRM\_CRTC\_VBLANK\_TIMER\_FUNCS

`DRM_CRTC_VBLANK_TIMER_FUNCS`

> > Default implementation for VBLANK timers
>
> **Description**
>
> This macro initializes struct [`drm_crtc_funcs`](drm-kms.html#c.drm_crtc_funcs "drm_crtc_funcs") to default helpers for
> VBLANK timers.

void drm\_crtc\_vblank\_atomic\_flush(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   Implements struct [`drm_crtc_helper_funcs.atomic_flush`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs")

**Parameters**

`struct drm_crtc *crtc`
:   The CRTC

`struct drm_atomic_state *state`
:   The atomic state to apply

**Description**

The helper [`drm_crtc_vblank_atomic_flush()`](#c.drm_crtc_vblank_atomic_flush "drm_crtc_vblank_atomic_flush") implements atomic\_flush of
[`struct drm_crtc_helper_funcs`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") for CRTCs that only need to send out a
VBLANK event.

See also struct [`drm_crtc_helper_funcs.atomic_flush`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs").

void drm\_crtc\_vblank\_atomic\_enable(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   Implements struct [`drm_crtc_helper_funcs.atomic_enable`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs")

**Parameters**

`struct drm_crtc *crtc`
:   The CRTC

`struct drm_atomic_state *state`
:   The atomic state

**Description**

The helper [`drm_crtc_vblank_atomic_enable()`](#c.drm_crtc_vblank_atomic_enable "drm_crtc_vblank_atomic_enable") implements atomic\_enable
of [`struct drm_crtc_helper_funcs`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") for CRTCs the only need to enable VBLANKs.

See also struct [`drm_crtc_helper_funcs.atomic_enable`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs").

void drm\_crtc\_vblank\_atomic\_disable(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   Implements struct [`drm_crtc_helper_funcs.atomic_disable`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs")

**Parameters**

`struct drm_crtc *crtc`
:   The CRTC

`struct drm_atomic_state *state`
:   The atomic state

**Description**

The helper [`drm_crtc_vblank_atomic_disable()`](#c.drm_crtc_vblank_atomic_disable "drm_crtc_vblank_atomic_disable") implements atomic\_disable
of [`struct drm_crtc_helper_funcs`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") for CRTCs the only need to disable VBLANKs.

See also struct [`drm_crtc_funcs.atomic_disable`](drm-kms.html#c.drm_crtc_funcs "drm_crtc_funcs").

int drm\_crtc\_vblank\_helper\_enable\_vblank\_timer(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc)
:   Implements struct [`drm_crtc_funcs.enable_vblank`](drm-kms.html#c.drm_crtc_funcs "drm_crtc_funcs")

**Parameters**

`struct drm_crtc *crtc`
:   The CRTC

**Description**

The helper [`drm_crtc_vblank_helper_enable_vblank_timer()`](#c.drm_crtc_vblank_helper_enable_vblank_timer "drm_crtc_vblank_helper_enable_vblank_timer") implements
enable\_vblank of [`struct drm_crtc_helper_funcs`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") for CRTCs that require
a VBLANK timer. It sets up the timer on the first invocation. The
started timer expires after the current frame duration. See struct
[`drm_vblank_crtc.framedur_ns`](drm-kms.html#c.drm_vblank_crtc "drm_vblank_crtc").

See also struct [`drm_crtc_helper_funcs.enable_vblank`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs").

**Return**

0 on success, or a negative errno code otherwise.

void drm\_crtc\_vblank\_helper\_disable\_vblank\_timer(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc)
:   Implements struct [`drm_crtc_funcs.disable_vblank`](drm-kms.html#c.drm_crtc_funcs "drm_crtc_funcs")

**Parameters**

`struct drm_crtc *crtc`
:   The CRTC

**Description**

The helper [`drm_crtc_vblank_helper_disable_vblank_timer()`](#c.drm_crtc_vblank_helper_disable_vblank_timer "drm_crtc_vblank_helper_disable_vblank_timer") implements
disable\_vblank of [`struct drm_crtc_funcs`](drm-kms.html#c.drm_crtc_funcs "drm_crtc_funcs") for CRTCs that require a
VBLANK timer.

See also struct [`drm_crtc_helper_funcs.disable_vblank`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs").

bool drm\_crtc\_vblank\_helper\_get\_vblank\_timestamp\_from\_timer(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, int \*max\_error, ktime\_t \*vblank\_time, bool in\_vblank\_irq)
:   Implements struct [`drm_crtc_funcs.get_vblank_timestamp`](drm-kms.html#c.drm_crtc_funcs "drm_crtc_funcs")

**Parameters**

`struct drm_crtc *crtc`
:   The CRTC

`int *max_error`
:   Maximum acceptable error

`ktime_t *vblank_time`
:   Returns the next vblank timestamp

`bool in_vblank_irq`
:   True is called from [`drm_crtc_handle_vblank()`](drm-kms.html#c.drm_crtc_handle_vblank "drm_crtc_handle_vblank")

**Description**

The helper `drm_crtc_helper_get_vblank_timestamp_from_timer()` implements
get\_vblank\_timestamp of [`struct drm_crtc_funcs`](drm-kms.html#c.drm_crtc_funcs "drm_crtc_funcs") for CRTCs that require a
VBLANK timer. It returns the timestamp according to the timer’s expiry
time.

See also struct [`drm_crtc_funcs.get_vblank_timestamp`](drm-kms.html#c.drm_crtc_funcs "drm_crtc_funcs").

**Return**

True on success, or false otherwise.

## fbdev Helper Functions Reference

The fb helper functions are useful to provide an fbdev on top of a drm kernel
mode setting driver. They can be used mostly independently from the crtc
helper functions used by many drivers to implement the kernel mode setting
interfaces. Drivers that use one of the shared memory managers, TTM, SHMEM,
DMA, should instead use the corresponding fbdev emulation.

For suspend/resume consider using [`drm_mode_config_helper_suspend()`](#c.drm_mode_config_helper_suspend "drm_mode_config_helper_suspend") and
[`drm_mode_config_helper_resume()`](#c.drm_mode_config_helper_resume "drm_mode_config_helper_resume") which takes care of fbdev as well.

All other functions exported by the fb helper library can be used to
implement the fbdev driver interface by the driver.

It is possible, though perhaps somewhat tricky, to implement race-free
hotplug detection using the fbdev helpers. The [`drm_fb_helper_prepare()`](#c.drm_fb_helper_prepare "drm_fb_helper_prepare")
helper must be called first to initialize the minimum required to make
hotplug detection work. Drivers also need to make sure to properly set up
the [`drm_mode_config.funcs`](drm-kms.html#c.drm_mode_config "drm_mode_config") member. After calling [`drm_kms_helper_poll_init()`](#c.drm_kms_helper_poll_init "drm_kms_helper_poll_init")
it is safe to enable interrupts and start processing hotplug events. At the
same time, drivers should initialize all modeset objects such as CRTCs,
encoders and connectors. To finish up the fbdev helper initialization, the
[`drm_fb_helper_init()`](#c.drm_fb_helper_init "drm_fb_helper_init") function is called. To probe for all attached displays
and set up an initial configuration using the detected hardware, drivers
should call [`drm_fb_helper_initial_config()`](#c.drm_fb_helper_initial_config "drm_fb_helper_initial_config").

If [`drm_framebuffer_funcs.dirty`](drm-kms.html#c.drm_framebuffer_funcs "drm_framebuffer_funcs") is set, the
drm\_fb\_helper\_{cfb,sys}\_{write,fillrect,copyarea,imageblit} functions will
accumulate changes and schedule [`drm_fb_helper.dirty_work`](#c.drm_fb_helper "drm_fb_helper") to run right
away. This worker then calls the `dirty()` function ensuring that it will
always run in process context since the fb\_\*() function could be running in
atomic context. If [`drm_fb_helper_deferred_io()`](#c.drm_fb_helper_deferred_io "drm_fb_helper_deferred_io") is used as the deferred\_io
callback it will also schedule dirty\_work with the damage collected from the
mmap page writes.

struct drm\_fb\_helper\_surface\_size
:   describes fbdev size and scanout surface size

**Definition**:

```
struct drm_fb_helper_surface_size {
    u32 fb_width;
    u32 fb_height;
    u32 surface_width;
    u32 surface_height;
    u32 surface_bpp;
    u32 surface_depth;
};
```

**Members**

`fb_width`
:   fbdev width

`fb_height`
:   fbdev height

`surface_width`
:   scanout buffer width

`surface_height`
:   scanout buffer height

`surface_bpp`
:   scanout buffer bpp

`surface_depth`
:   scanout buffer depth

**Description**

Note that the scanout surface width/height may be larger than the fbdev
width/height. In case of multiple displays, the scanout surface is sized
according to the largest width/height (so it is large enough for all CRTCs
to scanout). But the fbdev width/height is sized to the minimum width/
height of all the displays. This ensures that fbcon fits on the smallest
of the attached displays. fb\_width/fb\_height is used by
[`drm_fb_helper_fill_info()`](#c.drm_fb_helper_fill_info "drm_fb_helper_fill_info") to fill out the `fb_info.var` structure.

struct drm\_fb\_helper\_funcs
:   driver callbacks for the fbdev emulation library

**Definition**:

```
struct drm_fb_helper_funcs {
    int (*fb_dirty)(struct drm_fb_helper *helper, struct drm_clip_rect *clip);
    void (*fb_restore)(struct drm_fb_helper *helper);
    void (*fb_set_suspend)(struct drm_fb_helper *helper, bool suspend);
};
```

**Members**

`fb_dirty`
:   Driver callback to update the framebuffer memory. If set, fbdev
    emulation will invoke this callback in regular intervals after
    the framebuffer has been written.

    This callback is optional.

    Returns:
    0 on success, or an error code otherwise.

`fb_restore`
:   Driver callback to restore internal fbdev state. If set, fbdev
    emulation will invoke this callback after restoring the display
    mode.

    Only for i915. Do not use in new code.

    TODO: Fix i915 to not require this callback.

`fb_set_suspend`
:   Driver callback to suspend or resume, if set, fbdev emulation will
    invoke this callback during suspend and resume. Driver should call
    [`fb_set_suspend()`](../driver-api/frame-buffer.html#c.fb_set_suspend "fb_set_suspend") from their implementation. If not set, fbdev
    emulation will invoke [`fb_set_suspend()`](../driver-api/frame-buffer.html#c.fb_set_suspend "fb_set_suspend") directly.

    Only for i915. Do not use in new code.

    TODO: Fix i915 to not require this callback.

**Description**

Driver callbacks used by the fbdev emulation helper library.

struct drm\_fb\_helper
:   main structure to emulate fbdev on top of KMS

**Definition**:

```
struct drm_fb_helper {
    struct drm_client_dev client;
    struct drm_client_buffer *buffer;
    struct drm_framebuffer *fb;
    struct drm_device *dev;
    const struct drm_fb_helper_funcs *funcs;
    struct fb_info *info;
    u32 pseudo_palette[17];
    struct drm_clip_rect damage_clip;
    spinlock_t damage_lock;
    struct work_struct damage_work;
    struct work_struct resume_work;
    struct mutex lock;
    bool delayed_hotplug;
    bool deferred_setup;
    int preferred_bpp;
#ifdef CONFIG_FB_DEFERRED_IO;
    struct fb_deferred_io fbdefio;
#endif;
};
```

**Members**

`client`
:   DRM client used by the generic fbdev emulation.

`buffer`
:   Framebuffer used by the generic fbdev emulation.

`fb`
:   Scanout framebuffer object

`dev`
:   DRM device

`funcs`
:   driver callbacks for fb helper

`info`
:   emulated fbdev device info struct

`pseudo_palette`
:   fake palette of 16 colors

`damage_clip`
:   clip rectangle used with deferred\_io to accumulate damage to
    the screen buffer

`damage_lock`
:   spinlock protecting **damage\_clip**

`damage_work`
:   worker used to flush the framebuffer

`resume_work`
:   worker used during resume if the console lock is already taken

`lock`
:   Top-level FBDEV helper lock. This protects all internal data
    structures and lists, such as **connector\_info** and **crtc\_info**.

    FIXME: fbdev emulation locking is a mess and long term we want to
    protect all helper internal state with this lock as well as reduce
    core KMS locking as much as possible.

`delayed_hotplug`
:   A hotplug was received while fbdev wasn’t in control of the DRM
    device, i.e. another KMS master was active. The output configuration
    needs to be reprobe when fbdev is in control again.

`deferred_setup`
:   If no outputs are connected (disconnected or unknown) the FB helper
    code will defer setup until at least one of the outputs shows up.
    This field keeps track of the status so that setup can be retried
    at every hotplug event until it succeeds eventually.

    Protected by **lock**.

`preferred_bpp`
:   Temporary storage for the driver’s preferred BPP setting passed to
    FB helper initialization. This needs to be tracked so that deferred
    FB helper setup can pass this on.

    See also: **deferred\_setup**

`fbdefio`
:   Temporary storage for the driver’s FB deferred I/O handler. If the
    driver uses the DRM fbdev emulation layer, this is set by the core
    to a generic deferred I/O handler if a driver is preferring to use
    a shadow buffer.

**Description**

This is the main structure used by the fbdev helpers. Drivers supporting
fbdev emulation should embedded this into their overall driver structure.
Drivers must also fill out a [`struct drm_fb_helper_funcs`](#c.drm_fb_helper_funcs "drm_fb_helper_funcs") with a few
operations.

DRM\_FB\_HELPER\_DEFAULT\_OPS

`DRM_FB_HELPER_DEFAULT_OPS`

> > helper define for drm drivers
>
> **Description**
>
> Helper define to register default implementations of drm\_fb\_helper
> functions. To be used in `struct fb_ops` of drm drivers.

int drm\_fb\_helper\_restore\_fbdev\_mode\_unlocked(struct [drm\_fb\_helper](#c.drm_fb_helper "drm_fb_helper") \*fb\_helper, bool force)
:   restore fbdev configuration

**Parameters**

`struct drm_fb_helper *fb_helper`
:   driver-allocated fbdev helper, can be NULL

`bool force`
:   ignore present DRM master

**Description**

This helper should be called from fbdev emulation’s [`drm_client_funcs.restore`](drm-client.html#c.drm_client_funcs "drm_client_funcs")
callback. It ensures that the user isn’t greeted with a black screen when the
userspace compositor releases the display device.

**Return**

0 on success, or a negative errno code otherwise.

int drm\_fb\_helper\_blank(int blank, struct fb\_info \*info)
:   implementation for `fb_ops.fb_blank`

**Parameters**

`int blank`
:   desired blanking state

`struct fb_info *info`
:   fbdev registered by the helper

void drm\_fb\_helper\_prepare(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_fb\_helper](#c.drm_fb_helper "drm_fb_helper") \*helper, unsigned int preferred\_bpp, const struct [drm\_fb\_helper\_funcs](#c.drm_fb_helper_funcs "drm_fb_helper_funcs") \*funcs)
:   setup a drm\_fb\_helper structure

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_fb_helper *helper`
:   driver-allocated fbdev helper structure to set up

`unsigned int preferred_bpp`
:   Preferred bits per pixel for the device.

`const struct drm_fb_helper_funcs *funcs`
:   pointer to structure of functions associate with this helper

**Description**

Sets up the bare minimum to make the framebuffer helper usable. This is
useful to implement race-free initialization of the polling helpers.

void drm\_fb\_helper\_unprepare(struct [drm\_fb\_helper](#c.drm_fb_helper "drm_fb_helper") \*fb\_helper)
:   clean up a drm\_fb\_helper structure

**Parameters**

`struct drm_fb_helper *fb_helper`
:   driver-allocated fbdev helper structure to set up

**Description**

Cleans up the framebuffer helper. Inverse of [`drm_fb_helper_prepare()`](#c.drm_fb_helper_prepare "drm_fb_helper_prepare").

int drm\_fb\_helper\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_fb\_helper](#c.drm_fb_helper "drm_fb_helper") \*fb\_helper)
:   initialize a [`struct drm_fb_helper`](#c.drm_fb_helper "drm_fb_helper")

**Parameters**

`struct drm_device *dev`
:   drm device

`struct drm_fb_helper *fb_helper`
:   driver-allocated fbdev helper structure to initialize

**Description**

This allocates the structures for the fbdev helper with the given limits.
Note that this won’t yet touch the hardware (through the driver interfaces)
nor register the fbdev. This is only done in [`drm_fb_helper_initial_config()`](#c.drm_fb_helper_initial_config "drm_fb_helper_initial_config")
to allow driver writes more control over the exact init sequence.

Drivers must call [`drm_fb_helper_prepare()`](#c.drm_fb_helper_prepare "drm_fb_helper_prepare") before calling this function.

**Return**

Zero if everything went ok, nonzero otherwise.

void drm\_fb\_helper\_unregister\_info(struct [drm\_fb\_helper](#c.drm_fb_helper "drm_fb_helper") \*fb\_helper)
:   unregister fb\_info framebuffer device

**Parameters**

`struct drm_fb_helper *fb_helper`
:   driver-allocated fbdev helper, must not be NULL

**Description**

A wrapper around unregister\_framebuffer, to release the fb\_info
framebuffer device. This must be called before releasing all resources for
**fb\_helper** by calling [`drm_fb_helper_fini()`](#c.drm_fb_helper_fini "drm_fb_helper_fini").

void drm\_fb\_helper\_fini(struct [drm\_fb\_helper](#c.drm_fb_helper "drm_fb_helper") \*fb\_helper)
:   finialize a [`struct drm_fb_helper`](#c.drm_fb_helper "drm_fb_helper")

**Parameters**

`struct drm_fb_helper *fb_helper`
:   driver-allocated fbdev helper, can be NULL

**Description**

This cleans up all remaining resources associated with **fb\_helper**.

void drm\_fb\_helper\_deferred\_io(struct fb\_info \*info, struct list\_head \*pagereflist)
:   fbdev deferred\_io callback function

**Parameters**

`struct fb_info *info`
:   fb\_info `struct pointer`

`struct list_head *pagereflist`
:   list of mmap framebuffer pages that have to be flushed

**Description**

This function is used as the `fb_deferred_io.deferred_io`
callback function for flushing the fbdev mmap writes.

void drm\_fb\_helper\_set\_suspend(struct [drm\_fb\_helper](#c.drm_fb_helper "drm_fb_helper") \*fb\_helper, bool suspend)
:   wrapper around fb\_set\_suspend

**Parameters**

`struct drm_fb_helper *fb_helper`
:   driver-allocated fbdev helper, can be NULL

`bool suspend`
:   whether to suspend or resume

**Description**

A wrapper around fb\_set\_suspend implemented by fbdev core.
Use [`drm_fb_helper_set_suspend_unlocked()`](#c.drm_fb_helper_set_suspend_unlocked "drm_fb_helper_set_suspend_unlocked") if you don’t need to take
the lock yourself

void drm\_fb\_helper\_set\_suspend\_unlocked(struct [drm\_fb\_helper](#c.drm_fb_helper "drm_fb_helper") \*fb\_helper, bool suspend)
:   wrapper around fb\_set\_suspend that also takes the console lock

**Parameters**

`struct drm_fb_helper *fb_helper`
:   driver-allocated fbdev helper, can be NULL

`bool suspend`
:   whether to suspend or resume

**Description**

A wrapper around [`fb_set_suspend()`](../driver-api/frame-buffer.html#c.fb_set_suspend "fb_set_suspend") that takes the console lock. If the lock
isn’t available on resume, a worker is tasked with waiting for the lock
to become available. The console lock can be pretty contented on resume
due to all the printk activity.

This function can be called multiple times with the same state since
`fb_info.state` is checked to see if fbdev is running or not before locking.

Use [`drm_fb_helper_set_suspend()`](#c.drm_fb_helper_set_suspend "drm_fb_helper_set_suspend") if you need to take the lock yourself.

int drm\_fb\_helper\_setcmap(struct fb\_cmap \*cmap, struct fb\_info \*info)
:   implementation for `fb_ops.fb_setcmap`

**Parameters**

`struct fb_cmap *cmap`
:   cmap to set

`struct fb_info *info`
:   fbdev registered by the helper

int drm\_fb\_helper\_ioctl(struct fb\_info \*info, unsigned int cmd, unsigned long arg)
:   legacy ioctl implementation

**Parameters**

`struct fb_info *info`
:   fbdev registered by the helper

`unsigned int cmd`
:   ioctl command

`unsigned long arg`
:   ioctl argument

**Description**

A helper to implement the standard fbdev ioctl. Only
FBIO\_WAITFORVSYNC is implemented for now.

int drm\_fb\_helper\_check\_var(struct fb\_var\_screeninfo \*var, struct fb\_info \*info)
:   implementation for `fb_ops.fb_check_var`

**Parameters**

`struct fb_var_screeninfo *var`
:   screeninfo to check

`struct fb_info *info`
:   fbdev registered by the helper

int drm\_fb\_helper\_set\_par(struct fb\_info \*info)
:   implementation for `fb_ops.fb_set_par`

**Parameters**

`struct fb_info *info`
:   fbdev registered by the helper

**Description**

This will let fbcon do the mode init and is called at initialization time by
the fbdev core when registering the driver, and later on through the hotplug
callback.

int drm\_fb\_helper\_pan\_display(struct fb\_var\_screeninfo \*var, struct fb\_info \*info)
:   implementation for `fb_ops.fb_pan_display`

**Parameters**

`struct fb_var_screeninfo *var`
:   updated screen information

`struct fb_info *info`
:   fbdev registered by the helper

void drm\_fb\_helper\_fill\_info(struct fb\_info \*info, struct [drm\_fb\_helper](#c.drm_fb_helper "drm_fb_helper") \*fb\_helper, struct [drm\_fb\_helper\_surface\_size](#c.drm_fb_helper_surface_size "drm_fb_helper_surface_size") \*sizes)
:   initializes fbdev information

**Parameters**

`struct fb_info *info`
:   fbdev instance to set up

`struct drm_fb_helper *fb_helper`
:   fb helper instance to use as template

`struct drm_fb_helper_surface_size *sizes`
:   describes fbdev size and scanout surface size

**Description**

Sets up the variable and fixed fbdev metainformation from the given fb helper
instance and the drm framebuffer allocated in [`drm_fb_helper.fb`](#c.drm_fb_helper "drm_fb_helper").

Drivers should call this (or their equivalent setup code) from their
[`drm_driver.fbdev_probe`](drm-internals.html#c.drm_driver "drm_driver") callback after having allocated the fbdev
backing storage framebuffer.

int drm\_fb\_helper\_initial\_config(struct [drm\_fb\_helper](#c.drm_fb_helper "drm_fb_helper") \*fb\_helper)
:   setup a sane initial connector configuration

**Parameters**

`struct drm_fb_helper *fb_helper`
:   fb\_helper device struct

**Description**

Scans the CRTCs and connectors and tries to put together an initial setup.
At the moment, this is a cloned configuration across all heads with
a new framebuffer object as the backing store.

Note that this also registers the fbdev and so allows userspace to call into
the driver through the fbdev interfaces.

This function will call down into the [`drm_driver.fbdev_probe`](drm-internals.html#c.drm_driver "drm_driver") callback
to let the driver allocate and initialize the fbdev info structure and the
drm framebuffer used to back the fbdev. [`drm_fb_helper_fill_info()`](#c.drm_fb_helper_fill_info "drm_fb_helper_fill_info") is provided
as a helper to setup simple default values for the fbdev info structure.

HANG DEBUGGING:

When you have fbcon support built-in or already loaded, this function will do
a full modeset to setup the fbdev console. Due to locking misdesign in the
VT/fbdev subsystem that entire modeset sequence has to be done while holding
console\_lock. Until console\_unlock is called no dmesg lines will be sent out
to consoles, not even serial console. This means when your driver crashes,
you will see absolutely nothing else but a system stuck in this function,
with no further output. Any kind of [`printk()`](../core-api/printk-basics.html#c.printk "printk") you place within your own driver
or in the drm core modeset code will also never show up.

Standard debug practice is to run the fbcon setup without taking the
console\_lock as a hack, to be able to see backtraces and crashes on the
serial line. This can be done by setting the fb.lockless\_register\_fb=1 kernel
cmdline option.

The other option is to just disable fbdev emulation since very likely the
first modeset from userspace will crash in the same way, and is even easier
to debug. This can be done by setting the drm\_kms\_helper.fbdev\_emulation=0
kernel cmdline option.

**Return**

Zero if everything went ok, nonzero otherwise.

int drm\_fb\_helper\_hotplug\_event(struct [drm\_fb\_helper](#c.drm_fb_helper "drm_fb_helper") \*fb\_helper)
:   respond to a hotplug notification by probing all the outputs attached to the fb

**Parameters**

`struct drm_fb_helper *fb_helper`
:   driver-allocated fbdev helper, can be NULL

**Description**

Scan the connectors attached to the fb\_helper and try to put together a
setup after notification of a change in output configuration.

Called at runtime, takes the mode config locks to be able to check/change the
modeset configuration. Must be run from process context (which usually means
either the output polling work or a work item launched from the driver’s
hotplug interrupt).

Note that drivers may call this even before calling
drm\_fb\_helper\_initial\_config but only after drm\_fb\_helper\_init. This allows
for a race-free fbcon setup and will make sure that the fbdev emulation will
not miss any hotplug events.

**Return**

0 on success and a non-zero error code otherwise.

bool drm\_fb\_helper\_gem\_is\_fb(const struct [drm\_fb\_helper](#c.drm_fb_helper "drm_fb_helper") \*fb\_helper, const struct [drm\_gem\_object](drm-mm.html#c.drm_gem_object "drm_gem_object") \*obj)
:   Tests if GEM object is framebuffer

**Parameters**

`const struct drm_fb_helper *fb_helper`
:   fb\_helper instance, can be NULL

`const struct drm_gem_object *obj`
:   The GEM object to test, can be NULL

**Description**

Call drm\_fb\_helper\_gem\_is\_fb to test is a DRM device’s fbdev emulation
uses the specified GEM object for its framebuffer. The result is always
false if either poiner is NULL.

**Return**

True if fbdev emulation uses the provided GEM object, or false otherwise.

## format Helper Functions Reference

void drm\_format\_conv\_state\_init(struct drm\_format\_conv\_state \*state)
:   Initialize format-conversion state

**Parameters**

`struct drm_format_conv_state *state`
:   The state to initialize

**Description**

Clears all fields in `struct drm_format_conv_state`. The state will
be empty with no preallocated resources.

void drm\_format\_conv\_state\_copy(struct drm\_format\_conv\_state \*state, const struct drm\_format\_conv\_state \*old\_state)
:   Copy format-conversion state

**Parameters**

`struct drm_format_conv_state *state`
:   Destination state

`const struct drm_format_conv_state *old_state`
:   Source state

**Description**

Copies format-conversion state from **old\_state** to **state**; except for
temporary storage.

void \*drm\_format\_conv\_state\_reserve(struct drm\_format\_conv\_state \*state, size\_t new\_size, gfp\_t flags)
:   Allocates storage for format conversion

**Parameters**

`struct drm_format_conv_state *state`
:   The format-conversion state

`size_t new_size`
:   The minimum allocation size

`gfp_t flags`
:   Flags for [`kmalloc()`](../core-api/mm-api.html#c.kmalloc "kmalloc")

**Description**

Allocates at least **new\_size** bytes and returns a pointer to the memory
range. After calling this function, previously returned memory blocks
are invalid. It’s best to collect all memory requirements of a format
conversion and call this function once to allocate the range.

**Return**

A pointer to the allocated memory range, or NULL otherwise.

void drm\_format\_conv\_state\_release(struct drm\_format\_conv\_state \*state)
:   Releases an format-conversion storage

**Parameters**

`struct drm_format_conv_state *state`
:   The format-conversion state

**Description**

Releases the memory range references by the format-conversion state.
After this call, all pointers to the memory are invalid. Prefer
[`drm_format_conv_state_init()`](#c.drm_format_conv_state_init "drm_format_conv_state_init") for cleaning up and unloading a driver.

unsigned int drm\_fb\_clip\_offset(unsigned int pitch, const struct [drm\_format\_info](drm-kms.html#c.drm_format_info "drm_format_info") \*format, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip)
:   Returns the clipping rectangles byte-offset in a framebuffer

**Parameters**

`unsigned int pitch`
:   Framebuffer line pitch in byte

`const struct drm_format_info *format`
:   Framebuffer format

`const struct drm_rect *clip`
:   Clip rectangle

**Return**

The byte offset of the clip rectangle’s top-left corner within the framebuffer.

void drm\_fb\_memcpy(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip)
:   Copy clip buffer

**Parameters**

`struct iosys_map *dst`
:   Array of destination buffers

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of source buffers

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

**Description**

This function copies parts of a framebuffer to display memory. Destination and
framebuffer formats must match. No conversion takes place. The parameters **dst**,
**dst\_pitch** and **src** refer to arrays. Each array must have at least as many entries
as there are planes in **fb**’s format. Each entry stores the value for the format’s
respective color plane at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner).

void drm\_fb\_swab(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, bool cached, struct drm\_format\_conv\_state \*state)
:   Swap bytes into clip buffer

**Parameters**

`struct iosys_map *dst`
:   Array of destination buffers

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of source buffers

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`bool cached`
:   Source buffer is mapped cached (eg. not write-combined)

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and swaps per-pixel
bytes during the process. Destination and framebuffer formats must match. The
parameters **dst**, **dst\_pitch** and **src** refer to arrays. Each array must have at
least as many entries as there are planes in **fb**’s format. Each entry stores the
value for the format’s respective color plane at the same index. If **cached** is
false a temporary buffer is used to cache one pixel line at a time to speed up
slow uncached reads.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner).

void drm\_fb\_xrgb8888\_to\_rgb332(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, struct drm\_format\_conv\_state \*state)
:   Convert XRGB8888 to RGB332 clip buffer

**Parameters**

`struct iosys_map *dst`
:   Array of RGB332 destination buffers

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of XRGB8888 source buffers

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and converts the
color format during the process. Destination and framebuffer formats must match. The
parameters **dst**, **dst\_pitch** and **src** refer to arrays. Each array must have at
least as many entries as there are planes in **fb**’s format. Each entry stores the
value for the format’s respective color plane at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner).

Drivers can use this function for RGB332 devices that don’t support XRGB8888 natively.

void drm\_fb\_xrgb8888\_to\_rgb565(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, struct drm\_format\_conv\_state \*state)
:   Convert XRGB8888 to RGB565 clip buffer

**Parameters**

`struct iosys_map *dst`
:   Array of RGB565 destination buffers

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of XRGB8888 source buffer

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and converts the
color format during the process. Destination and framebuffer formats must match. The
parameters **dst**, **dst\_pitch** and **src** refer to arrays. Each array must have at
least as many entries as there are planes in **fb**’s format. Each entry stores the
value for the format’s respective color plane at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner).

Drivers can use this function for RGB565 devices that don’t support XRGB8888 natively.

void drm\_fb\_xrgb8888\_to\_rgb565be(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, struct drm\_format\_conv\_state \*state)
:   Convert XRGB8888 to RGB565|DRM\_FORMAT\_BIG\_ENDIAN clip buffer

**Parameters**

`struct iosys_map *dst`
:   Array of RGB565BE destination buffers

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of XRGB8888 source buffer

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and converts the
color format during the process. Destination and framebuffer formats must match. The
parameters **dst**, **dst\_pitch** and **src** refer to arrays. Each array must have at
least as many entries as there are planes in **fb**’s format. Each entry stores the
value for the format’s respective color plane at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner).

Drivers can use this function for RGB565BE devices that don’t support XRGB8888 natively.

void drm\_fb\_xrgb8888\_to\_xrgb1555(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, struct drm\_format\_conv\_state \*state)
:   Convert XRGB8888 to XRGB1555 clip buffer

**Parameters**

`struct iosys_map *dst`
:   Array of XRGB1555 destination buffers

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of XRGB8888 source buffer

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and converts
the color format during the process. The parameters **dst**, **dst\_pitch** and
**src** refer to arrays. Each array must have at least as many entries as
there are planes in **fb**’s format. Each entry stores the value for the
format’s respective color plane at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner).

Drivers can use this function for XRGB1555 devices that don’t support
XRGB8888 natively.

void drm\_fb\_xrgb8888\_to\_argb1555(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, struct drm\_format\_conv\_state \*state)
:   Convert XRGB8888 to ARGB1555 clip buffer

**Parameters**

`struct iosys_map *dst`
:   Array of ARGB1555 destination buffers

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of XRGB8888 source buffer

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and converts
the color format during the process. The parameters **dst**, **dst\_pitch** and
**src** refer to arrays. Each array must have at least as many entries as
there are planes in **fb**’s format. Each entry stores the value for the
format’s respective color plane at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner).

Drivers can use this function for ARGB1555 devices that don’t support
XRGB8888 natively. It sets an opaque alpha channel as part of the conversion.

void drm\_fb\_xrgb8888\_to\_rgba5551(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, struct drm\_format\_conv\_state \*state)
:   Convert XRGB8888 to RGBA5551 clip buffer

**Parameters**

`struct iosys_map *dst`
:   Array of RGBA5551 destination buffers

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of XRGB8888 source buffer

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and converts
the color format during the process. The parameters **dst**, **dst\_pitch** and
**src** refer to arrays. Each array must have at least as many entries as
there are planes in **fb**’s format. Each entry stores the value for the
format’s respective color plane at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner).

Drivers can use this function for RGBA5551 devices that don’t support
XRGB8888 natively. It sets an opaque alpha channel as part of the conversion.

void drm\_fb\_xrgb8888\_to\_rgb888(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, struct drm\_format\_conv\_state \*state)
:   Convert XRGB8888 to RGB888 clip buffer

**Parameters**

`struct iosys_map *dst`
:   Array of RGB888 destination buffers

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of XRGB8888 source buffers

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and converts the
color format during the process. Destination and framebuffer formats must match. The
parameters **dst**, **dst\_pitch** and **src** refer to arrays. Each array must have at
least as many entries as there are planes in **fb**’s format. Each entry stores the
value for the format’s respective color plane at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner).

Drivers can use this function for RGB888 devices that don’t natively
support XRGB8888.

void drm\_fb\_xrgb8888\_to\_bgr888(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, struct drm\_format\_conv\_state \*state)
:   Convert XRGB8888 to BGR888 clip buffer

**Parameters**

`struct iosys_map *dst`
:   Array of BGR888 destination buffers

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of XRGB8888 source buffers

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and converts the
color format during the process. Destination and framebuffer formats must match. The
parameters **dst**, **dst\_pitch** and **src** refer to arrays. Each array must have at
least as many entries as there are planes in **fb**’s format. Each entry stores the
value for the format’s respective color plane at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner).

Drivers can use this function for BGR888 devices that don’t natively
support XRGB8888.

void drm\_fb\_xrgb8888\_to\_argb8888(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, struct drm\_format\_conv\_state \*state)
:   Convert XRGB8888 to ARGB8888 clip buffer

**Parameters**

`struct iosys_map *dst`
:   Array of ARGB8888 destination buffers

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of XRGB8888 source buffer

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and converts the
color format during the process. The parameters **dst**, **dst\_pitch** and **src** refer
to arrays. Each array must have at least as many entries as there are planes in
**fb**’s format. Each entry stores the value for the format’s respective color plane
at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner).

Drivers can use this function for ARGB8888 devices that don’t support XRGB8888
natively. It sets an opaque alpha channel as part of the conversion.

void drm\_fb\_xrgb8888\_to\_abgr8888(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, struct drm\_format\_conv\_state \*state)
:   Convert XRGB8888 to ABGR8888 clip buffer

**Parameters**

`struct iosys_map *dst`
:   Array of ABGR8888 destination buffers

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of XRGB8888 source buffer

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and converts the
color format during the process. The parameters **dst**, **dst\_pitch** and **src** refer
to arrays. Each array must have at least as many entries as there are planes in
**fb**’s format. Each entry stores the value for the format’s respective color plane
at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner).

Drivers can use this function for ABGR8888 devices that don’t support XRGB8888
natively. It sets an opaque alpha channel as part of the conversion.

void drm\_fb\_xrgb8888\_to\_xbgr8888(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, struct drm\_format\_conv\_state \*state)
:   Convert XRGB8888 to XBGR8888 clip buffer

**Parameters**

`struct iosys_map *dst`
:   Array of XBGR8888 destination buffers

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of XRGB8888 source buffer

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and converts the
color format during the process. The parameters **dst**, **dst\_pitch** and **src** refer
to arrays. Each array must have at least as many entries as there are planes in
**fb**’s format. Each entry stores the value for the format’s respective color plane
at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner).

Drivers can use this function for XBGR8888 devices that don’t support XRGB8888
natively.

void drm\_fb\_xrgb8888\_to\_bgrx8888(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, struct drm\_format\_conv\_state \*state)
:   Convert XRGB8888 to BGRX8888 clip buffer

**Parameters**

`struct iosys_map *dst`
:   Array of BGRX8888 destination buffers

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of XRGB8888 source buffer

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and converts the
color format during the process. The parameters **dst**, **dst\_pitch** and **src** refer
to arrays. Each array must have at least as many entries as there are planes in
**fb**’s format. Each entry stores the value for the format’s respective color plane
at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner).

Drivers can use this function for BGRX8888 devices that don’t support XRGB8888
natively.

void drm\_fb\_xrgb8888\_to\_xrgb2101010(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, struct drm\_format\_conv\_state \*state)
:   Convert XRGB8888 to XRGB2101010 clip buffer

**Parameters**

`struct iosys_map *dst`
:   Array of XRGB2101010 destination buffers

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of XRGB8888 source buffers

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and converts the
color format during the process. Destination and framebuffer formats must match. The
parameters **dst**, **dst\_pitch** and **src** refer to arrays. Each array must have at
least as many entries as there are planes in **fb**’s format. Each entry stores the
value for the format’s respective color plane at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner).

Drivers can use this function for XRGB2101010 devices that don’t support XRGB8888
natively.

void drm\_fb\_xrgb8888\_to\_argb2101010(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, struct drm\_format\_conv\_state \*state)
:   Convert XRGB8888 to ARGB2101010 clip buffer

**Parameters**

`struct iosys_map *dst`
:   Array of ARGB2101010 destination buffers

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of XRGB8888 source buffers

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and converts
the color format during the process. The parameters **dst**, **dst\_pitch** and
**src** refer to arrays. Each array must have at least as many entries as
there are planes in **fb**’s format. Each entry stores the value for the
format’s respective color plane at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner).

Drivers can use this function for ARGB2101010 devices that don’t support XRGB8888
natively.

void drm\_fb\_xrgb8888\_to\_gray8(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, struct drm\_format\_conv\_state \*state)
:   Convert XRGB8888 to grayscale

**Parameters**

`struct iosys_map *dst`
:   Array of 8-bit grayscale destination buffers

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of XRGB8888 source buffers

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and converts the
color format during the process. Destination and framebuffer formats must match. The
parameters **dst**, **dst\_pitch** and **src** refer to arrays. Each array must have at
least as many entries as there are planes in **fb**’s format. Each entry stores the
value for the format’s respective color plane at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner).

DRM doesn’t have native monochrome or grayscale support. Drivers can use this
function for grayscale devices that don’t support XRGB8888 natively.Such
drivers can announce the commonly supported XR24 format to userspace and use
this function to convert to the native format. Monochrome drivers will use the
most significant bit, where 1 means foreground color and 0 background color.
ITU BT.601 is being used for the RGB -> luma (brightness) conversion.

void drm\_fb\_argb8888\_to\_argb4444(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, struct drm\_format\_conv\_state \*state)
:   Convert ARGB8888 to ARGB4444 clip buffer

**Parameters**

`struct iosys_map *dst`
:   Array of ARGB4444 destination buffers

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of ARGB8888 source buffer

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and converts
the color format during the process. The parameters **dst**, **dst\_pitch** and
**src** refer to arrays. Each array must have at least as many entries as
there are planes in **fb**’s format. Each entry stores the value for the
format’s respective color plane at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner).

Drivers can use this function for ARGB4444 devices that don’t support
ARGB8888 natively.

void drm\_fb\_xrgb8888\_to\_mono(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, struct drm\_format\_conv\_state \*state)
:   Convert XRGB8888 to monochrome

**Parameters**

`struct iosys_map *dst`
:   Array of monochrome destination buffers (0=black, 1=white)

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of XRGB8888 source buffers

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and converts the
color format during the process. Destination and framebuffer formats must match. The
parameters **dst**, **dst\_pitch** and **src** refer to arrays. Each array must have at
least as many entries as there are planes in **fb**’s format. Each entry stores the
value for the format’s respective color plane at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner). The first pixel (upper left corner of the clip rectangle) will
be converted and copied to the first bit (LSB) in the first byte of the monochrome
destination buffer. If the caller requires that the first pixel in a byte must
be located at an x-coordinate that is a multiple of 8, then the caller must take
care itself of supplying a suitable clip rectangle.

DRM doesn’t have native monochrome support. Drivers can use this function for
monochrome devices that don’t support XRGB8888 natively. Such drivers can
announce the commonly supported XR24 format to userspace and use this function
to convert to the native format.

This function uses [`drm_fb_xrgb8888_to_gray8()`](#c.drm_fb_xrgb8888_to_gray8 "drm_fb_xrgb8888_to_gray8") to convert to grayscale and
then the result is converted from grayscale to monochrome.

void drm\_fb\_xrgb8888\_to\_gray2(struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*dst, const unsigned int \*dst\_pitch, const struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, const struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, struct drm\_format\_conv\_state \*state)
:   Convert XRGB8888 to gray2

**Parameters**

`struct iosys_map *dst`
:   Array of gray2 destination buffer

`const unsigned int *dst_pitch`
:   Array of numbers of bytes between the start of two consecutive scanlines
    within **dst**; can be NULL if scanlines are stored next to each other.

`const struct iosys_map *src`
:   Array of XRGB8888 source buffers

`const struct drm_framebuffer *fb`
:   DRM framebuffer

`const struct drm_rect *clip`
:   Clip rectangle area to copy

`struct drm_format_conv_state *state`
:   Transform and conversion state

**Description**

This function copies parts of a framebuffer to display memory and converts the
color format during the process. Destination and framebuffer formats must match. The
parameters **dst**, **dst\_pitch** and **src** refer to arrays. Each array must have at
least as many entries as there are planes in **fb**’s format. Each entry stores the
value for the format’s respective color plane at the same index.

This function does not apply clipping on **dst** (i.e. the destination is at the
top-left corner). The first pixel (upper left corner of the clip rectangle) will
be converted and copied to the two first bits (LSB) in the first byte of the gray2
destination buffer. If the caller requires that the first pixel in a byte must
be located at an x-coordinate that is a multiple of 8, then the caller must take
care itself of supplying a suitable clip rectangle.

DRM doesn’t have native gray2 support. Drivers can use this function for
gray2 devices that don’t support XRGB8888 natively. Such drivers can
announce the commonly supported XR24 format to userspace and use this function
to convert to the native format.

## Framebuffer DMA Helper Functions Reference

Provides helper functions for creating a DMA-contiguous framebuffer.

Depending on the platform, the buffers may be physically non-contiguous and
mapped through an IOMMU or a similar mechanism, or allocated from
physically-contiguous memory (using, for instance, CMA or a pool of memory
reserved at early boot). This is handled behind the scenes by the DMA mapping
API.

[`drm_gem_fb_create()`](#c.drm_gem_fb_create "drm_gem_fb_create") is used in the [`drm_mode_config_funcs.fb_create`](drm-kms.html#c.drm_mode_config_funcs "drm_mode_config_funcs")
callback function to create a DMA-contiguous framebuffer.

struct [drm\_gem\_dma\_object](drm-mm.html#c.drm_gem_dma_object "drm_gem_dma_object") \*drm\_fb\_dma\_get\_gem\_obj(struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, unsigned int plane)
:   Get DMA GEM object for framebuffer

**Parameters**

`struct drm_framebuffer *fb`
:   The framebuffer

`unsigned int plane`
:   Which plane

**Description**

Return the DMA GEM object for given framebuffer.

This function will usually be called from the CRTC callback functions.

dma\_addr\_t drm\_fb\_dma\_get\_gem\_addr(struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*state, unsigned int plane)
:   Get DMA (bus) address for framebuffer, for pixel formats where values are grouped in blocks this will get you the beginning of the block

**Parameters**

`struct drm_framebuffer *fb`
:   The framebuffer

`struct drm_plane_state *state`
:   Which state of drm plane

`unsigned int plane`
:   Which plane
    Return the DMA GEM address for given framebuffer.

**Description**

This function will usually be called from the PLANE callback functions.

void drm\_fb\_dma\_sync\_non\_coherent(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*drm, struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*old\_state, struct [drm\_plane\_state](drm-kms.html#c.drm_plane_state "drm_plane_state") \*state)
:   Sync GEM object to non-coherent backing memory

**Parameters**

`struct drm_device *drm`
:   DRM device

`struct drm_plane_state *old_state`
:   Old plane state

`struct drm_plane_state *state`
:   New plane state

**Description**

This function can be used by drivers that use damage clips and have
DMA GEM objects backed by non-coherent memory. Calling this function
in a plane’s .atomic\_update ensures that all the data in the backing
memory have been written to RAM.

int drm\_fb\_dma\_get\_scanout\_buffer(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_scanout\_buffer](drm-kms.html#c.drm_scanout_buffer "drm_scanout_buffer") \*sb)
:   Provide a scanout buffer in case of panic

**Parameters**

`struct drm_plane *plane`
:   DRM primary plane

`struct drm_scanout_buffer *sb`
:   scanout buffer for the panic handler

**Return**

0 or negative error code

**Description**

Generic `get_scanout_buffer()` implementation, for drivers that uses the
drm\_fb\_dma\_helper. It won’t call vmap in the panic context, so the driver
should make sure the primary plane is vmapped, otherwise the panic screen
won’t get displayed.

## Framebuffer GEM Helper Reference

This library provides helpers for drivers that don’t subclass
[`drm_framebuffer`](drm-kms.html#c.drm_framebuffer "drm_framebuffer") and use [`drm_gem_object`](drm-mm.html#c.drm_gem_object "drm_gem_object") for their backing storage.

Drivers without additional needs to validate framebuffers can simply use
[`drm_gem_fb_create()`](#c.drm_gem_fb_create "drm_gem_fb_create") and everything is wired up automatically. Other drivers
can use all parts independently.

struct [drm\_gem\_object](drm-mm.html#c.drm_gem_object "drm_gem_object") \*drm\_gem\_fb\_get\_obj(struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, unsigned int plane)
:   Get GEM object backing the framebuffer

**Parameters**

`struct drm_framebuffer *fb`
:   Framebuffer

`unsigned int plane`
:   Plane index

**Description**

No additional reference is taken beyond the one that the `drm_frambuffer`
already holds.

**Return**

Pointer to [`drm_gem_object`](drm-mm.html#c.drm_gem_object "drm_gem_object") for the given framebuffer and plane index or NULL
if it does not exist.

void drm\_gem\_fb\_destroy(struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb)
:   Free GEM backed framebuffer

**Parameters**

`struct drm_framebuffer *fb`
:   Framebuffer

**Description**

Frees a GEM backed framebuffer with its backing buffer(s) and the structure
itself. Drivers can use this as their [`drm_framebuffer_funcs->destroy`](drm-kms.html#c.drm_framebuffer_funcs "drm_framebuffer_funcs")
callback.

int drm\_gem\_fb\_create\_handle(struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file, unsigned int \*handle)
:   Create handle for GEM backed framebuffer

**Parameters**

`struct drm_framebuffer *fb`
:   Framebuffer

`struct drm_file *file`
:   DRM file to register the handle for

`unsigned int *handle`
:   Pointer to return the created handle

**Description**

This function creates a handle for the GEM object backing the framebuffer.
Drivers can use this as their [`drm_framebuffer_funcs->create_handle`](drm-kms.html#c.drm_framebuffer_funcs "drm_framebuffer_funcs")
callback. The GETFB IOCTL calls into this callback.

**Return**

0 on success or a negative error code on failure.

int drm\_gem\_fb\_init\_with\_funcs(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file, const struct [drm\_format\_info](drm-kms.html#c.drm_format_info "drm_format_info") \*info, const struct [drm\_mode\_fb\_cmd2](drm-uapi.html#c.drm_mode_fb_cmd2 "drm_mode_fb_cmd2") \*mode\_cmd, const struct [drm\_framebuffer\_funcs](drm-kms.html#c.drm_framebuffer_funcs "drm_framebuffer_funcs") \*funcs)
:   Helper function for implementing [`drm_mode_config_funcs.fb_create`](drm-kms.html#c.drm_mode_config_funcs "drm_mode_config_funcs") callback in cases when the driver allocates a subclass of [`struct drm_framebuffer`](drm-kms.html#c.drm_framebuffer "drm_framebuffer")

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_framebuffer *fb`
:   framebuffer object

`struct drm_file *file`
:   DRM file that holds the GEM handle(s) backing the framebuffer

`const struct drm_format_info *info`
:   pixel format information

`const struct drm_mode_fb_cmd2 *mode_cmd`
:   Metadata from the userspace framebuffer creation request

`const struct drm_framebuffer_funcs *funcs`
:   vtable to be used for the new framebuffer object

**Description**

This function can be used to set [`drm_framebuffer_funcs`](drm-kms.html#c.drm_framebuffer_funcs "drm_framebuffer_funcs") for drivers that need
custom framebuffer callbacks. Use [`drm_gem_fb_create()`](#c.drm_gem_fb_create "drm_gem_fb_create") if you don’t need to
change [`drm_framebuffer_funcs`](drm-kms.html#c.drm_framebuffer_funcs "drm_framebuffer_funcs"). The function does buffer size validation.
The buffer size validation is for a general case, though, so users should
pay attention to the checks being appropriate for them or, at least,
non-conflicting.

**Return**

Zero or a negative error code.

struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*drm\_gem\_fb\_create\_with\_funcs(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file, const struct [drm\_format\_info](drm-kms.html#c.drm_format_info "drm_format_info") \*info, const struct [drm\_mode\_fb\_cmd2](drm-uapi.html#c.drm_mode_fb_cmd2 "drm_mode_fb_cmd2") \*mode\_cmd, const struct [drm\_framebuffer\_funcs](drm-kms.html#c.drm_framebuffer_funcs "drm_framebuffer_funcs") \*funcs)
:   Helper function for the [`drm_mode_config_funcs.fb_create`](drm-kms.html#c.drm_mode_config_funcs "drm_mode_config_funcs") callback

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_file *file`
:   DRM file that holds the GEM handle(s) backing the framebuffer

`const struct drm_format_info *info`
:   pixel format information

`const struct drm_mode_fb_cmd2 *mode_cmd`
:   Metadata from the userspace framebuffer creation request

`const struct drm_framebuffer_funcs *funcs`
:   vtable to be used for the new framebuffer object

**Description**

This function can be used to set [`drm_framebuffer_funcs`](drm-kms.html#c.drm_framebuffer_funcs "drm_framebuffer_funcs") for drivers that need
custom framebuffer callbacks. Use [`drm_gem_fb_create()`](#c.drm_gem_fb_create "drm_gem_fb_create") if you don’t need to
change [`drm_framebuffer_funcs`](drm-kms.html#c.drm_framebuffer_funcs "drm_framebuffer_funcs"). The function does buffer size validation.

**Return**

Pointer to a [`drm_framebuffer`](drm-kms.html#c.drm_framebuffer "drm_framebuffer") on success or an error pointer on failure.

struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*drm\_gem\_fb\_create(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file, const struct [drm\_format\_info](drm-kms.html#c.drm_format_info "drm_format_info") \*info, const struct [drm\_mode\_fb\_cmd2](drm-uapi.html#c.drm_mode_fb_cmd2 "drm_mode_fb_cmd2") \*mode\_cmd)
:   Helper function for the [`drm_mode_config_funcs.fb_create`](drm-kms.html#c.drm_mode_config_funcs "drm_mode_config_funcs") callback

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_file *file`
:   DRM file that holds the GEM handle(s) backing the framebuffer

`const struct drm_format_info *info`
:   pixel format information

`const struct drm_mode_fb_cmd2 *mode_cmd`
:   Metadata from the userspace framebuffer creation request

**Description**

This function creates a new framebuffer object described by
[`drm_mode_fb_cmd2`](drm-uapi.html#c.drm_mode_fb_cmd2 "drm_mode_fb_cmd2"). This description includes handles for the buffer(s)
backing the framebuffer.

If your hardware has special alignment or pitch requirements these should be
checked before calling this function. The function does buffer size
validation. Use [`drm_gem_fb_create_with_dirty()`](#c.drm_gem_fb_create_with_dirty "drm_gem_fb_create_with_dirty") if you need framebuffer
flushing.

Drivers can use this as their [`drm_mode_config_funcs.fb_create`](drm-kms.html#c.drm_mode_config_funcs "drm_mode_config_funcs") callback.
The ADDFB2 IOCTL calls into this callback.

**Return**

Pointer to a [`drm_framebuffer`](drm-kms.html#c.drm_framebuffer "drm_framebuffer") on success or an error pointer on failure.

struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*drm\_gem\_fb\_create\_with\_dirty(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_file](drm-internals.html#c.drm_file "drm_file") \*file, const struct [drm\_format\_info](drm-kms.html#c.drm_format_info "drm_format_info") \*info, const struct [drm\_mode\_fb\_cmd2](drm-uapi.html#c.drm_mode_fb_cmd2 "drm_mode_fb_cmd2") \*mode\_cmd)
:   Helper function for the [`drm_mode_config_funcs.fb_create`](drm-kms.html#c.drm_mode_config_funcs "drm_mode_config_funcs") callback

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_file *file`
:   DRM file that holds the GEM handle(s) backing the framebuffer

`const struct drm_format_info *info`
:   pixel format information

`const struct drm_mode_fb_cmd2 *mode_cmd`
:   Metadata from the userspace framebuffer creation request

**Description**

This function creates a new framebuffer object described by
[`drm_mode_fb_cmd2`](drm-uapi.html#c.drm_mode_fb_cmd2 "drm_mode_fb_cmd2"). This description includes handles for the buffer(s)
backing the framebuffer. [`drm_atomic_helper_dirtyfb()`](drm-kms.html#c.drm_atomic_helper_dirtyfb "drm_atomic_helper_dirtyfb") is used for the dirty
callback giving framebuffer flushing through the atomic machinery. Use
[`drm_gem_fb_create()`](#c.drm_gem_fb_create "drm_gem_fb_create") if you don’t need the dirty callback.
The function does buffer size validation.

Drivers should also call [`drm_plane_enable_fb_damage_clips()`](drm-kms.html#c.drm_plane_enable_fb_damage_clips "drm_plane_enable_fb_damage_clips") on all planes
to enable userspace to use damage clips also with the ATOMIC IOCTL.

Drivers can use this as their [`drm_mode_config_funcs.fb_create`](drm-kms.html#c.drm_mode_config_funcs "drm_mode_config_funcs") callback.
The ADDFB2 IOCTL calls into this callback.

**Return**

Pointer to a [`drm_framebuffer`](drm-kms.html#c.drm_framebuffer "drm_framebuffer") on success or an error pointer on failure.

int drm\_gem\_fb\_vmap(struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*map, struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*data)
:   maps all framebuffer BOs into kernel address space

**Parameters**

`struct drm_framebuffer *fb`
:   the framebuffer

`struct iosys_map *map`
:   returns the mapping’s address for each BO

`struct iosys_map *data`
:   returns the data address for each BO, can be NULL

**Description**

This function maps all buffer objects of the given framebuffer into
kernel address space and stores them in [`struct iosys_map`](../driver-api/device-io.html#c.iosys_map "iosys_map"). If the
mapping operation fails for one of the BOs, the function unmaps the
already established mappings automatically.

Callers that want to access a BO’s stored data should pass **data**.
The argument returns the addresses of the data stored in each BO. This
is different from **map** if the framebuffer’s offsets field is non-zero.

Both, **map** and **data**, must each refer to arrays with at least
fb->format->num\_planes elements.

See [`drm_gem_fb_vunmap()`](#c.drm_gem_fb_vunmap "drm_gem_fb_vunmap") for unmapping.

**Return**

0 on success, or a negative errno code otherwise.

void drm\_gem\_fb\_vunmap(struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*map)
:   unmaps framebuffer BOs from kernel address space

**Parameters**

`struct drm_framebuffer *fb`
:   the framebuffer

`struct iosys_map *map`
:   mapping addresses as returned by [`drm_gem_fb_vmap()`](#c.drm_gem_fb_vmap "drm_gem_fb_vmap")

**Description**

This function unmaps all buffer objects of the given framebuffer.

See [`drm_gem_fb_vmap()`](#c.drm_gem_fb_vmap "drm_gem_fb_vmap") for more information.

int drm\_gem\_fb\_begin\_cpu\_access(struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, enum dma\_data\_direction dir)
:   prepares GEM buffer objects for CPU access

**Parameters**

`struct drm_framebuffer *fb`
:   the framebuffer

`enum dma_data_direction dir`
:   access mode

**Description**

Prepares a framebuffer’s GEM buffer objects for CPU access. This function
must be called before accessing the BO data within the kernel. For imported
BOs, the function calls [`dma_buf_begin_cpu_access()`](../driver-api/dma-buf.html#c.dma_buf_begin_cpu_access "dma_buf_begin_cpu_access").

See [`drm_gem_fb_end_cpu_access()`](#c.drm_gem_fb_end_cpu_access "drm_gem_fb_end_cpu_access") for signalling the end of CPU access.

**Return**

0 on success, or a negative errno code otherwise.

void drm\_gem\_fb\_end\_cpu\_access(struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, enum dma\_data\_direction dir)
:   signals end of CPU access to GEM buffer objects

**Parameters**

`struct drm_framebuffer *fb`
:   the framebuffer

`enum dma_data_direction dir`
:   access mode

**Description**

Signals the end of CPU access to the given framebuffer’s GEM buffer objects. This
function must be paired with a corresponding call to [`drm_gem_fb_begin_cpu_access()`](#c.drm_gem_fb_begin_cpu_access "drm_gem_fb_begin_cpu_access").
For imported BOs, the function calls [`dma_buf_end_cpu_access()`](../driver-api/dma-buf.html#c.dma_buf_end_cpu_access "dma_buf_end_cpu_access").

See also [`drm_gem_fb_begin_cpu_access()`](#c.drm_gem_fb_begin_cpu_access "drm_gem_fb_begin_cpu_access").

int drm\_gem\_fb\_afbc\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, const struct [drm\_format\_info](drm-kms.html#c.drm_format_info "drm_format_info") \*info, const struct [drm\_mode\_fb\_cmd2](drm-uapi.html#c.drm_mode_fb_cmd2 "drm_mode_fb_cmd2") \*mode\_cmd, struct [drm\_afbc\_framebuffer](drm-kms.html#c.drm_afbc_framebuffer "drm_afbc_framebuffer") \*afbc\_fb)
:   Helper function for drivers using afbc to fill and validate all the afbc-specific [`struct drm_afbc_framebuffer`](drm-kms.html#c.drm_afbc_framebuffer "drm_afbc_framebuffer") members

**Parameters**

`struct drm_device *dev`
:   DRM device

`const struct drm_format_info *info`
:   pixel format information

`const struct drm_mode_fb_cmd2 *mode_cmd`
:   Metadata from the userspace framebuffer creation request

`struct drm_afbc_framebuffer *afbc_fb`
:   afbc framebuffer

**Description**

This function can be used by drivers which support afbc to complete
the preparation of [`struct drm_afbc_framebuffer`](drm-kms.html#c.drm_afbc_framebuffer "drm_afbc_framebuffer"). It must be called after
allocating the said `struct and` calling [`drm_gem_fb_init_with_funcs()`](#c.drm_gem_fb_init_with_funcs "drm_gem_fb_init_with_funcs").
It is caller’s responsibility to put afbc\_fb->base.obj objects in case
the call is unsuccessful.

**Return**

Zero on success or a negative error value on failure.

## Bridges

### Overview

[`struct drm_bridge`](#c.drm_bridge "drm_bridge") represents a device that hangs on to an encoder. These are
handy when a regular [`drm_encoder`](drm-kms.html#c.drm_encoder "drm_encoder") entity isn’t enough to represent the entire
encoder chain.

A bridge is always attached to a single [`drm_encoder`](drm-kms.html#c.drm_encoder "drm_encoder") at a time, but can be
either connected to it directly, or through a chain of bridges:

```
[ CRTC ---> ] Encoder ---> Bridge A ---> Bridge B
```

Here, the output of the encoder feeds to bridge A, and that furthers feeds to
bridge B. Bridge chains can be arbitrarily long, and shall be fully linear:
Chaining multiple bridges to the output of a bridge, or the same bridge to
the output of different bridges, is not supported.

[`drm_bridge`](#c.drm_bridge "drm_bridge"), like [`drm_panel`](#c.drm_panel "drm_panel"), aren’t [`drm_mode_object`](drm-kms.html#c.drm_mode_object "drm_mode_object") entities like planes,
CRTCs, encoders or connectors and hence are not visible to userspace. They
just provide additional hooks to get the desired output at the end of the
encoder chain.

### Display Driver Integration

Display drivers are responsible for linking encoders with the first bridge
in the chains. This is done by acquiring the appropriate bridge with
[`devm_drm_of_get_bridge()`](#c.devm_drm_of_get_bridge "devm_drm_of_get_bridge"). Once acquired, the bridge shall be attached to the
encoder with a call to [`drm_bridge_attach()`](#c.drm_bridge_attach "drm_bridge_attach").

Bridges are responsible for linking themselves with the next bridge in the
chain, if any. This is done the same way as for encoders, with the call to
[`drm_bridge_attach()`](#c.drm_bridge_attach "drm_bridge_attach") occurring in the [`drm_bridge_funcs.attach`](#c.drm_bridge_funcs "drm_bridge_funcs") operation.

Once these links are created, the bridges can participate along with encoder
functions to perform mode validation and fixup (through
[`drm_bridge_chain_mode_valid()`](#c.drm_bridge_chain_mode_valid "drm_bridge_chain_mode_valid") and [`drm_atomic_bridge_chain_check()`](#c.drm_atomic_bridge_chain_check "drm_atomic_bridge_chain_check")), mode
setting (through [`drm_bridge_chain_mode_set()`](#c.drm_bridge_chain_mode_set "drm_bridge_chain_mode_set")), enable (through
[`drm_atomic_bridge_chain_pre_enable()`](#c.drm_atomic_bridge_chain_pre_enable "drm_atomic_bridge_chain_pre_enable") and [`drm_atomic_bridge_chain_enable()`](#c.drm_atomic_bridge_chain_enable "drm_atomic_bridge_chain_enable"))
and disable (through [`drm_atomic_bridge_chain_disable()`](#c.drm_atomic_bridge_chain_disable "drm_atomic_bridge_chain_disable") and
[`drm_atomic_bridge_chain_post_disable()`](#c.drm_atomic_bridge_chain_post_disable "drm_atomic_bridge_chain_post_disable")). Those functions call the
corresponding operations provided in [`drm_bridge_funcs`](#c.drm_bridge_funcs "drm_bridge_funcs") in sequence for all
bridges in the chain.

For display drivers that use the atomic helpers
[`drm_atomic_helper_check_modeset()`](#c.drm_atomic_helper_check_modeset "drm_atomic_helper_check_modeset"),
[`drm_atomic_helper_commit_modeset_enables()`](#c.drm_atomic_helper_commit_modeset_enables "drm_atomic_helper_commit_modeset_enables") and
[`drm_atomic_helper_commit_modeset_disables()`](#c.drm_atomic_helper_commit_modeset_disables "drm_atomic_helper_commit_modeset_disables") (either directly in hand-rolled
commit check and commit tail handlers, or through the higher-level
[`drm_atomic_helper_check()`](#c.drm_atomic_helper_check "drm_atomic_helper_check") and [`drm_atomic_helper_commit_tail()`](#c.drm_atomic_helper_commit_tail "drm_atomic_helper_commit_tail") or
[`drm_atomic_helper_commit_tail_rpm()`](#c.drm_atomic_helper_commit_tail_rpm "drm_atomic_helper_commit_tail_rpm") helpers), this is done transparently and
requires no intervention from the driver. For other drivers, the relevant
DRM bridge chain functions shall be called manually.

Bridges also participate in implementing the [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector") at the end of
the bridge chain. Display drivers may use the [`drm_bridge_connector_init()`](#c.drm_bridge_connector_init "drm_bridge_connector_init")
helper to create the [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector"), or implement it manually on top of the
connector-related operations exposed by the bridge (see the overview
documentation of bridge operations for more details).

### Special Care with MIPI-DSI bridges

The interaction between the bridges and other frameworks involved in
the probing of the upstream driver and the bridge driver can be
challenging. Indeed, there’s multiple cases that needs to be
considered:

* The upstream driver doesn’t use the component framework and isn’t a
  MIPI-DSI host. In this case, the bridge driver will probe at some
  point and the upstream driver should try to probe again by returning
  EPROBE\_DEFER as long as the bridge driver hasn’t probed.
* The upstream driver doesn’t use the component framework, but is a
  MIPI-DSI host. The bridge device uses the MIPI-DCS commands to be
  controlled. In this case, the bridge device is a child of the
  display device and when it will probe it’s assured that the display
  device (and MIPI-DSI host) is present. The upstream driver will be
  assured that the bridge driver is connected between the
  [`mipi_dsi_host_ops.attach`](#c.mipi_dsi_host_ops "mipi_dsi_host_ops") and [`mipi_dsi_host_ops.detach`](#c.mipi_dsi_host_ops "mipi_dsi_host_ops") operations.
  Therefore, it must run `mipi_dsi_host_register()` in its probe
  function, and then run [`drm_bridge_attach()`](#c.drm_bridge_attach "drm_bridge_attach") in its
  [`mipi_dsi_host_ops.attach`](#c.mipi_dsi_host_ops "mipi_dsi_host_ops") hook.
* The upstream driver uses the component framework and is a MIPI-DSI
  host. The bridge device uses the MIPI-DCS commands to be
  controlled. This is the same situation than above, and can run
  `mipi_dsi_host_register()` in either its probe or bind hooks.
* The upstream driver uses the component framework and is a MIPI-DSI
  host. The bridge device uses a separate bus (such as I2C) to be
  controlled. In this case, there’s no correlation between the probe
  of the bridge and upstream drivers, so care must be taken to avoid
  an endless EPROBE\_DEFER loop, with each driver waiting for the
  other to probe.

The ideal pattern to cover the last item (and all the others in the
MIPI-DSI host driver case) is to split the operations like this:

* The MIPI-DSI host driver must run `mipi_dsi_host_register()` in its
  probe hook. It will make sure that the MIPI-DSI host sticks around,
  and that the driver’s bind can be called.
* In its probe hook, the bridge driver must try to find its MIPI-DSI
  host, register as a MIPI-DSI device and attach the MIPI-DSI device
  to its host. The bridge driver is now functional.
* In its [`struct mipi_dsi_host_ops`](#c.mipi_dsi_host_ops "mipi_dsi_host_ops").attach hook, the MIPI-DSI host can
  now add its component. Its bind hook will now be called and since
  the bridge driver is attached and registered, we can now look for
  and attach it.

At this point, we’re now certain that both the upstream driver and
the bridge driver are functional and we can’t have a deadlock-like
situation when probing.

### Bridge Operations

Bridge drivers expose operations through the [`drm_bridge_funcs`](#c.drm_bridge_funcs "drm_bridge_funcs") structure.
The DRM internals (atomic and CRTC helpers) use the helpers defined in
drm\_bridge.c to call bridge operations. Those operations are divided in
three big categories to support different parts of the bridge usage.

* The encoder-related operations support control of the bridges in the
  chain, and are roughly counterparts to the [`drm_encoder_helper_funcs`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs")
  operations. They are used by the legacy CRTC and the atomic modeset
  helpers to perform mode validation, fixup and setting, and enable and
  disable the bridge automatically.

  The enable and disable operations are split in
  [`drm_bridge_funcs.pre_enable`](#c.drm_bridge_funcs "drm_bridge_funcs"), [`drm_bridge_funcs.enable`](#c.drm_bridge_funcs "drm_bridge_funcs"),
  [`drm_bridge_funcs.disable`](#c.drm_bridge_funcs "drm_bridge_funcs") and [`drm_bridge_funcs.post_disable`](#c.drm_bridge_funcs "drm_bridge_funcs") to provide
  finer-grained control.

  Bridge drivers may implement the legacy version of those operations, or
  the atomic version (prefixed with atomic\_), in which case they shall also
  implement the atomic state bookkeeping operations
  ([`drm_bridge_funcs.atomic_duplicate_state`](#c.drm_bridge_funcs "drm_bridge_funcs"),
  [`drm_bridge_funcs.atomic_destroy_state`](#c.drm_bridge_funcs "drm_bridge_funcs") and [`drm_bridge_funcs.reset`](#c.drm_bridge_funcs "drm_bridge_funcs")).
  Mixing atomic and non-atomic versions of the operations is not supported.
* The bus format negotiation operations
  [`drm_bridge_funcs.atomic_get_output_bus_fmts`](#c.drm_bridge_funcs "drm_bridge_funcs") and
  [`drm_bridge_funcs.atomic_get_input_bus_fmts`](#c.drm_bridge_funcs "drm_bridge_funcs") allow bridge drivers to
  negotiate the formats transmitted between bridges in the chain when
  multiple formats are supported. Negotiation for formats is performed
  transparently for display drivers by the atomic modeset helpers. Only
  atomic versions of those operations exist, bridge drivers that need to
  implement them shall thus also implement the atomic version of the
  encoder-related operations. This feature is not supported by the legacy
  CRTC helpers.
* The connector-related operations support implementing a [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector")
  based on a chain of bridges. DRM bridges traditionally create a
  [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector") for bridges meant to be used at the end of the chain. This
  puts additional burden on bridge drivers, especially for bridges that may
  be used in the middle of a chain or at the end of it. Furthermore, it
  requires all operations of the [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector") to be handled by a single
  bridge, which doesn’t always match the hardware architecture.

  To simplify bridge drivers and make the connector implementation more
  flexible, a new model allows bridges to unconditionally skip creation of
  [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector") and instead expose [`drm_bridge_funcs`](#c.drm_bridge_funcs "drm_bridge_funcs") operations to support
  an externally-implemented [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector"). Those operations are
  [`drm_bridge_funcs.detect`](#c.drm_bridge_funcs "drm_bridge_funcs"), [`drm_bridge_funcs.get_modes`](#c.drm_bridge_funcs "drm_bridge_funcs"),
  [`drm_bridge_funcs.get_edid`](#c.drm_bridge_funcs "drm_bridge_funcs"), [`drm_bridge_funcs.hpd_notify`](#c.drm_bridge_funcs "drm_bridge_funcs"),
  [`drm_bridge_funcs.hpd_enable`](#c.drm_bridge_funcs "drm_bridge_funcs") and [`drm_bridge_funcs.hpd_disable`](#c.drm_bridge_funcs "drm_bridge_funcs"). When
  implemented, display drivers shall create a [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector") instance for
  each chain of bridges, and implement those connector instances based on
  the bridge connector operations.

  Bridge drivers shall implement the connector-related operations for all
  the features that the bridge hardware support. For instance, if a bridge
  supports reading EDID, the [`drm_bridge_funcs.get_edid`](#c.drm_bridge_funcs "drm_bridge_funcs") shall be
  implemented. This however doesn’t mean that the DDC lines are wired to the
  bridge on a particular platform, as they could also be connected to an I2C
  controller of the SoC. Support for the connector-related operations on the
  running platform is reported through the [`drm_bridge.ops`](#c.drm_bridge "drm_bridge") flags. Bridge
  drivers shall detect which operations they can support on the platform
  (usually this information is provided by ACPI or DT), and set the
  [`drm_bridge.ops`](#c.drm_bridge "drm_bridge") flags for all supported operations. A flag shall only be
  set if the corresponding [`drm_bridge_funcs`](#c.drm_bridge_funcs "drm_bridge_funcs") operation is implemented, but
  an implemented operation doesn’t necessarily imply that the corresponding
  flag will be set. Display drivers shall use the [`drm_bridge.ops`](#c.drm_bridge "drm_bridge") flags to
  decide which bridge to delegate a connector operation to. This mechanism
  allows providing a single static const [`drm_bridge_funcs`](#c.drm_bridge_funcs "drm_bridge_funcs") instance in
  bridge drivers, improving security by storing function pointers in
  read-only memory.

  In order to ease transition, bridge drivers may support both the old and
  new models by making connector creation optional and implementing the
  connected-related bridge operations. Connector creation is then controlled
  by the flags argument to the [`drm_bridge_attach()`](#c.drm_bridge_attach "drm_bridge_attach") function. Display drivers
  that support the new model and create connectors themselves shall set the
  `DRM_BRIDGE_ATTACH_NO_CONNECTOR` flag, and bridge drivers shall then skip
  connector creation. For intermediate bridges in the chain, the flag shall
  be passed to the [`drm_bridge_attach()`](#c.drm_bridge_attach "drm_bridge_attach") call for the downstream bridge.
  Bridge drivers that implement the new model only shall return an error
  from their [`drm_bridge_funcs.attach`](#c.drm_bridge_funcs "drm_bridge_funcs") handler when the
  `DRM_BRIDGE_ATTACH_NO_CONNECTOR` flag is not set. New display drivers
  should use the new model, and convert the bridge drivers they use if
  needed, in order to gradually transition to the new model.

### Bridge Connector Helper

The DRM bridge connector helper object provides a DRM connector
implementation that wraps a chain of [`struct drm_bridge`](#c.drm_bridge "drm_bridge"). The connector
operations are fully implemented based on the operations of the bridges in
the chain, and don’t require any intervention from the display controller
driver at runtime.

To use the helper, display controller drivers create a bridge connector with
a call to [`drm_bridge_connector_init()`](#c.drm_bridge_connector_init "drm_bridge_connector_init"). This associates the newly created
connector with the chain of bridges passed to the function and registers it
with the DRM device. At that point the connector becomes fully usable, no
further operation is needed.

The DRM bridge connector operations are implemented based on the operations
provided by the bridges in the chain. Each connector operation is delegated
to the bridge closest to the connector (at the end of the chain) that
provides the relevant functionality.

To make use of this helper, all bridges in the chain shall report bridge
operation flags ([`drm_bridge->ops`](#c.drm_bridge "drm_bridge")) and bridge output type
([`drm_bridge->type`](#c.drm_bridge "drm_bridge")), as well as the DRM\_BRIDGE\_ATTACH\_NO\_CONNECTOR attach
flag (none of the bridges shall create a DRM connector directly).

### Bridge Helper Reference

enum drm\_bridge\_attach\_flags
:   Flags for [`drm_bridge_funcs.attach`](#c.drm_bridge_funcs "drm_bridge_funcs")

**Constants**

`DRM_BRIDGE_ATTACH_NO_CONNECTOR`
:   When this flag is set the bridge
    shall not create a drm\_connector.

struct drm\_bridge\_funcs
:   drm\_bridge control functions

**Definition**:

```
struct drm_bridge_funcs {
    int (*attach)(struct drm_bridge *bridge, struct drm_encoder *encoder, enum drm_bridge_attach_flags flags);
    void (*destroy)(struct drm_bridge *bridge);
    void (*detach)(struct drm_bridge *bridge);
    enum drm_mode_status (*mode_valid)(struct drm_bridge *bridge, const struct drm_display_info *info, const struct drm_display_mode *mode);
    bool (*mode_fixup)(struct drm_bridge *bridge, const struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode);
    void (*disable)(struct drm_bridge *bridge);
    void (*post_disable)(struct drm_bridge *bridge);
    void (*mode_set)(struct drm_bridge *bridge, const struct drm_display_mode *mode, const struct drm_display_mode *adjusted_mode);
    void (*pre_enable)(struct drm_bridge *bridge);
    void (*enable)(struct drm_bridge *bridge);
    void (*atomic_pre_enable)(struct drm_bridge *bridge, struct drm_atomic_state *state);
    void (*atomic_enable)(struct drm_bridge *bridge, struct drm_atomic_state *state);
    void (*atomic_disable)(struct drm_bridge *bridge, struct drm_atomic_state *state);
    void (*atomic_post_disable)(struct drm_bridge *bridge, struct drm_atomic_state *state);
    struct drm_bridge_state *(*atomic_duplicate_state)(struct drm_bridge *bridge);
    void (*atomic_destroy_state)(struct drm_bridge *bridge, struct drm_bridge_state *state);
    u32 *(*atomic_get_output_bus_fmts)(struct drm_bridge *bridge, struct drm_bridge_state *bridge_state, struct drm_crtc_state *crtc_state, struct drm_connector_state *conn_state, unsigned int *num_output_fmts);
    u32 *(*atomic_get_input_bus_fmts)(struct drm_bridge *bridge, struct drm_bridge_state *bridge_state, struct drm_crtc_state *crtc_state, struct drm_connector_state *conn_state, u32 output_fmt, unsigned int *num_input_fmts);
    int (*atomic_check)(struct drm_bridge *bridge, struct drm_bridge_state *bridge_state, struct drm_crtc_state *crtc_state, struct drm_connector_state *conn_state);
    struct drm_bridge_state *(*atomic_reset)(struct drm_bridge *bridge);
    enum drm_connector_status (*detect)(struct drm_bridge *bridge, struct drm_connector *connector);
    int (*get_modes)(struct drm_bridge *bridge, struct drm_connector *connector);
    const struct drm_edid *(*edid_read)(struct drm_bridge *bridge, struct drm_connector *connector);
    void (*hpd_notify)(struct drm_bridge *bridge, struct drm_connector *connector, enum drm_connector_status status);
    void (*hpd_enable)(struct drm_bridge *bridge);
    void (*hpd_disable)(struct drm_bridge *bridge);
    enum drm_mode_status (*hdmi_tmds_char_rate_valid)(const struct drm_bridge *bridge, const struct drm_display_mode *mode, unsigned long long tmds_rate);
    int (*hdmi_clear_avi_infoframe)(struct drm_bridge *bridge);
    int (*hdmi_write_avi_infoframe)(struct drm_bridge *bridge, const u8 *buffer, size_t len);
    int (*hdmi_clear_hdmi_infoframe)(struct drm_bridge *bridge);
    int (*hdmi_write_hdmi_infoframe)(struct drm_bridge *bridge, const u8 *buffer, size_t len);
    int (*hdmi_clear_hdr_drm_infoframe)(struct drm_bridge *bridge);
    int (*hdmi_write_hdr_drm_infoframe)(struct drm_bridge *bridge, const u8 *buffer, size_t len);
    int (*hdmi_clear_spd_infoframe)(struct drm_bridge *bridge);
    int (*hdmi_write_spd_infoframe)(struct drm_bridge *bridge, const u8 *buffer, size_t len);
    int (*hdmi_clear_audio_infoframe)(struct drm_bridge *bridge);
    int (*hdmi_write_audio_infoframe)(struct drm_bridge *bridge, const u8 *buffer, size_t len);
    int (*hdmi_audio_startup)(struct drm_bridge *bridge, struct drm_connector *connector);
    int (*hdmi_audio_prepare)(struct drm_bridge *bridge, struct drm_connector *connector, struct hdmi_codec_daifmt *fmt, struct hdmi_codec_params *hparms);
    void (*hdmi_audio_shutdown)(struct drm_bridge *bridge, struct drm_connector *connector);
    int (*hdmi_audio_mute_stream)(struct drm_bridge *bridge, struct drm_connector *connector, bool enable, int direction);
    int (*hdmi_cec_init)(struct drm_bridge *bridge, struct drm_connector *connector);
    int (*hdmi_cec_enable)(struct drm_bridge *bridge, bool enable);
    int (*hdmi_cec_log_addr)(struct drm_bridge *bridge, u8 logical_addr);
    int (*hdmi_cec_transmit)(struct drm_bridge *bridge, u8 attempts, u32 signal_free_time, struct cec_msg *msg);
    int (*dp_audio_startup)(struct drm_bridge *bridge, struct drm_connector *connector);
    int (*dp_audio_prepare)(struct drm_bridge *bridge, struct drm_connector *connector, struct hdmi_codec_daifmt *fmt, struct hdmi_codec_params *hparms);
    void (*dp_audio_shutdown)(struct drm_bridge *bridge, struct drm_connector *connector);
    int (*dp_audio_mute_stream)(struct drm_bridge *bridge, struct drm_connector *connector, bool enable, int direction);
    void (*debugfs_init)(struct drm_bridge *bridge, struct dentry *root);
};
```

**Members**

`attach`
:   This callback is invoked whenever our bridge is being attached to a
    [`drm_encoder`](drm-kms.html#c.drm_encoder "drm_encoder"). The flags argument tunes the behaviour of the attach
    operation (see DRM\_BRIDGE\_ATTACH\_\*).

    The **attach** callback is optional.

    RETURNS:

    Zero on success, error code on failure.

`destroy`
:   This callback is invoked when the bridge is about to be
    deallocated.

    The **destroy** callback is optional.

`detach`
:   This callback is invoked whenever our bridge is being detached from a
    [`drm_encoder`](drm-kms.html#c.drm_encoder "drm_encoder").

    The **detach** callback is optional.

`mode_valid`
:   This callback is used to check if a specific mode is valid in this
    bridge. This should be implemented if the bridge has some sort of
    restriction in the modes it can display. For example, a given bridge
    may be responsible to set a clock value. If the clock can not
    produce all the values for the available modes then this callback
    can be used to restrict the number of modes to only the ones that
    can be displayed.

    This hook is used by the probe helpers to filter the mode list in
    [`drm_helper_probe_single_connector_modes()`](#c.drm_helper_probe_single_connector_modes "drm_helper_probe_single_connector_modes"), and it is used by the
    atomic helpers to validate modes supplied by userspace in
    [`drm_atomic_helper_check_modeset()`](#c.drm_atomic_helper_check_modeset "drm_atomic_helper_check_modeset").

    The **mode\_valid** callback is optional.

    NOTE:

    Since this function is both called from the check phase of an atomic
    commit, and the mode validation in the probe paths it is not allowed
    to look at anything else but the passed-in mode, and validate it
    against configuration-invariant hardware constraints. Any further
    limits which depend upon the configuration can only be checked in
    **mode\_fixup**.

    RETURNS:

    drm\_mode\_status Enum

`mode_fixup`
:   This callback is used to validate and adjust a mode. The parameter
    mode is the display mode that should be fed to the next element in
    the display chain, either the final [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector") or the next
    [`drm_bridge`](#c.drm_bridge "drm_bridge"). The parameter adjusted\_mode is the input mode the bridge
    requires. It can be modified by this callback and does not need to
    match mode. See also [`drm_crtc_state.adjusted_mode`](drm-kms.html#c.drm_crtc_state "drm_crtc_state") for more details.

    This is the only hook that allows a bridge to reject a modeset. If
    this function passes all other callbacks must succeed for this
    configuration.

    The mode\_fixup callback is optional. [`drm_bridge_funcs.mode_fixup()`](#c.drm_bridge_funcs "drm_bridge_funcs")
    is not called when [`drm_bridge_funcs.atomic_check()`](#c.drm_bridge_funcs "drm_bridge_funcs") is implemented,
    so only one of them should be provided.

    NOTE:

    This function is called in the check phase of atomic modesets, which
    can be aborted for any reason (including on userspace’s request to
    just check whether a configuration would be possible). Drivers MUST
    NOT touch any persistent state (hardware or software) or data
    structures except the passed in **state** parameter.

    Also beware that userspace can request its own custom modes, neither
    core nor helpers filter modes to the list of probe modes reported by
    the GETCONNECTOR IOCTL and stored in [`drm_connector.modes`](drm-kms.html#c.drm_connector "drm_connector"). To ensure
    that modes are filtered consistently put any bridge constraints and
    limits checks into **mode\_valid**.

    RETURNS:

    True if an acceptable configuration is possible, false if the modeset
    operation should be rejected.

`disable`
:   This callback should disable the bridge. It is called right before
    the preceding element in the display pipe is disabled. If the
    preceding element is a bridge this means it’s called before that
    bridge’s **disable** vfunc. If the preceding element is a [`drm_encoder`](drm-kms.html#c.drm_encoder "drm_encoder")
    it’s called right before the [`drm_encoder_helper_funcs.disable`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs"),
    [`drm_encoder_helper_funcs.prepare`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") or [`drm_encoder_helper_funcs.dpms`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs")
    hook.

    The bridge can assume that the display pipe (i.e. clocks and timing
    signals) feeding it is still running when this callback is called.

    The **disable** callback is optional.

    NOTE:

    This is deprecated, do not use!
    New drivers shall use [`drm_bridge_funcs.atomic_disable`](#c.drm_bridge_funcs "drm_bridge_funcs").

`post_disable`
:   This callback should disable the bridge. It is called right after the
    preceding element in the display pipe is disabled. If the preceding
    element is a bridge this means it’s called after that bridge’s
    **post\_disable** function. If the preceding element is a [`drm_encoder`](drm-kms.html#c.drm_encoder "drm_encoder")
    it’s called right after the encoder’s
    [`drm_encoder_helper_funcs.disable`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs"), [`drm_encoder_helper_funcs.prepare`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs")
    or [`drm_encoder_helper_funcs.dpms`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") hook.

    The bridge must assume that the display pipe (i.e. clocks and timing
    signals) feeding it is no longer running when this callback is
    called.

    The **post\_disable** callback is optional.

    NOTE:

    This is deprecated, do not use!
    New drivers shall use [`drm_bridge_funcs.atomic_post_disable`](#c.drm_bridge_funcs "drm_bridge_funcs").

`mode_set`
:   This callback should set the given mode on the bridge. It is called
    after the **mode\_set** callback for the preceding element in the display
    pipeline has been called already. If the bridge is the first element
    then this would be [`drm_encoder_helper_funcs.mode_set`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs"). The display
    pipe (i.e. clocks and timing signals) is off when this function is
    called.

    The adjusted\_mode parameter is the mode output by the CRTC for the
    first bridge in the chain. It can be different from the mode
    parameter that contains the desired mode for the connector at the end
    of the bridges chain, for instance when the first bridge in the chain
    performs scaling. The adjusted mode is mostly useful for the first
    bridge in the chain and is likely irrelevant for the other bridges.

    For atomic drivers the adjusted\_mode is the mode stored in
    [`drm_crtc_state.adjusted_mode`](drm-kms.html#c.drm_crtc_state "drm_crtc_state").

    NOTE:

    This is deprecated, do not use!
    New drivers shall set their mode in the
    [`drm_bridge_funcs.atomic_enable`](#c.drm_bridge_funcs "drm_bridge_funcs") operation.

`pre_enable`
:   This callback should enable the bridge. It is called right before
    the preceding element in the display pipe is enabled. If the
    preceding element is a bridge this means it’s called before that
    bridge’s **pre\_enable** function. If the preceding element is a
    [`drm_encoder`](drm-kms.html#c.drm_encoder "drm_encoder") it’s called right before the encoder’s
    [`drm_encoder_helper_funcs.enable`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs"), [`drm_encoder_helper_funcs.commit`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") or
    [`drm_encoder_helper_funcs.dpms`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") hook.

    The display pipe (i.e. clocks and timing signals) feeding this bridge
    will not yet be running when this callback is called. The bridge must
    not enable the display link feeding the next bridge in the chain (if
    there is one) when this callback is called.

    The **pre\_enable** callback is optional.

    NOTE:

    This is deprecated, do not use!
    New drivers shall use [`drm_bridge_funcs.atomic_pre_enable`](#c.drm_bridge_funcs "drm_bridge_funcs").

`enable`
:   This callback should enable the bridge. It is called right after
    the preceding element in the display pipe is enabled. If the
    preceding element is a bridge this means it’s called after that
    bridge’s **enable** function. If the preceding element is a
    [`drm_encoder`](drm-kms.html#c.drm_encoder "drm_encoder") it’s called right after the encoder’s
    [`drm_encoder_helper_funcs.enable`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs"), [`drm_encoder_helper_funcs.commit`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") or
    [`drm_encoder_helper_funcs.dpms`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") hook.

    The bridge can assume that the display pipe (i.e. clocks and timing
    signals) feeding it is running when this callback is called. This
    callback must enable the display link feeding the next bridge in the
    chain if there is one.

    The **enable** callback is optional.

    NOTE:

    This is deprecated, do not use!
    New drivers shall use [`drm_bridge_funcs.atomic_enable`](#c.drm_bridge_funcs "drm_bridge_funcs").

`atomic_pre_enable`
:   This callback should enable the bridge. It is called right before
    the preceding element in the display pipe is enabled. If the
    preceding element is a bridge this means it’s called before that
    bridge’s **atomic\_pre\_enable** or **pre\_enable** function. If the preceding
    element is a [`drm_encoder`](drm-kms.html#c.drm_encoder "drm_encoder") it’s called right before the encoder’s
    [`drm_encoder_helper_funcs.atomic_enable`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") hook.

    The display pipe (i.e. clocks and timing signals) feeding this bridge
    will not yet be running when this callback is called. The bridge must
    not enable the display link feeding the next bridge in the chain (if
    there is one) when this callback is called.

    The **atomic\_pre\_enable** callback is optional.

`atomic_enable`
:   This callback should enable the bridge. It is called right after
    the preceding element in the display pipe is enabled. If the
    preceding element is a bridge this means it’s called after that
    bridge’s **atomic\_enable** or **enable** function. If the preceding element
    is a [`drm_encoder`](drm-kms.html#c.drm_encoder "drm_encoder") it’s called right after the encoder’s
    [`drm_encoder_helper_funcs.atomic_enable`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") hook.

    The bridge can assume that the display pipe (i.e. clocks and timing
    signals) feeding it is running when this callback is called. This
    callback must enable the display link feeding the next bridge in the
    chain if there is one.

    The **atomic\_enable** callback is optional.

`atomic_disable`
:   This callback should disable the bridge. It is called right before
    the preceding element in the display pipe is disabled. If the
    preceding element is a bridge this means it’s called before that
    bridge’s **atomic\_disable** or **disable** vfunc. If the preceding element
    is a [`drm_encoder`](drm-kms.html#c.drm_encoder "drm_encoder") it’s called right before the
    [`drm_encoder_helper_funcs.atomic_disable`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") hook.

    The bridge can assume that the display pipe (i.e. clocks and timing
    signals) feeding it is still running when this callback is called.

    The **atomic\_disable** callback is optional.

`atomic_post_disable`
:   This callback should disable the bridge. It is called right after the
    preceding element in the display pipe is disabled. If the preceding
    element is a bridge this means it’s called after that bridge’s
    **atomic\_post\_disable** or **post\_disable** function. If the preceding
    element is a [`drm_encoder`](drm-kms.html#c.drm_encoder "drm_encoder") it’s called right after the encoder’s
    [`drm_encoder_helper_funcs.atomic_disable`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") hook.

    The bridge must assume that the display pipe (i.e. clocks and timing
    signals) feeding it is no longer running when this callback is
    called.

    The **atomic\_post\_disable** callback is optional.

`atomic_duplicate_state`
:   Duplicate the current bridge state object (which is guaranteed to be
    non-NULL).

    The atomic\_duplicate\_state hook is mandatory if the bridge
    implements any of the atomic hooks, and should be left unassigned
    otherwise. For bridges that don’t subclass [`drm_bridge_state`](drm-kms.html#c.drm_bridge_state "drm_bridge_state"), the
    [`drm_atomic_helper_bridge_duplicate_state()`](#c.drm_atomic_helper_bridge_duplicate_state "drm_atomic_helper_bridge_duplicate_state") helper function shall be
    used to implement this hook.

    RETURNS:
    A valid drm\_bridge\_state object or NULL if the allocation fails.

`atomic_destroy_state`
:   Destroy a bridge state object previously allocated by
    [`drm_bridge_funcs.atomic_duplicate_state()`](#c.drm_bridge_funcs "drm_bridge_funcs").

    The atomic\_destroy\_state hook is mandatory if the bridge implements
    any of the atomic hooks, and should be left unassigned otherwise.
    For bridges that don’t subclass [`drm_bridge_state`](drm-kms.html#c.drm_bridge_state "drm_bridge_state"), the
    [`drm_atomic_helper_bridge_destroy_state()`](#c.drm_atomic_helper_bridge_destroy_state "drm_atomic_helper_bridge_destroy_state") helper function shall be
    used to implement this hook.

`atomic_get_output_bus_fmts`
:   Return the supported bus formats on the output end of a bridge.
    The returned array must be allocated with [`kmalloc()`](../core-api/mm-api.html#c.kmalloc "kmalloc") and will be
    freed by the caller. If the allocation fails, NULL should be
    returned. num\_output\_fmts must be set to the returned array size.
    Formats listed in the returned array should be listed in decreasing
    preference order (the core will try all formats until it finds one
    that works).

    This method is only called on the last element of the bridge chain
    as part of the bus format negotiation process that happens in
    [`` drm_atomic_bridge_chain_select_bus_fmts`().
    This method is optional. When not implemented, the core will
    fall back to :c:type:`drm_connector.display_info ``](drm-kms.html#c.drm_connector "drm_connector").bus\_formats[0] if
    [`drm_connector.display_info`](drm-kms.html#c.drm_connector "drm_connector").num\_bus\_formats > 0,
    or to MEDIA\_BUS\_FMT\_FIXED otherwise.

`atomic_get_input_bus_fmts`
:   Return the supported bus formats on the input end of a bridge for
    a specific output bus format.

    The returned array must be allocated with [`kmalloc()`](../core-api/mm-api.html#c.kmalloc "kmalloc") and will be
    freed by the caller. If the allocation fails, NULL should be
    returned. num\_input\_fmts must be set to the returned array size.
    Formats listed in the returned array should be listed in decreasing
    preference order (the core will try all formats until it finds one
    that works). When the format is not supported NULL should be
    returned and num\_input\_fmts should be set to 0.

    This method is called on all elements of the bridge chain as part of
    the bus format negotiation process that happens in
    `drm_atomic_bridge_chain_select_bus_fmts()`.
    This method is optional. When not implemented, the core will bypass
    bus format negotiation on this element of the bridge without
    failing, and the previous element in the chain will be passed
    MEDIA\_BUS\_FMT\_FIXED as its output bus format.

    Bridge drivers that need to support being linked to bridges that are
    not supporting bus format negotiation should handle the
    output\_fmt == MEDIA\_BUS\_FMT\_FIXED case appropriately, by selecting a
    sensible default value or extracting this information from somewhere
    else (FW property, [`drm_display_mode`](drm-kms.html#c.drm_display_mode "drm_display_mode"), [`drm_display_info`](drm-kms.html#c.drm_display_info "drm_display_info"), ...)

    Note: Even if input format selection on the first bridge has no
    impact on the negotiation process (bus format negotiation stops once
    we reach the first element of the chain), drivers are expected to
    return accurate input formats as the input format may be used to
    configure the CRTC output appropriately.

`atomic_check`
:   This method is responsible for checking bridge state correctness.
    It can also check the state of the surrounding components in chain
    to make sure the whole pipeline can work properly.

    [`drm_bridge_funcs.atomic_check()`](#c.drm_bridge_funcs "drm_bridge_funcs") hooks are called in reverse
    order (from the last to the first bridge).

    This method is optional. [`drm_bridge_funcs.mode_fixup()`](#c.drm_bridge_funcs "drm_bridge_funcs") is not
    called when [`drm_bridge_funcs.atomic_check()`](#c.drm_bridge_funcs "drm_bridge_funcs") is implemented, so only
    one of them should be provided.

    If drivers need to tweak [`drm_bridge_state.input_bus_cfg`](drm-kms.html#c.drm_bridge_state "drm_bridge_state").flags or
    [`drm_bridge_state.output_bus_cfg`](drm-kms.html#c.drm_bridge_state "drm_bridge_state").flags it should happen in
    this function. By default the [`drm_bridge_state.output_bus_cfg`](drm-kms.html#c.drm_bridge_state "drm_bridge_state").flags
    field is set to the next bridge
    [`drm_bridge_state.input_bus_cfg`](drm-kms.html#c.drm_bridge_state "drm_bridge_state").flags value or
    [`drm_connector.display_info`](drm-kms.html#c.drm_connector "drm_connector").bus\_flags if the bridge is the last
    element in the chain.

    RETURNS:
    zero if the check passed, a negative error code otherwise.

`atomic_reset`
:   Reset the bridge to a predefined state (or retrieve its current
    state) and return a [`drm_bridge_state`](drm-kms.html#c.drm_bridge_state "drm_bridge_state") object matching this state.
    This function is called at attach time.

    The atomic\_reset hook is mandatory if the bridge implements any of
    the atomic hooks, and should be left unassigned otherwise. For
    bridges that don’t subclass [`drm_bridge_state`](drm-kms.html#c.drm_bridge_state "drm_bridge_state"), the
    [`drm_atomic_helper_bridge_reset()`](#c.drm_atomic_helper_bridge_reset "drm_atomic_helper_bridge_reset") helper function shall be used to
    implement this hook.

    Note that the `atomic_reset()` semantics is not exactly matching the
    `reset()` semantics found on other components (connector, plane, ...).

    1. The reset operation happens when the bridge is attached, not when
       [`drm_mode_config_reset()`](drm-kms.html#c.drm_mode_config_reset "drm_mode_config_reset") is called
    2. It’s meant to be used exclusively on bridges that have been
       converted to the ATOMIC API

    RETURNS:
    A valid drm\_bridge\_state object in case of success, an [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR")
    giving the reason of the failure otherwise.

`detect`
:   Check if anything is attached to the bridge output.

    This callback is optional, if not implemented the bridge will be
    considered as always having a component attached to its output.
    Bridges that implement this callback shall set the
    DRM\_BRIDGE\_OP\_DETECT flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

    RETURNS:

    drm\_connector\_status indicating the bridge output status.

`get_modes`
:   Fill all modes currently valid for the sink into the [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector")
    with [`drm_mode_probed_add()`](drm-kms.html#c.drm_mode_probed_add "drm_mode_probed_add").

    The **get\_modes** callback is mostly intended to support non-probeable
    displays such as many fixed panels. Bridges that support reading
    EDID shall leave **get\_modes** unimplemented and implement the
    [`drm_bridge_funcs->edid_read`](#c.drm_bridge_funcs "drm_bridge_funcs") callback instead.

    This callback is optional. Bridges that implement it shall set the
    DRM\_BRIDGE\_OP\_MODES flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

    The connector parameter shall be used for the sole purpose of
    filling modes, and shall not be stored internally by bridge drivers
    for future usage.

    RETURNS:

    The number of modes added by calling [`drm_mode_probed_add()`](drm-kms.html#c.drm_mode_probed_add "drm_mode_probed_add").

`edid_read`
:   Read the EDID data of the connected display.

    The **edid\_read** callback is the preferred way of reporting mode
    information for a display connected to the bridge output. Bridges
    that support reading EDID shall implement this callback and leave
    the **get\_modes** callback unimplemented.

    The caller of this operation shall first verify the output
    connection status and refrain from reading EDID from a disconnected
    output.

    This callback is optional. Bridges that implement it shall set the
    DRM\_BRIDGE\_OP\_EDID flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

    The connector parameter shall be used for the sole purpose of EDID
    retrieval, and shall not be stored internally by bridge drivers for
    future usage.

    RETURNS:

    An edid structure newly allocated with [`drm_edid_alloc()`](#c.drm_edid_alloc "drm_edid_alloc") or returned
    from [`drm_edid_read()`](#c.drm_edid_read "drm_edid_read") family of functions on success, or NULL
    otherwise. The caller is responsible for freeing the returned edid
    structure with [`drm_edid_free()`](#c.drm_edid_free "drm_edid_free").

`hpd_notify`
:   Notify the bridge of hot plug detection.

    This callback is optional, it may be implemented by bridges that
    need to be notified of display connection or disconnection for
    internal reasons. One use case is to reset the internal state of CEC
    controllers for HDMI bridges.

`hpd_enable`
:   Enable hot plug detection. From now on the bridge shall call
    [`drm_bridge_hpd_notify()`](#c.drm_bridge_hpd_notify "drm_bridge_hpd_notify") each time a change is detected in the output
    connection status, until hot plug detection gets disabled with
    **hpd\_disable**.

    This callback is optional and shall only be implemented by bridges
    that support hot-plug notification without polling. Bridges that
    implement it shall also implement the **hpd\_disable** callback and set
    the DRM\_BRIDGE\_OP\_HPD flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

`hpd_disable`
:   Disable hot plug detection. Once this function returns the bridge
    shall not call [`drm_bridge_hpd_notify()`](#c.drm_bridge_hpd_notify "drm_bridge_hpd_notify") when a change in the output
    connection status occurs.

    This callback is optional and shall only be implemented by bridges
    that support hot-plug notification without polling. Bridges that
    implement it shall also implement the **hpd\_enable** callback and set
    the DRM\_BRIDGE\_OP\_HPD flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

`hdmi_tmds_char_rate_valid`
:   Check whether a particular TMDS character rate is supported by the
    driver.

    This callback is optional and should only be implemented by the
    bridges that take part in the HDMI connector implementation. Bridges
    that implement it shall set the DRM\_BRIDGE\_OP\_HDMI flag in their
    [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

    Returns:

    Either [`drm_mode_status.MODE_OK`](drm-kms.html#c.drm_mode_status "drm_mode_status") or one of the failure reasons
    in [`enum drm_mode_status`](drm-kms.html#c.drm_mode_status "drm_mode_status").

`hdmi_clear_avi_infoframe`
:   This callback clears the infoframes in the hardware during commit.

    This callback is optional but it must be implemented by bridges that
    set the DRM\_BRIDGE\_OP\_HDMI flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

`hdmi_write_avi_infoframe`
:   Program the infoframe into the hardware.

    This callback is optional but it must be implemented by bridges that
    set the DRM\_BRIDGE\_OP\_HDMI flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

`hdmi_clear_hdmi_infoframe`
:   This callback clears the infoframes in the hardware during commit.

    This callback is optional but it must be implemented by bridges that
    set the DRM\_BRIDGE\_OP\_HDMI flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

`hdmi_write_hdmi_infoframe`
:   Program the infoframe into the hardware.

    This callback is optional but it must be implemented by bridges that
    set the DRM\_BRIDGE\_OP\_HDMI flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

`hdmi_clear_hdr_drm_infoframe`
:   This callback clears the infoframes in the hardware during commit.

    This callback is optional but it must be implemented by bridges that
    set the DRM\_BRIDGE\_OP\_HDMI\_HDR\_DRM\_INFOFRAME flag in their
    [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

`hdmi_write_hdr_drm_infoframe`
:   Program the infoframe into the hardware.

    This callback is optional but it must be implemented by bridges that
    set the DRM\_BRIDGE\_OP\_HDMI\_HDR\_DRM\_INFOFRAME flag in their
    [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

`hdmi_clear_spd_infoframe`
:   This callback clears the infoframes in the hardware during commit.

    This callback is optional but it must be implemented by bridges that
    set the DRM\_BRIDGE\_OP\_HDMI\_SPD\_INFOFRAME flag in their
    [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

`hdmi_write_spd_infoframe`
:   Program the infoframe into the hardware.

    This callback is optional but it must be implemented by bridges that
    set the DRM\_BRIDGE\_OP\_HDMI\_SPD\_INFOFRAME flag in their
    [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

`hdmi_clear_audio_infoframe`
:   This callback clears the infoframes in the hardware during commit.

    This callback is optional but it must be implemented by bridges that
    set the DRM\_BRIDGE\_OP\_HDMI\_AUDIO flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

`hdmi_write_audio_infoframe`
:   Program the infoframe into the hardware.

    This callback is optional but it must be implemented by bridges that
    set the DRM\_BRIDGE\_OP\_HDMI\_AUDIO flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

`hdmi_audio_startup`
:   Called when ASoC starts an audio stream setup.

    This callback is optional, it can be implemented by bridges that
    set the **DRM\_BRIDGE\_OP\_HDMI\_AUDIO** flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

    Returns:
    0 on success, a negative error code otherwise

`hdmi_audio_prepare`
:   Configures HDMI-encoder for audio stream. Can be called multiple
    times for each setup.

    This callback is optional but it must be implemented by bridges that
    set the **DRM\_BRIDGE\_OP\_HDMI\_AUDIO** flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

    Returns:
    0 on success, a negative error code otherwise

`hdmi_audio_shutdown`
:   Shut down the audio stream.

    This callback is optional but it must be implemented by bridges that
    set the **DRM\_BRIDGE\_OP\_HDMI\_AUDIO** flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

    Returns:
    0 on success, a negative error code otherwise

`hdmi_audio_mute_stream`
:   Mute/unmute HDMI audio stream.

    This callback is optional, it can be implemented by bridges that
    set the **DRM\_BRIDGE\_OP\_HDMI\_AUDIO** flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

    Returns:
    0 on success, a negative error code otherwise

`hdmi_cec_init`
:   Initialize CEC part of the bridge.

    This callback is optional, it can be implemented by bridges that
    set the **DRM\_BRIDGE\_OP\_HDMI\_CEC\_ADAPTER** flag in their
    [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

    Returns:
    0 on success, a negative error code otherwise

`hdmi_cec_enable`
:   Enable or disable the CEC adapter inside the bridge.

    This callback is optional, it can be implemented by bridges that
    set the **DRM\_BRIDGE\_OP\_HDMI\_CEC\_ADAPTER** flag in their
    [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

    Returns:
    0 on success, a negative error code otherwise

`hdmi_cec_log_addr`
:   Set the logical address of the CEC adapter inside the bridge.

    This callback is optional, it can be implemented by bridges that
    set the **DRM\_BRIDGE\_OP\_HDMI\_CEC\_ADAPTER** flag in their
    [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

    Returns:
    0 on success, a negative error code otherwise

`hdmi_cec_transmit`
:   Transmit the message using the CEC adapter inside the bridge.

    This callback is optional, it can be implemented by bridges that
    set the **DRM\_BRIDGE\_OP\_HDMI\_CEC\_ADAPTER** flag in their
    [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

    Returns:
    0 on success, a negative error code otherwise

`dp_audio_startup`
:   Called when ASoC starts a DisplayPort audio stream setup.

    This callback is optional, it can be implemented by bridges that
    set the **DRM\_BRIDGE\_OP\_DP\_AUDIO** flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

    Returns:
    0 on success, a negative error code otherwise

`dp_audio_prepare`
:   Configures DisplayPort audio stream. Can be called multiple
    times for each setup.

    This callback is optional but it must be implemented by bridges that
    set the **DRM\_BRIDGE\_OP\_DP\_AUDIO** flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

    Returns:
    0 on success, a negative error code otherwise

`dp_audio_shutdown`
:   Shut down the DisplayPort audio stream.

    This callback is optional but it must be implemented by bridges that
    set the **DRM\_BRIDGE\_OP\_DP\_AUDIO** flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

    Returns:
    0 on success, a negative error code otherwise

`dp_audio_mute_stream`
:   Mute/unmute DisplayPort audio stream.

    This callback is optional, it can be implemented by bridges that
    set the **DRM\_BRIDGE\_OP\_DP\_AUDIO** flag in their [`drm_bridge->ops`](#c.drm_bridge "drm_bridge").

    Returns:
    0 on success, a negative error code otherwise

`debugfs_init`
:   Allows bridges to create bridge-specific debugfs files.

struct drm\_bridge\_timings
:   timing information for the bridge

**Definition**:

```
struct drm_bridge_timings {
    u32 input_bus_flags;
    u32 setup_time_ps;
    u32 hold_time_ps;
    bool dual_link;
};
```

**Members**

`input_bus_flags`
:   Tells what additional settings for the pixel data on the bus
    this bridge requires (like pixel signal polarity). See also
    [`drm_display_info->bus_flags`](drm-kms.html#c.drm_display_info "drm_display_info").

`setup_time_ps`
:   Defines the time in picoseconds the input data lines must be
    stable before the clock edge.

`hold_time_ps`
:   Defines the time in picoseconds taken for the bridge to sample the
    input signal after the clock edge.

`dual_link`
:   True if the bus operates in dual-link mode. The exact meaning is
    dependent on the bus type. For LVDS buses, this indicates that even-
    and odd-numbered pixels are received on separate links.

enum drm\_bridge\_ops
:   Bitmask of operations supported by the bridge

**Constants**

`DRM_BRIDGE_OP_DETECT`
:   The bridge can detect displays connected to
    its output. Bridges that set this flag shall implement the
    [`drm_bridge_funcs->detect`](#c.drm_bridge_funcs "drm_bridge_funcs") callback.

`DRM_BRIDGE_OP_EDID`
:   The bridge can retrieve the EDID of the display
    connected to its output. Bridges that set this flag shall implement
    the [`drm_bridge_funcs->edid_read`](#c.drm_bridge_funcs "drm_bridge_funcs") callback.

`DRM_BRIDGE_OP_HPD`
:   The bridge can detect hot-plug and hot-unplug
    without requiring polling. Bridges that set this flag shall
    implement the [`drm_bridge_funcs->hpd_enable`](#c.drm_bridge_funcs "drm_bridge_funcs") and
    [`drm_bridge_funcs->hpd_disable`](#c.drm_bridge_funcs "drm_bridge_funcs") callbacks if they support enabling
    and disabling hot-plug detection dynamically.

`DRM_BRIDGE_OP_MODES`
:   The bridge can retrieve the modes supported
    by the display at its output. This does not include reading EDID
    which is separately covered by **DRM\_BRIDGE\_OP\_EDID**. Bridges that set
    this flag shall implement the [`drm_bridge_funcs->get_modes`](#c.drm_bridge_funcs "drm_bridge_funcs") callback.

`DRM_BRIDGE_OP_HDMI`
:   The bridge provides HDMI connector operations,
    including infoframes support. Bridges that set this flag must
    provide HDMI-related information and implement the
    [`drm_bridge_funcs->clear_avi_infoframe`](#c.drm_bridge_funcs "drm_bridge_funcs"),
    [`drm_bridge_funcs->write_avi_infoframe`](#c.drm_bridge_funcs "drm_bridge_funcs"),
    [`drm_bridge_funcs->clear_hdmi_infoframe`](#c.drm_bridge_funcs "drm_bridge_funcs") and
    [`drm_bridge_funcs->write_hdmi_infoframe`](#c.drm_bridge_funcs "drm_bridge_funcs") callbacks.

    Note: currently there can be at most one bridge in a chain that sets
    this bit. This is to simplify corresponding glue code in connector
    drivers.

`DRM_BRIDGE_OP_HDMI_AUDIO`
:   The bridge provides HDMI audio operations.
    Bridges that set this flag must implement the
    [`drm_bridge_funcs->hdmi_audio_prepare`](#c.drm_bridge_funcs "drm_bridge_funcs") and
    [`drm_bridge_funcs->hdmi_audio_shutdown`](#c.drm_bridge_funcs "drm_bridge_funcs") callbacks.
    If the bridge implements **DRM\_BRIDGE\_OP\_HDMI**, it also must implement
    [`drm_bridge_funcs->hdmi_write_audio_infoframe`](#c.drm_bridge_funcs "drm_bridge_funcs") and
    [`drm_bridge_funcs->hdmi_cleaer_audio_infoframe`](#c.drm_bridge_funcs "drm_bridge_funcs") callbacks.

    Note: currently there can be at most one bridge in a chain that sets
    this bit. This is to simplify corresponding glue code in connector
    drivers. Also it is not possible to have a bridge in the chain that
    sets **DRM\_BRIDGE\_OP\_DP\_AUDIO** if there is a bridge that sets this
    flag.

`DRM_BRIDGE_OP_DP_AUDIO`
:   The bridge provides DisplayPort audio operations.
    Bridges that set this flag must implement the
    [`drm_bridge_funcs->dp_audio_prepare`](#c.drm_bridge_funcs "drm_bridge_funcs") and
    [`drm_bridge_funcs->dp_audio_shutdown`](#c.drm_bridge_funcs "drm_bridge_funcs") callbacks.

    Note: currently there can be at most one bridge in a chain that sets
    this bit. This is to simplify corresponding glue code in connector
    drivers. Also it is not possible to have a bridge in the chain that
    sets **DRM\_BRIDGE\_OP\_HDMI\_AUDIO** if there is a bridge that sets this
    flag.

`DRM_BRIDGE_OP_HDMI_CEC_NOTIFIER`
:   The bridge requires CEC notifier
    to be present.

`DRM_BRIDGE_OP_HDMI_CEC_ADAPTER`
:   The bridge requires CEC adapter
    to be present.

`DRM_BRIDGE_OP_HDMI_HDR_DRM_INFOFRAME`
:   The bridge supports
    [`drm_bridge_funcs->hdmi_write_hdr_drm_infoframe`](#c.drm_bridge_funcs "drm_bridge_funcs") and
    [`drm_bridge_funcs->hdmi_clear_hdr_drm_infoframe`](#c.drm_bridge_funcs "drm_bridge_funcs") callbacks.

`DRM_BRIDGE_OP_HDMI_SPD_INFOFRAME`
:   The bridge supports
    [`drm_bridge_funcs->hdmi_write_spd_infoframe`](#c.drm_bridge_funcs "drm_bridge_funcs") and
    [`drm_bridge_funcs->hdmi_clear_spd_infoframe`](#c.drm_bridge_funcs "drm_bridge_funcs") callbacks.

struct drm\_bridge
:   central DRM bridge control structure

**Definition**:

```
struct drm_bridge {
    struct drm_private_obj base;
    struct drm_device *dev;
    struct drm_encoder *encoder;
    struct list_head chain_node;
    struct device_node *of_node;
    struct list_head list;
    const struct drm_bridge_timings *timings;
    const struct drm_bridge_funcs *funcs;
    void *container;
    struct kref refcount;
    bool unplugged;
    void *driver_private;
    enum drm_bridge_ops ops;
    int type;
    bool interlace_allowed;
    bool ycbcr_420_allowed;
    bool pre_enable_prev_first;
    bool support_hdcp;
    struct i2c_adapter *ddc;
    const char *vendor;
    const char *product;
    unsigned int supported_formats;
    unsigned int max_bpc;
    struct device *hdmi_cec_dev;
    struct device *hdmi_audio_dev;
    int hdmi_audio_max_i2s_playback_channels;
    u64 hdmi_audio_i2s_formats;
    unsigned int hdmi_audio_spdif_playback : 1;
    int hdmi_audio_dai_port;
    const char *hdmi_cec_adapter_name;
    u8 hdmi_cec_available_las;
    struct mutex hpd_mutex;
    void (*hpd_cb)(void *data, enum drm_connector_status status);
    void *hpd_data;
    struct drm_bridge *next_bridge;
};
```

**Members**

`base`
:   inherit from `drm_private_object`

`dev`
:   DRM device this bridge belongs to

`encoder`
:   encoder to which this bridge is connected

`chain_node`
:   used to form a bridge chain

`of_node`
:   device node pointer to the bridge

`list`
:   to keep track of all added bridges

`timings`
:   the timing specification for the bridge, if any (may be NULL)

`funcs`
:   control functions

`container`
:   Pointer to the private driver `struct embedding` this
    **struct** drm\_bridge.

`refcount`
:   reference count of users referencing this bridge.

`unplugged`
:   Flag to tell if the bridge has been unplugged.
    See [`drm_bridge_enter()`](#c.drm_bridge_enter "drm_bridge_enter") and [`drm_bridge_unplug()`](#c.drm_bridge_unplug "drm_bridge_unplug").

`driver_private`
:   pointer to the bridge driver’s internal context

`ops`
:   bitmask of operations supported by the bridge

`type`
:   Type of the connection at the bridge output
    (DRM\_MODE\_CONNECTOR\_\*). For bridges at the end of this chain this
    identifies the type of connected display.

`interlace_allowed`
:   Indicate that the bridge can handle interlaced
    modes.

`ycbcr_420_allowed`
:   Indicate that the bridge can handle YCbCr 420
    output.

`pre_enable_prev_first`
:   The bridge requires that the prev
    bridge **pre\_enable** function is called before its **pre\_enable**,
    and conversely for post\_disable. This is most frequently a
    requirement for DSI devices which need the host to be initialised
    before the peripheral.

`support_hdcp`
:   Indicate that the bridge supports HDCP.

`ddc`
:   Associated I2C adapter for DDC access, if any.

`vendor`
:   Vendor of the product to be used for the SPD InfoFrame
    generation. This is required if **DRM\_BRIDGE\_OP\_HDMI** is set.

`product`
:   Name of the product to be used for the SPD InfoFrame
    generation. This is required if **DRM\_BRIDGE\_OP\_HDMI** is set.

`supported_formats`
:   Bitmask of **drm\_output\_color\_format** listing
    supported output formats. This is only relevant if
    **DRM\_BRIDGE\_OP\_HDMI** is set.

`max_bpc`
:   Maximum bits per char the HDMI bridge supports. Allowed
    values are 8, 10 and 12. This is only relevant if
    **DRM\_BRIDGE\_OP\_HDMI** is set.

`hdmi_cec_dev`
:   device to be used as a containing device for CEC
    functions.

`hdmi_audio_dev`
:   device to be used as a parent for the HDMI Codec if
    either of **DRM\_BRIDGE\_OP\_HDMI\_AUDIO** or **DRM\_BRIDGE\_OP\_DP\_AUDIO** is set.

`hdmi_audio_max_i2s_playback_channels`
:   maximum number of playback
    I2S channels for the **DRM\_BRIDGE\_OP\_HDMI\_AUDIO** or
    **DRM\_BRIDGE\_OP\_DP\_AUDIO**.

`hdmi_audio_i2s_formats`
:   supported I2S formats, optional. The
    default is to allow all formats supported by the corresponding I2S
    bus driver. This is only used for bridges setting
    **DRM\_BRIDGE\_OP\_HDMI\_AUDIO** or **DRM\_BRIDGE\_OP\_DP\_AUDIO**.

`hdmi_audio_spdif_playback`
:   set if this bridge has S/PDIF playback
    port for **DRM\_BRIDGE\_OP\_HDMI\_AUDIO** or **DRM\_BRIDGE\_OP\_DP\_AUDIO**.

`hdmi_audio_dai_port`
:   sound DAI port for either of
    **DRM\_BRIDGE\_OP\_HDMI\_AUDIO** and **DRM\_BRIDGE\_OP\_DP\_AUDIO**, -1 if it is
    not used.

`hdmi_cec_adapter_name`
:   the name of the adapter to register

`hdmi_cec_available_las`
:   number of logical addresses, CEC\_MAX\_LOG\_ADDRS if unset

`hpd_mutex`
:   Protects the **hpd\_cb** and **hpd\_data** fields.

`hpd_cb`
:   Hot plug detection callback, registered with
    [`drm_bridge_hpd_enable()`](#c.drm_bridge_hpd_enable "drm_bridge_hpd_enable").

`hpd_data`
:   Private data passed to the Hot plug detection callback
    **hpd\_cb**.

`next_bridge`
:   Pointer to the following bridge, automatically put
    when this bridge is freed (i.e. at destroy time). This is for
    drivers needing to store a pointer to the next bridge in the
    chain, and ensures any code still holding a reference to this
    bridge after its removal cannot use-after-free the next
    bridge. Any other bridge pointers stored by the driver must be
    put in the .destroy callback by driver code.

devm\_drm\_bridge\_alloc

`devm_drm_bridge_alloc (dev, type, member, funcs)`

> Allocate and initialize a bridge

**Parameters**

`dev`
:   [`struct device`](../driver-api/infrastructure.html#c.device "device") of the bridge device

`type`
:   the type of the struct which contains struct [`drm_bridge`](#c.drm_bridge "drm_bridge")

`member`
:   the name of the [`drm_bridge`](#c.drm_bridge "drm_bridge") within **type**

`funcs`
:   callbacks for this bridge

**Description**

The reference count of the returned bridge is initialized to 1. This
reference will be automatically dropped via devm (by calling
[`drm_bridge_put()`](#c.drm_bridge_put "drm_bridge_put")) when **dev** is removed.

**Return**

Pointer to new bridge, or ERR\_PTR on failure.

struct [drm\_bridge\_state](drm-kms.html#c.drm_bridge_state "drm_bridge_state") \*drm\_bridge\_get\_current\_state(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge)
:   Get the current bridge state

**Parameters**

`struct drm_bridge *bridge`
:   bridge object

**Description**

This function must be called with the modeset lock held.

The current bridge state, or NULL if there is none.

struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*drm\_bridge\_get\_next\_bridge(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge)
:   Get the next bridge in the chain

**Parameters**

`struct drm_bridge *bridge`
:   bridge object

**Description**

The caller is responsible of having a reference to **bridge** via
[`drm_bridge_get()`](#c.drm_bridge_get "drm_bridge_get") or equivalent. This function leaves the refcount of
**bridge** unmodified.

The refcount of the returned bridge is incremented. Use [`drm_bridge_put()`](#c.drm_bridge_put "drm_bridge_put")
when done with it.

**Return**

the next bridge in the chain after **bridge**, or NULL if **bridge** is the last.

struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*drm\_bridge\_get\_prev\_bridge(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge)
:   Get the previous bridge in the chain

**Parameters**

`struct drm_bridge *bridge`
:   bridge object

**Description**

The caller is responsible of having a reference to **bridge** via
[`drm_bridge_get()`](#c.drm_bridge_get "drm_bridge_get") or equivalent. This function leaves the refcount of
**bridge** unmodified.

The refcount of the returned bridge is incremented. Use [`drm_bridge_put()`](#c.drm_bridge_put "drm_bridge_put")
when done with it.

**Return**

the previous bridge in the chain, or NULL if **bridge** is the first.

struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*drm\_bridge\_chain\_get\_first\_bridge(struct [drm\_encoder](drm-kms.html#c.drm_encoder "drm_encoder") \*encoder)
:   Get the first bridge in the chain

**Parameters**

`struct drm_encoder *encoder`
:   encoder object

**Description**

The refcount of the returned bridge is incremented. Use [`drm_bridge_put()`](#c.drm_bridge_put "drm_bridge_put")
when done with it.

**Return**

the first bridge in the chain, or NULL if **encoder** has no bridge attached
to it.

struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*drm\_bridge\_chain\_get\_last\_bridge(struct [drm\_encoder](drm-kms.html#c.drm_encoder "drm_encoder") \*encoder)
:   Get the last bridge in the chain

**Parameters**

`struct drm_encoder *encoder`
:   encoder object

**Description**

The refcount of the returned bridge is incremented. Use [`drm_bridge_put()`](#c.drm_bridge_put "drm_bridge_put")
when done with it.

**Return**

the last bridge in the chain, or NULL if **encoder** has no bridge attached
to it.

struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*drm\_bridge\_get\_next\_bridge\_and\_put(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge)
:   Get the next bridge in the chain and put the previous

**Parameters**

`struct drm_bridge *bridge`
:   bridge object

**Description**

Same as [`drm_bridge_get_next_bridge()`](#c.drm_bridge_get_next_bridge "drm_bridge_get_next_bridge") but additionally puts the **bridge**.

**Return**

the next bridge in the chain after **bridge**, or NULL if **bridge** is the last.

drm\_for\_each\_bridge\_in\_chain\_scoped

`drm_for_each_bridge_in_chain_scoped (encoder, bridge)`

> iterate over all bridges attached to an encoder

**Parameters**

`encoder`
:   the encoder to iterate bridges on

`bridge`
:   a bridge pointer updated to point to the current bridge at each
    iteration

**Description**

Iterate over all bridges present in the bridge chain attached to **encoder**.

Automatically gets/puts the bridge reference while iterating, and puts
the reference even if returning or breaking in the middle of the loop.

drm\_for\_each\_bridge\_in\_chain\_from

`drm_for_each_bridge_in_chain_from (first_bridge, bridge)`

> iterate over all bridges starting from the given bridge

**Parameters**

`first_bridge`
:   the bridge to start from

`bridge`
:   a bridge pointer updated to point to the current bridge at each
    iteration

**Description**

Iterate over all bridges in the encoder chain starting from
**first\_bridge**, included.

Automatically gets/puts the bridge reference while iterating, and puts
the reference even if returning or breaking in the middle of the loop.

bool drm\_bridge\_enter(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge, int \*idx)
:   Enter DRM bridge critical section

**Parameters**

`struct drm_bridge *bridge`
:   DRM bridge

`int *idx`
:   Pointer to index that will be passed to the matching [`drm_bridge_exit()`](#c.drm_bridge_exit "drm_bridge_exit")

**Description**

This function marks and protects the beginning of a section that should not
be entered after the bridge has been unplugged. The section end is marked
with [`drm_bridge_exit()`](#c.drm_bridge_exit "drm_bridge_exit"). Calls to this function can be nested.

**Return**

True if it is OK to enter the section, false otherwise.

void drm\_bridge\_exit(int idx)
:   Exit DRM bridge critical section

**Parameters**

`int idx`
:   index returned by [`drm_bridge_enter()`](#c.drm_bridge_enter "drm_bridge_enter")

**Description**

This function marks the end of a section that should not be entered after
the bridge has been unplugged.

void drm\_bridge\_unplug(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge)
:   declare a DRM bridge was unplugged and remove it

**Parameters**

`struct drm_bridge *bridge`
:   DRM bridge

**Description**

This tells the bridge has been physically unplugged and no operations on
device resources must be done anymore. Entry-points can use
[`drm_bridge_enter()`](#c.drm_bridge_enter "drm_bridge_enter") and [`drm_bridge_exit()`](#c.drm_bridge_exit "drm_bridge_exit") to protect device resources in
a race free manner.

Also unregisters the bridge.

struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*drm\_bridge\_get(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge)
:   Acquire a bridge reference

**Parameters**

`struct drm_bridge *bridge`
:   DRM bridge

**Description**

This function increments the bridge’s refcount.

**Return**

Pointer to **bridge**.

void drm\_bridge\_put(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge)
:   Release a bridge reference

**Parameters**

`struct drm_bridge *bridge`
:   DRM bridge

**Description**

This function decrements the bridge’s reference count and frees the
object if the reference count drops to zero.

See also [`drm_bridge_clear_and_put()`](#c.drm_bridge_clear_and_put "drm_bridge_clear_and_put") if you also need to set the pointer
to NULL

void drm\_bridge\_clear\_and\_put(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*\*bridge\_pp)
:   Given a bridge pointer, clear the pointer then put the bridge

**Parameters**

`struct drm_bridge **bridge_pp`
:   pointer to pointer to a [`struct drm_bridge`](#c.drm_bridge "drm_bridge"); `bridge_pp`
    must be non-NULL; if `*bridge_pp` is NULL this function
    does nothing

**Description**

Helper to put a DRM bridge, but only after setting its pointer to
NULL. Useful when a [`struct drm_bridge`](#c.drm_bridge "drm_bridge") reference must be dropped without
leaving a use-after-free window where the pointed bridge might have been
freed while still holding a pointer to it.

For struct `drm_bridge *some_bridge`, this code:

```
drm_bridge_clear_and_put(&some_bridge);
```

is equivalent to the more complex:

```
struct drm_bridge *temp = some_bridge;
some_bridge = NULL;
drm_bridge_put(temp);
```

void drm\_bridge\_add(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge)
:   register a bridge

**Parameters**

`struct drm_bridge *bridge`
:   bridge control structure

**Description**

Add the given bridge to the global list of bridges, where they can be
found by users via [`of_drm_find_and_get_bridge()`](#c.of_drm_find_and_get_bridge "of_drm_find_and_get_bridge").

The bridge to be added must have been allocated by
[`devm_drm_bridge_alloc()`](#c.devm_drm_bridge_alloc "devm_drm_bridge_alloc").

int devm\_drm\_bridge\_add(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge)
:   devm managed version of [`drm_bridge_add()`](#c.drm_bridge_add "drm_bridge_add")

**Parameters**

`struct device *dev`
:   device to tie the bridge lifetime to

`struct drm_bridge *bridge`
:   bridge control structure

**Description**

This is the managed version of [`drm_bridge_add()`](#c.drm_bridge_add "drm_bridge_add") which automatically
calls [`drm_bridge_remove()`](#c.drm_bridge_remove "drm_bridge_remove") when **dev** is unbound.

**Return**

0 if no error or negative error code.

void drm\_bridge\_remove(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge)
:   unregister a bridge

**Parameters**

`struct drm_bridge *bridge`
:   bridge control structure

**Description**

Remove the given bridge from the global list of registered bridges, so
it won’t be found by users via [`of_drm_find_and_get_bridge()`](#c.of_drm_find_and_get_bridge "of_drm_find_and_get_bridge"), and add it
to the lingering bridge list, to keep track of it until its allocated
memory is eventually freed.

int drm\_bridge\_attach(struct [drm\_encoder](drm-kms.html#c.drm_encoder "drm_encoder") \*encoder, struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge, struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*previous, enum [drm\_bridge\_attach\_flags](#c.drm_bridge_attach_flags "drm_bridge_attach_flags") flags)
:   attach the bridge to an encoder’s chain

**Parameters**

`struct drm_encoder *encoder`
:   DRM encoder

`struct drm_bridge *bridge`
:   bridge to attach

`struct drm_bridge *previous`
:   previous bridge in the chain (optional)

`enum drm_bridge_attach_flags flags`
:   DRM\_BRIDGE\_ATTACH\_\* flags

**Description**

Called by a kms driver to link the bridge to an encoder’s chain. The previous
argument specifies the previous bridge in the chain. If NULL, the bridge is
linked directly at the encoder’s output. Otherwise it is linked at the
previous bridge’s output.

If non-NULL the previous bridge must be already attached by a call to this
function.

The bridge to be attached must have been previously added by
[`drm_bridge_add()`](#c.drm_bridge_add "drm_bridge_add").

Note that bridges attached to encoders are auto-detached during encoder
cleanup in [`drm_encoder_cleanup()`](drm-kms.html#c.drm_encoder_cleanup "drm_encoder_cleanup"), so [`drm_bridge_attach()`](#c.drm_bridge_attach "drm_bridge_attach") should generally
*not* be balanced with a `drm_bridge_detach()` in driver code.

**Return**

Zero on success, error code on failure

enum [drm\_mode\_status](drm-kms.html#c.drm_mode_status "drm_mode_status") drm\_bridge\_chain\_mode\_valid(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge, const struct [drm\_display\_info](drm-kms.html#c.drm_display_info "drm_display_info") \*info, const struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*mode)
:   validate the mode against all bridges in the encoder chain.

**Parameters**

`struct drm_bridge *bridge`
:   bridge control structure

`const struct drm_display_info *info`
:   display info against which the mode shall be validated

`const struct drm_display_mode *mode`
:   desired mode to be validated

**Description**

Calls [`drm_bridge_funcs.mode_valid`](#c.drm_bridge_funcs "drm_bridge_funcs") for all the bridges in the encoder
chain, starting from the first bridge to the last. If at least one bridge
does not accept the mode the function returns the error code.

**Note**

the bridge passed should be the one closest to the encoder.

**Return**

MODE\_OK on success, drm\_mode\_status Enum error code on failure

void drm\_bridge\_chain\_mode\_set(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge, const struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*mode, const struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*adjusted\_mode)
:   set proposed mode for all bridges in the encoder chain

**Parameters**

`struct drm_bridge *bridge`
:   bridge control structure

`const struct drm_display_mode *mode`
:   desired mode to be set for the encoder chain

`const struct drm_display_mode *adjusted_mode`
:   updated mode that works for this encoder chain

**Description**

Calls [`drm_bridge_funcs.mode_set`](#c.drm_bridge_funcs "drm_bridge_funcs") op for all the bridges in the
encoder chain, starting from the first bridge to the last.

**Note**

the bridge passed should be the one closest to the encoder

void drm\_atomic\_bridge\_chain\_disable(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   disables all bridges in the encoder chain

**Parameters**

`struct drm_bridge *bridge`
:   bridge control structure

`struct drm_atomic_state *state`
:   atomic state being committed

**Description**

Calls [`drm_bridge_funcs.atomic_disable`](#c.drm_bridge_funcs "drm_bridge_funcs") (falls back on
[`drm_bridge_funcs.disable`](#c.drm_bridge_funcs "drm_bridge_funcs")) op for all the bridges in the encoder chain,
starting from the last bridge to the first. These are called before calling
[`drm_encoder_helper_funcs.atomic_disable`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs")

**Note**

the bridge passed should be the one closest to the encoder

void drm\_atomic\_bridge\_chain\_post\_disable(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   cleans up after disabling all bridges in the encoder chain

**Parameters**

`struct drm_bridge *bridge`
:   bridge control structure

`struct drm_atomic_state *state`
:   atomic state being committed

**Description**

Calls [`drm_bridge_funcs.atomic_post_disable`](#c.drm_bridge_funcs "drm_bridge_funcs") (falls back on
[`drm_bridge_funcs.post_disable`](#c.drm_bridge_funcs "drm_bridge_funcs")) op for all the bridges in the encoder chain,
starting from the first bridge to the last. These are called after completing
[`drm_encoder_helper_funcs.atomic_disable`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs")

If a bridge sets **pre\_enable\_prev\_first**, then the **post\_disable** for that
bridge will be called before the previous one to reverse the **pre\_enable**
calling direction.

**Example**

Bridge A ---> Bridge B ---> Bridge C ---> Bridge D ---> Bridge E

With pre\_enable\_prev\_first flag enable in Bridge B, D, E then the resulting
**post\_disable** order would be,
Bridge B, Bridge A, Bridge E, Bridge D, Bridge C.

**Note**

the bridge passed should be the one closest to the encoder

void drm\_atomic\_bridge\_chain\_pre\_enable(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   prepares for enabling all bridges in the encoder chain

**Parameters**

`struct drm_bridge *bridge`
:   bridge control structure

`struct drm_atomic_state *state`
:   atomic state being committed

**Description**

Calls [`drm_bridge_funcs.atomic_pre_enable`](#c.drm_bridge_funcs "drm_bridge_funcs") (falls back on
[`drm_bridge_funcs.pre_enable`](#c.drm_bridge_funcs "drm_bridge_funcs")) op for all the bridges in the encoder chain,
starting from the last bridge to the first. These are called before calling
[`drm_encoder_helper_funcs.atomic_enable`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs")

If a bridge sets **pre\_enable\_prev\_first**, then the pre\_enable for the
prev bridge will be called before pre\_enable of this bridge.

**Example**

Bridge A ---> Bridge B ---> Bridge C ---> Bridge D ---> Bridge E

With pre\_enable\_prev\_first flag enable in Bridge B, D, E then the resulting
**pre\_enable** order would be,
Bridge C, Bridge D, Bridge E, Bridge A, Bridge B.

**Note**

the bridge passed should be the one closest to the encoder

void drm\_atomic\_bridge\_chain\_enable(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   enables all bridges in the encoder chain

**Parameters**

`struct drm_bridge *bridge`
:   bridge control structure

`struct drm_atomic_state *state`
:   atomic state being committed

**Description**

Calls [`drm_bridge_funcs.atomic_enable`](#c.drm_bridge_funcs "drm_bridge_funcs") (falls back on
[`drm_bridge_funcs.enable`](#c.drm_bridge_funcs "drm_bridge_funcs")) op for all the bridges in the encoder chain,
starting from the first bridge to the last. These are called after completing
[`drm_encoder_helper_funcs.atomic_enable`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs")

**Note**

the bridge passed should be the one closest to the encoder

int drm\_atomic\_bridge\_chain\_check(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge, struct [drm\_crtc\_state](drm-kms.html#c.drm_crtc_state "drm_crtc_state") \*crtc\_state, struct [drm\_connector\_state](drm-kms.html#c.drm_connector_state "drm_connector_state") \*conn\_state)
:   Do an atomic check on the bridge chain

**Parameters**

`struct drm_bridge *bridge`
:   bridge control structure

`struct drm_crtc_state *crtc_state`
:   new CRTC state

`struct drm_connector_state *conn_state`
:   new connector state

**Description**

First trigger a bus format negotiation before calling
[`drm_bridge_funcs.atomic_check()`](#c.drm_bridge_funcs "drm_bridge_funcs") (falls back on
[`drm_bridge_funcs.mode_fixup()`](#c.drm_bridge_funcs "drm_bridge_funcs")) op for all the bridges in the encoder chain,
starting from the last bridge to the first. These are called before calling
[`drm_encoder_helper_funcs.atomic_check()`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs")

**Return**

0 on success, a negative error code on failure

enum [drm\_connector\_status](drm-kms.html#c.drm_connector_status "drm_connector_status") drm\_bridge\_detect(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge, struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   check if anything is attached to the bridge output

**Parameters**

`struct drm_bridge *bridge`
:   bridge control structure

`struct drm_connector *connector`
:   attached connector

**Description**

If the bridge supports output detection, as reported by the
DRM\_BRIDGE\_OP\_DETECT bridge ops flag, call [`drm_bridge_funcs.detect`](#c.drm_bridge_funcs "drm_bridge_funcs") for the
bridge and return the connection status. Otherwise return
connector\_status\_unknown.

**Return**

The detection status on success, or connector\_status\_unknown if the bridge
doesn’t support output detection.

int drm\_bridge\_get\_modes(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge, struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   fill all modes currently valid for the sink into the **connector**

**Parameters**

`struct drm_bridge *bridge`
:   bridge control structure

`struct drm_connector *connector`
:   the connector to fill with modes

**Description**

If the bridge supports output modes retrieval, as reported by the
DRM\_BRIDGE\_OP\_MODES bridge ops flag, call [`drm_bridge_funcs.get_modes`](#c.drm_bridge_funcs "drm_bridge_funcs") to
fill the connector with all valid modes and return the number of modes
added. Otherwise return 0.

**Return**

The number of modes added to the connector.

const struct drm\_edid \*drm\_bridge\_edid\_read(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge, struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   read the EDID data of the connected display

**Parameters**

`struct drm_bridge *bridge`
:   bridge control structure

`struct drm_connector *connector`
:   the connector to read EDID for

**Description**

If the bridge supports output EDID retrieval, as reported by the
DRM\_BRIDGE\_OP\_EDID bridge ops flag, call [`drm_bridge_funcs.edid_read`](#c.drm_bridge_funcs "drm_bridge_funcs") to get
the EDID and return it. Otherwise return NULL.

**Return**

The retrieved EDID on success, or NULL otherwise.

void drm\_bridge\_hpd\_enable(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge, void (\*cb)(void \*data, enum [drm\_connector\_status](drm-kms.html#c.drm_connector_status "drm_connector_status") status), void \*data)
:   enable hot plug detection for the bridge

**Parameters**

`struct drm_bridge *bridge`
:   bridge control structure

`void (*cb)(void *data, enum drm_connector_status status)`
:   hot-plug detection callback

`void *data`
:   data to be passed to the hot-plug detection callback

**Description**

Call [`drm_bridge_funcs.hpd_enable`](#c.drm_bridge_funcs "drm_bridge_funcs") if implemented and register the given **cb**
and **data** as hot plug notification callback. From now on the **cb** will be
called with **data** when an output status change is detected by the bridge,
until hot plug notification gets disabled with [`drm_bridge_hpd_disable()`](#c.drm_bridge_hpd_disable "drm_bridge_hpd_disable").

Hot plug detection is supported only if the DRM\_BRIDGE\_OP\_HPD flag is set in
bridge->ops. This function shall not be called when the flag is not set.

Only one hot plug detection callback can be registered at a time, it is an
error to call this function when hot plug detection is already enabled for
the bridge.

void drm\_bridge\_hpd\_disable(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge)
:   disable hot plug detection for the bridge

**Parameters**

`struct drm_bridge *bridge`
:   bridge control structure

**Description**

Call [`drm_bridge_funcs.hpd_disable`](#c.drm_bridge_funcs "drm_bridge_funcs") if implemented and unregister the hot
plug detection callback previously registered with [`drm_bridge_hpd_enable()`](#c.drm_bridge_hpd_enable "drm_bridge_hpd_enable").
Once this function returns the callback will not be called by the bridge
when an output status change occurs.

Hot plug detection is supported only if the DRM\_BRIDGE\_OP\_HPD flag is set in
bridge->ops. This function shall not be called when the flag is not set.

void drm\_bridge\_hpd\_notify(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge, enum [drm\_connector\_status](drm-kms.html#c.drm_connector_status "drm_connector_status") status)
:   notify hot plug detection events

**Parameters**

`struct drm_bridge *bridge`
:   bridge control structure

`enum drm_connector_status status`
:   output connection status

**Description**

Bridge drivers shall call this function to report hot plug events when they
detect a change in the output status, when hot plug detection has been
enabled by [`drm_bridge_hpd_enable()`](#c.drm_bridge_hpd_enable "drm_bridge_hpd_enable").

This function shall be called in a context that can sleep.

struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*of\_drm\_find\_and\_get\_bridge(struct device\_node \*np)
:   find the bridge corresponding to the device node in the global bridge list

**Parameters**

`struct device_node *np`
:   device node

**Description**

The refcount of the returned bridge is incremented. Use [`drm_bridge_put()`](#c.drm_bridge_put "drm_bridge_put")
when done with it.

**Return**

drm\_bridge control `struct on` success, NULL on failure

struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*of\_drm\_find\_bridge(struct device\_node \*np)
:   find the bridge corresponding to the device node in the global bridge list

**Parameters**

`struct device_node *np`
:   device node

**Description**

This function is deprecated. Convert to [`of_drm_find_and_get_bridge()`](#c.of_drm_find_and_get_bridge "of_drm_find_and_get_bridge")
instead for proper refcounting.

The bridge returned by this function is not refcounted. This is
dangerous because the bridge might be deallocated even before the caller
has a chance to use it. To use this function you have to do one of:

* get a reference with [`drm_bridge_get()`](#c.drm_bridge_get "drm_bridge_get") as soon as possible to
  minimize the race window, and then [`drm_bridge_put()`](#c.drm_bridge_put "drm_bridge_put") when no longer
  using the pointer
* not call [`drm_bridge_get()`](#c.drm_bridge_get "drm_bridge_get") or [`drm_bridge_put()`](#c.drm_bridge_put "drm_bridge_put") at all, which used to
  be the correct practice before dynamic bridge lifetime was introduced
* again, convert to [`of_drm_find_and_get_bridge()`](#c.of_drm_find_and_get_bridge "of_drm_find_and_get_bridge"), which is the only safe
  thing to do

**Return**

drm\_bridge control `struct on` success, NULL on failure

void devm\_drm\_put\_bridge(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge)
:   Release a bridge reference obtained via devm

**Parameters**

`struct device *dev`
:   device that got the bridge via devm

`struct drm_bridge *bridge`
:   pointer to a [`struct drm_bridge`](#c.drm_bridge "drm_bridge") obtained via devm

**Description**

Same as [`drm_bridge_put()`](#c.drm_bridge_put "drm_bridge_put") for bridge pointers obtained via devm functions
such as [`devm_drm_bridge_alloc()`](#c.devm_drm_bridge_alloc "devm_drm_bridge_alloc").

This function is a temporary workaround and MUST NOT be used. Manual
handling of bridge lifetime is inherently unsafe.

### MIPI-DSI bridge operation

DSI host interfaces are expected to be implemented as bridges rather than
encoders, however there are a few aspects of their operation that need to
be defined in order to provide a consistent interface.

A DSI host should keep the PHY powered down until the pre\_enable operation is
called. All lanes are in an undefined idle state up to this point, and it
must not be assumed that it is LP-11.
pre\_enable should initialise the PHY, set the data lanes to LP-11, and the
clock lane to either LP-11 or HS depending on the mode\_flag
`MIPI_DSI_CLOCK_NON_CONTINUOUS`.

Ordinarily the downstream bridge DSI peripheral pre\_enable will have been
called before the DSI host. If the DSI peripheral requires LP-11 and/or
the clock lane to be in HS mode prior to pre\_enable, then it can set the
`pre_enable_prev_first` flag to request the pre\_enable (and
post\_disable) order to be altered to enable the DSI host first.

Either the CRTC being enabled, or the DSI host enable operation should switch
the host to actively transmitting video on the data lanes.

The reverse also applies. The DSI host disable operation or stopping the CRTC
should stop transmitting video, and the data lanes should return to the LP-11
state. The DSI host `post_disable` operation should disable the PHY.
If the `pre_enable_prev_first` flag is set, then the DSI peripheral’s
bridge `post_disable` will be called before the DSI host’s post\_disable.

Whilst it is valid to call `host_transfer` prior to pre\_enable or after
post\_disable, the exact state of the lanes is undefined at this point. The
DSI host should initialise the interface, transmit the data, and then disable
the interface again.

Ultra Low Power State (ULPS) is not explicitly supported by DRM. If
implemented, it therefore needs to be handled entirely within the DSI Host
driver.

### Bridge Connector Helper Reference

struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*drm\_bridge\_connector\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*drm, struct [drm\_encoder](drm-kms.html#c.drm_encoder "drm_encoder") \*encoder)
:   Initialise a connector for a chain of bridges

**Parameters**

`struct drm_device *drm`
:   the DRM device

`struct drm_encoder *encoder`
:   the encoder where the bridge chain starts

**Description**

Allocate, initialise and register a `drm_bridge_connector` with the **drm**
device. The connector is associated with a chain of bridges that starts at
the **encoder**. All bridges in the chain shall report bridge operation flags
([`drm_bridge->ops`](#c.drm_bridge "drm_bridge")) and bridge output type ([`drm_bridge->type`](#c.drm_bridge "drm_bridge")), and none of
them may create a DRM connector directly.

Returns a pointer to the new connector on success, or a negative error
pointer otherwise.

### Panel-Bridge Helper Reference

bool drm\_bridge\_is\_panel(const struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge)
:   Checks if a drm\_bridge is a panel\_bridge.

**Parameters**

`const struct drm_bridge *bridge`
:   The drm\_bridge to be checked.

**Description**

Returns true if the bridge is a panel bridge, or false otherwise.

struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*drm\_panel\_bridge\_add(struct [drm\_panel](#c.drm_panel "drm_panel") \*panel)
:   Creates a [`drm_bridge`](#c.drm_bridge "drm_bridge") and [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector") that just calls the appropriate functions from [`drm_panel`](#c.drm_panel "drm_panel").

**Parameters**

`struct drm_panel *panel`
:   The drm\_panel being wrapped. Must be non-NULL.

**Description**

For drivers converting from directly using drm\_panel: The expected
usage pattern is that during either encoder module probe or DSI
host attach, a drm\_panel will be looked up through
[`drm_of_find_panel_or_bridge()`](#c.drm_of_find_panel_or_bridge "drm_of_find_panel_or_bridge"). [`drm_panel_bridge_add()`](#c.drm_panel_bridge_add "drm_panel_bridge_add") is used to
wrap that panel in the new bridge, and the result can then be
passed to [`drm_bridge_attach()`](#c.drm_bridge_attach "drm_bridge_attach"). The [`drm_panel_prepare()`](#c.drm_panel_prepare "drm_panel_prepare") and related
functions can be dropped from the encoder driver (they’re now
called by the KMS helpers before calling into the encoder), along
with connector creation. When done with the bridge (after
[`drm_mode_config_cleanup()`](drm-kms.html#c.drm_mode_config_cleanup "drm_mode_config_cleanup") if the bridge has already been attached), then
[`drm_panel_bridge_remove()`](#c.drm_panel_bridge_remove "drm_panel_bridge_remove") to free it.

The connector type is set to **panel->connector\_type**, which must be set to a
known type. Calling this function with a panel whose connector type is
DRM\_MODE\_CONNECTOR\_Unknown will return ERR\_PTR(-EINVAL).

See [`devm_drm_panel_bridge_add()`](#c.devm_drm_panel_bridge_add "devm_drm_panel_bridge_add") for an automatically managed version of this
function.

struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*drm\_panel\_bridge\_add\_typed(struct [drm\_panel](#c.drm_panel "drm_panel") \*panel, u32 connector\_type)
:   Creates a [`drm_bridge`](#c.drm_bridge "drm_bridge") and [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector") with an explicit connector type.

**Parameters**

`struct drm_panel *panel`
:   The drm\_panel being wrapped. Must be non-NULL.

`u32 connector_type`
:   The connector type (DRM\_MODE\_CONNECTOR\_\*)

**Description**

This is just like [`drm_panel_bridge_add()`](#c.drm_panel_bridge_add "drm_panel_bridge_add"), but forces the connector type to
**connector\_type** instead of infering it from the panel.

This function is deprecated and should not be used in new drivers. Use
[`drm_panel_bridge_add()`](#c.drm_panel_bridge_add "drm_panel_bridge_add") instead, and fix panel drivers as necessary if they
don’t report a connector type.

void drm\_panel\_bridge\_remove(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge)
:   Unregisters and frees a drm\_bridge created by [`drm_panel_bridge_add()`](#c.drm_panel_bridge_add "drm_panel_bridge_add").

**Parameters**

`struct drm_bridge *bridge`
:   The drm\_bridge being freed.

int drm\_panel\_bridge\_set\_orientation(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge)
:   Set the connector’s panel orientation from the bridge that can be transformed to panel bridge.

**Parameters**

`struct drm_connector *connector`
:   The connector to be set panel orientation.

`struct drm_bridge *bridge`
:   The drm\_bridge to be transformed to panel bridge.

**Description**

Returns 0 on success, negative errno on failure.

struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*devm\_drm\_panel\_bridge\_add(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct [drm\_panel](#c.drm_panel "drm_panel") \*panel)
:   Creates a managed [`drm_bridge`](#c.drm_bridge "drm_bridge") and [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector") that just calls the appropriate functions from [`drm_panel`](#c.drm_panel "drm_panel").

**Parameters**

`struct device *dev`
:   device to tie the bridge lifetime to

`struct drm_panel *panel`
:   The drm\_panel being wrapped. Must be non-NULL.

**Description**

This is the managed version of [`drm_panel_bridge_add()`](#c.drm_panel_bridge_add "drm_panel_bridge_add") which automatically
calls [`drm_panel_bridge_remove()`](#c.drm_panel_bridge_remove "drm_panel_bridge_remove") when **dev** is unbound.

struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*devm\_drm\_panel\_bridge\_add\_typed(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct [drm\_panel](#c.drm_panel "drm_panel") \*panel, u32 connector\_type)
:   Creates a managed [`drm_bridge`](#c.drm_bridge "drm_bridge") and [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector") with an explicit connector type.

**Parameters**

`struct device *dev`
:   device to tie the bridge lifetime to

`struct drm_panel *panel`
:   The drm\_panel being wrapped. Must be non-NULL.

`u32 connector_type`
:   The connector type (DRM\_MODE\_CONNECTOR\_\*)

**Description**

This is just like [`devm_drm_panel_bridge_add()`](#c.devm_drm_panel_bridge_add "devm_drm_panel_bridge_add"), but forces the connector type
to **connector\_type** instead of infering it from the panel.

This function is deprecated and should not be used in new drivers. Use
[`devm_drm_panel_bridge_add()`](#c.devm_drm_panel_bridge_add "devm_drm_panel_bridge_add") instead, and fix panel drivers as necessary if
they don’t report a connector type.

struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*drmm\_panel\_bridge\_add(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*drm, struct [drm\_panel](#c.drm_panel "drm_panel") \*panel)
:   Creates a DRM-managed [`drm_bridge`](#c.drm_bridge "drm_bridge") and [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector") that just calls the appropriate functions from [`drm_panel`](#c.drm_panel "drm_panel").

**Parameters**

`struct drm_device *drm`
:   DRM device to tie the bridge lifetime to

`struct drm_panel *panel`
:   The drm\_panel being wrapped. Must be non-NULL.

**Description**

This is the DRM-managed version of [`drm_panel_bridge_add()`](#c.drm_panel_bridge_add "drm_panel_bridge_add") which
automatically calls [`drm_panel_bridge_remove()`](#c.drm_panel_bridge_remove "drm_panel_bridge_remove") when **dev** is cleaned
up.

struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*drm\_panel\_bridge\_connector(struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*bridge)
:   return the connector for the panel bridge

**Parameters**

`struct drm_bridge *bridge`
:   The drm\_bridge.

**Description**

drm\_panel\_bridge creates the connector.
This function gives external access to the connector.

**Return**

Pointer to drm\_connector

struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*devm\_drm\_of\_get\_bridge(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct device\_node \*np, u32 port, u32 endpoint)
:   Return next bridge in the chain

**Parameters**

`struct device *dev`
:   device to tie the bridge lifetime to

`struct device_node *np`
:   device tree node containing encoder output ports

`u32 port`
:   port in the device tree node

`u32 endpoint`
:   endpoint in the device tree node

**Description**

Given a DT node’s port and endpoint number, finds the connected node
and returns the associated bridge if any, or creates and returns a
drm panel bridge instance if a panel is connected.

Returns a pointer to the bridge if successful, or an error pointer
otherwise.

struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*drmm\_of\_get\_bridge(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*drm, struct device\_node \*np, u32 port, u32 endpoint)
:   Return next bridge in the chain

**Parameters**

`struct drm_device *drm`
:   device to tie the bridge lifetime to

`struct device_node *np`
:   device tree node containing encoder output ports

`u32 port`
:   port in the device tree node

`u32 endpoint`
:   endpoint in the device tree node

**Description**

Given a DT node’s port and endpoint number, finds the connected node
and returns the associated bridge if any, or creates and returns a
drm panel bridge instance if a panel is connected.

Returns a drmm managed pointer to the bridge if successful, or an error
pointer otherwise.

## Panel Helper Reference

The DRM panel helpers allow drivers to register panel objects with a
central registry and provide functions to retrieve those panels in display
drivers.

For easy integration into drivers using the [`drm_bridge`](#c.drm_bridge "drm_bridge") infrastructure please
take look at [`drm_panel_bridge_add()`](#c.drm_panel_bridge_add "drm_panel_bridge_add") and [`devm_drm_panel_bridge_add()`](#c.devm_drm_panel_bridge_add "devm_drm_panel_bridge_add").

struct drm\_panel\_funcs
:   perform operations on a given panel

**Definition**:

```
struct drm_panel_funcs {
    int (*prepare)(struct drm_panel *panel);
    int (*enable)(struct drm_panel *panel);
    int (*disable)(struct drm_panel *panel);
    int (*unprepare)(struct drm_panel *panel);
    int (*get_modes)(struct drm_panel *panel, struct drm_connector *connector);
    enum drm_panel_orientation (*get_orientation)(struct drm_panel *panel);
    int (*get_timings)(struct drm_panel *panel, unsigned int num_timings, struct display_timing *timings);
    void (*debugfs_init)(struct drm_panel *panel, struct dentry *root);
};
```

**Members**

`prepare`
:   Turn on panel and perform set up.

    This function is optional.

`enable`
:   Enable panel (turn on back light, etc.).

    This function is optional.

`disable`
:   Disable panel (turn off back light, etc.).

    This function is optional.

`unprepare`
:   Turn off panel.

    This function is optional.

`get_modes`
:   Add modes to the connector that the panel is attached to
    and returns the number of modes added.

    This function is mandatory.

`get_orientation`
:   Return the panel orientation set by device tree or EDID.

    This function is optional.

`get_timings`
:   Copy display timings into the provided array and return
    the number of display timings available.

    This function is optional.

`debugfs_init`
:   Allows panels to create panels-specific debugfs files.

**Description**

The .`prepare()` function is typically called before the display controller
starts to transmit video data. Panel drivers can use this to turn the panel
on and wait for it to become ready. If additional configuration is required
(via a control bus such as I2C, SPI or DSI for example) this is a good time
to do that.

After the display controller has started transmitting video data, it’s safe
to call the .`enable()` function. This will typically enable the backlight to
make the image on screen visible. Some panels require a certain amount of
time or frames before the image is displayed. This function is responsible
for taking this into account before enabling the backlight to avoid visual
glitches.

Before stopping video transmission from the display controller it can be
necessary to turn off the panel to avoid visual glitches. This is done in
the .`disable()` function. Analogously to .`enable()` this typically involves
turning off the backlight and waiting for some time to make sure no image
is visible on the panel. It is then safe for the display controller to
cease transmission of video data.

To save power when no video data is transmitted, a driver can power down
the panel. This is the job of the .`unprepare()` function.

Backlight can be handled automatically if configured using
[`drm_panel_of_backlight()`](#c.drm_panel_of_backlight "drm_panel_of_backlight") or [`drm_panel_dp_aux_backlight()`](#c.drm_panel_dp_aux_backlight "drm_panel_dp_aux_backlight"). Then the driver
does not need to implement the functionality to enable/disable backlight.

struct drm\_panel
:   DRM panel object

**Definition**:

```
struct drm_panel {
    struct device *dev;
    struct backlight_device *backlight;
    const struct drm_panel_funcs *funcs;
    int connector_type;
    struct list_head list;
    struct list_head followers;
    struct mutex follower_lock;
    bool prepare_prev_first;
    bool prepared;
    bool enabled;
    void *container;
    struct kref refcount;
};
```

**Members**

`dev`
:   Parent device of the panel.

`backlight`
:   Backlight device, used to turn on backlight after the call
    to `enable()`, and to turn off backlight before the call to
    `disable()`.
    backlight is set by [`drm_panel_of_backlight()`](#c.drm_panel_of_backlight "drm_panel_of_backlight") or
    [`drm_panel_dp_aux_backlight()`](#c.drm_panel_dp_aux_backlight "drm_panel_dp_aux_backlight") and drivers shall not assign it.

`funcs`
:   Operations that can be performed on the panel.

`connector_type`
:   Type of the panel as a DRM\_MODE\_CONNECTOR\_\* value. This is used to
    initialise the drm\_connector corresponding to the panel with the
    correct connector type.

`list`
:   Panel entry in registry.

`followers`
:   A list of `struct drm_panel_follower` dependent on this panel.

`follower_lock`
:   Lock for followers list.

`prepare_prev_first`
:   The previous controller should be prepared first, before the prepare
    for the panel is called. This is largely required for DSI panels
    where the DSI host controller should be initialised to LP-11 before
    the panel is powered up.

`prepared`
:   If true then the panel has been prepared.

`enabled`
:   If true then the panel has been enabled.

`container`
:   Pointer to the private driver `struct embedding` this
    **struct** drm\_panel.

`refcount`
:   reference count of users referencing this panel.

devm\_drm\_panel\_alloc

`devm_drm_panel_alloc (dev, type, member, funcs, connector_type)`

> Allocate and initialize a refcounted panel.

**Parameters**

`dev`
:   [`struct device`](../driver-api/infrastructure.html#c.device "device") of the panel device

`type`
:   the type of the struct which contains struct [`drm_panel`](#c.drm_panel "drm_panel")

`member`
:   the name of the [`drm_panel`](#c.drm_panel "drm_panel") within **type**

`funcs`
:   callbacks for this panel

`connector_type`
:   the connector type (DRM\_MODE\_CONNECTOR\_\*) corresponding to
    the panel interface

**Description**

The reference count of the returned panel is initialized to 1. This
reference will be automatically dropped via devm (by calling
[`drm_panel_put()`](#c.drm_panel_put "drm_panel_put")) when **dev** is removed.

**Return**

Pointer to container structure embedding the panel, ERR\_PTR on failure.

void drm\_panel\_init(struct [drm\_panel](#c.drm_panel "drm_panel") \*panel, struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, const struct [drm\_panel\_funcs](#c.drm_panel_funcs "drm_panel_funcs") \*funcs, int connector\_type)
:   initialize a panel

**Parameters**

`struct drm_panel *panel`
:   DRM panel

`struct device *dev`
:   parent device of the panel

`const struct drm_panel_funcs *funcs`
:   panel operations

`int connector_type`
:   the connector type (DRM\_MODE\_CONNECTOR\_\*) corresponding to
    the panel interface (must NOT be DRM\_MODE\_CONNECTOR\_Unknown)

**Description**

Initialize the panel structure for subsequent registration with
[`drm_panel_add()`](#c.drm_panel_add "drm_panel_add").

void drm\_panel\_add(struct [drm\_panel](#c.drm_panel "drm_panel") \*panel)
:   add a panel to the global registry

**Parameters**

`struct drm_panel *panel`
:   panel to add

**Description**

Add a panel to the global registry so that it can be looked
up by display drivers. The panel to be added must have been
allocated by [`devm_drm_panel_alloc()`](#c.devm_drm_panel_alloc "devm_drm_panel_alloc").

void drm\_panel\_remove(struct [drm\_panel](#c.drm_panel "drm_panel") \*panel)
:   remove a panel from the global registry

**Parameters**

`struct drm_panel *panel`
:   DRM panel

**Description**

Removes a panel from the global registry.

void drm\_panel\_prepare(struct [drm\_panel](#c.drm_panel "drm_panel") \*panel)
:   power on a panel

**Parameters**

`struct drm_panel *panel`
:   DRM panel

**Description**

Calling this function will enable power and deassert any reset signals to
the panel. After this has completed it is possible to communicate with any
integrated circuitry via a command bus. This function cannot fail (as it is
called from the pre\_enable call chain). There will always be a call to
[`drm_panel_disable()`](#c.drm_panel_disable "drm_panel_disable") afterwards.

void drm\_panel\_unprepare(struct [drm\_panel](#c.drm_panel "drm_panel") \*panel)
:   power off a panel

**Parameters**

`struct drm_panel *panel`
:   DRM panel

**Description**

Calling this function will completely power off a panel (assert the panel’s
reset, turn off power supplies, ...). After this function has completed, it
is usually no longer possible to communicate with the panel until another
call to [`drm_panel_prepare()`](#c.drm_panel_prepare "drm_panel_prepare").

void drm\_panel\_enable(struct [drm\_panel](#c.drm_panel "drm_panel") \*panel)
:   enable a panel

**Parameters**

`struct drm_panel *panel`
:   DRM panel

**Description**

Calling this function will cause the panel display drivers to be turned on
and the backlight to be enabled. Content will be visible on screen after
this call completes. This function cannot fail (as it is called from the
enable call chain). There will always be a call to [`drm_panel_disable()`](#c.drm_panel_disable "drm_panel_disable")
afterwards.

void drm\_panel\_disable(struct [drm\_panel](#c.drm_panel "drm_panel") \*panel)
:   disable a panel

**Parameters**

`struct drm_panel *panel`
:   DRM panel

**Description**

This will typically turn off the panel’s backlight or disable the display
drivers. For smart panels it should still be possible to communicate with
the integrated circuitry via any command bus after this call.

int drm\_panel\_get\_modes(struct [drm\_panel](#c.drm_panel "drm_panel") \*panel, struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   probe the available display modes of a panel

**Parameters**

`struct drm_panel *panel`
:   DRM panel

`struct drm_connector *connector`
:   DRM connector

**Description**

The modes probed from the panel are automatically added to the connector
that the panel is attached to.

**Return**

The number of modes available from the panel on success, or 0 on
failure (no modes).

struct [drm\_panel](#c.drm_panel "drm_panel") \*drm\_panel\_get(struct [drm\_panel](#c.drm_panel "drm_panel") \*panel)
:   Acquire a panel reference

**Parameters**

`struct drm_panel *panel`
:   DRM panel

**Description**

This function increments the panel’s refcount.

**Return**

Pointer to **panel**

void drm\_panel\_put(struct [drm\_panel](#c.drm_panel "drm_panel") \*panel)
:   Release a panel reference

**Parameters**

`struct drm_panel *panel`
:   DRM panel

**Description**

This function decrements the panel’s reference count and frees the
object if the reference count drops to zero.

struct [drm\_panel](#c.drm_panel "drm_panel") \*of\_drm\_find\_panel(const struct device\_node \*np)
:   look up a panel using a device tree node

**Parameters**

`const struct device_node *np`
:   device tree node of the panel

**Description**

Searches the set of registered panels for one that matches the given device
tree node. If a matching panel is found, return a pointer to it.

Possible error codes returned by this function:

* EPROBE\_DEFER: the panel device has not been probed yet, and the caller
  should retry later
* ENODEV: the device is not available (status != “okay” or “ok”)

**Return**

A pointer to the panel registered for the specified device tree
node or an [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") if no panel matching the device tree node can be found.

int of\_drm\_get\_panel\_orientation(const struct device\_node \*np, enum [drm\_panel\_orientation](drm-kms.html#c.drm_panel_orientation "drm_panel_orientation") \*orientation)
:   look up the orientation of the panel through the “rotation” binding from a device tree node

**Parameters**

`const struct device_node *np`
:   device tree node of the panel

`enum drm_panel_orientation *orientation`
:   orientation `enum to` be filled in

**Description**

Looks up the rotation of a panel in the device tree. The orientation of the
panel is expressed as a property name “rotation” in the device tree. The
rotation in the device tree is counter clockwise.

**Return**

0 when a valid rotation value (0, 90, 180, or 270) is read or the
rotation property doesn’t exist. Return a negative error code on failure.

bool drm\_is\_panel\_follower(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev)
:   Check if the device is a panel follower

**Parameters**

`struct device *dev`
:   The ‘[`struct device`](../driver-api/infrastructure.html#c.device "device")’ to check

**Description**

This checks to see if a device needs to be power sequenced together with
a panel using the panel follower API.

The “panel” property of the follower points to the panel to be followed.

**Return**

true if we should be power sequenced with a panel; false otherwise.

int drm\_panel\_add\_follower(struct [device](../driver-api/infrastructure.html#c.device "device") \*follower\_dev, struct drm\_panel\_follower \*follower)
:   Register something to follow panel state.

**Parameters**

`struct device *follower_dev`
:   The ‘[`struct device`](../driver-api/infrastructure.html#c.device "device")’ for the follower.

`struct drm_panel_follower *follower`
:   The panel follower descriptor for the follower.

**Description**

A panel follower is called right after preparing/enabling the panel and right
before unpreparing/disabling the panel. It’s primary intention is to power on
an associated touchscreen, though it could be used for any similar devices.
Multiple devices are allowed the follow the same panel.

If a follower is added to a panel that’s already been prepared/enabled, the
follower’s prepared/enabled callback is called right away.

The “panel” property of the follower points to the panel to be followed.

**Return**

0 or an error code. Note that -ENODEV means that we detected that
follower\_dev is not actually following a panel. The caller may
choose to ignore this return value if following a panel is optional.

void drm\_panel\_remove\_follower(struct drm\_panel\_follower \*follower)
:   Reverse [`drm_panel_add_follower()`](#c.drm_panel_add_follower "drm_panel_add_follower").

**Parameters**

`struct drm_panel_follower *follower`
:   The panel follower descriptor for the follower.

**Description**

Undo [`drm_panel_add_follower()`](#c.drm_panel_add_follower "drm_panel_add_follower"). This includes calling the follower’s
unpreparing/disabling function if we’re removed from a panel that’s currently
prepared/enabled.

**Return**

0 or an error code.

int devm\_drm\_panel\_add\_follower(struct [device](../driver-api/infrastructure.html#c.device "device") \*follower\_dev, struct drm\_panel\_follower \*follower)
:   devm version of [`drm_panel_add_follower()`](#c.drm_panel_add_follower "drm_panel_add_follower")

**Parameters**

`struct device *follower_dev`
:   The ‘[`struct device`](../driver-api/infrastructure.html#c.device "device")’ for the follower.

`struct drm_panel_follower *follower`
:   The panel follower descriptor for the follower.

**Description**

Handles calling [`drm_panel_remove_follower()`](#c.drm_panel_remove_follower "drm_panel_remove_follower") using devm on the follower\_dev.

**Return**

0 or an error code.

int drm\_panel\_of\_backlight(struct [drm\_panel](#c.drm_panel "drm_panel") \*panel)
:   use backlight device node for backlight

**Parameters**

`struct drm_panel *panel`
:   DRM panel

**Description**

Use this function to enable backlight handling if your panel
uses device tree and has a backlight phandle.

When the panel is enabled backlight will be enabled after a
successful call to [`drm_panel_funcs.enable()`](#c.drm_panel_funcs "drm_panel_funcs")

When the panel is disabled backlight will be disabled before the
call to [`drm_panel_funcs.disable()`](#c.drm_panel_funcs "drm_panel_funcs").

A typical implementation for a panel driver supporting device tree
will call this function at probe time. Backlight will then be handled
transparently without requiring any intervention from the driver.
[`drm_panel_of_backlight()`](#c.drm_panel_of_backlight "drm_panel_of_backlight") must be called after the call to [`drm_panel_init()`](#c.drm_panel_init "drm_panel_init").

**Return**

0 on success or a negative error code on failure.

int drm\_get\_panel\_orientation\_quirk(int width, int height)
:   Check for panel orientation quirks

**Parameters**

`int width`
:   width in pixels of the panel

`int height`
:   height in pixels of the panel

**Description**

This function checks for platform specific (e.g. DMI based) quirks
providing info on panel\_orientation for systems where this cannot be
probed from the hard-/firm-ware. To avoid false-positive this function
takes the panel resolution as argument and checks that against the
resolution expected by the quirk-table entry.

Note this function is also used outside of the drm-subsys, by for example
the efifb code. Because of this this function gets compiled into its own
kernel-module when built as a module.

**Return**

A DRM\_MODE\_PANEL\_ORIENTATION\_\* value if there is a quirk for this system,
or DRM\_MODE\_PANEL\_ORIENTATION\_UNKNOWN if there is no quirk.

const struct drm\_panel\_backlight\_quirk \*drm\_get\_panel\_backlight\_quirk(const struct drm\_edid \*edid)
:   Get backlight quirks for a panel

**Parameters**

`const struct drm_edid *edid`
:   EDID of the panel to check

**Description**

This function checks for platform specific (e.g. DMI based) quirks
providing info on the minimum backlight brightness for systems where this
cannot be probed correctly from the hard-/firm-ware and other sources.

**Return**

a drm\_panel\_backlight\_quirk struct if a quirk was found, otherwise an
error pointer.

## Panel Self Refresh Helper Reference

This helper library provides an easy way for drivers to leverage the atomic
framework to implement panel self refresh (SR) support. Drivers are
responsible for initializing and cleaning up the SR helpers on load/unload
(see [`drm_self_refresh_helper_init`](#c.drm_self_refresh_helper_init "drm_self_refresh_helper_init")/[`drm_self_refresh_helper_cleanup`](#c.drm_self_refresh_helper_cleanup "drm_self_refresh_helper_cleanup")).
The connector is responsible for setting
[`drm_connector_state.self_refresh_aware`](drm-kms.html#c.drm_connector_state "drm_connector_state") to true at runtime if it is SR-aware
(meaning it knows how to initiate self refresh on the panel).

Once a crtc has enabled SR using [`drm_self_refresh_helper_init`](#c.drm_self_refresh_helper_init "drm_self_refresh_helper_init"), the
helpers will monitor activity and call back into the driver to enable/disable
SR as appropriate. The best way to think about this is that it’s a DPMS
on/off request with [`drm_crtc_state.self_refresh_active`](drm-kms.html#c.drm_crtc_state "drm_crtc_state") set in crtc state
that tells you to disable/enable SR on the panel instead of power-cycling it.

During SR, drivers may choose to fully disable their crtc/encoder/bridge
hardware (in which case no driver changes are necessary), or they can inspect
[`drm_crtc_state.self_refresh_active`](drm-kms.html#c.drm_crtc_state "drm_crtc_state") if they want to enter low power mode
without full disable (in case full disable/enable is too slow).

SR will be deactivated if there are any atomic updates affecting the
pipe that is in SR mode. If a crtc is driving multiple connectors, all
connectors must be SR aware and all will enter/exit SR mode at the same time.

If the crtc and connector are SR aware, but the panel connected does not
support it (or is otherwise unable to enter SR), the driver should fail
atomic\_check when [`drm_crtc_state.self_refresh_active`](drm-kms.html#c.drm_crtc_state "drm_crtc_state") is true.

void drm\_self\_refresh\_helper\_update\_avg\_times(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state, unsigned int commit\_time\_ms, unsigned int new\_self\_refresh\_mask)
:   Updates a crtc’s SR time averages

**Parameters**

`struct drm_atomic_state *state`
:   the state which has just been applied to hardware

`unsigned int commit_time_ms`
:   the amount of time in ms that this commit took to complete

`unsigned int new_self_refresh_mask`
:   bitmask of crtc’s that have self\_refresh\_active in
    new state

**Description**

Called after [`drm_mode_config_funcs.atomic_commit_tail`](drm-kms.html#c.drm_mode_config_funcs "drm_mode_config_funcs"), this function will
update the average entry/exit self refresh times on self refresh transitions.
These averages will be used when calculating how long to delay before
entering self refresh mode after activity.

void drm\_self\_refresh\_helper\_alter\_state(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   Alters the atomic state for SR exit

**Parameters**

`struct drm_atomic_state *state`
:   the state currently being checked

**Description**

Called at the end of atomic check. This function checks the state for flags
incompatible with self refresh exit and changes them. This is a bit
disingenuous since userspace is expecting one thing and we’re giving it
another. However in order to keep self refresh entirely hidden from
userspace, this is required.

At the end, we queue up the self refresh entry work so we can enter PSR after
the desired delay.

int drm\_self\_refresh\_helper\_init(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc)
:   Initializes self refresh helpers for a crtc

**Parameters**

`struct drm_crtc *crtc`
:   the crtc which supports self refresh supported displays

**Description**

Returns zero if successful or -errno on failure

void drm\_self\_refresh\_helper\_cleanup(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc)
:   Cleans up self refresh helpers for a crtc

**Parameters**

`struct drm_crtc *crtc`
:   the crtc to cleanup

## HDMI Atomic State Helpers

### Overview

These functions contain an implementation of the HDMI specification
in the form of KMS helpers.

It contains TMDS character rate computation, automatic selection of
output formats, infoframes generation, etc.

#### Infoframes Compliance

Drivers using the helpers will expose the various infoframes
generated according to the HDMI specification in debugfs.

Compliance can then be tested using `edid-decode` from the `v4l-utils` project
(<https://git.linuxtv.org/v4l-utils.git/>). A sample run would look like:

```
# edid-decode \
        -I /sys/kernel/debug/dri/1/HDMI-A-1/infoframes/audio \
        -I /sys/kernel/debug/dri/1/HDMI-A-1/infoframes/avi \
        -I /sys/kernel/debug/dri/1/HDMI-A-1/infoframes/hdmi \
        -I /sys/kernel/debug/dri/1/HDMI-A-1/infoframes/hdr_drm \
        -I /sys/kernel/debug/dri/1/HDMI-A-1/infoframes/spd \
        /sys/class/drm/card1-HDMI-A-1/edid \
        -c

edid-decode (hex):

00 ff ff ff ff ff ff 00 1e 6d f4 5b 1e ef 06 00
07 20 01 03 80 2f 34 78 ea 24 05 af 4f 42 ab 25
0f 50 54 21 08 00 d1 c0 61 40 45 40 01 01 01 01
01 01 01 01 01 01 98 d0 00 40 a1 40 d4 b0 30 20
3a 00 d1 0b 12 00 00 1a 00 00 00 fd 00 3b 3d 1e
b2 31 00 0a 20 20 20 20 20 20 00 00 00 fc 00 4c
47 20 53 44 51 48 44 0a 20 20 20 20 00 00 00 ff
00 32 30 37 4e 54 52 4c 44 43 34 33 30 0a 01 46

02 03 42 72 23 09 07 07 4d 01 03 04 90 12 13 1f
22 5d 5e 5f 60 61 83 01 00 00 6d 03 0c 00 10 00
b8 3c 20 00 60 01 02 03 67 d8 5d c4 01 78 80 03
e3 0f 00 18 e2 00 6a e3 05 c0 00 e6 06 05 01 52
52 51 11 5d 00 a0 a0 40 29 b0 30 20 3a 00 d1 0b
12 00 00 1a 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 c3

----------------

Block 0, Base EDID:
  EDID Structure Version & Revision: 1.3
  Vendor & Product Identification:
    Manufacturer: GSM
    Model: 23540
    Serial Number: 454430 (0x0006ef1e)
    Made in: week 7 of 2022
  Basic Display Parameters & Features:
    Digital display
    Maximum image size: 47 cm x 52 cm
    Gamma: 2.20
    DPMS levels: Standby Suspend Off
    RGB color display
    First detailed timing is the preferred timing
  Color Characteristics:
    Red  : 0.6835, 0.3105
    Green: 0.2587, 0.6679
    Blue : 0.1445, 0.0585
    White: 0.3134, 0.3291
  Established Timings I & II:
    DMT 0x04:   640x480    59.940476 Hz   4:3     31.469 kHz     25.175000 MHz
    DMT 0x09:   800x600    60.316541 Hz   4:3     37.879 kHz     40.000000 MHz
    DMT 0x10:  1024x768    60.003840 Hz   4:3     48.363 kHz     65.000000 MHz
  Standard Timings:
    DMT 0x52:  1920x1080   60.000000 Hz  16:9     67.500 kHz    148.500000 MHz
    DMT 0x10:  1024x768    60.003840 Hz   4:3     48.363 kHz     65.000000 MHz
    DMT 0x09:   800x600    60.316541 Hz   4:3     37.879 kHz     40.000000 MHz
  Detailed Timing Descriptors:
    DTD 1:  2560x2880   59.966580 Hz   8:9    185.417 kHz    534.000000 MHz (465 mm x 523 mm)
                 Hfront   48 Hsync  32 Hback  240 Hpol P
                 Vfront    3 Vsync  10 Vback  199 Vpol N
    Display Range Limits:
      Monitor ranges (GTF): 59-61 Hz V, 30-178 kHz H, max dotclock 490 MHz
    Display Product Name: 'LG SDQHD'
    Display Product Serial Number: '207NTRLDC430'
  Extension blocks: 1
Checksum: 0x46

----------------

Block 1, CTA-861 Extension Block:
  Revision: 3
  Basic audio support
  Supports YCbCr 4:4:4
  Supports YCbCr 4:2:2
  Native detailed modes: 2
  Audio Data Block:
    Linear PCM:
      Max channels: 2
      Supported sample rates (kHz): 48 44.1 32
      Supported sample sizes (bits): 24 20 16
  Video Data Block:
    VIC   1:   640x480    59.940476 Hz   4:3     31.469 kHz     25.175000 MHz
    VIC   3:   720x480    59.940060 Hz  16:9     31.469 kHz     27.000000 MHz
    VIC   4:  1280x720    60.000000 Hz  16:9     45.000 kHz     74.250000 MHz
    VIC  16:  1920x1080   60.000000 Hz  16:9     67.500 kHz    148.500000 MHz (native)
    VIC  18:   720x576    50.000000 Hz  16:9     31.250 kHz     27.000000 MHz
    VIC  19:  1280x720    50.000000 Hz  16:9     37.500 kHz     74.250000 MHz
    VIC  31:  1920x1080   50.000000 Hz  16:9     56.250 kHz    148.500000 MHz
    VIC  34:  1920x1080   30.000000 Hz  16:9     33.750 kHz     74.250000 MHz
    VIC  93:  3840x2160   24.000000 Hz  16:9     54.000 kHz    297.000000 MHz
    VIC  94:  3840x2160   25.000000 Hz  16:9     56.250 kHz    297.000000 MHz
    VIC  95:  3840x2160   30.000000 Hz  16:9     67.500 kHz    297.000000 MHz
    VIC  96:  3840x2160   50.000000 Hz  16:9    112.500 kHz    594.000000 MHz
    VIC  97:  3840x2160   60.000000 Hz  16:9    135.000 kHz    594.000000 MHz
  Speaker Allocation Data Block:
    FL/FR - Front Left/Right
  Vendor-Specific Data Block (HDMI), OUI 00-0C-03:
    Source physical address: 1.0.0.0
    Supports_AI
    DC_36bit
    DC_30bit
    DC_Y444
    Maximum TMDS clock: 300 MHz
    Extended HDMI video details:
      HDMI VICs:
        HDMI VIC 1:  3840x2160   30.000000 Hz  16:9     67.500 kHz    297.000000 MHz
        HDMI VIC 2:  3840x2160   25.000000 Hz  16:9     56.250 kHz    297.000000 MHz
        HDMI VIC 3:  3840x2160   24.000000 Hz  16:9     54.000 kHz    297.000000 MHz
  Vendor-Specific Data Block (HDMI Forum), OUI C4-5D-D8:
    Version: 1
    Maximum TMDS Character Rate: 600 MHz
    SCDC Present
    Supports 12-bits/component Deep Color 4:2:0 Pixel Encoding
    Supports 10-bits/component Deep Color 4:2:0 Pixel Encoding
  YCbCr 4:2:0 Capability Map Data Block:
    VIC  96:  3840x2160   50.000000 Hz  16:9    112.500 kHz    594.000000 MHz
    VIC  97:  3840x2160   60.000000 Hz  16:9    135.000 kHz    594.000000 MHz
  Video Capability Data Block:
    YCbCr quantization: No Data
    RGB quantization: Selectable (via AVI Q)
    PT scan behavior: Always Underscanned
    IT scan behavior: Always Underscanned
    CE scan behavior: Always Underscanned
  Colorimetry Data Block:
    BT2020YCC
    BT2020RGB
  HDR Static Metadata Data Block:
    Electro optical transfer functions:
      Traditional gamma - SDR luminance range
      SMPTE ST2084
    Supported static metadata descriptors:
      Static metadata type 1
    Desired content max luminance: 82 (295.365 cd/m^2)
    Desired content max frame-average luminance: 82 (295.365 cd/m^2)
    Desired content min luminance: 81 (0.298 cd/m^2)
  Detailed Timing Descriptors:
    DTD 2:  2560x2880   29.986961 Hz   8:9     87.592 kHz    238.250000 MHz (465 mm x 523 mm)
                 Hfront   48 Hsync  32 Hback   80 Hpol P
                 Vfront    3 Vsync  10 Vback   28 Vpol N
Checksum: 0xc3  Unused space in Extension Block: 43 bytes

----------------

edid-decode 1.29.0-5346
edid-decode SHA: c363e9aa6d70 2025-03-11 11:41:18

Warnings:

Block 1, CTA-861 Extension Block:
  IT Video Formats are overscanned by default, but normally this should be underscanned.
  Video Data Block: VIC 1 and the first DTD are not identical. Is this intended?
  Video Data Block: All VICs are in ascending order, and the first (preferred) VIC <= 4, is that intended?
  Video Capability Data Block: Set Selectable YCbCr Quantization to avoid interop issues.
  Video Capability Data Block: S_PT is equal to S_IT and S_CE, so should be set to 0 instead.
  Colorimetry Data Block: Set the sRGB colorimetry bit to avoid interop issues.
  Display Product Serial Number is set, so the Serial Number in the Base EDID should be 0.
EDID:
  Base EDID: Some timings are out of range of the Monitor Ranges:
    Vertical Freq: 24.000 - 60.317 Hz (Monitor: 59.000 - 61.000 Hz)
    Horizontal Freq: 31.250 - 185.416 kHz (Monitor: 30.000 - 178.000 kHz)
    Maximum Clock: 594.000 MHz (Monitor: 490.000 MHz)

Failures:

Block 1, CTA-861 Extension Block:
  Video Capability Data Block: IT video formats are always underscanned, but bit 7 of Byte 3 of the CTA-861 Extension header is set to overscanned.
EDID:
  CTA-861: Native progressive timings are a mix of several resolutions.

EDID conformity: FAIL

================

InfoFrame of '/sys/kernel/debug/dri/1/HDMI-A-1/infoframes/audio' was empty.

================

edid-decode InfoFrame (hex):

82 02 0d 31 12 28 04 00 00 00 00 00 00 00 00 00
00

----------------

HDMI InfoFrame Checksum: 0x31

AVI InfoFrame
  Version: 2
  Length: 13
  Y: Color Component Sample Format: RGB
  A: Active Format Information Present: Yes
  B: Bar Data Present: Bar Data not present
  S: Scan Information: Composed for an underscanned display
  C: Colorimetry: No Data
  M: Picture Aspect Ratio: 16:9
  R: Active Portion Aspect Ratio: 8
  ITC: IT Content: No Data
  EC: Extended Colorimetry: xvYCC601
  Q: RGB Quantization Range: Limited Range
  SC: Non-Uniform Picture Scaling: No Known non-uniform scaling
  YQ: YCC Quantization Range: Limited Range
  CN: IT Content Type: Graphics
  PR: Pixel Data Repetition Count: 0
  Line Number of End of Top Bar: 0
  Line Number of Start of Bottom Bar: 0
  Pixel Number of End of Left Bar: 0
  Pixel Number of Start of Right Bar: 0

----------------

AVI InfoFrame conformity: PASS

================

edid-decode InfoFrame (hex):

81 01 05 49 03 0c 00 20 01

----------------

HDMI InfoFrame Checksum: 0x49

Vendor-Specific InfoFrame (HDMI), OUI 00-0C-03
  Version: 1
  Length: 5
  HDMI Video Format: HDMI_VIC is present
  HDMI VIC 1:  3840x2160   30.000000 Hz  16:9     67.500 kHz    297.000000 MHz

----------------

Vendor-Specific InfoFrame (HDMI), OUI 00-0C-03 conformity: PASS

================

InfoFrame of '/sys/kernel/debug/dri/1/HDMI-A-1/infoframes/hdr_drm' was empty.

================

edid-decode InfoFrame (hex):

83 01 19 93 42 72 6f 61 64 63 6f 6d 56 69 64 65
6f 63 6f 72 65 00 00 00 00 00 00 00 09

----------------

HDMI InfoFrame Checksum: 0x93

Source Product Description InfoFrame
  Version: 1
  Length: 25
  Vendor Name: 'Broadcom'
  Product Description: 'Videocore'
  Source Information: PC general

----------------

Source Product Description InfoFrame conformity: PASS
```

#### Testing

The helpers have unit testing and can be tested using kunit with:

```
$ ./tools/testing/kunit/kunit.py run \
        --kunitconfig=drivers/gpu/drm/tests \
        drm_atomic_helper_connector_hdmi_*
```

### Functions Reference

void \_\_drm\_atomic\_helper\_connector\_hdmi\_reset(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct [drm\_connector\_state](drm-kms.html#c.drm_connector_state "drm_connector_state") \*new\_conn\_state)
:   Initializes all HDMI **drm\_connector\_state** resources

**Parameters**

`struct drm_connector *connector`
:   DRM connector

`struct drm_connector_state *new_conn_state`
:   connector state to reset

**Description**

Initializes all HDMI resources from a **drm\_connector\_state** without
actually allocating it. This is useful for HDMI drivers, in
combination with [`__drm_atomic_helper_connector_reset()`](#c.__drm_atomic_helper_connector_reset "__drm_atomic_helper_connector_reset") or
[`drm_atomic_helper_connector_reset()`](#c.drm_atomic_helper_connector_reset "drm_atomic_helper_connector_reset").

int drm\_atomic\_helper\_connector\_hdmi\_check(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   Helper to check HDMI connector atomic state

**Parameters**

`struct drm_connector *connector`
:   DRM Connector

`struct drm_atomic_state *state`
:   the DRM State object

**Description**

Provides a default connector state check handler for HDMI connectors.
Checks that a desired connector update is valid, and updates various
fields of derived state.

**Return**

Zero on success, or an errno code otherwise.

enum [drm\_mode\_status](drm-kms.html#c.drm_mode_status "drm_mode_status") drm\_hdmi\_connector\_mode\_valid(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, const struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*mode)
:   Check if mode is valid for HDMI connector

**Parameters**

`struct drm_connector *connector`
:   DRM connector to validate the mode

`const struct drm_display_mode *mode`
:   Display mode to validate

**Description**

Generic .mode\_valid implementation for HDMI connectors.

int drm\_atomic\_helper\_connector\_hdmi\_update\_infoframes(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   Update the Infoframes

**Parameters**

`struct drm_connector *connector`
:   A pointer to the HDMI connector

`struct drm_atomic_state *state`
:   The HDMI connector state to generate the infoframe from

**Description**

This function is meant for HDMI connector drivers to write their
infoframes. It will typically be used in a
**drm\_connector\_helper\_funcs.atomic\_enable** implementation.

**Return**

Zero on success, error code on failure.

int drm\_atomic\_helper\_connector\_hdmi\_update\_audio\_infoframe(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct hdmi\_audio\_infoframe \*frame)
:   Update the Audio Infoframe

**Parameters**

`struct drm_connector *connector`
:   A pointer to the HDMI connector

`struct hdmi_audio_infoframe *frame`
:   A pointer to the audio infoframe to write

**Description**

This function is meant for HDMI connector drivers to update their
audio infoframe. It will typically be used in one of the ALSA hooks
(most likely prepare).

**Return**

Zero on success, error code on failure.

int drm\_atomic\_helper\_connector\_hdmi\_clear\_audio\_infoframe(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   Stop sending the Audio Infoframe

**Parameters**

`struct drm_connector *connector`
:   A pointer to the HDMI connector

**Description**

This function is meant for HDMI connector drivers to stop sending their
audio infoframe. It will typically be used in one of the ALSA hooks
(most likely shutdown).

**Return**

Zero on success, error code on failure.

void drm\_atomic\_helper\_connector\_hdmi\_hotplug(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, enum [drm\_connector\_status](drm-kms.html#c.drm_connector_status "drm_connector_status") status)
:   Handle the hotplug event for the HDMI connector

**Parameters**

`struct drm_connector *connector`
:   A pointer to the HDMI connector

`enum drm_connector_status status`
:   Connection status

**Description**

This function should be called as a part of the .`detect()` / .`detect_ctx()`
callbacks for all status changes.

void drm\_atomic\_helper\_connector\_hdmi\_force(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   HDMI Connector implementation of the force callback

**Parameters**

`struct drm_connector *connector`
:   A pointer to the HDMI connector

**Description**

This function implements the .`force()` callback for the HDMI connectors. It
can either be used directly as the callback or should be called from within
the .`force()` callback implementation to maintain the HDMI-specific
connector’s data.

## HDCP Helper Functions Reference

int drm\_hdcp\_check\_ksvs\_revoked(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*drm\_dev, u8 \*ksvs, u32 ksv\_count)
:   Check the revoked status of the IDs

**Parameters**

`struct drm_device *drm_dev`
:   drm\_device for which HDCP revocation check is requested

`u8 *ksvs`
:   List of KSVs (HDCP receiver IDs)

`u32 ksv_count`
:   KSV count passed in through **ksvs**

**Description**

This function reads the HDCP System renewability Message(SRM Table)
from userspace as a firmware and parses it for the revoked HDCP
KSVs(Receiver IDs) detected by DCP LLC. Once the revoked KSVs are known,
revoked state of the KSVs in the list passed in by display drivers are
decided and response is sent.

SRM should be presented in the name of “display\_hdcp\_srm.bin”.

Format of the SRM table, that userspace needs to write into the binary file,
is defined at:
1. Renewability chapter on 55th page of HDCP 1.4 specification
<https://www.digital-cp.com/sites/default/files/specifications>/HDCP``20Specification````20Rev1\_4\_Secure``.pdf
2. Renewability chapter on 63rd page of HDCP 2.2 specification
<https://www.digital-cp.com/sites/default/files/specifications>/HDCP``20on````20HDMI````20Specification````20Rev2\_2\_Final1``.pdf

**Return**

Count of the revoked KSVs or -ve error number in case of the failure.

int drm\_connector\_attach\_content\_protection\_property(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, bool hdcp\_content\_type)
:   attach content protection property

**Parameters**

`struct drm_connector *connector`
:   connector to attach CP property on.

`bool hdcp_content_type`
:   is HDCP Content Type property needed for connector

**Description**

This is used to add support for content protection on select connectors.
Content Protection is intentionally vague to allow for different underlying
technologies, however it is most implemented by HDCP.

When hdcp\_content\_type is true `enum property` called HDCP Content Type is
created (if it is not already) and attached to the connector.

This property is used for sending the protected content’s stream type
from userspace to kernel on selected connectors. Protected content provider
will decide their type of their content and declare the same to kernel.

Content type will be used during the HDCP 2.2 authentication.
Content type will be set to [`drm_connector_state.hdcp_content_type`](drm-kms.html#c.drm_connector_state "drm_connector_state").

The content protection will be set to [`drm_connector_state.content_protection`](drm-kms.html#c.drm_connector_state "drm_connector_state")

When kernel triggered content protection state change like DESIRED->ENABLED
and ENABLED->DESIRED, will use [`drm_hdcp_update_content_protection()`](#c.drm_hdcp_update_content_protection "drm_hdcp_update_content_protection") to update
the content protection state of a connector.

**Return**

Zero on success, negative errno on failure.

void drm\_hdcp\_update\_content\_protection(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, u64 val)
:   Updates the content protection state of a connector

**Parameters**

`struct drm_connector *connector`
:   drm\_connector on which content protection state needs an update

`u64 val`
:   New state of the content protection property

**Description**

This function can be used by display drivers, to update the kernel triggered
content protection state changes of a drm\_connector such as DESIRED->ENABLED
and ENABLED->DESIRED. No uevent for DESIRED->UNDESIRED or ENABLED->UNDESIRED,
as userspace is triggering such state change and kernel performs it without
fail.This function update the new state of the property into the connector’s
state and generate an uevent to notify the userspace.

## Display Port Helper Functions Reference

These functions contain some common logic and helpers at various abstraction
levels to deal with Display Port sink devices and related things like DP aux
channel transfers, EDID reading over DP aux channels, decoding certain DPCD
blocks, ...

The DisplayPort AUX channel is an abstraction to allow generic, driver-
independent access to AUX functionality. Drivers can take advantage of
this by filling in the fields of the drm\_dp\_aux structure.

Transactions are described using a hardware-independent drm\_dp\_aux\_msg
structure, which is passed into a driver’s .`transfer()` implementation.
Both native and I2C-over-AUX transactions are supported.

struct dp\_sdp\_header
:   DP secondary data packet header

**Definition**:

```
struct dp_sdp_header {
    u8 HB0;
    u8 HB1;
    u8 HB2;
    u8 HB3;
};
```

**Members**

`HB0`
:   Secondary Data Packet ID

`HB1`
:   Secondary Data Packet Type

`HB2`
:   Secondary Data Packet Specific header, Byte 0

`HB3`
:   Secondary Data packet Specific header, Byte 1

struct dp\_sdp
:   DP secondary data packet

**Definition**:

```
struct dp_sdp {
    struct dp_sdp_header sdp_header;
    u8 db[32];
};
```

**Members**

`sdp_header`
:   DP secondary data packet header

`db`
:   DP secondaray data packet data blocks
    VSC SDP Payload for PSR
    db[0]: Stereo Interface
    db[1]: 0 - PSR State; 1 - Update RFB; 2 - CRC Valid
    db[2]: CRC value bits 7:0 of the R or Cr component
    db[3]: CRC value bits 15:8 of the R or Cr component
    db[4]: CRC value bits 7:0 of the G or Y component
    db[5]: CRC value bits 15:8 of the G or Y component
    db[6]: CRC value bits 7:0 of the B or Cb component
    db[7]: CRC value bits 15:8 of the B or Cb component
    db[8] - db[31]: Reserved
    VSC SDP Payload for Pixel Encoding/Colorimetry Format
    db[0] - db[15]: Reserved
    db[16]: Pixel Encoding and Colorimetry Formats
    db[17]: Dynamic Range and Component Bit Depth
    db[18]: Content Type
    db[19] - db[31]: Reserved

enum dp\_pixelformat
:   drm DP Pixel encoding formats

**Constants**

`DP_PIXELFORMAT_RGB`
:   RGB pixel encoding format

`DP_PIXELFORMAT_YUV444`
:   YCbCr 4:4:4 pixel encoding format

`DP_PIXELFORMAT_YUV422`
:   YCbCr 4:2:2 pixel encoding format

`DP_PIXELFORMAT_YUV420`
:   YCbCr 4:2:0 pixel encoding format

`DP_PIXELFORMAT_Y_ONLY`
:   Y Only pixel encoding format

`DP_PIXELFORMAT_RAW`
:   RAW pixel encoding format

`DP_PIXELFORMAT_RESERVED`
:   Reserved pixel encoding format

**Description**

This `enum is` used to indicate DP VSC SDP Pixel encoding formats.
It is based on DP 1.4 spec [Table 2-117: VSC SDP Payload for DB16 through
DB18]

enum dp\_colorimetry
:   drm DP Colorimetry formats

**Constants**

`DP_COLORIMETRY_DEFAULT`
:   sRGB (IEC 61966-2-1) or
    ITU-R BT.601 colorimetry format

`DP_COLORIMETRY_RGB_WIDE_FIXED`
:   RGB wide gamut fixed point colorimetry format

`DP_COLORIMETRY_BT709_YCC`
:   ITU-R BT.709 colorimetry format

`DP_COLORIMETRY_RGB_WIDE_FLOAT`
:   RGB wide gamut floating point
    (scRGB (IEC 61966-2-2)) colorimetry format

`DP_COLORIMETRY_XVYCC_601`
:   xvYCC601 colorimetry format

`DP_COLORIMETRY_OPRGB`
:   OpRGB colorimetry format

`DP_COLORIMETRY_XVYCC_709`
:   xvYCC709 colorimetry format

`DP_COLORIMETRY_DCI_P3_RGB`
:   DCI-P3 (SMPTE RP 431-2) colorimetry format

`DP_COLORIMETRY_SYCC_601`
:   sYCC601 colorimetry format

`DP_COLORIMETRY_RGB_CUSTOM`
:   RGB Custom Color Profile colorimetry format

`DP_COLORIMETRY_OPYCC_601`
:   opYCC601 colorimetry format

`DP_COLORIMETRY_BT2020_RGB`
:   ITU-R BT.2020 R’ G’ B’ colorimetry format

`DP_COLORIMETRY_BT2020_CYCC`
:   ITU-R BT.2020 Y’c C’bc C’rc colorimetry format

`DP_COLORIMETRY_BT2020_YCC`
:   ITU-R BT.2020 Y’ C’b C’r colorimetry format

**Description**

This `enum is` used to indicate DP VSC SDP Colorimetry formats.
It is based on DP 1.4 spec [Table 2-117: VSC SDP Payload for DB16 through
DB18] and a name of `enum member` follows `enum drm_colorimetry` definition.

enum dp\_dynamic\_range
:   drm DP Dynamic Range

**Constants**

`DP_DYNAMIC_RANGE_VESA`
:   VESA range

`DP_DYNAMIC_RANGE_CTA`
:   CTA range

**Description**

This `enum is` used to indicate DP VSC SDP Dynamic Range.
It is based on DP 1.4 spec [Table 2-117: VSC SDP Payload for DB16 through
DB18]

enum dp\_content\_type
:   drm DP Content Type

**Constants**

`DP_CONTENT_TYPE_NOT_DEFINED`
:   Not defined type

`DP_CONTENT_TYPE_GRAPHICS`
:   Graphics type

`DP_CONTENT_TYPE_PHOTO`
:   Photo type

`DP_CONTENT_TYPE_VIDEO`
:   Video type

`DP_CONTENT_TYPE_GAME`
:   Game type

**Description**

This `enum is` used to indicate DP VSC SDP Content Types.
It is based on DP 1.4 spec [Table 2-117: VSC SDP Payload for DB16 through
DB18]
CTA-861-G defines content types and expected processing by a sink device

struct drm\_dp\_vsc\_sdp
:   drm DP VSC SDP

**Definition**:

```
struct drm_dp_vsc_sdp {
    unsigned char sdp_type;
    unsigned char revision;
    unsigned char length;
    enum dp_pixelformat pixelformat;
    enum dp_colorimetry colorimetry;
    int bpc;
    enum dp_dynamic_range dynamic_range;
    enum dp_content_type content_type;
};
```

**Members**

`sdp_type`
:   secondary-data packet type

`revision`
:   revision number

`length`
:   number of valid data bytes

`pixelformat`
:   pixel encoding format

`colorimetry`
:   colorimetry format

`bpc`
:   bit per color

`dynamic_range`
:   dynamic range information

`content_type`
:   CTA-861-G defines content types and expected processing by a sink device

**Description**

This structure represents a DP VSC SDP of drm
It is based on DP 1.4 spec [Table 2-116: VSC SDP Header Bytes] and
[Table 2-117: VSC SDP Payload for DB16 through DB18]

struct drm\_dp\_as\_sdp
:   drm DP Adaptive Sync SDP

**Definition**:

```
struct drm_dp_as_sdp {
    unsigned char sdp_type;
    unsigned char revision;
    unsigned char length;
    int vtotal;
    int target_rr;
    int duration_incr_ms;
    int duration_decr_ms;
    bool target_rr_divider;
    enum operation_mode mode;
};
```

**Members**

`sdp_type`
:   Secondary-data packet type

`revision`
:   Revision Number

`length`
:   Number of valid data bytes

`vtotal`
:   Minimum Vertical Vtotal

`target_rr`
:   Target Refresh

`duration_incr_ms`
:   Successive frame duration increase

`duration_decr_ms`
:   Successive frame duration decrease

`target_rr_divider`
:   Target refresh rate divider

`mode`
:   Adaptive Sync Operation Mode

**Description**

This structure represents a DP AS SDP of drm
It is based on DP 2.1 spec [Table 2-126: Adaptive-Sync SDP Header Bytes] and
[Table 2-127: Adaptive-Sync SDP Payload for DB0 through DB8]

bool drm\_dp\_dsc\_sink\_supports\_format(const u8 dsc\_dpcd[DP\_DSC\_RECEIVER\_CAP\_SIZE], u8 output\_format)
:   check if sink supports DSC with given output format

**Parameters**

`const u8 dsc_dpcd[DP_DSC_RECEIVER_CAP_SIZE]`
:   DSC-capability DPCDs of the sink

`u8 output_format`
:   output\_format which is to be checked

**Description**

Returns true if the sink supports DSC with the given output\_format, false otherwise.

bool drm\_edp\_backlight\_supported(const u8 edp\_dpcd[EDP\_DISPLAY\_CTL\_CAP\_SIZE])
:   Check an eDP DPCD for VESA backlight support

**Parameters**

`const u8 edp_dpcd[EDP_DISPLAY_CTL_CAP_SIZE]`
:   The DPCD to check

**Description**

Note that currently this function will return `false` for panels which support various DPCD
backlight features but which require the brightness be set through PWM, and don’t support setting
the brightness level via the DPCD.

**Return**

`True` if **edp\_dpcd** indicates that VESA backlight controls are supported, `false`
otherwise

bool drm\_dp\_is\_uhbr\_rate(int link\_rate)
:   Determine if a link rate is UHBR

**Parameters**

`int link_rate`
:   link rate in 10kbits/s units

**Description**

Determine if the provided link rate is an UHBR rate.

**Return**

`True` if **link\_rate** is an UHBR rate.

struct drm\_dp\_aux\_msg
:   DisplayPort AUX channel transaction

**Definition**:

```
struct drm_dp_aux_msg {
    unsigned int address;
    u8 request;
    u8 reply;
    void *buffer;
    size_t size;
};
```

**Members**

`address`
:   address of the (first) register to access

`request`
:   contains the type of transaction (see DP\_AUX\_\* macros)

`reply`
:   upon completion, contains the reply type of the transaction

`buffer`
:   pointer to a transmission or reception buffer

`size`
:   size of **buffer**

struct drm\_dp\_aux\_cec
:   DisplayPort CEC-Tunneling-over-AUX

**Definition**:

```
struct drm_dp_aux_cec {
    struct mutex lock;
    struct cec_adapter *adap;
    struct drm_connector *connector;
    struct delayed_work unregister_work;
};
```

**Members**

`lock`
:   mutex protecting this struct

`adap`
:   the CEC adapter for CEC-Tunneling-over-AUX support.

`connector`
:   the connector this CEC adapter is associated with

`unregister_work`
:   unregister the CEC adapter

struct drm\_dp\_aux
:   DisplayPort AUX channel

**Definition**:

```
struct drm_dp_aux {
    const char *name;
    struct i2c_adapter ddc;
    struct device *dev;
    struct drm_device *drm_dev;
    struct drm_crtc *crtc;
    struct mutex hw_mutex;
    struct work_struct crc_work;
    u8 crc_count;
    ssize_t (*transfer)(struct drm_dp_aux *aux, struct drm_dp_aux_msg *msg);
    int (*wait_hpd_asserted)(struct drm_dp_aux *aux, unsigned long wait_us);
    unsigned i2c_nack_count;
    unsigned i2c_defer_count;
    struct drm_dp_aux_cec cec;
    bool is_remote;
    bool powered_down;
    bool no_zero_sized;
    bool dpcd_probe_disabled;
};
```

**Members**

`name`
:   user-visible name of this AUX channel and the
    I2C-over-AUX adapter.

    It’s also used to specify the name of the I2C adapter. If set
    to `NULL`, [`dev_name()`](../driver-api/infrastructure.html#c.dev_name "dev_name") of **dev** will be used.

`ddc`
:   I2C adapter that can be used for I2C-over-AUX
    communication

`dev`
:   pointer to [`struct device`](../driver-api/infrastructure.html#c.device "device") that is the parent for this
    AUX channel.

`drm_dev`
:   pointer to the [`drm_device`](drm-internals.html#c.drm_device "drm_device") that owns this AUX channel.
    Beware, this may be `NULL` before [`drm_dp_aux_register()`](#c.drm_dp_aux_register "drm_dp_aux_register") has been
    called.

    It should be set to the [`drm_device`](drm-internals.html#c.drm_device "drm_device") that will be using this AUX
    channel as early as possible. For many graphics drivers this should
    happen before [`drm_dp_aux_init()`](#c.drm_dp_aux_init "drm_dp_aux_init"), however it’s perfectly fine to set
    this field later so long as it’s assigned before calling
    [`drm_dp_aux_register()`](#c.drm_dp_aux_register "drm_dp_aux_register").

`crtc`
:   backpointer to the crtc that is currently using this
    AUX channel

`hw_mutex`
:   internal mutex used for locking transfers.

    Note that if the underlying hardware is shared among multiple
    channels, the driver needs to do additional locking to
    prevent concurrent access.

`crc_work`
:   worker that captures CRCs for each frame

`crc_count`
:   counter of captured frame CRCs

`transfer`
:   transfers a message representing a single AUX
    transaction.

    This is a hardware-specific implementation of how
    transactions are executed that the drivers must provide.

    A pointer to a [`drm_dp_aux_msg`](#c.drm_dp_aux_msg "drm_dp_aux_msg") structure describing the
    transaction is passed into this function. Upon success, the
    implementation should return the number of payload bytes that
    were transferred, or a negative error-code on failure.

    Helpers will propagate these errors, with the exception of
    the `-EBUSY` error, which causes a transaction to be retried.
    On a short, helpers will return `-EPROTO` to make it simpler
    to check for failure.

    The **`transfer()`** function must only modify the reply field of
    the [`drm_dp_aux_msg`](#c.drm_dp_aux_msg "drm_dp_aux_msg") structure. The retry logic and i2c
    helpers assume this is the case.

    Also note that this callback can be called no matter the
    state **dev** is in and also no matter what state the panel is
    in. It’s expected:

    * If the **dev** providing the AUX bus is currently unpowered then
      it will power itself up for the transfer.
    * If we’re on eDP (using a drm\_panel) and the panel is not in a
      state where it can respond (it’s not powered or it’s in a
      low power state) then this function may return an error, but
      not crash. It’s up to the caller of this code to make sure that
      the panel is powered on if getting an error back is not OK. If a
      drm\_panel driver is initiating a DP AUX transfer it may power
      itself up however it wants. All other code should ensure that
      the `pre_enable()` bridge chain (which eventually calls the
      drm\_panel prepare function) has powered the panel.

`wait_hpd_asserted`
:   wait for HPD to be asserted

    This is mainly useful for eDP panels drivers to wait for an eDP
    panel to finish powering on. It is optional for DP AUX controllers
    to implement this function. It is required for DP AUX endpoints
    (panel drivers) to call this function after powering up but before
    doing AUX transfers unless the DP AUX endpoint driver knows that
    we’re not using the AUX controller’s HPD. One example of the panel
    driver not needing to call this is if HPD is hooked up to a GPIO
    that the panel driver can read directly.

    If a DP AUX controller does not implement this function then it
    may still support eDP panels that use the AUX controller’s built-in
    HPD signal by implementing a long wait for HPD in the `transfer()`
    callback, though this is deprecated.

    This function will efficiently wait for the HPD signal to be
    asserted. The wait\_us parameter that is passed in says that we
    know that the HPD signal is expected to be asserted within wait\_us
    microseconds. This function could wait for longer than wait\_us if
    the logic in the DP controller has a long debouncing time. The
    important thing is that if this function returns success that the
    DP controller is ready to send AUX transactions.

    This function returns 0 if HPD was asserted or -ETIMEDOUT if time
    expired and HPD wasn’t asserted. This function should not print
    timeout errors to the log.

    The semantics of this function are designed to match the
    `readx_poll_timeout()` function. That means a wait\_us of 0 means
    to wait forever. Like `readx_poll_timeout()`, this function may sleep.

    NOTE: this function specifically reports the state of the HPD pin
    that’s associated with the DP AUX channel. This is different from
    the HPD concept in much of the rest of DRM which is more about
    physical presence of a display. For eDP, for instance, a display is
    assumed always present even if the HPD pin is deasserted.

`i2c_nack_count`
:   Counts I2C NACKs, used for DP validation.

`i2c_defer_count`
:   Counts I2C DEFERs, used for DP validation.

`cec`
:   `struct containing` fields used for CEC-Tunneling-over-AUX.

`is_remote`
:   Is this AUX CH actually using sideband messaging.

`powered_down`
:   If true then the remote endpoint is powered down.

`no_zero_sized`
:   If the hw can’t use zero sized transfers (NVIDIA)

`dpcd_probe_disabled`
:   If probing before a DPCD access is disabled.

**Description**

An AUX channel can also be used to transport I2C messages to a sink. A
typical application of that is to access an EDID that’s present in the sink
device. The **`transfer()`** function can also be used to execute such
transactions. The [`drm_dp_aux_register()`](#c.drm_dp_aux_register "drm_dp_aux_register") function registers an I2C adapter
that can be passed to [`drm_probe_ddc()`](#c.drm_probe_ddc "drm_probe_ddc"). Upon removal, drivers should call
[`drm_dp_aux_unregister()`](#c.drm_dp_aux_unregister "drm_dp_aux_unregister") to remove the I2C adapter. The I2C adapter uses long
transfers by default; if a partial response is received, the adapter will
drop down to the size given by the partial response for this transaction
only.

ssize\_t drm\_dp\_dpcd\_readb(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, unsigned int offset, u8 \*valuep)
:   read a single byte from the DPCD

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`unsigned int offset`
:   address of the register to read

`u8 *valuep`
:   location where the value of the register will be stored

**Description**

Returns the number of bytes transferred (1) on success, or a negative
error code on failure. In most of the cases you should be using
[`drm_dp_dpcd_read_byte()`](#c.drm_dp_dpcd_read_byte "drm_dp_dpcd_read_byte") instead.

int drm\_dp\_dpcd\_read\_data(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, unsigned int offset, void \*buffer, size\_t size)
:   read a series of bytes from the DPCD

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel (SST or MST)

`unsigned int offset`
:   address of the (first) register to read

`void *buffer`
:   buffer to store the register values

`size_t size`
:   number of bytes in **buffer**

**Description**

Returns zero (0) on success, or a negative error
code on failure. -EIO is returned if the request was NAKed by the sink or
if the retry count was exceeded. If not all bytes were transferred, this
function returns -EPROTO. Errors from the underlying AUX channel transfer
function, with the exception of -EBUSY (which causes the transaction to
be retried), are propagated to the caller.

int drm\_dp\_dpcd\_write\_data(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, unsigned int offset, void \*buffer, size\_t size)
:   write a series of bytes to the DPCD

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel (SST or MST)

`unsigned int offset`
:   address of the (first) register to write

`void *buffer`
:   buffer containing the values to write

`size_t size`
:   number of bytes in **buffer**

**Description**

Returns zero (0) on success, or a negative error
code on failure. -EIO is returned if the request was NAKed by the sink or
if the retry count was exceeded. If not all bytes were transferred, this
function returns -EPROTO. Errors from the underlying AUX channel transfer
function, with the exception of -EBUSY (which causes the transaction to
be retried), are propagated to the caller.

ssize\_t drm\_dp\_dpcd\_writeb(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, unsigned int offset, u8 value)
:   write a single byte to the DPCD

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`unsigned int offset`
:   address of the register to write

`u8 value`
:   value to write to the register

**Description**

Returns the number of bytes transferred (1) on success, or a negative
error code on failure. In most of the cases you should be using
[`drm_dp_dpcd_write_byte()`](#c.drm_dp_dpcd_write_byte "drm_dp_dpcd_write_byte") instead.

int drm\_dp\_dpcd\_read\_byte(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, unsigned int offset, u8 \*valuep)
:   read a single byte from the DPCD

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`unsigned int offset`
:   address of the register to read

`u8 *valuep`
:   location where the value of the register will be stored

**Description**

Returns zero (0) on success, or a negative error code on failure.

int drm\_dp\_dpcd\_write\_byte(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, unsigned int offset, u8 value)
:   write a single byte to the DPCD

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`unsigned int offset`
:   address of the register to write

`u8 value`
:   value to write to the register

**Description**

Returns zero (0) on success, or a negative error code on failure.

struct drm\_dp\_desc
:   DP branch/sink device descriptor

**Definition**:

```
struct drm_dp_desc {
    struct drm_dp_dpcd_ident ident;
    u32 quirks;
};
```

**Members**

`ident`
:   DP device identification from DPCD 0x400 (sink) or 0x500 (branch).

`quirks`
:   Quirks; use [`drm_dp_has_quirk()`](#c.drm_dp_has_quirk "drm_dp_has_quirk") to query for the quirks.

enum drm\_dp\_quirk
:   Display Port sink/branch device specific quirks

**Constants**

`DP_DPCD_QUIRK_CONSTANT_N`
:   The device requires main link attributes Mvid and Nvid to be limited
    to 16 bits. So will give a constant value (0x8000) for compatability.

`DP_DPCD_QUIRK_NO_PSR`
:   The device does not support PSR even if reports that it supports or
    driver still need to implement proper handling for such device.

`DP_DPCD_QUIRK_NO_SINK_COUNT`
:   The device does not set SINK\_COUNT to a non-zero value.
    The driver should ignore SINK\_COUNT during detection. Note that
    [`drm_dp_read_sink_count_cap()`](#c.drm_dp_read_sink_count_cap "drm_dp_read_sink_count_cap") automatically checks for this quirk.

`DP_DPCD_QUIRK_DSC_WITHOUT_VIRTUAL_DPCD`
:   The device supports MST DSC despite not supporting Virtual DPCD.
    The DSC caps can be read from the physical aux instead.

`DP_DPCD_QUIRK_CAN_DO_MAX_LINK_RATE_3_24_GBPS`
:   The device supports a link rate of 3.24 Gbps (multiplier 0xc) despite
    the DP\_MAX\_LINK\_RATE register reporting a lower max multiplier.

`DP_DPCD_QUIRK_HBLANK_EXPANSION_REQUIRES_DSC`
:   The device applies HBLANK expansion for some modes, but this
    requires enabling DSC.

`DP_DPCD_QUIRK_DSC_THROUGHPUT_BPP_LIMIT`
:   The device doesn’t support DSC decompression at the maximum DSC
    pixel throughput and compressed bpp it indicates via its DPCD DSC
    capabilities. The compressed bpp must be limited above a device
    specific DSC pixel throughput.

**Description**

Display Port sink and branch devices in the wild have a variety of bugs, try
to collect them here. The quirks are shared, but it’s up to the drivers to
implement workarounds for them.

bool drm\_dp\_has\_quirk(const struct [drm\_dp\_desc](#c.drm_dp_desc "drm_dp_desc") \*desc, enum [drm\_dp\_quirk](#c.drm_dp_quirk "drm_dp_quirk") quirk)
:   does the DP device have a specific quirk

**Parameters**

`const struct drm_dp_desc *desc`
:   Device descriptor filled by [`drm_dp_read_desc()`](#c.drm_dp_read_desc "drm_dp_read_desc")

`enum drm_dp_quirk quirk`
:   Quirk to query for

**Description**

Return true if DP device identified by **desc** has **quirk**.

struct drm\_edp\_backlight\_info
:   Probed eDP backlight info struct

**Definition**:

```
struct drm_edp_backlight_info {
    u8 pwmgen_bit_count;
    u8 pwm_freq_pre_divider;
    u32 max;
    bool lsb_reg_used : 1;
    bool aux_enable : 1;
    bool aux_set : 1;
    bool luminance_set : 1;
};
```

**Members**

`pwmgen_bit_count`
:   The pwmgen bit count

`pwm_freq_pre_divider`
:   The PWM frequency pre-divider value being used for this backlight, if any

`max`
:   The maximum backlight level that may be set

`lsb_reg_used`
:   Do we also write values to the DP\_EDP\_BACKLIGHT\_BRIGHTNESS\_LSB register?

`aux_enable`
:   Does the panel support the AUX enable cap?

`aux_set`
:   Does the panel support setting the brightness through AUX?

`luminance_set`
:   Does the panel support setting the brightness through AUX using luminance values?

**Description**

This structure contains various data about an eDP backlight, which can be populated by using
[`drm_edp_backlight_init()`](#c.drm_edp_backlight_init "drm_edp_backlight_init").

struct drm\_dp\_phy\_test\_params
:   DP Phy Compliance parameters

**Definition**:

```
struct drm_dp_phy_test_params {
    int link_rate;
    u8 num_lanes;
    u8 phy_pattern;
    u8 hbr2_reset[2];
    u8 custom80[10];
    bool enhanced_frame_cap;
};
```

**Members**

`link_rate`
:   Requested Link rate from DPCD 0x219

`num_lanes`
:   Number of lanes requested by sing through DPCD 0x220

`phy_pattern`
:   DP Phy test pattern from DPCD 0x248

`hbr2_reset`
:   DP HBR2\_COMPLIANCE\_SCRAMBLER\_RESET from DCPD 0x24A and 0x24B

`custom80`
:   DP Test\_80BIT\_CUSTOM\_PATTERN from DPCDs 0x250 through 0x259

`enhanced_frame_cap`
:   flag for enhanced frame capability.

const char \*drm\_dp\_phy\_name(enum drm\_dp\_phy dp\_phy)
:   Get the name of the given DP PHY

**Parameters**

`enum drm_dp_phy dp_phy`
:   The DP PHY identifier

**Description**

Given the **dp\_phy**, get a user friendly name of the DP PHY, either “DPRX” or
“LTTPR <N>”, or “<INVALID DP PHY>” on errors. The returned string is always
non-NULL and valid.

**Return**

Name of the DP PHY.

void drm\_dp\_lttpr\_wake\_timeout\_setup(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, bool transparent\_mode)
:   Grant extended time for sink to wake up

**Parameters**

`struct drm_dp_aux *aux`
:   The DP AUX channel to use

`bool transparent_mode`
:   This is true if lttpr is in transparent mode

**Description**

This function checks if the sink needs any extended wake time, if it does
it grants this request. Post this setup the source device can keep trying
the Aux transaction till the granted wake timeout.
If this function is not called all Aux transactions are expected to take
a default of 1ms before they throw an error.

int drm\_dp\_dpcd\_probe(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, unsigned int offset)
:   probe a given DPCD address with a 1-byte read access

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel (SST)

`unsigned int offset`
:   address of the register to probe

**Description**

Probe the provided DPCD address by reading 1 byte from it. The function can
be used to trigger some side-effect the read access has, like waking up the
sink, without the need for the read-out value.

Returns 0 if the read access suceeded, or a negative error code on failure.

void drm\_dp\_dpcd\_set\_powered(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, bool powered)
:   Set whether the DP device is powered

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel; for convenience it’s OK to pass NULL here
    and the function will be a no-op.

`bool powered`
:   true if powered; false if not

**Description**

If the endpoint device on the DP AUX bus is known to be powered down
then this function can be called to make future transfers fail immediately
instead of needing to time out.

If this function is never called then a device defaults to being powered.

void drm\_dp\_dpcd\_set\_probe(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, bool enable)
:   Set whether a probing before DPCD access is done

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`bool enable`
:   Enable the probing if required

ssize\_t drm\_dp\_dpcd\_read(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, unsigned int offset, void \*buffer, size\_t size)
:   read a series of bytes from the DPCD

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel (SST or MST)

`unsigned int offset`
:   address of the (first) register to read

`void *buffer`
:   buffer to store the register values

`size_t size`
:   number of bytes in **buffer**

**Description**

Returns the number of bytes transferred on success, or a negative error
code on failure. -EIO is returned if the request was NAKed by the sink or
if the retry count was exceeded. If not all bytes were transferred, this
function returns -EPROTO. Errors from the underlying AUX channel transfer
function, with the exception of -EBUSY (which causes the transaction to
be retried), are propagated to the caller.

In most of the cases you want to use [`drm_dp_dpcd_read_data()`](#c.drm_dp_dpcd_read_data "drm_dp_dpcd_read_data") instead.

ssize\_t drm\_dp\_dpcd\_write(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, unsigned int offset, void \*buffer, size\_t size)
:   write a series of bytes to the DPCD

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel (SST or MST)

`unsigned int offset`
:   address of the (first) register to write

`void *buffer`
:   buffer containing the values to write

`size_t size`
:   number of bytes in **buffer**

**Description**

Returns the number of bytes transferred on success, or a negative error
code on failure. -EIO is returned if the request was NAKed by the sink or
if the retry count was exceeded. If not all bytes were transferred, this
function returns -EPROTO. Errors from the underlying AUX channel transfer
function, with the exception of -EBUSY (which causes the transaction to
be retried), are propagated to the caller.

In most of the cases you want to use [`drm_dp_dpcd_write_data()`](#c.drm_dp_dpcd_write_data "drm_dp_dpcd_write_data") instead.

int drm\_dp\_dpcd\_read\_link\_status(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, u8 status[DP\_LINK\_STATUS\_SIZE])
:   read DPCD link status (bytes 0x202-0x207)

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`u8 status[DP_LINK_STATUS_SIZE]`
:   buffer to store the link status in (must be at least 6 bytes)

**Description**

Returns a negative error code on failure or 0 on success.

int drm\_dp\_dpcd\_read\_phy\_link\_status(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, enum drm\_dp\_phy dp\_phy, u8 link\_status[DP\_LINK\_STATUS\_SIZE])
:   get the link status information for a DP PHY

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`enum drm_dp_phy dp_phy`
:   the DP PHY to get the link status for

`u8 link_status[DP_LINK_STATUS_SIZE]`
:   buffer to return the status in

**Description**

Fetch the AUX DPCD registers for the DPRX or an LTTPR PHY link status. The
layout of the returned **link\_status** matches the DPCD register layout of the
DPRX PHY link status.

Returns 0 if the information was read successfully or a negative error code
on failure.

int drm\_dp\_link\_power\_up(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, unsigned char revision)
:   power up a DisplayPort link

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`unsigned char revision`
:   DPCD revision supported on the link

**Description**

Returns 0 on success or a negative error code on failure.

int drm\_dp\_link\_power\_down(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, unsigned char revision)
:   power down a DisplayPort link

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`unsigned char revision`
:   DPCD revision supported on the link

**Description**

Returns 0 on success or a negative error code on failure.

int drm\_dp\_dpcd\_write\_payload(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, int vcpid, u8 start\_time\_slot, u8 time\_slot\_count)
:   Write Virtual Channel information to payload table

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`int vcpid`
:   Virtual Channel Payload ID

`u8 start_time_slot`
:   Starting time slot

`u8 time_slot_count`
:   Time slot count

**Description**

Write the Virtual Channel payload allocation table, checking the payload
update status and retrying as necessary.

**Return**

0 on success, negative error otherwise

int drm\_dp\_dpcd\_clear\_payload(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux)
:   Clear the entire VC Payload ID table

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

**Description**

Clear the entire VC Payload ID table.

**Return**

0 on success, negative error code on errors.

int drm\_dp\_dpcd\_poll\_act\_handled(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, int timeout\_ms)
:   Poll for ACT handled status

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`int timeout_ms`
:   Timeout in ms

**Description**

Try waiting for the sink to finish updating its payload table by polling for
the ACT handled bit of DP\_PAYLOAD\_TABLE\_UPDATE\_STATUS for up to **timeout\_ms**
milliseconds, defaulting to 3000 ms if 0.

**Return**

0 if the ACT was handled in time, negative error code on failure.

bool drm\_dp\_downstream\_is\_type(const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE], const u8 port\_cap[4], u8 type)
:   is the downstream facing port of certain type?

**Parameters**

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   DisplayPort configuration data

`const u8 port_cap[4]`
:   port capabilities

`u8 type`
:   port type to be checked. Can be:
    `DP_DS_PORT_TYPE_DP`, `DP_DS_PORT_TYPE_VGA`, `DP_DS_PORT_TYPE_DVI`,
    `DP_DS_PORT_TYPE_HDMI`, `DP_DS_PORT_TYPE_NON_EDID`,
    `DP_DS_PORT_TYPE_DP_DUALMODE` or `DP_DS_PORT_TYPE_WIRELESS`.

**Description**

Caveat: Only works with DPCD 1.1+ port caps.

**Return**

whether the downstream facing port matches the type.

bool drm\_dp\_downstream\_is\_tmds(const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE], const u8 port\_cap[4], const struct [drm\_edid](#c.drm_dp_downstream_is_tmds "drm_edid") \*drm\_edid)
:   is the downstream facing port TMDS?

**Parameters**

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   DisplayPort configuration data

`const u8 port_cap[4]`
:   port capabilities

`const struct drm_edid *drm_edid`
:   EDID

**Return**

whether the downstream facing port is TMDS (HDMI/DVI).

bool drm\_dp\_send\_real\_edid\_checksum(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, u8 real\_edid\_checksum)
:   send back real edid checksum value

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`u8 real_edid_checksum`
:   real edid checksum for the last block

**Return**

True on success

int drm\_dp\_read\_dpcd\_caps(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, u8 dpcd[DP\_RECEIVER\_CAP\_SIZE])
:   read DPCD caps and extended DPCD caps if available

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   Buffer to store the resulting DPCD in

**Description**

Attempts to read the base DPCD caps for **aux**. Additionally, this function
checks for and reads the extended DPRX caps (`DP_DP13_DPCD_REV`) if
present.

**Return**

`0` if the DPCD was read successfully, negative error code
otherwise.

int drm\_dp\_read\_downstream\_info(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE], u8 downstream\_ports[DP\_MAX\_DOWNSTREAM\_PORTS])
:   read DPCD downstream port info if available

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   A cached copy of the port’s DPCD

`u8 downstream_ports[DP_MAX_DOWNSTREAM_PORTS]`
:   buffer to store the downstream port info in

**Description**

See also:
`drm_dp_downstream_max_clock()`
[`drm_dp_downstream_max_bpc()`](#c.drm_dp_downstream_max_bpc "drm_dp_downstream_max_bpc")

**Return**

0 if either the downstream port info was read successfully or
there was no downstream info to read, or a negative error code otherwise.

int drm\_dp\_downstream\_max\_dotclock(const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE], const u8 port\_cap[4])
:   extract downstream facing port max dot clock

**Parameters**

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   DisplayPort configuration data

`const u8 port_cap[4]`
:   port capabilities

**Return**

Downstream facing port max dot clock in kHz on success,
or 0 if max clock not defined

int drm\_dp\_downstream\_max\_tmds\_clock(const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE], const u8 port\_cap[4], const struct [drm\_edid](#c.drm_dp_downstream_max_tmds_clock "drm_edid") \*drm\_edid)
:   extract downstream facing port max TMDS clock

**Parameters**

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   DisplayPort configuration data

`const u8 port_cap[4]`
:   port capabilities

`const struct drm_edid *drm_edid`
:   EDID

**Return**

HDMI/DVI downstream facing port max TMDS clock in kHz on success,
or 0 if max TMDS clock not defined

int drm\_dp\_downstream\_min\_tmds\_clock(const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE], const u8 port\_cap[4], const struct [drm\_edid](#c.drm_dp_downstream_min_tmds_clock "drm_edid") \*drm\_edid)
:   extract downstream facing port min TMDS clock

**Parameters**

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   DisplayPort configuration data

`const u8 port_cap[4]`
:   port capabilities

`const struct drm_edid *drm_edid`
:   EDID

**Return**

HDMI/DVI downstream facing port min TMDS clock in kHz on success,
or 0 if max TMDS clock not defined

int drm\_dp\_downstream\_max\_bpc(const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE], const u8 port\_cap[4], const struct [drm\_edid](#c.drm_dp_downstream_max_bpc "drm_edid") \*drm\_edid)
:   extract downstream facing port max bits per component

**Parameters**

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   DisplayPort configuration data

`const u8 port_cap[4]`
:   downstream facing port capabilities

`const struct drm_edid *drm_edid`
:   EDID

**Return**

Max bpc on success or 0 if max bpc not defined

bool drm\_dp\_downstream\_420\_passthrough(const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE], const u8 port\_cap[4])
:   determine downstream facing port YCbCr 4:2:0 pass-through capability

**Parameters**

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   DisplayPort configuration data

`const u8 port_cap[4]`
:   downstream facing port capabilities

**Return**

whether the downstream facing port can pass through YCbCr 4:2:0

bool drm\_dp\_downstream\_444\_to\_420\_conversion(const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE], const u8 port\_cap[4])
:   determine downstream facing port YCbCr 4:4:4->4:2:0 conversion capability

**Parameters**

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   DisplayPort configuration data

`const u8 port_cap[4]`
:   downstream facing port capabilities

**Return**

whether the downstream facing port can convert YCbCr 4:4:4 to 4:2:0

bool drm\_dp\_downstream\_rgb\_to\_ycbcr\_conversion(const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE], const u8 port\_cap[4], u8 color\_spc)
:   determine downstream facing port RGB->YCbCr conversion capability

**Parameters**

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   DisplayPort configuration data

`const u8 port_cap[4]`
:   downstream facing port capabilities

`u8 color_spc`
:   Colorspace for which conversion cap is sought

**Return**

whether the downstream facing port can convert RGB->YCbCr for a given
colorspace.

struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*drm\_dp\_downstream\_mode(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE], const u8 port\_cap[4])
:   return a mode for downstream facing port

**Parameters**

`struct drm_device *dev`
:   DRM device

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   DisplayPort configuration data

`const u8 port_cap[4]`
:   port capabilities

**Description**

Provides a suitable mode for downstream facing ports without EDID.

**Return**

A new drm\_display\_mode on success or NULL on failure

int drm\_dp\_downstream\_id(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, char id[6])
:   identify branch device

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`char id[6]`
:   DisplayPort branch device id

**Description**

Returns branch device id on success or NULL on failure

void drm\_dp\_downstream\_debug(struct seq\_file \*m, const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE], const u8 port\_cap[4], const struct [drm\_edid](#c.drm_dp_downstream_debug "drm_edid") \*drm\_edid, struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux)
:   debug DP branch devices

**Parameters**

`struct seq_file *m`
:   pointer for debugfs file

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   DisplayPort configuration data

`const u8 port_cap[4]`
:   port capabilities

`const struct drm_edid *drm_edid`
:   EDID

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

enum drm\_mode\_subconnector drm\_dp\_subconnector\_type(const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE], const u8 port\_cap[4])
:   get DP branch device type

**Parameters**

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   DisplayPort configuration data

`const u8 port_cap[4]`
:   port capabilities

void drm\_dp\_set\_subconnector\_property(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, enum [drm\_connector\_status](drm-kms.html#c.drm_connector_status "drm_connector_status") status, const u8 \*dpcd, const u8 port\_cap[4])
:   set subconnector for DP connector

**Parameters**

`struct drm_connector *connector`
:   connector to set property on

`enum drm_connector_status status`
:   connector status

`const u8 *dpcd`
:   DisplayPort configuration data

`const u8 port_cap[4]`
:   port capabilities

**Description**

Called by a driver on every detect event.

bool drm\_dp\_read\_sink\_count\_cap(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE], const struct [drm\_dp\_desc](#c.drm_dp_desc "drm_dp_desc") \*desc)
:   Check whether a given connector has a valid sink count

**Parameters**

`struct drm_connector *connector`
:   The DRM connector to check

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   A cached copy of the connector’s DPCD RX capabilities

`const struct drm_dp_desc *desc`
:   A cached copy of the connector’s DP descriptor

**Description**

See also: [`drm_dp_read_sink_count()`](#c.drm_dp_read_sink_count "drm_dp_read_sink_count")

**Return**

`True` if the (e)DP connector has a valid sink count that should
be probed, `false` otherwise.

int drm\_dp\_read\_sink\_count(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux)
:   Retrieve the sink count for a given sink

**Parameters**

`struct drm_dp_aux *aux`
:   The DP AUX channel to use

**Description**

See also: [`drm_dp_read_sink_count_cap()`](#c.drm_dp_read_sink_count_cap "drm_dp_read_sink_count_cap")

**Return**

The current sink count reported by **aux**, or a negative error code
otherwise.

void drm\_dp\_remote\_aux\_init(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux)
:   minimally initialise a remote aux channel

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

**Description**

Used for remote aux channel in general. Merely initialize the crc work
struct.

void drm\_dp\_aux\_init(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux)
:   minimally initialise an aux channel

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

**Description**

If you need to use the drm\_dp\_aux’s i2c adapter prior to registering it with
the outside world, call [`drm_dp_aux_init()`](#c.drm_dp_aux_init "drm_dp_aux_init") first. For drivers which are
grandparents to their AUX adapters (e.g. the AUX adapter is parented by a
[`drm_connector`](drm-kms.html#c.drm_connector "drm_connector")), you must still call [`drm_dp_aux_register()`](#c.drm_dp_aux_register "drm_dp_aux_register") once the connector
has been registered to allow userspace access to the auxiliary DP channel.
Likewise, for such drivers you should also assign [`drm_dp_aux.drm_dev`](#c.drm_dp_aux "drm_dp_aux") as
early as possible so that the [`drm_device`](drm-internals.html#c.drm_device "drm_device") that corresponds to the AUX adapter
may be mentioned in debugging output from the DRM DP helpers.

For devices which use a separate platform device for their AUX adapters, this
may be called as early as required by the driver.

int drm\_dp\_aux\_register(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux)
:   initialise and register aux channel

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

**Description**

Automatically calls [`drm_dp_aux_init()`](#c.drm_dp_aux_init "drm_dp_aux_init") if this hasn’t been done yet. This
should only be called once the parent of **aux**, [`drm_dp_aux.dev`](#c.drm_dp_aux "drm_dp_aux"), is
initialized. For devices which are grandparents of their AUX channels,
[`drm_dp_aux.dev`](#c.drm_dp_aux "drm_dp_aux") will typically be the [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector") [`device`](../driver-api/infrastructure.html#c.device "device") which
corresponds to **aux**. For these devices, it’s advised to call
[`drm_dp_aux_register()`](#c.drm_dp_aux_register "drm_dp_aux_register") in [`drm_connector_funcs.late_register`](drm-kms.html#c.drm_connector_funcs "drm_connector_funcs"), and likewise to
call [`drm_dp_aux_unregister()`](#c.drm_dp_aux_unregister "drm_dp_aux_unregister") in [`drm_connector_funcs.early_unregister`](drm-kms.html#c.drm_connector_funcs "drm_connector_funcs").
Functions which don’t follow this will likely Oops when
`CONFIG_DRM_DISPLAY_DP_AUX_CHARDEV` is enabled.

For devices where the AUX channel is a device that exists independently of
the [`drm_device`](drm-internals.html#c.drm_device "drm_device") that uses it, such as SoCs and bridge devices, it is
recommended to call [`drm_dp_aux_register()`](#c.drm_dp_aux_register "drm_dp_aux_register") after a [`drm_device`](drm-internals.html#c.drm_device "drm_device") has been
assigned to [`drm_dp_aux.drm_dev`](#c.drm_dp_aux "drm_dp_aux"), and likewise to call
[`drm_dp_aux_unregister()`](#c.drm_dp_aux_unregister "drm_dp_aux_unregister") once the [`drm_device`](drm-internals.html#c.drm_device "drm_device") should no longer be associated
with the AUX channel (e.g. on bridge detach).

Drivers which need to use the aux channel before either of the two points
mentioned above need to call [`drm_dp_aux_init()`](#c.drm_dp_aux_init "drm_dp_aux_init") in order to use the AUX
channel before registration.

Returns 0 on success or a negative error code on failure.

void drm\_dp\_aux\_unregister(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux)
:   unregister an AUX adapter

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

int drm\_dp\_psr\_setup\_time(const u8 psr\_cap[EDP\_PSR\_RECEIVER\_CAP\_SIZE])
:   PSR setup in time usec

**Parameters**

`const u8 psr_cap[EDP_PSR_RECEIVER_CAP_SIZE]`
:   PSR capabilities from DPCD

**Return**

PSR setup time for the panel in microseconds, negative
error code on failure.

int drm\_dp\_start\_crc(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc)
:   start capture of frame CRCs

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`struct drm_crtc *crtc`
:   CRTC displaying the frames whose CRCs are to be captured

**Description**

Returns 0 on success or a negative error code on failure.

int drm\_dp\_stop\_crc(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux)
:   stop capture of frame CRCs

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

**Description**

Returns 0 on success or a negative error code on failure.

int drm\_dp\_read\_desc(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, struct [drm\_dp\_desc](#c.drm_dp_desc "drm_dp_desc") \*desc, bool is\_branch)
:   read sink/branch descriptor from DPCD

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`struct drm_dp_desc *desc`
:   Device descriptor to fill from DPCD

`bool is_branch`
:   true for branch devices, false for sink devices

**Description**

Read DPCD 0x400 (sink) or 0x500 (branch) into **desc**. Also debug log the
identification.

Returns 0 on success or a negative error code on failure.

int drm\_dp\_dump\_lttpr\_desc(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, enum drm\_dp\_phy dp\_phy)
:   read and dump the DPCD descriptor for an LTTPR PHY

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`enum drm_dp_phy dp_phy`
:   LTTPR PHY instance

**Description**

Read the DPCD LTTPR PHY descriptor for **dp\_phy** and print a debug message
with its details to dmesg.

Returns 0 on success or a negative error code on failure.

u8 drm\_dp\_dsc\_sink\_bpp\_incr(const u8 dsc\_dpcd[DP\_DSC\_RECEIVER\_CAP\_SIZE])
:   Get bits per pixel increment

**Parameters**

`const u8 dsc_dpcd[DP_DSC_RECEIVER_CAP_SIZE]`
:   DSC capabilities from DPCD

**Description**

Returns the bpp precision supported by the DP sink.

u32 drm\_dp\_dsc\_slice\_count\_to\_mask(int slice\_count)
:   Convert a slice count to a slice count mask

**Parameters**

`int slice_count`
:   slice count

**Description**

Convert **slice\_count** to a slice count mask.

Returns the slice count mask.

u32 drm\_dp\_dsc\_sink\_slice\_count\_mask(const u8 dsc\_dpcd[DP\_DSC\_RECEIVER\_CAP\_SIZE], bool is\_edp)
:   Get the mask of valid DSC sink slice counts

**Parameters**

`const u8 dsc_dpcd[DP_DSC_RECEIVER_CAP_SIZE]`
:   the sink’s DSC DPCD capabilities

`bool is_edp`
:   `true` for an eDP sink

**Description**

Get the mask of supported slice counts from the sink’s DSC DPCD register.

**Return**

Mask of slice counts supported by the DSC sink:
- > 0: bit#0,1,3,5..,23 set if the sink supports 1,2,4,6..,24 slices
- 0: if the sink doesn’t support any slices

u8 drm\_dp\_dsc\_sink\_max\_slice\_count(const u8 dsc\_dpcd[DP\_DSC\_RECEIVER\_CAP\_SIZE], bool is\_edp)
:   Get the max slice count supported by the DSC sink.

**Parameters**

`const u8 dsc_dpcd[DP_DSC_RECEIVER_CAP_SIZE]`
:   DSC capabilities from DPCD

`bool is_edp`
:   true if its eDP, false for DP

**Description**

Read the slice capabilities DPCD register from DSC sink to get
the maximum slice count supported. This is used to populate
the DSC parameters in the [`struct drm_dsc_config`](#c.drm_dsc_config "drm_dsc_config") by the driver.
Driver creates an infoframe using these parameters to populate
[`struct drm_dsc_pps_infoframe`](#c.drm_dsc_pps_infoframe "drm_dsc_pps_infoframe"). These are sent to the sink using DSC
infoframe using the helper function `drm_dsc_pps_infoframe_pack()`

**Return**

Maximum slice count supported by DSC sink or 0 its invalid

u8 drm\_dp\_dsc\_sink\_line\_buf\_depth(const u8 dsc\_dpcd[DP\_DSC\_RECEIVER\_CAP\_SIZE])
:   Get the line buffer depth in bits

**Parameters**

`const u8 dsc_dpcd[DP_DSC_RECEIVER_CAP_SIZE]`
:   DSC capabilities from DPCD

**Description**

Read the DSC DPCD register to parse the line buffer depth in bits which is
number of bits of precision within the decoder line buffer supported by
the DSC sink. This is used to populate the DSC parameters in the
[`struct drm_dsc_config`](#c.drm_dsc_config "drm_dsc_config") by the driver.
Driver creates an infoframe using these parameters to populate
[`struct drm_dsc_pps_infoframe`](#c.drm_dsc_pps_infoframe "drm_dsc_pps_infoframe"). These are sent to the sink using DSC
infoframe using the helper function `drm_dsc_pps_infoframe_pack()`

**Return**

Line buffer depth supported by DSC panel or 0 its invalid

int drm\_dp\_dsc\_sink\_supported\_input\_bpcs(const u8 dsc\_dpcd[DP\_DSC\_RECEIVER\_CAP\_SIZE], u8 dsc\_bpc[3])
:   Get all the input bits per component values supported by the DSC sink.

**Parameters**

`const u8 dsc_dpcd[DP_DSC_RECEIVER_CAP_SIZE]`
:   DSC capabilities from DPCD

`u8 dsc_bpc[3]`
:   An array to be filled by this helper with supported
    input bpcs.

**Description**

Read the DSC DPCD from the sink device to parse the supported bits per
component values. This is used to populate the DSC parameters
in the [`struct drm_dsc_config`](#c.drm_dsc_config "drm_dsc_config") by the driver.
Driver creates an infoframe using these parameters to populate
[`struct drm_dsc_pps_infoframe`](#c.drm_dsc_pps_infoframe "drm_dsc_pps_infoframe"). These are sent to the sink using DSC
infoframe using the helper function `drm_dsc_pps_infoframe_pack()`

**Return**

Number of input BPC values parsed from the DPCD

int drm\_dp\_dsc\_sink\_max\_slice\_throughput(const u8 dsc\_dpcd[DP\_DSC\_RECEIVER\_CAP\_SIZE], int peak\_pixel\_rate, bool is\_rgb\_yuv444)
:   Get a DSC sink’s maximum pixel throughput per slice

**Parameters**

`const u8 dsc_dpcd[DP_DSC_RECEIVER_CAP_SIZE]`
:   DSC sink’s capabilities from DPCD

`int peak_pixel_rate`
:   Cumulative peak pixel rate in kHz

`bool is_rgb_yuv444`
:   The mode is either RGB or YUV444

**Description**

Return the DSC sink device’s maximum pixel throughput per slice, based on
the device’s **dsc\_dpcd** capabilities, the **peak\_pixel\_rate** of the transferred
stream(s) and whether the output format **is\_rgb\_yuv444** or yuv422/yuv420.

Note that **peak\_pixel\_rate** is the total pixel rate transferred to the same
DSC/display sink. For instance to calculate a tile’s slice count of an MST
multi-tiled display sink (not considering here the required
rounding/alignment of slice count):

```
@peak_pixel_rate = tile_pixel_rate * tile_count
total_slice_count = @peak_pixel_rate / drm_dp_dsc_sink_max_slice_throughput(@peak_pixel_rate)
tile_slice_count = total_slice_count / tile_count
```

**Return**

The maximum pixel throughput per slice supported by the DSC sink device
in kPixels/sec.

int drm\_dp\_dsc\_branch\_max\_overall\_throughput(const u8 dsc\_branch\_dpcd[DP\_DSC\_BRANCH\_CAP\_SIZE], bool is\_rgb\_yuv444)
:   Branch device’s max overall DSC pixel throughput

**Parameters**

`const u8 dsc_branch_dpcd[DP_DSC_BRANCH_CAP_SIZE]`
:   DSC branch capabilities from DPCD

`bool is_rgb_yuv444`
:   The mode is either RGB or YUV444

**Description**

Return the branch device’s maximum overall DSC pixel throughput, based on
the device’s DPCD DSC branch capabilities, and whether the output
format **is\_rgb\_yuv444** or yuv422/yuv420.

**Return**

* 0: The maximum overall throughput capability is not indicated by
  :   the device separately and it must be determined from the per-slice
      max throughput (see **`drm_dp_dsc_branch_slice_max_throughput()`**)
      and the maximum slice count supported by the device.
* > 0: The maximum overall DSC pixel throughput supported by the branch
  :   device in kPixels/sec.

int drm\_dp\_dsc\_branch\_max\_line\_width(const u8 dsc\_branch\_dpcd[DP\_DSC\_BRANCH\_CAP\_SIZE])
:   Branch device’s max DSC line width

**Parameters**

`const u8 dsc_branch_dpcd[DP_DSC_BRANCH_CAP_SIZE]`
:   DSC branch capabilities from DPCD

**Description**

Return the branch device’s maximum overall DSC line width, based on
the device’s **dsc\_branch\_dpcd** capabilities.

**Return**

* 0: The maximum line width is not indicated by the device
  :   separately and it must be determined from the maximum
      slice count and slice-width supported by the device.
* `-EINVAL`: The device indicates an invalid maximum line width
  :   (< 5120 pixels).
* >= 5120: The maximum line width in pixels.

int drm\_dp\_read\_lttpr\_common\_caps(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE], u8 caps[DP\_LTTPR\_COMMON\_CAP\_SIZE])
:   read the LTTPR common capabilities

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   DisplayPort configuration data

`u8 caps[DP_LTTPR_COMMON_CAP_SIZE]`
:   buffer to return the capability info in

**Description**

Read capabilities common to all LTTPRs.

Returns 0 on success or a negative error code on failure.

int drm\_dp\_read\_lttpr\_phy\_caps(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE], enum drm\_dp\_phy dp\_phy, u8 caps[DP\_LTTPR\_PHY\_CAP\_SIZE])
:   read the capabilities for a given LTTPR PHY

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   DisplayPort configuration data

`enum drm_dp_phy dp_phy`
:   LTTPR PHY to read the capabilities for

`u8 caps[DP_LTTPR_PHY_CAP_SIZE]`
:   buffer to return the capability info in

**Description**

Read the capabilities for the given LTTPR PHY.

Returns 0 on success or a negative error code on failure.

int drm\_dp\_lttpr\_count(const u8 caps[DP\_LTTPR\_COMMON\_CAP\_SIZE])
:   get the number of detected LTTPRs

**Parameters**

`const u8 caps[DP_LTTPR_COMMON_CAP_SIZE]`
:   LTTPR common capabilities

**Description**

Get the number of detected LTTPRs from the LTTPR common capabilities info.

**Return**

-ERANGE if more than supported number (8) of LTTPRs are detected
-EINVAL if the DP\_PHY\_REPEATER\_CNT register contains an invalid value
otherwise the number of detected LTTPRs

int drm\_dp\_lttpr\_max\_link\_rate(const u8 caps[DP\_LTTPR\_COMMON\_CAP\_SIZE])
:   get the maximum link rate supported by all LTTPRs

**Parameters**

`const u8 caps[DP_LTTPR_COMMON_CAP_SIZE]`
:   LTTPR common capabilities

**Description**

Returns the maximum link rate supported by all detected LTTPRs.

int drm\_dp\_lttpr\_set\_transparent\_mode(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, bool enable)
:   set the LTTPR in transparent mode

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`bool enable`
:   Enable or disable transparent mode

**Return**

0 on success or a negative error code on failure.

int drm\_dp\_lttpr\_init(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, int lttpr\_count)
:   init LTTPR transparency mode according to DP standard

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`int lttpr_count`
:   Number of LTTPRs. Between 0 and 8, according to DP standard.
    Negative error code for any non-valid number.
    See [`drm_dp_lttpr_count()`](#c.drm_dp_lttpr_count "drm_dp_lttpr_count").

**Return**

0 on success or a negative error code on failure.

int drm\_dp\_lttpr\_max\_lane\_count(const u8 caps[DP\_LTTPR\_COMMON\_CAP\_SIZE])
:   get the maximum lane count supported by all LTTPRs

**Parameters**

`const u8 caps[DP_LTTPR_COMMON_CAP_SIZE]`
:   LTTPR common capabilities

**Description**

Returns the maximum lane count supported by all detected LTTPRs.

bool drm\_dp\_lttpr\_voltage\_swing\_level\_3\_supported(const u8 caps[DP\_LTTPR\_PHY\_CAP\_SIZE])
:   check for LTTPR vswing3 support

**Parameters**

`const u8 caps[DP_LTTPR_PHY_CAP_SIZE]`
:   LTTPR PHY capabilities

**Description**

Returns true if the **caps** for an LTTPR TX PHY indicate support for
voltage swing level 3.

bool drm\_dp\_lttpr\_pre\_emphasis\_level\_3\_supported(const u8 caps[DP\_LTTPR\_PHY\_CAP\_SIZE])
:   check for LTTPR preemph3 support

**Parameters**

`const u8 caps[DP_LTTPR_PHY_CAP_SIZE]`
:   LTTPR PHY capabilities

**Description**

Returns true if the **caps** for an LTTPR TX PHY indicate support for
pre-emphasis level 3.

int drm\_dp\_get\_phy\_test\_pattern(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, struct [drm\_dp\_phy\_test\_params](#c.drm_dp_phy_test_params "drm_dp_phy_test_params") \*data)
:   get the requested pattern from the sink.

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`struct drm_dp_phy_test_params *data`
:   DP phy compliance test parameters.

**Description**

Returns 0 on success or a negative error code on failure.

int drm\_dp\_set\_phy\_test\_pattern(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, struct [drm\_dp\_phy\_test\_params](#c.drm_dp_phy_test_params "drm_dp_phy_test_params") \*data, u8 dp\_rev)
:   set the pattern to the sink.

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`struct drm_dp_phy_test_params *data`
:   DP phy compliance test parameters.

`u8 dp_rev`
:   DP revision to use for compliance testing

**Description**

Returns 0 on success or a negative error code on failure.

bool drm\_dp\_as\_sdp\_supported(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE])
:   check if adaptive sync sdp is supported

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   DisplayPort configuration data

**Description**

Returns true if adaptive sync sdp is supported, else returns false

bool drm\_dp\_vsc\_sdp\_supported(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE])
:   check if vsc sdp is supported

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   DisplayPort configuration data

**Description**

Returns true if vsc sdp is supported, else returns false

ssize\_t drm\_dp\_vsc\_sdp\_pack(const struct [drm\_dp\_vsc\_sdp](#c.drm_dp_vsc_sdp "drm_dp_vsc_sdp") \*vsc, struct [dp\_sdp](#c.dp_sdp "dp_sdp") \*sdp)
:   pack a given vsc sdp into generic dp\_sdp

**Parameters**

`const struct drm_dp_vsc_sdp *vsc`
:   vsc sdp initialized according to its purpose as defined in
    table 2-118 - table 2-120 in DP 1.4a specification

`struct dp_sdp *sdp`
:   valid handle to the generic dp\_sdp which will be packed

**Description**

Returns length of sdp on success and error code on failure

int drm\_dp\_get\_pcon\_max\_frl\_bw(const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE], const u8 port\_cap[4])
:   maximum frl supported by PCON

**Parameters**

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   DisplayPort configuration data

`const u8 port_cap[4]`
:   port capabilities

**Description**

Returns maximum frl bandwidth supported by PCON in GBPS,
returns 0 if not supported.

int drm\_dp\_pcon\_frl\_prepare(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, bool enable\_frl\_ready\_hpd)
:   Prepare PCON for FRL.

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`bool enable_frl_ready_hpd`
:   Configure DP\_PCON\_ENABLE\_HPD\_READY.

**Description**

Returns 0 if success, else returns negative error code.

bool drm\_dp\_pcon\_is\_frl\_ready(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux)
:   Is PCON ready for FRL

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

**Description**

Returns true if success, else returns false.

int drm\_dp\_pcon\_frl\_configure\_1(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, int max\_frl\_gbps, u8 frl\_mode)
:   Set HDMI LINK Configuration-Step1

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`int max_frl_gbps`
:   maximum frl bw to be configured between PCON and HDMI sink

`u8 frl_mode`
:   FRL Training mode, it can be either Concurrent or Sequential.
    In Concurrent Mode, the FRL link bring up can be done along with
    DP Link training. In Sequential mode, the FRL link bring up is done prior to
    the DP Link training.

**Description**

Returns 0 if success, else returns negative error code.

int drm\_dp\_pcon\_frl\_configure\_2(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, int max\_frl\_mask, u8 frl\_type)
:   Set HDMI Link configuration Step-2

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`int max_frl_mask`
:   Max FRL BW to be tried by the PCON with HDMI Sink

`u8 frl_type`
:   FRL training type, can be Extended, or Normal.
    In Normal FRL training, the PCON tries each frl bw from the max\_frl\_mask
    starting from min, and stops when link training is successful. In Extended
    FRL training, all frl bw selected in the mask are trained by the PCON.

**Description**

Returns 0 if success, else returns negative error code.

int drm\_dp\_pcon\_reset\_frl\_config(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux)
:   Re-Set HDMI Link configuration.

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

**Description**

Returns 0 if success, else returns negative error code.

int drm\_dp\_pcon\_frl\_enable(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux)
:   Enable HDMI link through FRL

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

**Description**

Returns 0 if success, else returns negative error code.

bool drm\_dp\_pcon\_hdmi\_link\_active(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux)
:   check if the PCON HDMI LINK status is active.

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

**Description**

Returns true if link is active else returns false.

int drm\_dp\_pcon\_hdmi\_link\_mode(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, u8 \*frl\_trained\_mask)
:   get the PCON HDMI LINK MODE

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`u8 *frl_trained_mask`
:   pointer to store bitmask of the trained bw configuration.
    Valid only if the MODE returned is FRL. For Normal Link training mode
    only 1 of the bits will be set, but in case of Extended mode, more than
    one bits can be set.

**Description**

Returns the link mode : TMDS or FRL on success, else returns negative error
code.

void drm\_dp\_pcon\_hdmi\_frl\_link\_error\_count(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   print the error count per lane during link failure between PCON and HDMI sink

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`struct drm_connector *connector`
:   DRM connector
    code.

int drm\_dp\_pcon\_pps\_default(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux)
:   Let PCON fill the default pps parameters for DSC1.2 between PCON & HDMI2.1 sink

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

**Description**

Returns 0 on success, else returns negative error code.

int drm\_dp\_pcon\_pps\_override\_buf(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, u8 pps\_buf[128])
:   Configure PPS encoder override buffer for HDMI sink

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`u8 pps_buf[128]`
:   128 bytes to be written into PPS buffer for HDMI sink by PCON.

**Description**

Returns 0 on success, else returns negative error code.

int drm\_edp\_backlight\_set\_level(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, const struct [drm\_edp\_backlight\_info](#c.drm_edp_backlight_info "drm_edp_backlight_info") \*bl, u32 level)
:   Set the backlight level of an eDP panel via AUX

**Parameters**

`struct drm_dp_aux *aux`
:   The DP AUX channel to use

`const struct drm_edp_backlight_info *bl`
:   Backlight capability info from [`drm_edp_backlight_init()`](#c.drm_edp_backlight_init "drm_edp_backlight_init")

`u32 level`
:   The brightness level to set

**Description**

Sets the brightness level of an eDP panel’s backlight. Note that the panel’s backlight must
already have been enabled by the driver by calling [`drm_edp_backlight_enable()`](#c.drm_edp_backlight_enable "drm_edp_backlight_enable").

**Return**

`0` on success, negative error code on failure

int drm\_edp\_backlight\_enable(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, const struct [drm\_edp\_backlight\_info](#c.drm_edp_backlight_info "drm_edp_backlight_info") \*bl, const u32 level)
:   Enable an eDP panel’s backlight using DPCD

**Parameters**

`struct drm_dp_aux *aux`
:   The DP AUX channel to use

`const struct drm_edp_backlight_info *bl`
:   Backlight capability info from [`drm_edp_backlight_init()`](#c.drm_edp_backlight_init "drm_edp_backlight_init")

`const u32 level`
:   The initial backlight level to set via AUX, if there is one

**Description**

This function handles enabling DPCD backlight controls on a panel over DPCD, while additionally
restoring any important backlight state such as the given backlight level, the brightness byte
count, backlight frequency, etc.

Note that certain panels do not support being enabled or disabled via DPCD, but instead require
that the driver handle enabling/disabling the panel through implementation-specific means using
the EDP\_BL\_PWR GPIO. For such panels, [`drm_edp_backlight_info.aux_enable`](#c.drm_edp_backlight_info "drm_edp_backlight_info") will be set to `false`,
this function becomes a no-op, and the driver is expected to handle powering the panel on using
the EDP\_BL\_PWR GPIO.

**Return**

`0` on success, negative error code on failure.

int drm\_edp\_backlight\_disable(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, const struct [drm\_edp\_backlight\_info](#c.drm_edp_backlight_info "drm_edp_backlight_info") \*bl)
:   Disable an eDP backlight using DPCD, if supported

**Parameters**

`struct drm_dp_aux *aux`
:   The DP AUX channel to use

`const struct drm_edp_backlight_info *bl`
:   Backlight capability info from [`drm_edp_backlight_init()`](#c.drm_edp_backlight_init "drm_edp_backlight_init")

**Description**

This function handles disabling DPCD backlight controls on a panel over AUX.

Note that certain panels do not support being enabled or disabled via DPCD, but instead require
that the driver handle enabling/disabling the panel through implementation-specific means using
the EDP\_BL\_PWR GPIO. For such panels, [`drm_edp_backlight_info.aux_enable`](#c.drm_edp_backlight_info "drm_edp_backlight_info") will be set to `false`,
this function becomes a no-op, and the driver is expected to handle powering the panel off using
the EDP\_BL\_PWR GPIO.

**Return**

`0` on success or no-op, negative error code on failure.

int drm\_edp\_backlight\_init(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, struct [drm\_edp\_backlight\_info](#c.drm_edp_backlight_info "drm_edp_backlight_info") \*bl, u32 max\_luminance, u16 driver\_pwm\_freq\_hz, const u8 edp\_dpcd[EDP\_DISPLAY\_CTL\_CAP\_SIZE], u32 \*current\_level, u8 \*current\_mode, bool need\_luminance)
:   Probe a display panel’s TCON using the standard VESA eDP backlight interface.

**Parameters**

`struct drm_dp_aux *aux`
:   The DP aux device to use for probing

`struct drm_edp_backlight_info *bl`
:   The [`drm_edp_backlight_info`](#c.drm_edp_backlight_info "drm_edp_backlight_info") `struct to` fill out with information on the backlight

`u32 max_luminance`
:   max luminance when need luminance is set as true

`u16 driver_pwm_freq_hz`
:   Optional PWM frequency from the driver in hz

`const u8 edp_dpcd[EDP_DISPLAY_CTL_CAP_SIZE]`
:   A cached copy of the eDP DPCD

`u32 *current_level`
:   Where to store the probed brightness level, if any

`u8 *current_mode`
:   Where to store the currently set backlight control mode

`bool need_luminance`
:   Tells us if a we want to manipulate backlight using luminance values

**Description**

Initializes a [`drm_edp_backlight_info`](#c.drm_edp_backlight_info "drm_edp_backlight_info") `struct by` probing **aux** for it’s backlight capabilities,
along with also probing the current and maximum supported brightness levels.

If **driver\_pwm\_freq\_hz** is non-zero, this will be used as the backlight frequency. Otherwise, the
default frequency from the panel is used.

**Return**

`0` on success, negative error code on failure.

int drm\_panel\_dp\_aux\_backlight(struct [drm\_panel](#c.drm_panel "drm_panel") \*panel, struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux)
:   create and use DP AUX backlight

**Parameters**

`struct drm_panel *panel`
:   DRM panel

`struct drm_dp_aux *aux`
:   The DP AUX channel to use

**Description**

Use this function to create and handle backlight if your panel
supports backlight control over DP AUX channel using DPCD
registers as per VESA’s standard backlight control interface.

When the panel is enabled backlight will be enabled after a
successful call to [`drm_panel_funcs.enable()`](#c.drm_panel_funcs "drm_panel_funcs")

When the panel is disabled backlight will be disabled before the
call to [`drm_panel_funcs.disable()`](#c.drm_panel_funcs "drm_panel_funcs").

A typical implementation for a panel driver supporting backlight
control over DP AUX will call this function at probe time.
Backlight will then be handled transparently without requiring
any intervention from the driver.

[`drm_panel_dp_aux_backlight()`](#c.drm_panel_dp_aux_backlight "drm_panel_dp_aux_backlight") must be called after the call to [`drm_panel_init()`](#c.drm_panel_init "drm_panel_init").

**Return**

0 on success or a negative error code on failure.

int drm\_dp\_link\_symbol\_cycles(int lane\_count, int pixels, int dsc\_slice\_count, int bpp\_x16, int symbol\_size, bool is\_mst)
:   calculate the link symbol count with/without dsc

**Parameters**

`int lane_count`
:   DP link lane count

`int pixels`
:   number of pixels in a scanline

`int dsc_slice_count`
:   number of slices for DSC or ‘0’ for non-DSC

`int bpp_x16`
:   bits per pixel in .4 binary fixed format

`int symbol_size`
:   DP symbol size

`bool is_mst`
:   `true` for MST and `false` for SST

**Description**

Calculate the link symbol cycles for both DSC (**dsc\_slice\_count** !=0) and
non-DSC case (**dsc\_slice\_count** == 0) and return the count.

int drm\_dp\_bw\_overhead(int lane\_count, int hactive, int dsc\_slice\_count, int bpp\_x16, unsigned long flags)
:   Calculate the BW overhead of a DP link stream

**Parameters**

`int lane_count`
:   DP link lane count

`int hactive`
:   pixel count of the active period in one scanline of the stream

`int dsc_slice_count`
:   number of slices for DSC or ‘0’ for non-DSC

`int bpp_x16`
:   bits per pixel in .4 binary fixed point

`unsigned long flags`
:   DRM\_DP\_OVERHEAD\_x flags

**Description**

Calculate the BW allocation overhead of a DP link stream, depending
on the link’s
- **lane\_count**
- SST/MST mode (**flags** / `DRM_DP_OVERHEAD_MST`)
- symbol size (**flags** / `DRM_DP_OVERHEAD_UHBR`)
- FEC mode (**flags** / `DRM_DP_OVERHEAD_FEC`)
- SSC/REF\_CLK mode (**flags** / `DRM_DP_OVERHEAD_SSC_REF_CLK`)
as well as the stream’s
- **hactive** timing
- **bpp\_x16** color depth
- compression mode (**dsc\_slice\_count** != 0)
Note that this overhead doesn’t account for the 8b/10b, 128b/132b
channel coding efficiency, for that see
**`drm_dp_link_bw_channel_coding_efficiency()`**.

Returns the overhead as 100% + overhead% in 1ppm units.

int drm\_dp\_bw\_channel\_coding\_efficiency(bool is\_uhbr)
:   Get a DP link’s channel coding efficiency

**Parameters**

`bool is_uhbr`
:   Whether the link has a 128b/132b channel coding

**Description**

Return the channel coding efficiency of the given DP link type, which is
either 8b/10b or 128b/132b (aka UHBR). The corresponding overhead includes
the 8b -> 10b, 128b -> 132b pixel data to link symbol conversion overhead
and for 128b/132b any link or PHY level control symbol insertion overhead
(LLCP, FEC, PHY sync, see DP Standard v2.1 3.5.2.18). For 8b/10b the
corresponding FEC overhead is BW allocation specific, included in the value
returned by [`drm_dp_bw_overhead()`](#c.drm_dp_bw_overhead "drm_dp_bw_overhead").

Returns the efficiency in the 100%/coding-overhead% ratio in
1ppm units.

int drm\_dp\_max\_dprx\_data\_rate(int max\_link\_rate, int max\_lanes)
:   Get the max data bandwidth of a DPRX sink

**Parameters**

`int max_link_rate`
:   max DPRX link rate in 10kbps units

`int max_lanes`
:   max DPRX lane count

**Description**

Given a link rate and lanes, get the data bandwidth.

Data bandwidth is the actual payload rate, which depends on the data
bandwidth efficiency and the link rate.

Note that protocol layers above the DPRX link level considered here can
further limit the maximum data rate. Such layers are the MST topology (with
limits on the link between the source and first branch device as well as on
the whole MST path until the DPRX link) and (Thunderbolt) DP tunnels -
which in turn can encapsulate an MST link with its own limit - with each
SST or MST encapsulated tunnel sharing the BW of a tunnel group.

Returns the maximum data rate in kBps units.

## Display Port CEC Helper Functions Reference

These functions take care of supporting the CEC-Tunneling-over-AUX
feature of DisplayPort-to-HDMI adapters.

void drm\_dp\_cec\_irq(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux)
:   handle CEC interrupt, if any

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

**Description**

Should be called when handling an IRQ\_HPD request. If CEC-tunneling-over-AUX
is present, then it will check for a CEC\_IRQ and handle it accordingly.

void drm\_dp\_cec\_register\_connector(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   register a new connector

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

`struct drm_connector *connector`
:   drm connector

**Description**

A new connector was registered with associated CEC adapter name and
CEC adapter parent device. After registering the name and parent
`drm_dp_cec_set_edid()` is called to check if the connector supports
CEC and to register a CEC adapter if that is the case.

void drm\_dp\_cec\_unregister\_connector(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux)
:   unregister the CEC adapter, if any

**Parameters**

`struct drm_dp_aux *aux`
:   DisplayPort AUX channel

## Display Port Dual Mode Adaptor Helper Functions Reference

Helper functions to deal with DP dual mode (aka. DP++) adaptors.

Type 1:
Adaptor registers (if any) and the sink DDC bus may be accessed via I2C.

Type 2:
Adaptor registers and sink DDC bus can be accessed either via I2C or
I2C-over-AUX. Source devices may choose to implement either of these
access methods.

enum drm\_lspcon\_mode

**Constants**

`DRM_LSPCON_MODE_INVALID`
:   No LSPCON.

`DRM_LSPCON_MODE_LS`
:   Level shifter mode of LSPCON
    which drives DP++ to HDMI 1.4 conversion.

`DRM_LSPCON_MODE_PCON`
:   Protocol converter mode of LSPCON
    which drives DP++ to HDMI 2.0 active conversion.

enum drm\_dp\_dual\_mode\_type
:   Type of the DP dual mode adaptor

**Constants**

`DRM_DP_DUAL_MODE_NONE`
:   No DP dual mode adaptor

`DRM_DP_DUAL_MODE_UNKNOWN`
:   Could be either none or type 1 DVI adaptor

`DRM_DP_DUAL_MODE_TYPE1_DVI`
:   Type 1 DVI adaptor

`DRM_DP_DUAL_MODE_TYPE1_HDMI`
:   Type 1 HDMI adaptor

`DRM_DP_DUAL_MODE_TYPE2_DVI`
:   Type 2 DVI adaptor

`DRM_DP_DUAL_MODE_TYPE2_HDMI`
:   Type 2 HDMI adaptor

`DRM_DP_DUAL_MODE_LSPCON`
:   Level shifter / protocol converter

ssize\_t drm\_dp\_dual\_mode\_read(struct i2c\_adapter \*adapter, u8 offset, void \*buffer, size\_t size)
:   Read from the DP dual mode adaptor register(s)

**Parameters**

`struct i2c_adapter *adapter`
:   I2C adapter for the DDC bus

`u8 offset`
:   register offset

`void *buffer`
:   buffer for return data

`size_t size`
:   size of the buffer

**Description**

Reads **size** bytes from the DP dual mode adaptor registers
starting at **offset**.

**Return**

0 on success, negative error code on failure

ssize\_t drm\_dp\_dual\_mode\_write(struct i2c\_adapter \*adapter, u8 offset, const void \*buffer, size\_t size)
:   Write to the DP dual mode adaptor register(s)

**Parameters**

`struct i2c_adapter *adapter`
:   I2C adapter for the DDC bus

`u8 offset`
:   register offset

`const void *buffer`
:   buffer for write data

`size_t size`
:   size of the buffer

**Description**

Writes **size** bytes to the DP dual mode adaptor registers
starting at **offset**.

**Return**

0 on success, negative error code on failure

enum [drm\_dp\_dual\_mode\_type](#c.drm_dp_dual_mode_type "drm_dp_dual_mode_type") drm\_dp\_dual\_mode\_detect(const struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct i2c\_adapter \*adapter)
:   Identify the DP dual mode adaptor

**Parameters**

`const struct drm_device *dev`
:   [`drm_device`](drm-internals.html#c.drm_device "drm_device") to use

`struct i2c_adapter *adapter`
:   I2C adapter for the DDC bus

**Description**

Attempt to identify the type of the DP dual mode adaptor used.

Note that when the answer is **DRM\_DP\_DUAL\_MODE\_UNKNOWN** it’s not
certain whether we’re dealing with a native HDMI port or
a type 1 DVI dual mode adaptor. The driver will have to use
some other hardware/driver specific mechanism to make that
distinction.

**Return**

The type of the DP dual mode adaptor used

int drm\_dp\_dual\_mode\_max\_tmds\_clock(const struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, enum [drm\_dp\_dual\_mode\_type](#c.drm_dp_dual_mode_type "drm_dp_dual_mode_type") type, struct i2c\_adapter \*adapter)
:   Max TMDS clock for DP dual mode adaptor

**Parameters**

`const struct drm_device *dev`
:   [`drm_device`](drm-internals.html#c.drm_device "drm_device") to use

`enum drm_dp_dual_mode_type type`
:   DP dual mode adaptor type

`struct i2c_adapter *adapter`
:   I2C adapter for the DDC bus

**Description**

Determine the max TMDS clock the adaptor supports based on the
type of the dual mode adaptor and the DP\_DUAL\_MODE\_MAX\_TMDS\_CLOCK
register (on type2 adaptors). As some type 1 adaptors have
problems with registers (see comments in [`drm_dp_dual_mode_detect()`](#c.drm_dp_dual_mode_detect "drm_dp_dual_mode_detect"))
we don’t read the register on those, instead we simply assume
a 165 MHz limit based on the specification.

**Return**

Maximum supported TMDS clock rate for the DP dual mode adaptor in kHz.

int drm\_dp\_dual\_mode\_get\_tmds\_output(const struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, enum [drm\_dp\_dual\_mode\_type](#c.drm_dp_dual_mode_type "drm_dp_dual_mode_type") type, struct i2c\_adapter \*adapter, bool \*enabled)
:   Get the state of the TMDS output buffers in the DP dual mode adaptor

**Parameters**

`const struct drm_device *dev`
:   [`drm_device`](drm-internals.html#c.drm_device "drm_device") to use

`enum drm_dp_dual_mode_type type`
:   DP dual mode adaptor type

`struct i2c_adapter *adapter`
:   I2C adapter for the DDC bus

`bool *enabled`
:   current state of the TMDS output buffers

**Description**

Get the state of the TMDS output buffers in the adaptor. For
type2 adaptors this is queried from the DP\_DUAL\_MODE\_TMDS\_OEN
register. As some type 1 adaptors have problems with registers
(see comments in [`drm_dp_dual_mode_detect()`](#c.drm_dp_dual_mode_detect "drm_dp_dual_mode_detect")) we don’t read the
register on those, instead we simply assume that the buffers
are always enabled.

**Return**

0 on success, negative error code on failure

int drm\_dp\_dual\_mode\_set\_tmds\_output(const struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, enum [drm\_dp\_dual\_mode\_type](#c.drm_dp_dual_mode_type "drm_dp_dual_mode_type") type, struct i2c\_adapter \*adapter, bool enable)
:   Enable/disable TMDS output buffers in the DP dual mode adaptor

**Parameters**

`const struct drm_device *dev`
:   [`drm_device`](drm-internals.html#c.drm_device "drm_device") to use

`enum drm_dp_dual_mode_type type`
:   DP dual mode adaptor type

`struct i2c_adapter *adapter`
:   I2C adapter for the DDC bus

`bool enable`
:   enable (as opposed to disable) the TMDS output buffers

**Description**

Set the state of the TMDS output buffers in the adaptor. For
type2 this is set via the DP\_DUAL\_MODE\_TMDS\_OEN register.
Type1 adaptors do not support any register writes.

**Return**

0 on success, negative error code on failure

const char \*drm\_dp\_get\_dual\_mode\_type\_name(enum [drm\_dp\_dual\_mode\_type](#c.drm_dp_dual_mode_type "drm_dp_dual_mode_type") type)
:   Get the name of the DP dual mode adaptor type as a string

**Parameters**

`enum drm_dp_dual_mode_type type`
:   DP dual mode adaptor type

**Return**

String representation of the DP dual mode adaptor type

int drm\_lspcon\_get\_mode(const struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct i2c\_adapter \*adapter, enum [drm\_lspcon\_mode](#c.drm_lspcon_mode "drm_lspcon_mode") \*mode)
:   Get LSPCON’s current mode of operation by reading offset (0x80, 0x41)

**Parameters**

`const struct drm_device *dev`
:   [`drm_device`](drm-internals.html#c.drm_device "drm_device") to use

`struct i2c_adapter *adapter`
:   I2C-over-aux adapter

`enum drm_lspcon_mode *mode`
:   current lspcon mode of operation output variable

**Return**

0 on success, sets the current\_mode value to appropriate mode
-error on failure

int drm\_lspcon\_set\_mode(const struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct i2c\_adapter \*adapter, enum [drm\_lspcon\_mode](#c.drm_lspcon_mode "drm_lspcon_mode") mode, int time\_out)
:   Change LSPCON’s mode of operation by writing offset (0x80, 0x40)

**Parameters**

`const struct drm_device *dev`
:   [`drm_device`](drm-internals.html#c.drm_device "drm_device") to use

`struct i2c_adapter *adapter`
:   I2C-over-aux adapter

`enum drm_lspcon_mode mode`
:   required mode of operation

`int time_out`
:   LSPCON mode change settle timeout

**Return**

0 on success, -error on failure/timeout

## Display Port MST Helpers

### Overview

These functions contain parts of the DisplayPort 1.2a MultiStream Transport
protocol. The helpers contain a topology manager and bandwidth manager.
The helpers encapsulate the sending and received of sideband msgs.

#### Topology refcount overview

The refcounting schemes for [`struct drm_dp_mst_branch`](#c.drm_dp_mst_branch "drm_dp_mst_branch") and [`struct
drm_dp_mst_port`](#c.drm_dp_mst_port "drm_dp_mst_port") are somewhat unusual. Both ports and branch devices have
two different kinds of refcounts: topology refcounts, and malloc refcounts.

Topology refcounts are not exposed to drivers, and are handled internally
by the DP MST helpers. The helpers use them in order to prevent the
in-memory topology state from being changed in the middle of critical
operations like changing the internal state of payload allocations. This
means each branch and port will be considered to be connected to the rest
of the topology until its topology refcount reaches zero. Additionally,
for ports this means that their associated [`struct drm_connector`](drm-kms.html#c.drm_connector "drm_connector") will stay
registered with userspace until the port’s refcount reaches 0.

#### Malloc refcount overview

Malloc references are used to keep a [`struct drm_dp_mst_port`](#c.drm_dp_mst_port "drm_dp_mst_port") or [`struct
drm_dp_mst_branch`](#c.drm_dp_mst_branch "drm_dp_mst_branch") allocated even after all of its topology references have
been dropped, so that the driver or MST helpers can safely access each
branch’s last known state before it was disconnected from the topology.
When the malloc refcount of a port or branch reaches 0, the memory
allocation containing the [`struct drm_dp_mst_branch`](#c.drm_dp_mst_branch "drm_dp_mst_branch") or [`struct
drm_dp_mst_port`](#c.drm_dp_mst_port "drm_dp_mst_port") respectively will be freed.

For [`struct drm_dp_mst_branch`](#c.drm_dp_mst_branch "drm_dp_mst_branch"), malloc refcounts are not currently exposed
to drivers. As of writing this documentation, there are no drivers that
have a usecase for accessing [`struct drm_dp_mst_branch`](#c.drm_dp_mst_branch "drm_dp_mst_branch") outside of the MST
helpers. Exposing this API to drivers in a race-free manner would take more
tweaking of the refcounting scheme, however patches are welcome provided
there is a legitimate driver usecase for this.

#### Refcount relationships in a topology

Let’s take a look at why the relationship between topology and malloc
refcounts is designed the way it is.

![../_images/topology-figure-1.svg](../_images/topology-figure-1.svg)

An example of topology and malloc refs in a DP MST topology with two
active payloads. Topology refcount increments are indicated by solid
lines, and malloc refcount increments are indicated by dashed lines.
Each starts from the branch which incremented the refcount, and ends at
the branch to which the refcount belongs to, i.e. the arrow points the
same way as the C pointers used to reference a structure.

As you can see in the above figure, every branch increments the topology
refcount of its children, and increments the malloc refcount of its
parent. Additionally, every payload increments the malloc refcount of its
assigned port by 1.

So, what would happen if MSTB #3 from the above figure was unplugged from
the system, but the driver hadn’t yet removed payload #2 from port #3? The
topology would start to look like the figure below.

![../_images/topology-figure-2.svg](../_images/topology-figure-2.svg)

Ports and branch devices which have been released from memory are
colored grey, and references which have been removed are colored red.

Whenever a port or branch device’s topology refcount reaches zero, it will
decrement the topology refcounts of all its children, the malloc refcount
of its parent, and finally its own malloc refcount. For MSTB #4 and port
#4, this means they both have been disconnected from the topology and freed
from memory. But, because payload #2 is still holding a reference to port
#3, port #3 is removed from the topology but its [`struct drm_dp_mst_port`](#c.drm_dp_mst_port "drm_dp_mst_port")
is still accessible from memory. This also means port #3 has not yet
decremented the malloc refcount of MSTB #3, so its [`struct
drm_dp_mst_branch`](#c.drm_dp_mst_branch "drm_dp_mst_branch") will also stay allocated in memory until port #3’s
malloc refcount reaches 0.

This relationship is necessary because in order to release payload #2, we
need to be able to figure out the last relative of port #3 that’s still
connected to the topology. In this case, we would travel up the topology as
shown below.

![../_images/topology-figure-3.svg](../_images/topology-figure-3.svg)

And finally, remove payload #2 by communicating with port #2 through
sideband transactions.

### Functions Reference

struct drm\_dp\_mst\_port
:   MST port

**Definition**:

```
struct drm_dp_mst_port {
    struct kref topology_kref;
    struct kref malloc_kref;
#if IS_ENABLED(CONFIG_DRM_DEBUG_DP_MST_TOPOLOGY_REFS);
    struct drm_dp_mst_topology_ref_history topology_ref_history;
#endif;
    u8 port_num;
    bool input;
    bool mcs;
    bool ddps;
    u8 pdt;
    bool ldps;
    u8 dpcd_rev;
    u8 num_sdp_streams;
    u8 num_sdp_stream_sinks;
    uint16_t full_pbn;
    struct list_head next;
    struct drm_dp_mst_branch *mstb;
    struct drm_dp_aux aux;
    struct drm_dp_aux *passthrough_aux;
    struct drm_dp_mst_branch *parent;
    struct drm_connector *connector;
    struct drm_dp_mst_topology_mgr *mgr;
    const struct drm_edid *cached_edid;
    bool fec_capable;
};
```

**Members**

`topology_kref`
:   refcount for this port’s lifetime in the topology,
    only the DP MST helpers should need to touch this

`malloc_kref`
:   refcount for the memory allocation containing this
    structure. See [`drm_dp_mst_get_port_malloc()`](#c.drm_dp_mst_get_port_malloc "drm_dp_mst_get_port_malloc") and
    [`drm_dp_mst_put_port_malloc()`](#c.drm_dp_mst_put_port_malloc "drm_dp_mst_put_port_malloc").

`topology_ref_history`
:   A history of each topology
    reference/dereference. See CONFIG\_DRM\_DEBUG\_DP\_MST\_TOPOLOGY\_REFS.

`port_num`
:   port number

`input`
:   if this port is an input port. Protected by
    [`drm_dp_mst_topology_mgr.base`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr").lock.

`mcs`
:   message capability status - DP 1.2 spec. Protected by
    [`drm_dp_mst_topology_mgr.base`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr").lock.

`ddps`
:   DisplayPort Device Plug Status - DP 1.2. Protected by
    [`drm_dp_mst_topology_mgr.base`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr").lock.

`pdt`
:   Peer Device Type. Protected by
    [`drm_dp_mst_topology_mgr.base`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr").lock.

`ldps`
:   Legacy Device Plug Status. Protected by
    [`drm_dp_mst_topology_mgr.base`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr").lock.

`dpcd_rev`
:   DPCD revision of device on this port. Protected by
    [`drm_dp_mst_topology_mgr.base`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr").lock.

`num_sdp_streams`
:   Number of simultaneous streams. Protected by
    [`drm_dp_mst_topology_mgr.base`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr").lock.

`num_sdp_stream_sinks`
:   Number of stream sinks. Protected by
    [`drm_dp_mst_topology_mgr.base`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr").lock.

`full_pbn`
:   Max possible bandwidth for this port. Protected by
    [`drm_dp_mst_topology_mgr.base`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr").lock.

`next`
:   link to next port on this branch device

`mstb`
:   the branch device connected to this port, if there is one.
    This should be considered protected for reading by
    [`drm_dp_mst_topology_mgr.lock`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr"). There are two exceptions to this:
    [`drm_dp_mst_topology_mgr.up_req_work`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") and
    [`drm_dp_mst_topology_mgr.work`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr"), which do not grab
    [`drm_dp_mst_topology_mgr.lock`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") during reads but are the only
    updaters of this list and are protected from writing concurrently
    by [`drm_dp_mst_topology_mgr.probe_lock`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr").

`aux`
:   i2c aux transport to talk to device connected to this port, protected
    by [`drm_dp_mst_topology_mgr.base`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr").lock.

`passthrough_aux`
:   parent aux to which DSC pass-through requests should be
    sent, only set if DSC pass-through is possible.

`parent`
:   branch device parent of this port

`connector`
:   DRM connector this port is connected to. Protected by
    [`drm_dp_mst_topology_mgr.base`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr").lock.

`mgr`
:   topology manager this port lives under.

`cached_edid`
:   for DP logical ports - make tiling work by ensuring
    that the EDID for all connectors is read immediately.

`fec_capable`
:   bool indicating if FEC can be supported up to that
    point in the MST topology.

**Description**

This structure represents an MST port endpoint on a device somewhere
in the MST topology.

struct drm\_dp\_mst\_branch
:   MST branch device.

**Definition**:

```
struct drm_dp_mst_branch {
    struct kref topology_kref;
    struct kref malloc_kref;
#if IS_ENABLED(CONFIG_DRM_DEBUG_DP_MST_TOPOLOGY_REFS);
    struct drm_dp_mst_topology_ref_history topology_ref_history;
#endif;
    struct list_head destroy_next;
    u8 rad[8];
    u8 lct;
    int num_ports;
    struct list_head ports;
    struct drm_dp_mst_port *port_parent;
    struct drm_dp_mst_topology_mgr *mgr;
    bool link_address_sent;
    guid_t guid;
};
```

**Members**

`topology_kref`
:   refcount for this branch device’s lifetime in the
    topology, only the DP MST helpers should need to touch this

`malloc_kref`
:   refcount for the memory allocation containing this
    structure. See [`drm_dp_mst_get_mstb_malloc()`](#c.drm_dp_mst_get_mstb_malloc "drm_dp_mst_get_mstb_malloc") and
    [`drm_dp_mst_put_mstb_malloc()`](#c.drm_dp_mst_put_mstb_malloc "drm_dp_mst_put_mstb_malloc").

`topology_ref_history`
:   A history of each topology
    reference/dereference. See CONFIG\_DRM\_DEBUG\_DP\_MST\_TOPOLOGY\_REFS.

`destroy_next`
:   linked-list entry used by
    `drm_dp_delayed_destroy_work()`

`rad`
:   Relative Address of the MST branch.
    For [`drm_dp_mst_topology_mgr.mst_primary`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr"), it’s rad[8] are all 0,
    unset and unused. For MST branches connected after mst\_primary,
    in each element of rad[] the nibbles are ordered by the most
    signifcant 4 bits first and the least significant 4 bits second.

`lct`
:   Link count total to talk to this branch device.

`num_ports`
:   number of ports on the branch.

`ports`
:   the list of ports on this branch device. This should be
    considered protected for reading by [`drm_dp_mst_topology_mgr.lock`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr").
    There are two exceptions to this:
    [`drm_dp_mst_topology_mgr.up_req_work`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") and
    [`drm_dp_mst_topology_mgr.work`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr"), which do not grab
    [`drm_dp_mst_topology_mgr.lock`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") during reads but are the only
    updaters of this list and are protected from updating the list
    concurrently by **drm\_dp\_mst\_topology\_mgr.probe\_lock**

`port_parent`
:   pointer to the port parent, NULL if toplevel.

`mgr`
:   topology manager for this branch device.

`link_address_sent`
:   if a link address message has been sent to this device yet.

`guid`
:   guid for DP 1.2 branch device. port under this branch can be
    identified by port #.

**Description**

This structure represents an MST branch device, there is one
primary branch device at the root, along with any other branches connected
to downstream port of parent branches.

struct drm\_dp\_mst\_atomic\_payload
:   Atomic state struct for an MST payload

**Definition**:

```
struct drm_dp_mst_atomic_payload {
    struct drm_dp_mst_port *port;
    s8 vc_start_slot;
    u8 vcpi;
    int time_slots;
    int pbn;
    bool delete : 1;
    bool dsc_enabled : 1;
    enum drm_dp_mst_payload_allocation payload_allocation_status;
    struct list_head next;
};
```

**Members**

`port`
:   The MST port assigned to this payload

`vc_start_slot`
:   The time slot that this payload starts on. Because payload start slots
    can’t be determined ahead of time, the contents of this value are UNDEFINED at atomic
    check time. This shouldn’t usually matter, as the start slot should never be relevant for
    atomic state computations.

    Since this value is determined at commit time instead of check time, this value is
    protected by the MST helpers ensuring that async commits operating on the given topology
    never run in parallel. In the event that a driver does need to read this value (e.g. to
    inform hardware of the starting timeslot for a payload), the driver may either:

    * Read this field during the atomic commit after
      [`drm_dp_mst_atomic_wait_for_dependencies()`](#c.drm_dp_mst_atomic_wait_for_dependencies "drm_dp_mst_atomic_wait_for_dependencies") has been called, which will ensure the
      previous MST states payload start slots have been copied over to the new state. Note
      that a new start slot won’t be assigned/removed from this payload until
      [`drm_dp_add_payload_part1()`](#c.drm_dp_add_payload_part1 "drm_dp_add_payload_part1")/[`drm_dp_remove_payload_part2()`](#c.drm_dp_remove_payload_part2 "drm_dp_remove_payload_part2") have been called.
    * Acquire the MST modesetting lock, and then wait for any pending MST-related commits to
      get committed to hardware by calling [`drm_crtc_commit_wait()`](drm-kms.html#c.drm_crtc_commit_wait "drm_crtc_commit_wait") on each of the
      [`drm_crtc_commit`](drm-kms.html#c.drm_crtc_commit "drm_crtc_commit") structs in [`drm_dp_mst_topology_state.commit_deps`](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state").

    If neither of the two above solutions suffice (e.g. the driver needs to read the start
    slot in the middle of an atomic commit without waiting for some reason), then drivers
    should cache this value themselves after changing payloads.

`vcpi`
:   The Virtual Channel Payload Identifier

`time_slots`
:   The number of timeslots allocated to this payload from the source DP Tx to
    the immediate downstream DP Rx

`pbn`
:   The payload bandwidth for this payload

`delete`
:   Whether or not we intend to delete this payload during this atomic commit

`dsc_enabled`
:   Whether or not this payload has DSC enabled

`payload_allocation_status`
:   The allocation status of this payload

`next`
:   The list node for this payload

**Description**

The primary atomic state structure for a given MST payload. Stores information like current
bandwidth allocation, intended action for this payload, etc.

struct drm\_dp\_mst\_topology\_state
:   DisplayPort MST topology atomic state

**Definition**:

```
struct drm_dp_mst_topology_state {
    struct drm_private_state base;
    struct drm_dp_mst_topology_mgr *mgr;
    u32 pending_crtc_mask;
    struct drm_crtc_commit **commit_deps;
    size_t num_commit_deps;
    u32 payload_mask;
    struct list_head payloads;
    u8 total_avail_slots;
    u8 start_slot;
    fixed20_12 pbn_div;
};
```

**Members**

`base`
:   Base private state for atomic

`mgr`
:   The topology manager

`pending_crtc_mask`
:   A bitmask of all CRTCs this topology state touches, drivers may
    modify this to add additional dependencies if needed.

`commit_deps`
:   A list of all CRTC commits affecting this topology, this field isn’t
    populated until [`drm_dp_mst_atomic_wait_for_dependencies()`](#c.drm_dp_mst_atomic_wait_for_dependencies "drm_dp_mst_atomic_wait_for_dependencies") is called.

`num_commit_deps`
:   The number of CRTC commits in **commit\_deps**

`payload_mask`
:   A bitmask of allocated VCPIs, used for VCPI assignments

`payloads`
:   The list of payloads being created/destroyed in this state

`total_avail_slots`
:   The total number of slots this topology can handle (63 or 64)

`start_slot`
:   The first usable time slot in this topology (1 or 0)

`pbn_div`
:   The current PBN divisor for this topology. The driver is expected to fill this
    out itself.

**Description**

This `struct represents` the atomic state of the toplevel DisplayPort MST manager

struct drm\_dp\_mst\_topology\_mgr
:   DisplayPort MST manager

**Definition**:

```
struct drm_dp_mst_topology_mgr {
    struct drm_private_obj base;
    struct drm_device *dev;
    const struct drm_dp_mst_topology_cbs *cbs;
    int max_dpcd_transaction_bytes;
    struct drm_dp_aux *aux;
    int max_payloads;
    int conn_base_id;
    struct drm_dp_sideband_msg_rx up_req_recv;
    struct drm_dp_sideband_msg_rx down_rep_recv;
    struct mutex lock;
    struct mutex probe_lock;
    bool mst_state : 1;
    bool payload_id_table_cleared : 1;
    bool reset_rx_state : 1;
    u8 payload_count;
    u8 next_start_slot;
    struct drm_dp_mst_branch *mst_primary;
    u8 dpcd[DP_RECEIVER_CAP_SIZE];
    u8 sink_count;
    const struct drm_private_state_funcs *funcs;
    struct mutex qlock;
    struct list_head tx_msg_downq;
    wait_queue_head_t tx_waitq;
    struct work_struct work;
    struct work_struct tx_work;
    struct list_head destroy_port_list;
    struct list_head destroy_branch_device_list;
    struct mutex delayed_destroy_lock;
    struct workqueue_struct *delayed_destroy_wq;
    struct work_struct delayed_destroy_work;
    struct list_head up_req_list;
    struct mutex up_req_lock;
    struct work_struct up_req_work;
#if IS_ENABLED(CONFIG_DRM_DEBUG_DP_MST_TOPOLOGY_REFS);
    struct mutex topology_ref_history_lock;
#endif;
};
```

**Members**

`base`
:   Base private object for atomic

`dev`
:   device pointer for adding i2c devices etc.

`cbs`
:   callbacks for connector addition and destruction.

`max_dpcd_transaction_bytes`
:   maximum number of bytes to read/write
    in one go.

`aux`
:   AUX channel for the DP MST connector this topolgy mgr is
    controlling.

`max_payloads`
:   maximum number of payloads the GPU can generate.

`conn_base_id`
:   DRM connector ID this mgr is connected to. Only used
    to build the MST connector path value.

`up_req_recv`
:   Message receiver state for up requests.

`down_rep_recv`
:   Message receiver state for replies to down
    requests.

`lock`
:   protects **mst\_state**, **mst\_primary**, **dpcd**, and
    **payload\_id\_table\_cleared**.

`probe_lock`
:   Prevents **work** and **up\_req\_work**, the only writers of
    [`drm_dp_mst_port.mstb`](#c.drm_dp_mst_port "drm_dp_mst_port") and [`drm_dp_mst_branch.ports`](#c.drm_dp_mst_branch "drm_dp_mst_branch"), from racing
    while they update the topology.

`mst_state`
:   If this manager is enabled for an MST capable port. False
    if no MST sink/branch devices is connected.

`payload_id_table_cleared`
:   Whether or not we’ve cleared the payload
    ID table for **mst\_primary**. Protected by **lock**.

`reset_rx_state`
:   The down request’s reply and up request message
    receiver state must be reset, after the topology manager got
    removed. Protected by **lock**.

`payload_count`
:   The number of currently active payloads in hardware. This value is only
    intended to be used internally by MST helpers for payload tracking, and is only safe to
    read/write from the atomic commit (not check) context.

`next_start_slot`
:   The starting timeslot to use for new VC payloads. This value is used
    internally by MST helpers for payload tracking, and is only safe to read/write from the
    atomic commit (not check) context.

`mst_primary`
:   Pointer to the primary/first branch device.

`dpcd`
:   Cache of DPCD for primary port.

`sink_count`
:   Sink count from DEVICE\_SERVICE\_IRQ\_VECTOR\_ESI0.

`funcs`
:   Atomic helper callbacks

`qlock`
:   protects **tx\_msg\_downq** and `drm_dp_sideband_msg_tx.state`

`tx_msg_downq`
:   List of pending down requests

`tx_waitq`
:   Wait to queue stall for the tx worker.

`work`
:   Probe work.

`tx_work`
:   Sideband transmit worker. This can nest within the main
    **work** worker for each transaction **work** launches.

`destroy_port_list`
:   List of to be destroyed connectors.

`destroy_branch_device_list`
:   List of to be destroyed branch
    devices.

`delayed_destroy_lock`
:   Protects **destroy\_port\_list** and
    **destroy\_branch\_device\_list**.

`delayed_destroy_wq`
:   Workqueue used for delayed\_destroy\_work items.
    A dedicated WQ makes it possible to drain any requeued work items
    on it.

`delayed_destroy_work`
:   Work item to destroy MST port and branch
    devices, needed to avoid locking inversion.

`up_req_list`
:   List of pending up requests from the topology that
    need to be processed, in chronological order.

`up_req_lock`
:   Protects **up\_req\_list**

`up_req_work`
:   Work item to process up requests received from the
    topology. Needed to avoid blocking hotplug handling and sideband
    transmissions.

`topology_ref_history_lock`
:   protects
    [`drm_dp_mst_port.topology_ref_history`](#c.drm_dp_mst_port "drm_dp_mst_port") and
    [`drm_dp_mst_branch.topology_ref_history`](#c.drm_dp_mst_branch "drm_dp_mst_branch").

**Description**

This `struct represents` the toplevel displayport MST topology manager.
There should be one instance of this for every MST capable DP connector
on the GPU.

enum drm\_dp\_mst\_mode
:   sink’s MST mode capability

**Constants**

`DRM_DP_SST`
:   The sink does not support MST nor single stream sideband
    messaging.

`DRM_DP_MST`
:   Sink supports MST, more than one stream and single
    stream sideband messaging.

`DRM_DP_SST_SIDEBAND_MSG`
:   Sink supports only one stream and single
    stream sideband messaging.

bool \_\_drm\_dp\_mst\_state\_iter\_get(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*\*mgr, struct [drm\_dp\_mst\_topology\_state](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state") \*\*old\_state, struct [drm\_dp\_mst\_topology\_state](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state") \*\*new\_state, int i)
:   private atomic state iterator function for macro-internal use

**Parameters**

`struct drm_atomic_state *state`
:   [`struct drm_atomic_state`](drm-kms.html#c.drm_atomic_state "drm_atomic_state") pointer

`struct drm_dp_mst_topology_mgr **mgr`
:   pointer to the [`struct drm_dp_mst_topology_mgr`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") iteration cursor

`struct drm_dp_mst_topology_state **old_state`
:   optional pointer to the old [`struct drm_dp_mst_topology_state`](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state")
    iteration cursor

`struct drm_dp_mst_topology_state **new_state`
:   optional pointer to the new [`struct drm_dp_mst_topology_state`](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state")
    iteration cursor

`int i`
:   int iteration cursor, for macro-internal use

**Description**

Used by [`for_each_oldnew_mst_mgr_in_state()`](#c.for_each_oldnew_mst_mgr_in_state "for_each_oldnew_mst_mgr_in_state"),
[`for_each_old_mst_mgr_in_state()`](#c.for_each_old_mst_mgr_in_state "for_each_old_mst_mgr_in_state"), and [`for_each_new_mst_mgr_in_state()`](#c.for_each_new_mst_mgr_in_state "for_each_new_mst_mgr_in_state"). Don’t
call this directly.

**Return**

True if the current [`struct drm_private_obj`](drm-kms.html#c.drm_private_obj "drm_private_obj") is a [`struct
drm_dp_mst_topology_mgr`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr"), false otherwise.

for\_each\_oldnew\_mst\_mgr\_in\_state

`for_each_oldnew_mst_mgr_in_state (__state, mgr, old_state, new_state, __i)`

> iterate over all DP MST topology managers in an atomic update

**Parameters**

`__state`
:   [`struct drm_atomic_state`](drm-kms.html#c.drm_atomic_state "drm_atomic_state") pointer

`mgr`
:   [`struct drm_dp_mst_topology_mgr`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") iteration cursor

`old_state`
:   [`struct drm_dp_mst_topology_state`](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state") iteration cursor for the old
    state

`new_state`
:   [`struct drm_dp_mst_topology_state`](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state") iteration cursor for the new
    state

`__i`
:   int iteration cursor, for macro-internal use

**Description**

This iterates over all DRM DP MST topology managers in an atomic update,
tracking both old and new state. This is useful in places where the state
delta needs to be considered, for example in atomic check functions.

for\_each\_old\_mst\_mgr\_in\_state

`for_each_old_mst_mgr_in_state (__state, mgr, old_state, __i)`

> iterate over all DP MST topology managers in an atomic update

**Parameters**

`__state`
:   [`struct drm_atomic_state`](drm-kms.html#c.drm_atomic_state "drm_atomic_state") pointer

`mgr`
:   [`struct drm_dp_mst_topology_mgr`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") iteration cursor

`old_state`
:   [`struct drm_dp_mst_topology_state`](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state") iteration cursor for the old
    state

`__i`
:   int iteration cursor, for macro-internal use

**Description**

This iterates over all DRM DP MST topology managers in an atomic update,
tracking only the old state. This is useful in disable functions, where we
need the old state the hardware is still in.

for\_each\_new\_mst\_mgr\_in\_state

`for_each_new_mst_mgr_in_state (__state, mgr, new_state, __i)`

> iterate over all DP MST topology managers in an atomic update

**Parameters**

`__state`
:   [`struct drm_atomic_state`](drm-kms.html#c.drm_atomic_state "drm_atomic_state") pointer

`mgr`
:   [`struct drm_dp_mst_topology_mgr`](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") iteration cursor

`new_state`
:   [`struct drm_dp_mst_topology_state`](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state") iteration cursor for the new
    state

`__i`
:   int iteration cursor, for macro-internal use

**Description**

This iterates over all DRM DP MST topology managers in an atomic update,
tracking only the new state. This is useful in enable functions, where we
need the new state the hardware should be in when the atomic commit
operation has completed.

void drm\_dp\_mst\_get\_port\_malloc(struct [drm\_dp\_mst\_port](#c.drm_dp_mst_port "drm_dp_mst_port") \*port)
:   Increment the malloc refcount of an MST port

**Parameters**

`struct drm_dp_mst_port *port`
:   The [`struct drm_dp_mst_port`](#c.drm_dp_mst_port "drm_dp_mst_port") to increment the malloc refcount of

**Description**

Increments [`drm_dp_mst_port.malloc_kref`](#c.drm_dp_mst_port "drm_dp_mst_port"). When [`drm_dp_mst_port.malloc_kref`](#c.drm_dp_mst_port "drm_dp_mst_port")
reaches 0, the memory allocation for **port** will be released and **port** may
no longer be used.

Because **port** could potentially be freed at any time by the DP MST helpers
if [`drm_dp_mst_port.malloc_kref`](#c.drm_dp_mst_port "drm_dp_mst_port") reaches 0, including during a call to this
function, drivers that which to make use of [`struct drm_dp_mst_port`](#c.drm_dp_mst_port "drm_dp_mst_port") should
ensure that they grab at least one main malloc reference to their MST ports
in `drm_dp_mst_topology_cbs.add_connector`. This callback is called before
there is any chance for [`drm_dp_mst_port.malloc_kref`](#c.drm_dp_mst_port "drm_dp_mst_port") to reach 0.

See also: [`drm_dp_mst_put_port_malloc()`](#c.drm_dp_mst_put_port_malloc "drm_dp_mst_put_port_malloc")

void drm\_dp\_mst\_put\_port\_malloc(struct [drm\_dp\_mst\_port](#c.drm_dp_mst_port "drm_dp_mst_port") \*port)
:   Decrement the malloc refcount of an MST port

**Parameters**

`struct drm_dp_mst_port *port`
:   The [`struct drm_dp_mst_port`](#c.drm_dp_mst_port "drm_dp_mst_port") to decrement the malloc refcount of

**Description**

Decrements [`drm_dp_mst_port.malloc_kref`](#c.drm_dp_mst_port "drm_dp_mst_port"). When [`drm_dp_mst_port.malloc_kref`](#c.drm_dp_mst_port "drm_dp_mst_port")
reaches 0, the memory allocation for **port** will be released and **port** may
no longer be used.

See also: [`drm_dp_mst_get_port_malloc()`](#c.drm_dp_mst_get_port_malloc "drm_dp_mst_get_port_malloc")

int drm\_dp\_mst\_connector\_late\_register(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct [drm\_dp\_mst\_port](#c.drm_dp_mst_port "drm_dp_mst_port") \*port)
:   Late MST connector registration

**Parameters**

`struct drm_connector *connector`
:   The MST connector

`struct drm_dp_mst_port *port`
:   The MST port for this connector

**Description**

Helper to register the remote aux device for this MST port. Drivers should
call this from their mst connector’s late\_register hook to enable MST aux
devices.

**Return**

0 on success, negative error code on failure.

void drm\_dp\_mst\_connector\_early\_unregister(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct [drm\_dp\_mst\_port](#c.drm_dp_mst_port "drm_dp_mst_port") \*port)
:   Early MST connector unregistration

**Parameters**

`struct drm_connector *connector`
:   The MST connector

`struct drm_dp_mst_port *port`
:   The MST port for this connector

**Description**

Helper to unregister the remote aux device for this MST port, registered by
[`drm_dp_mst_connector_late_register()`](#c.drm_dp_mst_connector_late_register "drm_dp_mst_connector_late_register"). Drivers should call this from their mst
connector’s early\_unregister hook.

int drm\_dp\_add\_payload\_part1(struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr, struct [drm\_dp\_mst\_topology\_state](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state") \*mst\_state, struct [drm\_dp\_mst\_atomic\_payload](#c.drm_dp_mst_atomic_payload "drm_dp_mst_atomic_payload") \*payload)
:   Execute payload update part 1

**Parameters**

`struct drm_dp_mst_topology_mgr *mgr`
:   Manager to use.

`struct drm_dp_mst_topology_state *mst_state`
:   The MST atomic state

`struct drm_dp_mst_atomic_payload *payload`
:   The payload to write

**Description**

Determines the starting time slot for the given payload, and programs the VCPI for this payload
into the DPCD of DPRX. After calling this, the driver should generate ACT and payload packets.

**Return**

0 on success, error code on failure.

void drm\_dp\_remove\_payload\_part1(struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr, struct [drm\_dp\_mst\_topology\_state](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state") \*mst\_state, struct [drm\_dp\_mst\_atomic\_payload](#c.drm_dp_mst_atomic_payload "drm_dp_mst_atomic_payload") \*payload)
:   Remove an MST payload along the virtual channel

**Parameters**

`struct drm_dp_mst_topology_mgr *mgr`
:   Manager to use.

`struct drm_dp_mst_topology_state *mst_state`
:   The MST atomic state

`struct drm_dp_mst_atomic_payload *payload`
:   The payload to remove

**Description**

Removes a payload along the virtual channel if it was successfully allocated.
After calling this, the driver should set HW to generate ACT and then switch to new
payload allocation state.

void drm\_dp\_remove\_payload\_part2(struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr, struct [drm\_dp\_mst\_topology\_state](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state") \*mst\_state, const struct [drm\_dp\_mst\_atomic\_payload](#c.drm_dp_mst_atomic_payload "drm_dp_mst_atomic_payload") \*old\_payload, struct [drm\_dp\_mst\_atomic\_payload](#c.drm_dp_mst_atomic_payload "drm_dp_mst_atomic_payload") \*new\_payload)
:   Remove an MST payload locally

**Parameters**

`struct drm_dp_mst_topology_mgr *mgr`
:   Manager to use.

`struct drm_dp_mst_topology_state *mst_state`
:   The MST atomic state

`const struct drm_dp_mst_atomic_payload *old_payload`
:   The payload with its old state

`struct drm_dp_mst_atomic_payload *new_payload`
:   The payload with its latest state

**Description**

Updates the starting time slots of all other payloads which would have been shifted towards
the start of the payload ID table as a result of removing a payload. Driver should call this
function whenever it removes a payload in its HW. It’s independent to the result of payload
allocation/deallocation at branch devices along the virtual channel.

int drm\_dp\_add\_payload\_part2(struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr, struct [drm\_dp\_mst\_atomic\_payload](#c.drm_dp_mst_atomic_payload "drm_dp_mst_atomic_payload") \*payload)
:   Execute payload update part 2

**Parameters**

`struct drm_dp_mst_topology_mgr *mgr`
:   Manager to use.

`struct drm_dp_mst_atomic_payload *payload`
:   The payload to update

**Description**

If **payload** was successfully assigned a starting time slot by [`drm_dp_add_payload_part1()`](#c.drm_dp_add_payload_part1 "drm_dp_add_payload_part1"), this
function will send the sideband messages to finish allocating this payload.

**Return**

0 on success, negative error code on failure.

fixed20\_12 drm\_dp\_get\_vc\_payload\_bw(int link\_rate, int link\_lane\_count)
:   get the VC payload BW for an MTP link

**Parameters**

`int link_rate`
:   link rate in 10kbits/s units

`int link_lane_count`
:   lane count

**Description**

Calculate the total bandwidth of a MultiStream Transport link. The returned
value is in units of PBNs/(timeslots/1 MTP). This value can be used to
convert the number of PBNs required for a given stream to the number of
timeslots this stream requires in each MTP.

Returns the BW / timeslot value in 20.12 fixed point format.

enum [drm\_dp\_mst\_mode](#c.drm_dp_mst_mode "drm_dp_mst_mode") drm\_dp\_read\_mst\_cap(struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, const u8 dpcd[DP\_RECEIVER\_CAP\_SIZE])
:   Read the sink’s MST mode capability

**Parameters**

`struct drm_dp_aux *aux`
:   The DP AUX channel to use

`const u8 dpcd[DP_RECEIVER_CAP_SIZE]`
:   A cached copy of the DPCD capabilities for this sink

**Return**

[`enum drm_dp_mst_mode`](#c.drm_dp_mst_mode "drm_dp_mst_mode") to indicate MST mode capability

int drm\_dp\_mst\_topology\_mgr\_set\_mst(struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr, bool mst\_state)
:   Set the MST state for a topology manager

**Parameters**

`struct drm_dp_mst_topology_mgr *mgr`
:   manager to set state for

`bool mst_state`
:   true to enable MST on this connector - false to disable.

**Description**

This is called by the driver when it detects an MST capable device plugged
into a DP MST capable port, or when a DP MST capable device is unplugged.

void drm\_dp\_mst\_topology\_queue\_probe(struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr)
:   Queue a topology probe

**Parameters**

`struct drm_dp_mst_topology_mgr *mgr`
:   manager to probe

**Description**

Queue a work to probe the MST topology. Driver’s should call this only to
sync the topology’s HW->SW state after the MST link’s parameters have
changed in a way the state could’ve become out-of-sync. This is the case
for instance when the link rate between the source and first downstream
branch device has switched between UHBR and non-UHBR rates. Except of those
cases - for instance when a sink gets plugged/unplugged to a port - the SW
state will get updated automatically via MST UP message notifications.

void drm\_dp\_mst\_topology\_mgr\_suspend(struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr)
:   suspend the MST manager

**Parameters**

`struct drm_dp_mst_topology_mgr *mgr`
:   manager to suspend

**Description**

This function tells the MST device that we can’t handle UP messages
anymore. This should stop it from sending any since we are suspended.

int drm\_dp\_mst\_topology\_mgr\_resume(struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr, bool sync)
:   resume the MST manager

**Parameters**

`struct drm_dp_mst_topology_mgr *mgr`
:   manager to resume

`bool sync`
:   whether or not to perform topology reprobing synchronously

**Description**

This will fetch DPCD and see if the device is still there,
if it is, it will rewrite the MSTM control bits, and return.

If the device fails this returns -1, and the driver should do
a full MST reprobe, in case we were undocked.

During system resume (where it is assumed that the driver will be calling
[`drm_atomic_helper_resume()`](#c.drm_atomic_helper_resume "drm_atomic_helper_resume")) this function should be called beforehand with
**sync** set to true. In contexts like runtime resume where the driver is not
expected to be calling [`drm_atomic_helper_resume()`](#c.drm_atomic_helper_resume "drm_atomic_helper_resume"), this function should be
called with **sync** set to false in order to avoid deadlocking.

**Return**

-1 if the MST topology was removed while we were suspended, 0
otherwise.

int drm\_dp\_mst\_hpd\_irq\_handle\_event(struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr, const u8 \*esi, u8 \*ack, bool \*handled)
:   MST hotplug IRQ handle MST event

**Parameters**

`struct drm_dp_mst_topology_mgr *mgr`
:   manager to notify irq for.

`const u8 *esi`
:   4 bytes from SINK\_COUNT\_ESI

`u8 *ack`
:   4 bytes used to ack events starting from SINK\_COUNT\_ESI

`bool *handled`
:   whether the hpd interrupt was consumed or not

**Description**

This should be called from the driver when it detects a HPD IRQ,
along with the value of the DEVICE\_SERVICE\_IRQ\_VECTOR\_ESI0. The
topology manager will process the sideband messages received
as indicated in the DEVICE\_SERVICE\_IRQ\_VECTOR\_ESI0 and set the
corresponding flags that Driver has to ack the DP receiver later.

Note that driver shall also call
[`drm_dp_mst_hpd_irq_send_new_request()`](#c.drm_dp_mst_hpd_irq_send_new_request "drm_dp_mst_hpd_irq_send_new_request") if the ‘handled’ is set
after calling this function, to try to kick off a new request in
the queue if the previous message transaction is completed.

See also:
[`drm_dp_mst_hpd_irq_send_new_request()`](#c.drm_dp_mst_hpd_irq_send_new_request "drm_dp_mst_hpd_irq_send_new_request")

void drm\_dp\_mst\_hpd\_irq\_send\_new\_request(struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr)
:   MST hotplug IRQ kick off new request

**Parameters**

`struct drm_dp_mst_topology_mgr *mgr`
:   manager to notify irq for.

**Description**

This should be called from the driver when mst irq event is handled
and acked. Note that new down request should only be sent when
previous message transaction is completed. Source is not supposed to generate
interleaved message transactions.

int drm\_dp\_mst\_detect\_port(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct [drm\_modeset\_acquire\_ctx](drm-kms.html#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx, struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr, struct [drm\_dp\_mst\_port](#c.drm_dp_mst_port "drm_dp_mst_port") \*port)
:   get connection status for an MST port

**Parameters**

`struct drm_connector *connector`
:   DRM connector for this port

`struct drm_modeset_acquire_ctx *ctx`
:   The acquisition context to use for grabbing locks

`struct drm_dp_mst_topology_mgr *mgr`
:   manager for this port

`struct drm_dp_mst_port *port`
:   pointer to a port

**Description**

This returns the current connection state for a port.

const struct drm\_edid \*drm\_dp\_mst\_edid\_read(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr, struct [drm\_dp\_mst\_port](#c.drm_dp_mst_port "drm_dp_mst_port") \*port)
:   get EDID for an MST port

**Parameters**

`struct drm_connector *connector`
:   toplevel connector to get EDID for

`struct drm_dp_mst_topology_mgr *mgr`
:   manager for this port

`struct drm_dp_mst_port *port`
:   unverified pointer to a port.

**Description**

This returns an EDID for the port connected to a connector,
It validates the pointer still exists so the caller doesn’t require a
reference.

struct edid \*drm\_dp\_mst\_get\_edid(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr, struct [drm\_dp\_mst\_port](#c.drm_dp_mst_port "drm_dp_mst_port") \*port)
:   get EDID for an MST port

**Parameters**

`struct drm_connector *connector`
:   toplevel connector to get EDID for

`struct drm_dp_mst_topology_mgr *mgr`
:   manager for this port

`struct drm_dp_mst_port *port`
:   unverified pointer to a port.

**Description**

This function is deprecated; please use [`drm_dp_mst_edid_read()`](#c.drm_dp_mst_edid_read "drm_dp_mst_edid_read") instead.

This returns an EDID for the port connected to a connector,
It validates the pointer still exists so the caller doesn’t require a
reference.

int drm\_dp\_atomic\_find\_time\_slots(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr, struct [drm\_dp\_mst\_port](#c.drm_dp_mst_port "drm_dp_mst_port") \*port, int pbn)
:   Find and add time slots to the state

**Parameters**

`struct drm_atomic_state *state`
:   global atomic state

`struct drm_dp_mst_topology_mgr *mgr`
:   MST topology manager for the port

`struct drm_dp_mst_port *port`
:   port to find time slots for

`int pbn`
:   bandwidth required for the mode in PBN

**Description**

Allocates time slots to **port**, replacing any previous time slot allocations it may
have had. Any atomic drivers which support MST must call this function in
their [`drm_encoder_helper_funcs.atomic_check()`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") callback unconditionally to
change the current time slot allocation for the new state, and ensure the MST
atomic state is added whenever the state of payloads in the topology changes.

Allocations set by this function are not checked against the bandwidth
restraints of **mgr** until the driver calls [`drm_dp_mst_atomic_check()`](#c.drm_dp_mst_atomic_check "drm_dp_mst_atomic_check").

Additionally, it is OK to call this function multiple times on the same
**port** as needed. It is not OK however, to call this function and
[`drm_dp_atomic_release_time_slots()`](#c.drm_dp_atomic_release_time_slots "drm_dp_atomic_release_time_slots") in the same atomic check phase.

See also:
[`drm_dp_atomic_release_time_slots()`](#c.drm_dp_atomic_release_time_slots "drm_dp_atomic_release_time_slots")
[`drm_dp_mst_atomic_check()`](#c.drm_dp_mst_atomic_check "drm_dp_mst_atomic_check")

**Return**

Total slots in the atomic state assigned for this port, or a negative error
code if the port no longer exists

int drm\_dp\_atomic\_release\_time\_slots(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr, struct [drm\_dp\_mst\_port](#c.drm_dp_mst_port "drm_dp_mst_port") \*port)
:   Release allocated time slots

**Parameters**

`struct drm_atomic_state *state`
:   global atomic state

`struct drm_dp_mst_topology_mgr *mgr`
:   MST topology manager for the port

`struct drm_dp_mst_port *port`
:   The port to release the time slots from

**Description**

Releases any time slots that have been allocated to a port in the atomic
state. Any atomic drivers which support MST must call this function
unconditionally in their [`drm_connector_helper_funcs.atomic_check()`](#c.drm_connector_helper_funcs "drm_connector_helper_funcs") callback.
This helper will check whether time slots would be released by the new state and
respond accordingly, along with ensuring the MST state is always added to the
atomic state whenever a new state would modify the state of payloads on the
topology.

It is OK to call this even if **port** has been removed from the system.
Additionally, it is OK to call this function multiple times on the same
**port** as needed. It is not OK however, to call this function and
[`drm_dp_atomic_find_time_slots()`](#c.drm_dp_atomic_find_time_slots "drm_dp_atomic_find_time_slots") on the same **port** in a single atomic check
phase.

See also:
[`drm_dp_atomic_find_time_slots()`](#c.drm_dp_atomic_find_time_slots "drm_dp_atomic_find_time_slots")
[`drm_dp_mst_atomic_check()`](#c.drm_dp_mst_atomic_check "drm_dp_mst_atomic_check")

**Return**

0 on success, negative error code otherwise

int drm\_dp\_mst\_atomic\_setup\_commit(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   setup\_commit hook for MST helpers

**Parameters**

`struct drm_atomic_state *state`
:   global atomic state

**Description**

This function saves all of the [`drm_crtc_commit`](drm-kms.html#c.drm_crtc_commit "drm_crtc_commit") structs in an atomic state that touch any CRTCs
currently assigned to an MST topology. Drivers must call this hook from their
[`drm_mode_config_helper_funcs.atomic_commit_setup`](#c.drm_mode_config_helper_funcs "drm_mode_config_helper_funcs") hook.

**Return**

0 if all CRTC commits were retrieved successfully, negative error code otherwise

void drm\_dp\_mst\_atomic\_wait\_for\_dependencies(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   Wait for all pending commits on MST topologies, prepare new MST state for commit

**Parameters**

`struct drm_atomic_state *state`
:   global atomic state

**Description**

Goes through any MST topologies in this atomic state, and waits for any pending commits which
touched CRTCs that were/are on an MST topology to be programmed to hardware and flipped to before
returning. This is to prevent multiple non-blocking commits affecting an MST topology from racing
with eachother by forcing them to be executed sequentially in situations where the only resources
the modeset objects in these commits share are an MST topology.

This function also prepares the new MST state for commit by performing some state preparation
which can’t be done until this point, such as reading back the final VC start slots (which are
determined at commit-time) from the previous state.

All MST drivers must call this function after calling [`drm_atomic_helper_wait_for_dependencies()`](#c.drm_atomic_helper_wait_for_dependencies "drm_atomic_helper_wait_for_dependencies"),
or whatever their equivalent of that is.

int drm\_dp\_mst\_root\_conn\_atomic\_check(struct [drm\_connector\_state](drm-kms.html#c.drm_connector_state "drm_connector_state") \*new\_conn\_state, struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr)
:   Serialize CRTC commits on MST-capable connectors operating in SST mode

**Parameters**

`struct drm_connector_state *new_conn_state`
:   The new connector state of the [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector")

`struct drm_dp_mst_topology_mgr *mgr`
:   The MST topology manager for the [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector")

**Description**

Since MST uses fake [`drm_encoder`](drm-kms.html#c.drm_encoder "drm_encoder") structs, the generic atomic modesetting code isn’t able to
serialize non-blocking commits happening on the real DP connector of an MST topology switching
into/away from MST mode - as the CRTC on the real DP connector and the CRTCs on the connector’s
MST topology will never share the same [`drm_encoder`](drm-kms.html#c.drm_encoder "drm_encoder").

This function takes care of this serialization issue, by checking a root MST connector’s atomic
state to determine if it is about to have a modeset - and then pulling in the MST topology state
if so, along with adding any relevant CRTCs to [`drm_dp_mst_topology_state.pending_crtc_mask`](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state").

Drivers implementing MST must call this function from the
[`drm_connector_helper_funcs.atomic_check`](#c.drm_connector_helper_funcs "drm_connector_helper_funcs") hook of any physical DP [`drm_connector`](drm-kms.html#c.drm_connector "drm_connector") capable of
driving MST sinks.

**Return**

0 on success, negative error code otherwise

void drm\_dp\_mst\_update\_slots(struct [drm\_dp\_mst\_topology\_state](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state") \*mst\_state, uint8\_t link\_encoding\_cap)
:   updates the slot info depending on the DP ecoding format

**Parameters**

`struct drm_dp_mst_topology_state *mst_state`
:   mst\_state to update

`uint8_t link_encoding_cap`
:   the ecoding format on the link

int drm\_dp\_check\_act\_status(struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr)
:   Polls for ACT handled status.

**Parameters**

`struct drm_dp_mst_topology_mgr *mgr`
:   manager to use

**Description**

Tries waiting for the MST hub to finish updating it’s payload table by
polling for the ACT handled bit for up to 3 seconds (yes-some hubs really
take that long).

**Return**

0 if the ACT was handled in time, negative error code on failure.

int drm\_dp\_calc\_pbn\_mode(int clock, int bpp)
:   Calculate the PBN for a mode.

**Parameters**

`int clock`
:   dot clock

`int bpp`
:   bpp as .4 binary fixed point

**Description**

This uses the formula in the spec to calculate the PBN value for a mode.

void drm\_dp\_mst\_dump\_topology(struct seq\_file \*m, struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr)
:   dump topology to seq file.

**Parameters**

`struct seq_file *m`
:   seq\_file to dump output to

`struct drm_dp_mst_topology_mgr *mgr`
:   manager to dump current topology for.

**Description**

helper to dump MST topology to a seq file for debugfs.

bool drm\_dp\_mst\_port\_downstream\_of\_parent(struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr, struct [drm\_dp\_mst\_port](#c.drm_dp_mst_port "drm_dp_mst_port") \*port, struct [drm\_dp\_mst\_port](#c.drm_dp_mst_port "drm_dp_mst_port") \*parent)
:   check if a port is downstream of a parent port

**Parameters**

`struct drm_dp_mst_topology_mgr *mgr`
:   MST topology manager

`struct drm_dp_mst_port *port`
:   the port being looked up

`struct drm_dp_mst_port *parent`
:   the parent port

**Description**

The function returns `true` if **port** is downstream of **parent**. If **parent** is
`NULL` - denoting the root port - the function returns `true` if **port** is in
**mgr**’s topology.

int drm\_dp\_mst\_add\_affected\_dsc\_crtcs(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr)

**Parameters**

`struct drm_atomic_state *state`
:   Pointer to the new [`struct drm_dp_mst_topology_state`](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state")

`struct drm_dp_mst_topology_mgr *mgr`
:   MST topology manager

**Description**

Whenever there is a change in mst topology
DSC configuration would have to be recalculated
therefore we need to trigger modeset on all affected
CRTCs in that topology

See also:
[`drm_dp_mst_atomic_enable_dsc()`](#c.drm_dp_mst_atomic_enable_dsc "drm_dp_mst_atomic_enable_dsc")

int drm\_dp\_mst\_atomic\_enable\_dsc(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_dp\_mst\_port](#c.drm_dp_mst_port "drm_dp_mst_port") \*port, int pbn, bool enable)
:   Set DSC Enable Flag to On/Off

**Parameters**

`struct drm_atomic_state *state`
:   Pointer to the new drm\_atomic\_state

`struct drm_dp_mst_port *port`
:   Pointer to the affected MST Port

`int pbn`
:   Newly recalculated bw required for link with DSC enabled

`bool enable`
:   Boolean flag to enable or disable DSC on the port

**Description**

This function enables DSC on the given Port
by recalculating its vcpi from pbn provided
and sets dsc\_enable flag to keep track of which
ports have DSC enabled

int drm\_dp\_mst\_atomic\_check\_mgr(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr, struct [drm\_dp\_mst\_topology\_state](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state") \*mst\_state, struct [drm\_dp\_mst\_port](#c.drm_dp_mst_port "drm_dp_mst_port") \*\*failing\_port)
:   Check the atomic state of an MST topology manager

**Parameters**

`struct drm_atomic_state *state`
:   The global atomic state

`struct drm_dp_mst_topology_mgr *mgr`
:   Manager to check

`struct drm_dp_mst_topology_state *mst_state`
:   The MST atomic state for **mgr**

`struct drm_dp_mst_port **failing_port`
:   Returns the port with a BW limitation

**Description**

Checks the given MST manager’s topology state for an atomic update to ensure
that it’s valid. This includes checking whether there’s enough bandwidth to
support the new timeslot allocations in the atomic update.

Any atomic drivers supporting DP MST must make sure to call this or
the [`drm_dp_mst_atomic_check()`](#c.drm_dp_mst_atomic_check "drm_dp_mst_atomic_check") function after checking the rest of their state
in their [`drm_mode_config_funcs.atomic_check()`](drm-kms.html#c.drm_mode_config_funcs "drm_mode_config_funcs") callback.

See also:
[`drm_dp_mst_atomic_check()`](#c.drm_dp_mst_atomic_check "drm_dp_mst_atomic_check")
[`drm_dp_atomic_find_time_slots()`](#c.drm_dp_atomic_find_time_slots "drm_dp_atomic_find_time_slots")
[`drm_dp_atomic_release_time_slots()`](#c.drm_dp_atomic_release_time_slots "drm_dp_atomic_release_time_slots")

> > * The non-root port where a BW limit check failed
> >   with all the ports downstream of **failing\_port** passing
> >   the BW limit check.
> >   The returned port pointer is valid until at least
> >   one payload downstream of it exists.
> > * `NULL` if the BW limit check failed at the root port
> >   with all the ports downstream of the root port passing
> >   the BW limit check.
>
> * `-EINVAL`, if the new state is invalid, because the root port has
>   too many payloads.

**Return**

* 0 if the new state is valid
* `-ENOSPC`, if the new state is invalid, because of BW limitation
  :   **failing\_port** is set to:

int drm\_dp\_mst\_atomic\_check(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   Check that the new state of an MST topology in an atomic update is valid

**Parameters**

`struct drm_atomic_state *state`
:   Pointer to the new [`struct drm_dp_mst_topology_state`](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state")

**Description**

Checks the given topology state for an atomic update to ensure that it’s
valid, calling [`drm_dp_mst_atomic_check_mgr()`](#c.drm_dp_mst_atomic_check_mgr "drm_dp_mst_atomic_check_mgr") for all MST manager in the
atomic state. This includes checking whether there’s enough bandwidth to
support the new timeslot allocations in the atomic update.

Any atomic drivers supporting DP MST must make sure to call this after
checking the rest of their state in their
[`drm_mode_config_funcs.atomic_check()`](drm-kms.html#c.drm_mode_config_funcs "drm_mode_config_funcs") callback.

See also:
[`drm_dp_mst_atomic_check_mgr()`](#c.drm_dp_mst_atomic_check_mgr "drm_dp_mst_atomic_check_mgr")
[`drm_dp_atomic_find_time_slots()`](#c.drm_dp_atomic_find_time_slots "drm_dp_atomic_find_time_slots")
[`drm_dp_atomic_release_time_slots()`](#c.drm_dp_atomic_release_time_slots "drm_dp_atomic_release_time_slots")

**Return**

0 if the new state is valid, negative error code otherwise.

struct [drm\_dp\_mst\_topology\_state](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state") \*drm\_atomic\_get\_mst\_topology\_state(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr)
:   get MST topology state

**Parameters**

`struct drm_atomic_state *state`
:   global atomic state

`struct drm_dp_mst_topology_mgr *mgr`
:   MST topology manager, also the private object in this case

**Description**

This function wraps `drm_atomic_get_priv_obj_state()` passing in the MST atomic
state vtable so that the private object state returned is that of a MST
topology object.

**Return**

The MST topology state or error pointer.

struct [drm\_dp\_mst\_topology\_state](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state") \*drm\_atomic\_get\_old\_mst\_topology\_state(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr)
:   get old MST topology state in atomic state, if any

**Parameters**

`struct drm_atomic_state *state`
:   global atomic state

`struct drm_dp_mst_topology_mgr *mgr`
:   MST topology manager, also the private object in this case

**Description**

This function wraps [`drm_atomic_get_old_private_obj_state()`](drm-kms.html#c.drm_atomic_get_old_private_obj_state "drm_atomic_get_old_private_obj_state") passing in the MST atomic
state vtable so that the private object state returned is that of a MST
topology object.

**Return**

The old MST topology state, or NULL if there’s no topology state for this MST mgr
in the global atomic state

struct [drm\_dp\_mst\_topology\_state](#c.drm_dp_mst_topology_state "drm_dp_mst_topology_state") \*drm\_atomic\_get\_new\_mst\_topology\_state(struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state, struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr)
:   get new MST topology state in atomic state, if any

**Parameters**

`struct drm_atomic_state *state`
:   global atomic state

`struct drm_dp_mst_topology_mgr *mgr`
:   MST topology manager, also the private object in this case

**Description**

This function wraps [`drm_atomic_get_new_private_obj_state()`](drm-kms.html#c.drm_atomic_get_new_private_obj_state "drm_atomic_get_new_private_obj_state") passing in the MST atomic
state vtable so that the private object state returned is that of a MST
topology object.

**Return**

The new MST topology state, or NULL if there’s no topology state for this MST mgr
in the global atomic state

int drm\_dp\_mst\_topology\_mgr\_init(struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr, struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*aux, int max\_dpcd\_transaction\_bytes, int max\_payloads, int conn\_base\_id)
:   initialise a topology manager

**Parameters**

`struct drm_dp_mst_topology_mgr *mgr`
:   manager `struct to` initialise

`struct drm_device *dev`
:   device providing this structure - for i2c addition.

`struct drm_dp_aux *aux`
:   DP helper aux channel to talk to this device

`int max_dpcd_transaction_bytes`
:   hw specific DPCD transaction limit

`int max_payloads`
:   maximum number of payloads this GPU can source

`int conn_base_id`
:   the connector object ID the MST device is connected to.

**Description**

Return 0 for success, or negative error code on failure

void drm\_dp\_mst\_topology\_mgr\_destroy(struct [drm\_dp\_mst\_topology\_mgr](#c.drm_dp_mst_topology_mgr "drm_dp_mst_topology_mgr") \*mgr)
:   destroy topology manager.

**Parameters**

`struct drm_dp_mst_topology_mgr *mgr`
:   manager to destroy

struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*drm\_dp\_mst\_aux\_for\_parent(struct [drm\_dp\_mst\_port](#c.drm_dp_mst_port "drm_dp_mst_port") \*port)
:   Get the AUX device for an MST port’s parent

**Parameters**

`struct drm_dp_mst_port *port`
:   MST port whose parent’s AUX device is returned

**Description**

Return the AUX device for **port**’s parent or NULL if port’s parent is the
root port.

struct [drm\_dp\_aux](#c.drm_dp_aux "drm_dp_aux") \*drm\_dp\_mst\_dsc\_aux\_for\_port(struct [drm\_dp\_mst\_port](#c.drm_dp_mst_port "drm_dp_mst_port") \*port)
:   Find the correct aux for DSC

**Parameters**

`struct drm_dp_mst_port *port`
:   The port to check. A leaf of the MST tree with an attached display.

**Description**

Depending on the situation, DSC may be enabled via the endpoint aux,
the immediately upstream aux, or the connector’s physical aux.

This is both the correct aux to read DSC\_CAPABILITY and the
correct aux to write DSC\_ENABLED.

This operation can be expensive (up to four aux reads), so
the caller should cache the return.

**Return**

NULL if DSC cannot be enabled on this port, otherwise the aux device

### Topology Lifetime Internals

These functions aren’t exported to drivers, but are documented here to help make
the MST topology helpers easier to understand

void drm\_dp\_mst\_get\_mstb\_malloc(struct [drm\_dp\_mst\_branch](#c.drm_dp_mst_branch "drm_dp_mst_branch") \*mstb)
:   Increment the malloc refcount of a branch device

**Parameters**

`struct drm_dp_mst_branch *mstb`
:   The [`struct drm_dp_mst_branch`](#c.drm_dp_mst_branch "drm_dp_mst_branch") to increment the malloc refcount of

**Description**

Increments [`drm_dp_mst_branch.malloc_kref`](#c.drm_dp_mst_branch "drm_dp_mst_branch"). When
[`drm_dp_mst_branch.malloc_kref`](#c.drm_dp_mst_branch "drm_dp_mst_branch") reaches 0, the memory allocation for **mstb**
will be released and **mstb** may no longer be used.

See also: [`drm_dp_mst_put_mstb_malloc()`](#c.drm_dp_mst_put_mstb_malloc "drm_dp_mst_put_mstb_malloc")

void drm\_dp\_mst\_put\_mstb\_malloc(struct [drm\_dp\_mst\_branch](#c.drm_dp_mst_branch "drm_dp_mst_branch") \*mstb)
:   Decrement the malloc refcount of a branch device

**Parameters**

`struct drm_dp_mst_branch *mstb`
:   The [`struct drm_dp_mst_branch`](#c.drm_dp_mst_branch "drm_dp_mst_branch") to decrement the malloc refcount of

**Description**

Decrements [`drm_dp_mst_branch.malloc_kref`](#c.drm_dp_mst_branch "drm_dp_mst_branch"). When
[`drm_dp_mst_branch.malloc_kref`](#c.drm_dp_mst_branch "drm_dp_mst_branch") reaches 0, the memory allocation for **mstb**
will be released and **mstb** may no longer be used.

See also: [`drm_dp_mst_get_mstb_malloc()`](#c.drm_dp_mst_get_mstb_malloc "drm_dp_mst_get_mstb_malloc")

int drm\_dp\_mst\_topology\_try\_get\_mstb(struct [drm\_dp\_mst\_branch](#c.drm_dp_mst_branch "drm_dp_mst_branch") \*mstb)
:   Increment the topology refcount of a branch device unless it’s zero

**Parameters**

`struct drm_dp_mst_branch *mstb`
:   [`struct drm_dp_mst_branch`](#c.drm_dp_mst_branch "drm_dp_mst_branch") to increment the topology refcount of

**Description**

Attempts to grab a topology reference to **mstb**, if it hasn’t yet been
removed from the topology (e.g. [`drm_dp_mst_branch.topology_kref`](#c.drm_dp_mst_branch "drm_dp_mst_branch") has
reached 0). Holding a topology reference implies that a malloc reference
will be held to **mstb** as long as the user holds the topology reference.

Care should be taken to ensure that the user has at least one malloc
reference to **mstb**. If you already have a topology reference to **mstb**, you
should use [`drm_dp_mst_topology_get_mstb()`](#c.drm_dp_mst_topology_get_mstb "drm_dp_mst_topology_get_mstb") instead.

See also:
[`drm_dp_mst_topology_get_mstb()`](#c.drm_dp_mst_topology_get_mstb "drm_dp_mst_topology_get_mstb")
[`drm_dp_mst_topology_put_mstb()`](#c.drm_dp_mst_topology_put_mstb "drm_dp_mst_topology_put_mstb")

**Return**

* 1: A topology reference was grabbed successfully
* 0: **port** is no longer in the topology, no reference was grabbed

void drm\_dp\_mst\_topology\_get\_mstb(struct [drm\_dp\_mst\_branch](#c.drm_dp_mst_branch "drm_dp_mst_branch") \*mstb)
:   Increment the topology refcount of a branch device

**Parameters**

`struct drm_dp_mst_branch *mstb`
:   The [`struct drm_dp_mst_branch`](#c.drm_dp_mst_branch "drm_dp_mst_branch") to increment the topology refcount of

**Description**

Increments [`drm_dp_mst_branch.topology_refcount`](#c.drm_dp_mst_branch "drm_dp_mst_branch") without checking whether or
not it’s already reached 0. This is only valid to use in scenarios where
you are already guaranteed to have at least one active topology reference
to **mstb**. Otherwise, [`drm_dp_mst_topology_try_get_mstb()`](#c.drm_dp_mst_topology_try_get_mstb "drm_dp_mst_topology_try_get_mstb") must be used.

See also:
[`drm_dp_mst_topology_try_get_mstb()`](#c.drm_dp_mst_topology_try_get_mstb "drm_dp_mst_topology_try_get_mstb")
[`drm_dp_mst_topology_put_mstb()`](#c.drm_dp_mst_topology_put_mstb "drm_dp_mst_topology_put_mstb")

void drm\_dp\_mst\_topology\_put\_mstb(struct [drm\_dp\_mst\_branch](#c.drm_dp_mst_branch "drm_dp_mst_branch") \*mstb)
:   release a topology reference to a branch device

**Parameters**

`struct drm_dp_mst_branch *mstb`
:   The [`struct drm_dp_mst_branch`](#c.drm_dp_mst_branch "drm_dp_mst_branch") to release the topology reference from

**Description**

Releases a topology reference from **mstb** by decrementing
[`drm_dp_mst_branch.topology_kref`](#c.drm_dp_mst_branch "drm_dp_mst_branch").

See also:
[`drm_dp_mst_topology_try_get_mstb()`](#c.drm_dp_mst_topology_try_get_mstb "drm_dp_mst_topology_try_get_mstb")
[`drm_dp_mst_topology_get_mstb()`](#c.drm_dp_mst_topology_get_mstb "drm_dp_mst_topology_get_mstb")

int drm\_dp\_mst\_topology\_try\_get\_port(struct [drm\_dp\_mst\_port](#c.drm_dp_mst_port "drm_dp_mst_port") \*port)
:   Increment the topology refcount of a port unless it’s zero

**Parameters**

`struct drm_dp_mst_port *port`
:   [`struct drm_dp_mst_port`](#c.drm_dp_mst_port "drm_dp_mst_port") to increment the topology refcount of

**Description**

Attempts to grab a topology reference to **port**, if it hasn’t yet been
removed from the topology (e.g. [`drm_dp_mst_port.topology_kref`](#c.drm_dp_mst_port "drm_dp_mst_port") has reached
0). Holding a topology reference implies that a malloc reference will be
held to **port** as long as the user holds the topology reference.

Care should be taken to ensure that the user has at least one malloc
reference to **port**. If you already have a topology reference to **port**, you
should use [`drm_dp_mst_topology_get_port()`](#c.drm_dp_mst_topology_get_port "drm_dp_mst_topology_get_port") instead.

See also:
[`drm_dp_mst_topology_get_port()`](#c.drm_dp_mst_topology_get_port "drm_dp_mst_topology_get_port")
[`drm_dp_mst_topology_put_port()`](#c.drm_dp_mst_topology_put_port "drm_dp_mst_topology_put_port")

**Return**

* 1: A topology reference was grabbed successfully
* 0: **port** is no longer in the topology, no reference was grabbed

void drm\_dp\_mst\_topology\_get\_port(struct [drm\_dp\_mst\_port](#c.drm_dp_mst_port "drm_dp_mst_port") \*port)
:   Increment the topology refcount of a port

**Parameters**

`struct drm_dp_mst_port *port`
:   The [`struct drm_dp_mst_port`](#c.drm_dp_mst_port "drm_dp_mst_port") to increment the topology refcount of

**Description**

Increments [`drm_dp_mst_port.topology_refcount`](#c.drm_dp_mst_port "drm_dp_mst_port") without checking whether or
not it’s already reached 0. This is only valid to use in scenarios where
you are already guaranteed to have at least one active topology reference
to **port**. Otherwise, [`drm_dp_mst_topology_try_get_port()`](#c.drm_dp_mst_topology_try_get_port "drm_dp_mst_topology_try_get_port") must be used.

See also:
[`drm_dp_mst_topology_try_get_port()`](#c.drm_dp_mst_topology_try_get_port "drm_dp_mst_topology_try_get_port")
[`drm_dp_mst_topology_put_port()`](#c.drm_dp_mst_topology_put_port "drm_dp_mst_topology_put_port")

void drm\_dp\_mst\_topology\_put\_port(struct [drm\_dp\_mst\_port](#c.drm_dp_mst_port "drm_dp_mst_port") \*port)
:   release a topology reference to a port

**Parameters**

`struct drm_dp_mst_port *port`
:   The [`struct drm_dp_mst_port`](#c.drm_dp_mst_port "drm_dp_mst_port") to release the topology reference from

**Description**

Releases a topology reference from **port** by decrementing
[`drm_dp_mst_port.topology_kref`](#c.drm_dp_mst_port "drm_dp_mst_port").

See also:
[`drm_dp_mst_topology_try_get_port()`](#c.drm_dp_mst_topology_try_get_port "drm_dp_mst_topology_try_get_port")
[`drm_dp_mst_topology_get_port()`](#c.drm_dp_mst_topology_get_port "drm_dp_mst_topology_get_port")

## MIPI DBI Helper Functions Reference

This library provides helpers for MIPI Display Bus Interface (DBI)
compatible display controllers.

Many controllers for tiny lcd displays are MIPI compliant and can use this
library. If a controller uses registers 0x2A and 0x2B to set the area to
update and uses register 0x2C to write to frame memory, it is most likely
MIPI compliant.

Only MIPI Type 1 displays are supported since a full frame memory is needed.

There are 3 MIPI DBI implementation types:

1. Motorola 6800 type parallel bus
2. Intel 8080 type parallel bus
3. SPI type with 3 options:

   1. 9-bit with the Data/Command signal as the ninth bit
   2. Same as above except it’s sent as 16 bits
   3. 8-bit with the Data/Command signal as a separate D/CX pin

Currently mipi\_dbi only supports Type C options 1 and 3 with
[`mipi_dbi_spi_init()`](#c.mipi_dbi_spi_init "mipi_dbi_spi_init").

struct mipi\_dbi
:   MIPI DBI interface

**Definition**:

```
struct mipi_dbi {
    struct mutex cmdlock;
    int (*command)(struct mipi_dbi *dbi, u8 *cmd, u8 *param, size_t num);
    const u8 *read_commands;
    bool swap_bytes;
    struct gpio_desc *reset;
    struct spi_device *spi;
    unsigned int write_memory_bpw;
    struct gpio_desc *dc;
    void *tx_buf9;
    size_t tx_buf9_len;
};
```

**Members**

`cmdlock`
:   Command lock

`command`
:   Bus specific callback executing commands.

`read_commands`
:   Array of read commands terminated by a zero entry.
    :   Reading is disabled if this is NULL.

`swap_bytes`
:   Swap bytes in buffer before transfer

`reset`
:   Optional reset gpio

`spi`
:   SPI device

`write_memory_bpw`
:   Bits per word used on a MIPI\_DCS\_WRITE\_MEMORY\_START transfer

`dc`
:   Optional D/C gpio.

`tx_buf9`
:   Buffer used for Option 1 9-bit conversion

`tx_buf9_len`
:   Size of tx\_buf9.

struct mipi\_dbi\_dev
:   MIPI DBI device

**Definition**:

```
struct mipi_dbi_dev {
    struct drm_device drm;
    struct drm_display_mode mode;
    u32 pixel_format;
    u16 *tx_buf;
    unsigned int rotation;
    unsigned int left_offset;
    unsigned int top_offset;
    struct backlight_device *backlight;
    struct regulator *regulator;
    struct regulator *io_regulator;
    struct mipi_dbi dbi;
    void *driver_private;
};
```

**Members**

`drm`
:   DRM device

`mode`
:   Fixed display mode

`pixel_format`
:   Native pixel format (DRM\_FORMAT\_\*)

`tx_buf`
:   Buffer used for transfer (copy clip rect area)

`rotation`
:   initial rotation in degrees Counter Clock Wise

`left_offset`
:   Horizontal offset of the display relative to the
    :   controller’s driver array

`top_offset`
:   Vertical offset of the display relative to the
    :   controller’s driver array

`backlight`
:   backlight device (optional)

`regulator`
:   power regulator (Vdd) (optional)

`io_regulator`
:   I/O power regulator (Vddi) (optional)

`dbi`
:   MIPI DBI interface

`driver_private`
:   Driver private data.
    :   Necessary for drivers with private data since [`devm_drm_dev_alloc()`](drm-internals.html#c.devm_drm_dev_alloc "devm_drm_dev_alloc")
        can’t allocate structures that embed a structure which then again
        embeds drm\_device.

mipi\_dbi\_command

`mipi_dbi_command (dbi, cmd, seq...)`

> MIPI DCS command with optional parameter(s)

**Parameters**

`dbi`
:   MIPI DBI structure

`cmd`
:   Command

`seq...`
:   Optional parameter(s)

**Description**

Send MIPI DCS command to the controller. Use [`mipi_dbi_command_read()`](#c.mipi_dbi_command_read "mipi_dbi_command_read") for
get/read.

**Return**

Zero on success, negative error code on failure.

int mipi\_dbi\_command\_read(struct [mipi\_dbi](#c.mipi_dbi "mipi_dbi") \*dbi, u8 cmd, u8 \*val)
:   MIPI DCS read command

**Parameters**

`struct mipi_dbi *dbi`
:   MIPI DBI structure

`u8 cmd`
:   Command

`u8 *val`
:   Value read

**Description**

Send MIPI DCS read command to the controller.

**Return**

Zero on success, negative error code on failure.

int mipi\_dbi\_command\_buf(struct [mipi\_dbi](#c.mipi_dbi "mipi_dbi") \*dbi, u8 cmd, u8 \*data, size\_t len)
:   MIPI DCS command with parameter(s) in an array

**Parameters**

`struct mipi_dbi *dbi`
:   MIPI DBI structure

`u8 cmd`
:   Command

`u8 *data`
:   Parameter buffer

`size_t len`
:   Buffer length

**Return**

Zero on success, negative error code on failure.

int mipi\_dbi\_buf\_copy(void \*dst, struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*src, struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, struct [drm\_rect](#c.drm_rect "drm_rect") \*clip, bool swap, struct drm\_format\_conv\_state \*fmtcnv\_state)
:   Copy a framebuffer, transforming it if necessary

**Parameters**

`void *dst`
:   The destination buffer

`struct iosys_map *src`
:   The source buffer

`struct drm_framebuffer *fb`
:   The source framebuffer

`struct drm_rect *clip`
:   Clipping rectangle of the area to be copied

`bool swap`
:   When true, swap MSB/LSB of 16-bit values

`struct drm_format_conv_state *fmtcnv_state`
:   Format-conversion state

**Return**

Zero on success, negative error code on failure.

enum [drm\_mode\_status](drm-kms.html#c.drm_mode_status "drm_mode_status") drm\_mipi\_dbi\_crtc\_helper\_mode\_valid(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, const struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*mode)
:   MIPI DBI mode-valid helper

**Parameters**

`struct drm_crtc *crtc`
:   The CRTC

`const struct drm_display_mode *mode`
:   The mode to test

**Description**

This function validates a given display mode against the MIPI DBI’s hardware
display. Drivers can use this as their struct [`drm_crtc_helper_funcs.mode_valid`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs")
callback.

int drm\_mipi\_dbi\_plane\_helper\_atomic\_check(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   MIPI DBI plane check helper

**Parameters**

`struct drm_plane *plane`
:   Plane to check

`struct drm_atomic_state *state`
:   Atomic state

**Description**

This function performs the default checks on the primary plane
of a MIPI DBI device. Drivers can use this as their
struct [`drm_crtc_helper_funcs.atomic_check`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") callback.

**Return**

0 on success, or a negative errno code otherwise.

void drm\_mipi\_dbi\_plane\_helper\_atomic\_update(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   Display update helper

**Parameters**

`struct drm_plane *plane`
:   Plane

`struct drm_atomic_state *state`
:   Atomic state

**Description**

This function handles framebuffer flushing and vblank events. Drivers can use
this as their struct [`drm_plane_helper_funcs.atomic_update`](#c.drm_plane_helper_funcs "drm_plane_helper_funcs") callback.

int drm\_mipi\_dbi\_crtc\_helper\_atomic\_check(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   MIPI DBI CRTC check helper

**Parameters**

`struct drm_crtc *crtc`
:   CRTC to check

`struct drm_atomic_state *state`
:   Atomic state

**Description**

This function performs the default checks on the CRTC of a MIPI DBI
device and ensures that the primary plane as been set up correctly.
Drivers can use this as their struct [`drm_crtc_helper_funcs.atomic_check`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs")
callback.

**Return**

0 on success, or a negative errno code otherwise.

void drm\_mipi\_dbi\_crtc\_helper\_atomic\_disable(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   MIPI DBI CRTC disable helper

**Parameters**

`struct drm_crtc *crtc`
:   CRTC to disable

`struct drm_atomic_state *state`
:   Atomic state

**Description**

This function disables backlight if present, if not the display memory is
blanked. The regulator is disabled if in use. Drivers can use this as their
struct [`drm_crtc_helper_funcs.atomic_disable`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") callback.

int drm\_mipi\_dbi\_connector\_helper\_get\_modes(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   Duplicates the MIPI DBI mode for the connector

**Parameters**

`struct drm_connector *connector`
:   The connector

**Description**

Sets the connecto rmodes from the MIPI DBI mode. Drivers can use this as their
[`drm_connector_helper_funcs->get_mods`](#c.drm_connector_helper_funcs "drm_connector_helper_funcs") callback. See [`drm_gem_destroy_shadow_plane_state()`](#c.drm_gem_destroy_shadow_plane_state "drm_gem_destroy_shadow_plane_state")
for additional details.

**Return**

The number of created modes.

int drm\_mipi\_dbi\_dev\_init(struct [mipi\_dbi\_dev](#c.mipi_dbi_dev "mipi_dbi_dev") \*dbidev, const struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*mode, u32 format, unsigned int rotation, size\_t tx\_buf\_size)
:   MIPI DBI device initialization

**Parameters**

`struct mipi_dbi_dev *dbidev`
:   MIPI DBI device structure to initialize

`const struct drm_display_mode *mode`
:   Hardware display mode

`u32 format`
:   Hardware color format (DRM\_FORMAT\_\*).

`unsigned int rotation`
:   Initial rotation in degrees Counter Clock Wise

`size_t tx_buf_size`
:   Allocate a transmit buffer of at least this size.

**Description**

Initializes a MIPI-DBI device. The minimum size of the transmit buffer
in **tx\_buf\_size** is optional. Pass 0 to allocate enough memory to transmit
a single scanline of the display.

**Return**

Zero on success, negative error code on failure.

void mipi\_dbi\_hw\_reset(struct [mipi\_dbi](#c.mipi_dbi "mipi_dbi") \*dbi)
:   Hardware reset of controller

**Parameters**

`struct mipi_dbi *dbi`
:   MIPI DBI structure

**Description**

Reset controller if the [`mipi_dbi->reset`](#c.mipi_dbi "mipi_dbi") gpio is set.

bool mipi\_dbi\_display\_is\_on(struct [mipi\_dbi](#c.mipi_dbi "mipi_dbi") \*dbi)
:   Check if display is on

**Parameters**

`struct mipi_dbi *dbi`
:   MIPI DBI structure

**Description**

This function checks the Power Mode register (if readable) to see if
display output is turned on. This can be used to see if the bootloader
has already turned on the display avoiding flicker when the pipeline is
enabled.

**Return**

true if the display can be verified to be on, false otherwise.

int mipi\_dbi\_poweron\_reset(struct [mipi\_dbi\_dev](#c.mipi_dbi_dev "mipi_dbi_dev") \*dbidev)
:   MIPI DBI poweron and reset

**Parameters**

`struct mipi_dbi_dev *dbidev`
:   MIPI DBI device structure

**Description**

This function enables the regulator if used and does a hardware and software
reset.

**Return**

Zero on success, or a negative error code.

int mipi\_dbi\_poweron\_conditional\_reset(struct [mipi\_dbi\_dev](#c.mipi_dbi_dev "mipi_dbi_dev") \*dbidev)
:   MIPI DBI poweron and conditional reset

**Parameters**

`struct mipi_dbi_dev *dbidev`
:   MIPI DBI device structure

**Description**

This function enables the regulator if used and if the display is off, it
does a hardware and software reset. If [`mipi_dbi_display_is_on()`](#c.mipi_dbi_display_is_on "mipi_dbi_display_is_on") determines
that the display is on, no reset is performed.

**Return**

Zero if the controller was reset, 1 if the display was already on, or a
negative error code.

u32 mipi\_dbi\_spi\_cmd\_max\_speed(struct [spi\_device](../driver-api/spi.html#c.spi_device "spi_device") \*spi, size\_t len)
:   get the maximum SPI bus speed

**Parameters**

`struct spi_device *spi`
:   SPI device

`size_t len`
:   The transfer buffer length.

**Description**

Many controllers have a max speed of 10MHz, but can be pushed way beyond
that. Increase reliability by running pixel data at max speed and the rest
at 10MHz, preventing transfer glitches from messing up the init settings.

int mipi\_dbi\_spi\_init(struct [spi\_device](../driver-api/spi.html#c.spi_device "spi_device") \*spi, struct [mipi\_dbi](#c.mipi_dbi "mipi_dbi") \*dbi, struct gpio\_desc \*dc)
:   Initialize MIPI DBI SPI interface

**Parameters**

`struct spi_device *spi`
:   SPI device

`struct mipi_dbi *dbi`
:   MIPI DBI structure to initialize

`struct gpio_desc *dc`
:   D/C gpio (optional)

**Description**

This function sets [`mipi_dbi->command`](#c.mipi_dbi "mipi_dbi"), enables [`mipi_dbi->read_commands`](#c.mipi_dbi "mipi_dbi") for the
usual read commands. It should be followed by a call to `mipi_dbi_dev_init()` or
a driver-specific init.

If **dc** is set, a Type C Option 3 interface is assumed, if not
Type C Option 1.

If the command is `MIPI_DCS_WRITE_MEMORY_START` and the pixel format is RGB565, endianness has
to be taken into account. The MIPI DBI serial interface is big endian and framebuffers are
assumed stored in memory as little endian (`DRM_FORMAT_BIG_ENDIAN` is not supported).

This is how endianness is handled:

Option 1 (D/C as a bit): The buffer is sent on the wire byte by byte so the 16-bit buffer is
:   byteswapped before transfer.

Option 3 (D/C as a gpio): If the SPI controller supports 16 bits per word the buffer can be
:   sent as-is. If not the caller is responsible for swapping the bytes
    before calling [`mipi_dbi_command_buf()`](#c.mipi_dbi_command_buf "mipi_dbi_command_buf") and the buffer is sent 8 bpw.

This handling is optimised for `DRM_FORMAT_RGB565` framebuffers.

If the interface is Option 1 and the SPI controller doesn’t support 9 bits per word,
the buffer is sent as 9x 8-bit words, padded with MIPI DCS no-op commands if necessary.

**Return**

Zero on success, negative error code on failure.

int mipi\_dbi\_spi\_transfer(struct [spi\_device](../driver-api/spi.html#c.spi_device "spi_device") \*spi, u32 speed\_hz, u8 bpw, const void \*buf, size\_t len)
:   SPI transfer helper

**Parameters**

`struct spi_device *spi`
:   SPI device

`u32 speed_hz`
:   Override speed (optional)

`u8 bpw`
:   Bits per word

`const void *buf`
:   Buffer to transfer

`size_t len`
:   Buffer length

**Description**

This SPI transfer helper breaks up the transfer of **buf** into chunks which
the SPI controller driver can handle. The SPI bus must be locked when
calling this.

**Return**

Zero on success, negative error code on failure.

void mipi\_dbi\_debugfs\_init(struct [drm\_minor](drm-internals.html#c.drm_minor "drm_minor") \*minor)
:   Create debugfs entries

**Parameters**

`struct drm_minor *minor`
:   DRM minor

**Description**

This function creates a ‘command’ debugfs file for sending commands to the
controller or getting the read command values.
Drivers can use this as their [`drm_driver->debugfs_init`](drm-internals.html#c.drm_driver "drm_driver") callback.

## MIPI DSI Helper Functions Reference

These functions contain some common logic and helpers to deal with MIPI DSI
peripherals.

Helpers are provided for a number of standard MIPI DSI command as well as a
subset of the MIPI DCS command set.

struct mipi\_dsi\_msg
:   read/write DSI buffer

**Definition**:

```
struct mipi_dsi_msg {
    u8 channel;
    u8 type;
    u16 flags;
    size_t tx_len;
    const void *tx_buf;
    size_t rx_len;
    void *rx_buf;
};
```

**Members**

`channel`
:   virtual channel id

`type`
:   payload data type

`flags`
:   flags controlling this message transmission

`tx_len`
:   length of **tx\_buf**

`tx_buf`
:   data to be written

`rx_len`
:   length of **rx\_buf**

`rx_buf`
:   data to be read, or NULL

struct mipi\_dsi\_packet
:   represents a MIPI DSI packet in protocol format

**Definition**:

```
struct mipi_dsi_packet {
    size_t size;
    u8 header[4];
    size_t payload_length;
    const u8 *payload;
};
```

**Members**

`size`
:   size (in bytes) of the packet

`header`
:   the four bytes that make up the header (Data ID, Word Count or
    Packet Data, and ECC)

`payload_length`
:   number of bytes in the payload

`payload`
:   a pointer to a buffer containing the payload, if any

struct mipi\_dsi\_host\_ops
:   DSI bus operations

**Definition**:

```
struct mipi_dsi_host_ops {
    int (*attach)(struct mipi_dsi_host *host, struct mipi_dsi_device *dsi);
    int (*detach)(struct mipi_dsi_host *host, struct mipi_dsi_device *dsi);
    ssize_t (*transfer)(struct mipi_dsi_host *host, const struct mipi_dsi_msg *msg);
};
```

**Members**

`attach`
:   attach DSI device to DSI host

`detach`
:   detach DSI device from DSI host

`transfer`
:   transmit a DSI packet

**Description**

DSI packets transmitted by .`transfer()` are passed in as mipi\_dsi\_msg
structures. This structure contains information about the type of packet
being transmitted as well as the transmit and receive buffers. When an
error is encountered during transmission, this function will return a
negative error code. On success it shall return the number of bytes
transmitted for write packets or the number of bytes received for read
packets.

Note that typically DSI packet transmission is atomic, so the .`transfer()`
function will seldomly return anything other than the number of bytes
contained in the transmit buffer on success.

Also note that those callbacks can be called no matter the state the
host is in. Drivers that need the underlying device to be powered to
perform these operations will first need to make sure it’s been
properly enabled.

struct mipi\_dsi\_host
:   DSI host device

**Definition**:

```
struct mipi_dsi_host {
    struct device *dev;
    const struct mipi_dsi_host_ops *ops;
    struct list_head list;
};
```

**Members**

`dev`
:   driver model device node for this DSI host

`ops`
:   DSI host operations

`list`
:   list management

struct mipi\_dsi\_device\_info
:   template for creating a mipi\_dsi\_device

**Definition**:

```
struct mipi_dsi_device_info {
    char type[DSI_DEV_NAME_SIZE];
    u32 channel;
    struct device_node *node;
};
```

**Members**

`type`
:   DSI peripheral chip type

`channel`
:   DSI virtual channel assigned to peripheral

`node`
:   pointer to OF device node or NULL

**Description**

This is populated and passed to mipi\_dsi\_device\_new to create a new
DSI device

struct mipi\_dsi\_device
:   DSI peripheral device

**Definition**:

```
struct mipi_dsi_device {
    struct mipi_dsi_host *host;
    struct device dev;
    bool attached;
    char name[DSI_DEV_NAME_SIZE];
    unsigned int channel;
    unsigned int lanes;
    enum mipi_dsi_pixel_format format;
    unsigned long mode_flags;
    unsigned long hs_rate;
    unsigned long lp_rate;
    struct drm_dsc_config *dsc;
};
```

**Members**

`host`
:   DSI host for this peripheral

`dev`
:   driver model device node for this peripheral

`attached`
:   the DSI device has been successfully attached

`name`
:   DSI peripheral chip type

`channel`
:   virtual channel assigned to the peripheral

`lanes`
:   number of active data lanes

`format`
:   pixel format for video mode

`mode_flags`
:   DSI operation mode related flags

`hs_rate`
:   maximum lane frequency for high speed mode in hertz, this should
    be set to the real limits of the hardware, zero is only accepted for
    legacy drivers

`lp_rate`
:   maximum lane frequency for low power mode in hertz, this should
    be set to the real limits of the hardware, zero is only accepted for
    legacy drivers

`dsc`
:   panel/bridge DSC pps payload to be sent

struct mipi\_dsi\_multi\_context
:   Context to call multiple MIPI DSI funcs in a row

**Definition**:

```
struct mipi_dsi_multi_context {
    struct mipi_dsi_device *dsi;
    int accum_err;
};
```

**Members**

`dsi`
:   Pointer to the MIPI DSI device

`accum_err`
:   Storage for the accumulated error over the multiple calls

    Init to 0. If a function encounters an error then the error code
    will be stored here. If you call a function and this points to a
    non-zero value then the function will be a noop. This allows calling
    a function many times in a row and just checking the error at the
    end to see if any of them failed.

int mipi\_dsi\_pixel\_format\_to\_bpp(enum mipi\_dsi\_pixel\_format fmt)
:   obtain the number of bits per pixel for any given pixel format defined by the MIPI DSI specification

**Parameters**

`enum mipi_dsi_pixel_format fmt`
:   MIPI DSI pixel format

**Return**

The number of bits per pixel of the given pixel format.

enum mipi\_dsi\_dcs\_tear\_mode
:   Tearing Effect Output Line mode

**Constants**

`MIPI_DSI_DCS_TEAR_MODE_VBLANK`
:   the TE output line consists of V-Blanking
    information only

`MIPI_DSI_DCS_TEAR_MODE_VHBLANK`
:   the TE output line consists of both
    V-Blanking and H-Blanking information

mipi\_dsi\_generic\_write\_seq\_multi

`mipi_dsi_generic_write_seq_multi (ctx, seq...)`

> transmit data using a generic write packet

**Parameters**

`ctx`
:   Context for multiple DSI transactions

`seq...`
:   buffer containing the payload

**Description**

This macro will print errors for you and error handling is optimized for
callers that call this multiple times in a row.

mipi\_dsi\_generic\_write\_var\_seq\_multi

`mipi_dsi_generic_write_var_seq_multi (ctx, seq...)`

> transmit non-constant data using a generic write packet

**Parameters**

`ctx`
:   Context for multiple DSI transactions

`seq...`
:   buffer containing the payload

**Description**

This macro will print errors for you and error handling is optimized for
callers that call this multiple times in a row.

mipi\_dsi\_dcs\_write\_seq\_multi

`mipi_dsi_dcs_write_seq_multi (ctx, cmd, seq...)`

> transmit a DCS command with payload

**Parameters**

`ctx`
:   Context for multiple DSI transactions

`cmd`
:   Command

`seq...`
:   buffer containing data to be transmitted

**Description**

This macro will print errors for you and error handling is optimized for
callers that call this multiple times in a row.

mipi\_dsi\_dcs\_write\_var\_seq\_multi

`mipi_dsi_dcs_write_var_seq_multi (ctx, cmd, seq...)`

> transmit a DCS command with non-constant payload

**Parameters**

`ctx`
:   Context for multiple DSI transactions

`cmd`
:   Command

`seq...`
:   buffer containing data to be transmitted

**Description**

This macro will print errors for you and error handling is optimized for
callers that call this multiple times in a row.

mipi\_dsi\_dual

`mipi_dsi_dual (_func, _ctx, _dsi1, _dsi2, ...)`

> send the same MIPI DSI command to two interfaces

**Parameters**

`_func`
:   MIPI DSI function to pass context and arguments into

`_ctx`
:   Context for multiple DSI transactions

`_dsi1`
:   First DSI interface to act as recipient of the MIPI DSI command

`_dsi2`
:   Second DSI interface to act as recipient of the MIPI DSI command

`...`
:   Arguments to pass to MIPI DSI function or macro

**Description**

This macro will send the specified MIPI DSI command twice, once per each of
the two interfaces supplied. This is useful for reducing duplication of code
in panel drivers which use two parallel serial interfaces.

Note that the \_func parameter cannot accept a macro such as
[`mipi_dsi_generic_write_multi()`](#c.mipi_dsi_generic_write_multi "mipi_dsi_generic_write_multi") or [`mipi_dsi_dcs_write_buffer_multi()`](#c.mipi_dsi_dcs_write_buffer_multi "mipi_dsi_dcs_write_buffer_multi"). See
[`mipi_dsi_dual_generic_write_multi()`](#c.mipi_dsi_dual_generic_write_multi "mipi_dsi_dual_generic_write_multi") and
[`mipi_dsi_dual_dcs_write_buffer_multi()`](#c.mipi_dsi_dual_dcs_write_buffer_multi "mipi_dsi_dual_dcs_write_buffer_multi") instead.

WARNING: This macro reuses the \_func argument and the optional trailing
arguments twice each, which may cause unintended side effects. For example,
adding the postfix increment ++ operator to one of the arguments to be
passed to \_func will cause the variable to be incremented twice instead of
once and the variable will be its original value + 1 when sent to \_dsi2.

mipi\_dsi\_dual\_generic\_write\_seq\_multi

`mipi_dsi_dual_generic_write_seq_multi (_ctx, _dsi1, _dsi2, _seq...)`

> transmit data using a generic write packet to two dsi interfaces, one after the other

**Parameters**

`_ctx`
:   Context for multiple DSI transactions

`_dsi1`
:   First DSI interface to act as recipient of packet

`_dsi2`
:   Second DSI interface to act as recipient of packet

`_seq...`
:   buffer containing the payload

**Description**

This macro will send the specified generic packet twice, once per each of
the two interfaces supplied. This is useful for reducing duplication of code
in panel drivers which use two parallel serial interfaces.

Note that if an error occurs while transmitting the packet to the first DSI
interface, the packet will not be sent to the second DSI interface.

This macro will print errors for you and error handling is optimized for
callers that call this multiple times in a row.

mipi\_dsi\_dual\_dcs\_write\_seq\_multi

`mipi_dsi_dual_dcs_write_seq_multi (_ctx, _dsi1, _dsi2, _cmd, _seq...)`

> transmit a DCS command with payload to two dsi interfaces, one after the other

**Parameters**

`_ctx`
:   Context for multiple DSI transactions

`_dsi1`
:   First DSI interface to act as recipient of packet

`_dsi2`
:   Second DSI interface to act as recipient of packet

`_cmd`
:   Command

`_seq...`
:   buffer containing the payload

**Description**

This macro will send the specified DCS command with payload twice, once per
each of the two interfaces supplied. This is useful for reducing duplication
of code in panel drivers which use two parallel serial interfaces.

Note that if an error occurs while transmitting the payload to the first DSI
interface, the payload will not be sent to the second DSI interface.

This macro will print errors for you and error handling is optimized for
callers that call this multiple times in a row.

struct mipi\_dsi\_driver
:   DSI driver

**Definition**:

```
struct mipi_dsi_driver {
    struct device_driver driver;
    int(*probe)(struct mipi_dsi_device *dsi);
    void (*remove)(struct mipi_dsi_device *dsi);
    void (*shutdown)(struct mipi_dsi_device *dsi);
};
```

**Members**

`driver`
:   device driver model driver

`probe`
:   callback for device binding

`remove`
:   callback for device unbinding

`shutdown`
:   called at shutdown time to quiesce the device

struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*of\_find\_mipi\_dsi\_device\_by\_node(struct device\_node \*np)
:   find the MIPI DSI device matching a device tree node

**Parameters**

`struct device_node *np`
:   device tree node

**Return**

A pointer to the MIPI DSI device corresponding to **np** or NULL if no
such device exists (or has not been registered yet).

struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*mipi\_dsi\_device\_register\_full(struct [mipi\_dsi\_host](#c.mipi_dsi_host "mipi_dsi_host") \*host, const struct [mipi\_dsi\_device\_info](#c.mipi_dsi_device_info "mipi_dsi_device_info") \*info)
:   create a MIPI DSI device

**Parameters**

`struct mipi_dsi_host *host`
:   DSI host to which this device is connected

`const struct mipi_dsi_device_info *info`
:   pointer to template containing DSI device information

**Description**

Create a MIPI DSI device by using the device information provided by
mipi\_dsi\_device\_info template

**Return**

A pointer to the newly created MIPI DSI device, or, a pointer encoded
with an error

void mipi\_dsi\_device\_unregister(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi)
:   unregister MIPI DSI device

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*devm\_mipi\_dsi\_device\_register\_full(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct [mipi\_dsi\_host](#c.mipi_dsi_host "mipi_dsi_host") \*host, const struct [mipi\_dsi\_device\_info](#c.mipi_dsi_device_info "mipi_dsi_device_info") \*info)
:   create a managed MIPI DSI device

**Parameters**

`struct device *dev`
:   device to tie the MIPI-DSI device lifetime to

`struct mipi_dsi_host *host`
:   DSI host to which this device is connected

`const struct mipi_dsi_device_info *info`
:   pointer to template containing DSI device information

**Description**

Create a MIPI DSI device by using the device information provided by
mipi\_dsi\_device\_info template

This is the managed version of [`mipi_dsi_device_register_full()`](#c.mipi_dsi_device_register_full "mipi_dsi_device_register_full") which
automatically calls [`mipi_dsi_device_unregister()`](#c.mipi_dsi_device_unregister "mipi_dsi_device_unregister") when **dev** is
unbound.

**Return**

A pointer to the newly created MIPI DSI device, or, a pointer encoded
with an error

struct [mipi\_dsi\_host](#c.mipi_dsi_host "mipi_dsi_host") \*of\_find\_mipi\_dsi\_host\_by\_node(struct device\_node \*node)
:   find the MIPI DSI host matching a device tree node

**Parameters**

`struct device_node *node`
:   device tree node

**Return**

A pointer to the MIPI DSI host corresponding to **node** or NULL if no
such device exists (or has not been registered yet).

int mipi\_dsi\_attach(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi)
:   attach a DSI device to its DSI host

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral

int mipi\_dsi\_detach(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi)
:   detach a DSI device from its DSI host

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral

int devm\_mipi\_dsi\_attach(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi)
:   Attach a MIPI-DSI device to its DSI Host

**Parameters**

`struct device *dev`
:   device to tie the MIPI-DSI device attachment lifetime to

`struct mipi_dsi_device *dsi`
:   DSI peripheral

**Description**

This is the managed version of [`mipi_dsi_attach()`](#c.mipi_dsi_attach "mipi_dsi_attach") which automatically
calls [`mipi_dsi_detach()`](#c.mipi_dsi_detach "mipi_dsi_detach") when **dev** is unbound.

**Return**

0 on success, a negative error code on failure.

bool mipi\_dsi\_packet\_format\_is\_short(u8 type)
:   check if a packet is of the short format

**Parameters**

`u8 type`
:   MIPI DSI data type of the packet

**Return**

true if the packet for the given data type is a short packet, false
otherwise.

bool mipi\_dsi\_packet\_format\_is\_long(u8 type)
:   check if a packet is of the long format

**Parameters**

`u8 type`
:   MIPI DSI data type of the packet

**Return**

true if the packet for the given data type is a long packet, false
otherwise.

int mipi\_dsi\_create\_packet(struct [mipi\_dsi\_packet](#c.mipi_dsi_packet "mipi_dsi_packet") \*packet, const struct [mipi\_dsi\_msg](#c.mipi_dsi_msg "mipi_dsi_msg") \*msg)
:   create a packet from a message according to the DSI protocol

**Parameters**

`struct mipi_dsi_packet *packet`
:   pointer to a DSI packet structure

`const struct mipi_dsi_msg *msg`
:   message to translate into a packet

**Return**

0 on success or a negative error code on failure.

int mipi\_dsi\_shutdown\_peripheral(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi)
:   sends a Shutdown Peripheral command

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

**Return**

0 on success or a negative error code on failure.

int mipi\_dsi\_turn\_on\_peripheral(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi)
:   sends a Turn On Peripheral command

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

**Description**

This function is deprecated. Use [`mipi_dsi_turn_on_peripheral_multi()`](#c.mipi_dsi_turn_on_peripheral_multi "mipi_dsi_turn_on_peripheral_multi") instead.

**Return**

0 on success or a negative error code on failure.

int mipi\_dsi\_compression\_mode\_ext(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, bool enable, enum mipi\_dsi\_compression\_algo algo, unsigned int pps\_selector)
:   enable/disable DSC on the peripheral

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`bool enable`
:   Whether to enable or disable the DSC

`enum mipi_dsi_compression_algo algo`
:   Selected compression algorithm

`unsigned int pps_selector`
:   Select PPS from the table of pre-stored or uploaded PPS entries

**Description**

Enable or disable Display Stream Compression on the peripheral.
This function is deprecated. Use [`mipi_dsi_compression_mode_ext_multi()`](#c.mipi_dsi_compression_mode_ext_multi "mipi_dsi_compression_mode_ext_multi") instead.

**Return**

0 on success or a negative error code on failure.

int mipi\_dsi\_compression\_mode(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, bool enable)
:   enable/disable DSC on the peripheral

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`bool enable`
:   Whether to enable or disable the DSC

**Description**

Enable or disable Display Stream Compression on the peripheral using the
default Picture Parameter Set and VESA DSC 1.1 algorithm.

**Return**

0 on success or a negative error code on failure.

int mipi\_dsi\_picture\_parameter\_set(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, const struct [drm\_dsc\_picture\_parameter\_set](#c.drm_dsc_picture_parameter_set "drm_dsc_picture_parameter_set") \*pps)
:   transmit the DSC PPS to the peripheral

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`const struct drm_dsc_picture_parameter_set *pps`
:   VESA DSC 1.1 Picture Parameter Set

**Description**

Transmit the VESA DSC 1.1 Picture Parameter Set to the peripheral.
This function is deprecated. Use [`mipi_dsi_picture_parameter_set_multi()`](#c.mipi_dsi_picture_parameter_set_multi "mipi_dsi_picture_parameter_set_multi") instead.

**Return**

0 on success or a negative error code on failure.

ssize\_t mipi\_dsi\_generic\_write(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, const void \*payload, size\_t size)
:   transmit data using a generic write packet

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`const void *payload`
:   buffer containing the payload

`size_t size`
:   size of payload buffer

**Description**

This function will automatically choose the right data type depending on
the payload length.

**Return**

The number of bytes transmitted on success or a negative error code
on failure.

void mipi\_dsi\_generic\_write\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx, const void \*payload, size\_t size)
:   [`mipi_dsi_generic_write()`](#c.mipi_dsi_generic_write "mipi_dsi_generic_write") w/ accum\_err

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

`const void *payload`
:   buffer containing the payload

`size_t size`
:   size of payload buffer

**Description**

A wrapper around [`mipi_dsi_generic_write()`](#c.mipi_dsi_generic_write "mipi_dsi_generic_write") that deals with errors in a way
that makes it convenient to make several calls in a row.

void mipi\_dsi\_dual\_generic\_write\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx, struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi1, struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi2, const void \*payload, size\_t size)
:   [`mipi_dsi_generic_write_multi()`](#c.mipi_dsi_generic_write_multi "mipi_dsi_generic_write_multi") for two dsi channels, one after the other

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

`struct mipi_dsi_device *dsi1`
:   First dsi channel to write buffer to

`struct mipi_dsi_device *dsi2`
:   Second dsi channel to write buffer to

`const void *payload`
:   Buffer containing the payload

`size_t size`
:   Size of payload buffer

**Description**

A wrapper around [`mipi_dsi_generic_write_multi()`](#c.mipi_dsi_generic_write_multi "mipi_dsi_generic_write_multi") that allows the user to
conveniently write to two dsi channels, one after the other.

ssize\_t mipi\_dsi\_generic\_read(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, const void \*params, size\_t num\_params, void \*data, size\_t size)
:   receive data using a generic read packet

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`const void *params`
:   buffer containing the request parameters

`size_t num_params`
:   number of request parameters

`void *data`
:   buffer in which to return the received data

`size_t size`
:   size of receive buffer

**Description**

This function will automatically choose the right data type depending on
the number of parameters passed in.

**Return**

The number of bytes successfully read or a negative error code on
failure.

u32 drm\_mipi\_dsi\_get\_input\_bus\_fmt(enum mipi\_dsi\_pixel\_format dsi\_format)
:   Get the required MEDIA\_BUS\_FMT\_\* based input pixel format for a given DSI output pixel format

**Parameters**

`enum mipi_dsi_pixel_format dsi_format`
:   pixel format that a DSI host needs to output

**Description**

Various DSI hosts can use this function during their
[`drm_bridge_funcs.atomic_get_input_bus_fmts`](#c.drm_bridge_funcs "drm_bridge_funcs") operation to ascertain
the MEDIA\_BUS\_FMT\_\* pixel format required as input.

**Return**

a 32-bit MEDIA\_BUS\_FMT\_\* value on success or 0 in case of failure.

ssize\_t mipi\_dsi\_dcs\_write\_buffer(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, const void \*data, size\_t len)
:   transmit a DCS command with payload

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`const void *data`
:   buffer containing data to be transmitted

`size_t len`
:   size of transmission buffer

**Description**

This function will automatically choose the right data type depending on
the command payload length.

**Return**

The number of bytes successfully transmitted or a negative error
code on failure.

int mipi\_dsi\_dcs\_write\_buffer\_chatty(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, const void \*data, size\_t len)
:   [`mipi_dsi_dcs_write_buffer()`](#c.mipi_dsi_dcs_write_buffer "mipi_dsi_dcs_write_buffer") w/ an error log

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`const void *data`
:   buffer containing data to be transmitted

`size_t len`
:   size of transmission buffer

**Description**

Like [`mipi_dsi_dcs_write_buffer()`](#c.mipi_dsi_dcs_write_buffer "mipi_dsi_dcs_write_buffer") but includes a `dev_err()`
call for you and returns 0 upon success, not the number of bytes sent.

**Return**

0 on success or a negative error code on failure.

void mipi\_dsi\_dcs\_write\_buffer\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx, const void \*data, size\_t len)
:   [`mipi_dsi_dcs_write_buffer_chatty()`](#c.mipi_dsi_dcs_write_buffer_chatty "mipi_dsi_dcs_write_buffer_chatty") w/ accum\_err

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

`const void *data`
:   buffer containing data to be transmitted

`size_t len`
:   size of transmission buffer

**Description**

Like [`mipi_dsi_dcs_write_buffer_chatty()`](#c.mipi_dsi_dcs_write_buffer_chatty "mipi_dsi_dcs_write_buffer_chatty") but deals with errors in a way that
makes it convenient to make several calls in a row.

void mipi\_dsi\_dual\_dcs\_write\_buffer\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx, struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi1, struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi2, const void \*data, size\_t len)
:   [`mipi_dsi_dcs_write_buffer_multi()`](#c.mipi_dsi_dcs_write_buffer_multi "mipi_dsi_dcs_write_buffer_multi") for two dsi channels, one after the other

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

`struct mipi_dsi_device *dsi1`
:   First dsi channel to write buffer to

`struct mipi_dsi_device *dsi2`
:   Second dsi channel to write buffer to

`const void *data`
:   Buffer containing data to be transmitted

`size_t len`
:   Size of transmission buffer

**Description**

A wrapper around [`mipi_dsi_dcs_write_buffer_multi()`](#c.mipi_dsi_dcs_write_buffer_multi "mipi_dsi_dcs_write_buffer_multi") that allows the user to
conveniently write to two dsi channels, one after the other.

ssize\_t mipi\_dsi\_dcs\_write(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, u8 cmd, const void \*data, size\_t len)
:   send DCS write command

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`u8 cmd`
:   DCS command

`const void *data`
:   buffer containing the command payload

`size_t len`
:   command payload length

**Description**

This function will automatically choose the right data type depending on
the command payload length.

**Return**

The number of bytes successfully transmitted or a negative error
code on failure.

ssize\_t mipi\_dsi\_dcs\_read(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, u8 cmd, void \*data, size\_t len)
:   send DCS read request command

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`u8 cmd`
:   DCS command

`void *data`
:   buffer in which to receive data

`size_t len`
:   size of receive buffer

**Return**

The number of bytes read or a negative error code on failure.

void mipi\_dsi\_dcs\_read\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx, u8 cmd, void \*data, size\_t len)
:   [`mipi_dsi_dcs_read()`](#c.mipi_dsi_dcs_read "mipi_dsi_dcs_read") w/ accum\_err

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

`u8 cmd`
:   DCS command

`void *data`
:   buffer in which to receive data

`size_t len`
:   size of receive buffer

**Description**

Like [`mipi_dsi_dcs_read()`](#c.mipi_dsi_dcs_read "mipi_dsi_dcs_read") but deals with errors in a way that makes it
convenient to make several calls in a row.

int mipi\_dsi\_dcs\_nop(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi)
:   send DCS nop packet

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

**Description**

This function is deprecated. Use [`mipi_dsi_dcs_nop_multi()`](#c.mipi_dsi_dcs_nop_multi "mipi_dsi_dcs_nop_multi") instead.

**Return**

0 on success or a negative error code on failure.

int mipi\_dsi\_dcs\_soft\_reset(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi)
:   perform a software reset of the display module

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

**Description**

This function is deprecated. Use [`mipi_dsi_dcs_soft_reset_multi()`](#c.mipi_dsi_dcs_soft_reset_multi "mipi_dsi_dcs_soft_reset_multi") instead.

**Return**

0 on success or a negative error code on failure.

int mipi\_dsi\_dcs\_get\_power\_mode(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, u8 \*mode)
:   query the display module’s current power mode

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`u8 *mode`
:   return location for the current power mode

**Return**

0 on success or a negative error code on failure.

int mipi\_dsi\_dcs\_get\_pixel\_format(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, u8 \*format)
:   gets the pixel format for the RGB image data used by the interface

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`u8 *format`
:   return location for the pixel format

**Return**

0 on success or a negative error code on failure.

int mipi\_dsi\_dcs\_enter\_sleep\_mode(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi)
:   disable all unnecessary blocks inside the display module except interface communication

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

**Description**

This function is deprecated. Use [`mipi_dsi_dcs_enter_sleep_mode_multi()`](#c.mipi_dsi_dcs_enter_sleep_mode_multi "mipi_dsi_dcs_enter_sleep_mode_multi") instead.

**Return**

0 on success or a negative error code on failure.

int mipi\_dsi\_dcs\_exit\_sleep\_mode(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi)
:   enable all blocks inside the display module

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

**Description**

This function is deprecated. Use [`mipi_dsi_dcs_exit_sleep_mode_multi()`](#c.mipi_dsi_dcs_exit_sleep_mode_multi "mipi_dsi_dcs_exit_sleep_mode_multi") instead.

**Return**

0 on success or a negative error code on failure.

int mipi\_dsi\_dcs\_set\_display\_off(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi)
:   stop displaying the image data on the display device

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

**Description**

This function is deprecated. Use [`mipi_dsi_dcs_set_display_off_multi()`](#c.mipi_dsi_dcs_set_display_off_multi "mipi_dsi_dcs_set_display_off_multi") instead.

**Return**

0 on success or a negative error code on failure.

int mipi\_dsi\_dcs\_set\_display\_on(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi)
:   start displaying the image data on the display device

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

**Description**

This function is deprecated. Use [`mipi_dsi_dcs_set_display_on_multi()`](#c.mipi_dsi_dcs_set_display_on_multi "mipi_dsi_dcs_set_display_on_multi") instead.

**Return**

0 on success or a negative error code on failure

int mipi\_dsi\_dcs\_set\_column\_address(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, u16 start, u16 end)
:   define the column extent of the frame memory accessed by the host processor

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`u16 start`
:   first column of frame memory

`u16 end`
:   last column of frame memory

**Description**

This function is deprecated. Use [`mipi_dsi_dcs_set_column_address_multi()`](#c.mipi_dsi_dcs_set_column_address_multi "mipi_dsi_dcs_set_column_address_multi")
instead.

**Return**

0 on success or a negative error code on failure.

int mipi\_dsi\_dcs\_set\_page\_address(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, u16 start, u16 end)
:   define the page extent of the frame memory accessed by the host processor

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`u16 start`
:   first page of frame memory

`u16 end`
:   last page of frame memory

**Description**

This function is deprecated. Use [`mipi_dsi_dcs_set_page_address_multi()`](#c.mipi_dsi_dcs_set_page_address_multi "mipi_dsi_dcs_set_page_address_multi")
instead.

**Return**

0 on success or a negative error code on failure.

int mipi\_dsi\_dcs\_set\_tear\_on(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, enum [mipi\_dsi\_dcs\_tear\_mode](#c.mipi_dsi_dcs_tear_mode "mipi_dsi_dcs_tear_mode") mode)
:   turn on the display module’s Tearing Effect output signal on the TE signal line.

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`enum mipi_dsi_dcs_tear_mode mode`
:   the Tearing Effect Output Line mode

**Description**

This function is deprecated. Use [`mipi_dsi_dcs_set_tear_on_multi()`](#c.mipi_dsi_dcs_set_tear_on_multi "mipi_dsi_dcs_set_tear_on_multi") instead.

**Return**

0 on success or a negative error code on failure

int mipi\_dsi\_dcs\_set\_pixel\_format(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, u8 format)
:   sets the pixel format for the RGB image data used by the interface

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`u8 format`
:   pixel format

**Description**

This function is deprecated. Use [`mipi_dsi_dcs_set_pixel_format_multi()`](#c.mipi_dsi_dcs_set_pixel_format_multi "mipi_dsi_dcs_set_pixel_format_multi")
instead.

**Return**

0 on success or a negative error code on failure.

int mipi\_dsi\_dcs\_set\_tear\_scanline(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, u16 scanline)
:   set the scanline to use as trigger for the Tearing Effect output signal of the display module

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`u16 scanline`
:   scanline to use as trigger

**Description**

This function is deprecated. Use [`mipi_dsi_dcs_set_tear_scanline_multi()`](#c.mipi_dsi_dcs_set_tear_scanline_multi "mipi_dsi_dcs_set_tear_scanline_multi")
instead.

**Return**

0 on success or a negative error code on failure

int mipi\_dsi\_dcs\_set\_display\_brightness(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, u16 brightness)
:   sets the brightness value of the display

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`u16 brightness`
:   brightness value

**Description**

This function is deprecated. Use [`mipi_dsi_dcs_set_display_brightness_multi()`](#c.mipi_dsi_dcs_set_display_brightness_multi "mipi_dsi_dcs_set_display_brightness_multi")
instead.

**Return**

0 on success or a negative error code on failure.

int mipi\_dsi\_dcs\_get\_display\_brightness(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, u16 \*brightness)
:   gets the current brightness value of the display

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`u16 *brightness`
:   brightness value

**Return**

0 on success or a negative error code on failure.

int mipi\_dsi\_dcs\_set\_display\_brightness\_large(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, u16 brightness)
:   sets the 16-bit brightness value of the display

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`u16 brightness`
:   brightness value

**Return**

0 on success or a negative error code on failure.

int mipi\_dsi\_dcs\_get\_display\_brightness\_large(struct [mipi\_dsi\_device](#c.mipi_dsi_device "mipi_dsi_device") \*dsi, u16 \*brightness)
:   gets the current 16-bit brightness value of the display

**Parameters**

`struct mipi_dsi_device *dsi`
:   DSI peripheral device

`u16 *brightness`
:   brightness value

**Return**

0 on success or a negative error code on failure.

void mipi\_dsi\_picture\_parameter\_set\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx, const struct [drm\_dsc\_picture\_parameter\_set](#c.drm_dsc_picture_parameter_set "drm_dsc_picture_parameter_set") \*pps)
:   transmit the DSC PPS to the peripheral

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

`const struct drm_dsc_picture_parameter_set *pps`
:   VESA DSC 1.1 Picture Parameter Set

**Description**

Like [`mipi_dsi_picture_parameter_set()`](#c.mipi_dsi_picture_parameter_set "mipi_dsi_picture_parameter_set") but deals with errors in a way that
makes it convenient to make several calls in a row.

void mipi\_dsi\_compression\_mode\_ext\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx, bool enable, enum mipi\_dsi\_compression\_algo algo, unsigned int pps\_selector)
:   enable/disable DSC on the peripheral

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

`bool enable`
:   Whether to enable or disable the DSC

`enum mipi_dsi_compression_algo algo`
:   Selected compression algorithm

`unsigned int pps_selector`
:   Select PPS from the table of pre-stored or uploaded PPS entries

**Description**

Like [`mipi_dsi_compression_mode_ext()`](#c.mipi_dsi_compression_mode_ext "mipi_dsi_compression_mode_ext") but deals with errors in a way that
makes it convenient to make several calls in a row.

void mipi\_dsi\_compression\_mode\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx, bool enable)
:   enable/disable DSC on the peripheral

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

`bool enable`
:   Whether to enable or disable the DSC

**Description**

Enable or disable Display Stream Compression on the peripheral using the
default Picture Parameter Set and VESA DSC 1.1 algorithm.

void mipi\_dsi\_dcs\_nop\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx)
:   send DCS NOP packet

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

**Description**

Like [`mipi_dsi_dcs_nop()`](#c.mipi_dsi_dcs_nop "mipi_dsi_dcs_nop") but deals with errors in a way that
makes it convenient to make several calls in a row.

void mipi\_dsi\_dcs\_enter\_sleep\_mode\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx)
:   send DCS ENTER\_SLEEP\_MODE packet

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

**Description**

Like [`mipi_dsi_dcs_enter_sleep_mode()`](#c.mipi_dsi_dcs_enter_sleep_mode "mipi_dsi_dcs_enter_sleep_mode") but deals with errors in a way that
makes it convenient to make several calls in a row.

void mipi\_dsi\_dcs\_exit\_sleep\_mode\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx)
:   send DCS EXIT\_SLEEP\_MODE packet

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

**Description**

Like [`mipi_dsi_dcs_exit_sleep_mode()`](#c.mipi_dsi_dcs_exit_sleep_mode "mipi_dsi_dcs_exit_sleep_mode") but deals with errors in a way that
makes it convenient to make several calls in a row.

void mipi\_dsi\_dcs\_set\_display\_off\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx)
:   send DCS SET\_DISPLAY\_OFF packet

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

**Description**

Like [`mipi_dsi_dcs_set_display_off()`](#c.mipi_dsi_dcs_set_display_off "mipi_dsi_dcs_set_display_off") but deals with errors in a way that
makes it convenient to make several calls in a row.

void mipi\_dsi\_dcs\_set\_display\_on\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx)
:   send DCS SET\_DISPLAY\_ON packet

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

**Description**

Like [`mipi_dsi_dcs_set_display_on()`](#c.mipi_dsi_dcs_set_display_on "mipi_dsi_dcs_set_display_on") but deals with errors in a way that
makes it convenient to make several calls in a row.

void mipi\_dsi\_dcs\_set\_tear\_on\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx, enum [mipi\_dsi\_dcs\_tear\_mode](#c.mipi_dsi_dcs_tear_mode "mipi_dsi_dcs_tear_mode") mode)
:   send DCS SET\_TEAR\_ON packet

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

`enum mipi_dsi_dcs_tear_mode mode`
:   the Tearing Effect Output Line mode

**Description**

Like [`mipi_dsi_dcs_set_tear_on()`](#c.mipi_dsi_dcs_set_tear_on "mipi_dsi_dcs_set_tear_on") but deals with errors in a way that
makes it convenient to make several calls in a row.

void mipi\_dsi\_turn\_on\_peripheral\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx)
:   sends a Turn On Peripheral command

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

**Description**

Like [`mipi_dsi_turn_on_peripheral()`](#c.mipi_dsi_turn_on_peripheral "mipi_dsi_turn_on_peripheral") but deals with errors in a way that
makes it convenient to make several calls in a row.

void mipi\_dsi\_dcs\_set\_tear\_off\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx)
:   turn off the display module’s Tearing Effect output signal on the TE signal line

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

void mipi\_dsi\_dcs\_soft\_reset\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx)
:   perform a software reset of the display module

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

**Description**

Like [`mipi_dsi_dcs_soft_reset()`](#c.mipi_dsi_dcs_soft_reset "mipi_dsi_dcs_soft_reset") but deals with errors in a way that
makes it convenient to make several calls in a row.

void mipi\_dsi\_dcs\_set\_display\_brightness\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx, u16 brightness)
:   sets the brightness value of the display

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

`u16 brightness`
:   brightness value

**Description**

Like [`mipi_dsi_dcs_set_display_brightness()`](#c.mipi_dsi_dcs_set_display_brightness "mipi_dsi_dcs_set_display_brightness") but deals with errors in a way that
makes it convenient to make several calls in a row.

void mipi\_dsi\_dcs\_set\_pixel\_format\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx, u8 format)
:   sets the pixel format for the RGB image data used by the interface

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

`u8 format`
:   pixel format

**Description**

Like [`mipi_dsi_dcs_set_pixel_format()`](#c.mipi_dsi_dcs_set_pixel_format "mipi_dsi_dcs_set_pixel_format") but deals with errors in a way that
makes it convenient to make several calls in a row.

void mipi\_dsi\_dcs\_set\_column\_address\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx, u16 start, u16 end)
:   define the column extent of the frame memory accessed by the host processor

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

`u16 start`
:   first column of frame memory

`u16 end`
:   last column of frame memory

**Description**

Like [`mipi_dsi_dcs_set_column_address()`](#c.mipi_dsi_dcs_set_column_address "mipi_dsi_dcs_set_column_address") but deals with errors in a way that
makes it convenient to make several calls in a row.

void mipi\_dsi\_dcs\_set\_page\_address\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx, u16 start, u16 end)
:   define the page extent of the frame memory accessed by the host processor

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

`u16 start`
:   first page of frame memory

`u16 end`
:   last page of frame memory

**Description**

Like [`mipi_dsi_dcs_set_page_address()`](#c.mipi_dsi_dcs_set_page_address "mipi_dsi_dcs_set_page_address") but deals with errors in a way that
makes it convenient to make several calls in a row.

void mipi\_dsi\_dcs\_set\_tear\_scanline\_multi(struct [mipi\_dsi\_multi\_context](#c.mipi_dsi_multi_context "mipi_dsi_multi_context") \*ctx, u16 scanline)
:   set the scanline to use as trigger for the Tearing Effect output signal of the display module

**Parameters**

`struct mipi_dsi_multi_context *ctx`
:   Context for multiple DSI transactions

`u16 scanline`
:   scanline to use as trigger

**Description**

Like [`mipi_dsi_dcs_set_tear_scanline()`](#c.mipi_dsi_dcs_set_tear_scanline "mipi_dsi_dcs_set_tear_scanline") but deals with errors in a way that
makes it convenient to make several calls in a row.

int mipi\_dsi\_driver\_register\_full(struct [mipi\_dsi\_driver](#c.mipi_dsi_driver "mipi_dsi_driver") \*drv, struct module \*owner)
:   register a driver for DSI devices

**Parameters**

`struct mipi_dsi_driver *drv`
:   DSI driver structure

`struct module *owner`
:   owner module

**Return**

0 on success or a negative error code on failure.

void mipi\_dsi\_driver\_unregister(struct [mipi\_dsi\_driver](#c.mipi_dsi_driver "mipi_dsi_driver") \*drv)
:   unregister a driver for DSI devices

**Parameters**

`struct mipi_dsi_driver *drv`
:   DSI driver structure

**Return**

0 on success or a negative error code on failure.

## Display Stream Compression Helper Functions Reference

VESA specification for DP 1.4 adds a new feature called Display Stream
Compression (DSC) used to compress the pixel bits before sending it on
DP/eDP/MIPI DSI interface. DSC is required to be enabled so that the existing
display interfaces can support high resolutions at higher frames rates uisng
the maximum available link capacity of these interfaces.

These functions contain some common logic and helpers to deal with VESA
Display Stream Compression standard required for DSC on Display Port/eDP or
MIPI display interfaces.

struct drm\_dsc\_rc\_range\_parameters
:   DSC Rate Control range parameters

**Definition**:

```
struct drm_dsc_rc_range_parameters {
    u8 range_min_qp;
    u8 range_max_qp;
    u8 range_bpg_offset;
};
```

**Members**

`range_min_qp`
:   Min Quantization Parameters allowed for this range

`range_max_qp`
:   Max Quantization Parameters allowed for this range

`range_bpg_offset`
:   Bits/group offset to apply to target for this group

**Description**

This defines different rate control parameters used by the DSC engine
to compress the frame.

struct drm\_dsc\_config
:   Parameters required to configure DSC

**Definition**:

```
struct drm_dsc_config {
    u8 line_buf_depth;
    u8 bits_per_component;
    bool convert_rgb;
    u8 slice_count;
    u16 slice_width;
    u16 slice_height;
    bool simple_422;
    u16 pic_width;
    u16 pic_height;
    u8 rc_tgt_offset_high;
    u8 rc_tgt_offset_low;
    u16 bits_per_pixel;
    u8 rc_edge_factor;
    u8 rc_quant_incr_limit1;
    u8 rc_quant_incr_limit0;
    u16 initial_xmit_delay;
    u16 initial_dec_delay;
    bool block_pred_enable;
    u8 first_line_bpg_offset;
    u16 initial_offset;
    u16 rc_buf_thresh[DSC_NUM_BUF_RANGES - 1];
    struct drm_dsc_rc_range_parameters rc_range_params[DSC_NUM_BUF_RANGES];
    u16 rc_model_size;
    u8 flatness_min_qp;
    u8 flatness_max_qp;
    u8 initial_scale_value;
    u16 scale_decrement_interval;
    u16 scale_increment_interval;
    u16 nfl_bpg_offset;
    u16 slice_bpg_offset;
    u16 final_offset;
    bool vbr_enable;
    u8 mux_word_size;
    u16 slice_chunk_size;
    u16 rc_bits;
    u8 dsc_version_minor;
    u8 dsc_version_major;
    bool native_422;
    bool native_420;
    u8 second_line_bpg_offset;
    u16 nsl_bpg_offset;
    u16 second_line_offset_adj;
};
```

**Members**

`line_buf_depth`
:   Bits per component for previous reconstructed line buffer

`bits_per_component`
:   Bits per component to code (8/10/12)

`convert_rgb`
:   Flag to indicate if RGB - YCoCg conversion is needed
    True if RGB input, False if YCoCg input

`slice_count`
:   Number fo slices per line used by the DSC encoder

`slice_width`
:   Width of each slice in pixels

`slice_height`
:   Slice height in pixels

`simple_422`
:   True if simple 4\_2\_2 mode is enabled else False

`pic_width`
:   Width of the input display frame in pixels

`pic_height`
:   Vertical height of the input display frame

`rc_tgt_offset_high`
:   Offset to bits/group used by RC to determine QP adjustment

`rc_tgt_offset_low`
:   Offset to bits/group used by RC to determine QP adjustment

`bits_per_pixel`
:   Target bits per pixel with 4 fractional bits, bits\_per\_pixel << 4

`rc_edge_factor`
:   Factor to determine if an edge is present based on the bits produced

`rc_quant_incr_limit1`
:   Slow down incrementing once the range reaches this value

`rc_quant_incr_limit0`
:   Slow down incrementing once the range reaches this value

`initial_xmit_delay`
:   Number of pixels to delay the initial transmission

`initial_dec_delay`
:   Initial decoder delay, number of pixel times that the decoder
    accumulates data in its rate buffer before starting to decode
    and output pixels.

`block_pred_enable`
:   True if block prediction is used to code any groups within the
    picture. False if BP not used

`first_line_bpg_offset`
:   Number of additional bits allocated for each group on the first
    line of slice.

`initial_offset`
:   Value to use for RC model offset at slice start

`rc_buf_thresh`
:   Thresholds defining each of the buffer ranges

`rc_range_params`
:   Parameters for each of the RC ranges defined in
    [`struct drm_dsc_rc_range_parameters`](#c.drm_dsc_rc_range_parameters "drm_dsc_rc_range_parameters")

`rc_model_size`
:   Total size of RC model

`flatness_min_qp`
:   Minimum QP where flatness information is sent

`flatness_max_qp`
:   Maximum QP where flatness information is sent

`initial_scale_value`
:   Initial value for the scale factor

`scale_decrement_interval`
:   Specifies number of group times between decrementing the scale factor
    at beginning of a slice.

`scale_increment_interval`
:   Number of group times between incrementing the scale factor value
    used at the beginning of a slice.

`nfl_bpg_offset`
:   Non first line BPG offset to be used

`slice_bpg_offset`
:   BPG offset used to enforce slice bit

`final_offset`
:   Final RC linear transformation offset value

`vbr_enable`
:   True if VBR mode is enabled, false if disabled

`mux_word_size`
:   Mux word size (in bits) for SSM mode

`slice_chunk_size`
:   The (max) size in bytes of the “chunks” that are used in slice
    multiplexing.

`rc_bits`
:   Rate control buffer size in bits

`dsc_version_minor`
:   DSC minor version

`dsc_version_major`
:   DSC major version

`native_422`
:   True if Native 4:2:2 supported, else false

`native_420`
:   True if Native 4:2:0 supported else false.

`second_line_bpg_offset`
:   Additional bits/grp for seconnd line of slice for native 4:2:0

`nsl_bpg_offset`
:   Num of bits deallocated for each grp that is not in second line of
    slice

`second_line_offset_adj`
:   Offset adjustment for second line in Native 4:2:0 mode

**Description**

Driver populates this structure with all the parameters required
to configure the display stream compression on the source.

struct drm\_dsc\_picture\_parameter\_set
:   Represents 128 bytes of Picture Parameter Set

**Definition**:

```
struct drm_dsc_picture_parameter_set {
    u8 dsc_version;
    u8 pps_identifier;
    u8 pps_reserved;
    u8 pps_3;
    u8 pps_4;
    u8 bits_per_pixel_low;
    __be16 pic_height;
    __be16 pic_width;
    __be16 slice_height;
    __be16 slice_width;
    __be16 chunk_size;
    u8 initial_xmit_delay_high;
    u8 initial_xmit_delay_low;
    __be16 initial_dec_delay;
    u8 pps20_reserved;
    u8 initial_scale_value;
    __be16 scale_increment_interval;
    u8 scale_decrement_interval_high;
    u8 scale_decrement_interval_low;
    u8 pps26_reserved;
    u8 first_line_bpg_offset;
    __be16 nfl_bpg_offset;
    __be16 slice_bpg_offset;
    __be16 initial_offset;
    __be16 final_offset;
    u8 flatness_min_qp;
    u8 flatness_max_qp;
    __be16 rc_model_size;
    u8 rc_edge_factor;
    u8 rc_quant_incr_limit0;
    u8 rc_quant_incr_limit1;
    u8 rc_tgt_offset;
    u8 rc_buf_thresh[DSC_NUM_BUF_RANGES - 1];
    __be16 rc_range_parameters[DSC_NUM_BUF_RANGES];
    u8 native_422_420;
    u8 second_line_bpg_offset;
    __be16 nsl_bpg_offset;
    __be16 second_line_offset_adj;
    u32 pps_long_94_reserved;
    u32 pps_long_98_reserved;
    u32 pps_long_102_reserved;
    u32 pps_long_106_reserved;
    u32 pps_long_110_reserved;
    u32 pps_long_114_reserved;
    u32 pps_long_118_reserved;
    u32 pps_long_122_reserved;
    __be16 pps_short_126_reserved;
};
```

**Members**

`dsc_version`
:   PPS0[3:0] - dsc\_version\_minor: Contains Minor version of DSC
    PPS0[7:4] - dsc\_version\_major: Contains major version of DSC

`pps_identifier`
:   PPS1[7:0] - Application specific identifier that can be
    used to differentiate between different PPS tables.

`pps_reserved`
:   PPS2[7:0]- RESERVED Byte

`pps_3`
:   PPS3[3:0] - linebuf\_depth: Contains linebuffer bit depth used to
    generate the bitstream. (0x0 - 16 bits for DSC 1.2, 0x8 - 8 bits,
    0xA - 10 bits, 0xB - 11 bits, 0xC - 12 bits, 0xD - 13 bits,
    0xE - 14 bits for DSC1.2, 0xF - 14 bits for DSC 1.2.
    PPS3[7:4] - bits\_per\_component: Bits per component for the original
    pixels of the encoded picture.
    0x0 = 16bpc (allowed only when dsc\_version\_minor = 0x2)
    0x8 = 8bpc, 0xA = 10bpc, 0xC = 12bpc, 0xE = 14bpc (also
    allowed only when dsc\_minor\_version = 0x2)

`pps_4`
:   PPS4[1:0] -These are the most significant 2 bits of
    compressed BPP bits\_per\_pixel[9:0] syntax element.
    PPS4[2] - vbr\_enable: 0 = VBR disabled, 1 = VBR enabled
    PPS4[3] - simple\_422: Indicates if decoder drops samples to
    reconstruct the 4:2:2 picture.
    PPS4[4] - Convert\_rgb: Indicates if DSC color space conversion is
    active.
    PPS4[5] - blobk\_pred\_enable: Indicates if BP is used to code any
    groups in picture
    PPS4[7:6] - Reseved bits

`bits_per_pixel_low`
:   PPS5[7:0] - This indicates the lower significant 8 bits of
    the compressed BPP bits\_per\_pixel[9:0] element.

`pic_height`
:   PPS6[7:0], PPS7[7:0] -pic\_height: Specifies the number of pixel rows
    within the raster.

`pic_width`
:   PPS8[7:0], PPS9[7:0] - pic\_width: Number of pixel columns within
    the raster.

`slice_height`
:   PPS10[7:0], PPS11[7:0] - Slice height in units of pixels.

`slice_width`
:   PPS12[7:0], PPS13[7:0] - Slice width in terms of pixels.

`chunk_size`
:   PPS14[7:0], PPS15[7:0] - Size in units of bytes of the chunks
    that are used for slice multiplexing.

`initial_xmit_delay_high`
:   PPS16[1:0] - Most Significant two bits of initial transmission delay.
    It specifies the number of pixel times that the encoder waits before
    transmitting data from its rate buffer.
    PPS16[7:2] - Reserved

`initial_xmit_delay_low`
:   PPS17[7:0] - Least significant 8 bits of initial transmission delay.

`initial_dec_delay`
:   PPS18[7:0], PPS19[7:0] - Initial decoding delay which is the number
    of pixel times that the decoder accumulates data in its rate buffer
    before starting to decode and output pixels.

`pps20_reserved`
:   PPS20[7:0] - Reserved

`initial_scale_value`
:   PPS21[5:0] - Initial rcXformScale factor used at beginning
    of a slice.
    PPS21[7:6] - Reserved

`scale_increment_interval`
:   PPS22[7:0], PPS23[7:0] - Number of group times between incrementing
    the rcXformScale factor at end of a slice.

`scale_decrement_interval_high`
:   PPS24[3:0] - Higher 4 bits indicating number of group times between
    decrementing the rcXformScale factor at beginning of a slice.
    PPS24[7:4] - Reserved

`scale_decrement_interval_low`
:   PPS25[7:0] - Lower 8 bits of scale decrement interval

`pps26_reserved`
:   PPS26[7:0]

`first_line_bpg_offset`
:   PPS27[4:0] - Number of additional bits that are allocated
    for each group on first line of a slice.
    PPS27[7:5] - Reserved

`nfl_bpg_offset`
:   PPS28[7:0], PPS29[7:0] - Number of bits including frac bits
    deallocated for each group for groups after the first line of slice.

`slice_bpg_offset`
:   PPS30, PPS31[7:0] - Number of bits that are deallocated for each
    group to enforce the slice constraint.

`initial_offset`
:   PPS32,33[7:0] - Initial value for rcXformOffset

`final_offset`
:   PPS34,35[7:0] - Maximum end-of-slice value for rcXformOffset

`flatness_min_qp`
:   PPS36[4:0] - Minimum QP at which flatness is signaled and
    flatness QP adjustment is made.
    PPS36[7:5] - Reserved

`flatness_max_qp`
:   PPS37[4:0] - Max QP at which flatness is signalled and
    the flatness adjustment is made.
    PPS37[7:5] - Reserved

`rc_model_size`
:   PPS38,39[7:0] - Number of bits within RC Model.

`rc_edge_factor`
:   PPS40[3:0] - Ratio of current activity vs, previous
    activity to determine presence of edge.
    PPS40[7:4] - Reserved

`rc_quant_incr_limit0`
:   PPS41[4:0] - QP threshold used in short term RC
    PPS41[7:5] - Reserved

`rc_quant_incr_limit1`
:   PPS42[4:0] - QP threshold used in short term RC
    PPS42[7:5] - Reserved

`rc_tgt_offset`
:   PPS43[3:0] - Lower end of the variability range around the target
    bits per group that is allowed by short term RC.
    PPS43[7:4]- Upper end of the variability range around the target
    bits per group that i allowed by short term rc.

`rc_buf_thresh`
:   PPS44[7:0] - PPS57[7:0] - Specifies the thresholds in RC model for
    the 15 ranges defined by 14 thresholds.

`rc_range_parameters`
:   PPS58[7:0] - PPS87[7:0]
    Parameters that correspond to each of the 15 ranges.

`native_422_420`
:   PPS88[0] - 0 = Native 4:2:2 not used
    1 = Native 4:2:2 used
    PPS88[1] - 0 = Native 4:2:0 not use
    1 = Native 4:2:0 used
    PPS88[7:2] - Reserved 6 bits

`second_line_bpg_offset`
:   PPS89[4:0] - Additional bits/group budget for the
    second line of a slice in Native 4:2:0 mode.
    Set to 0 if DSC minor version is 1 or native420 is 0.
    PPS89[7:5] - Reserved

`nsl_bpg_offset`
:   PPS90[7:0], PPS91[7:0] - Number of bits that are deallocated
    for each group that is not in the second line of a slice.

`second_line_offset_adj`
:   PPS92[7:0], PPS93[7:0] - Used as offset adjustment for the second
    line in Native 4:2:0 mode.

`pps_long_94_reserved`
:   PPS 94, 95, 96, 97 - Reserved

`pps_long_98_reserved`
:   PPS 98, 99, 100, 101 - Reserved

`pps_long_102_reserved`
:   PPS 102, 103, 104, 105 - Reserved

`pps_long_106_reserved`
:   PPS 106, 107, 108, 109 - reserved

`pps_long_110_reserved`
:   PPS 110, 111, 112, 113 - reserved

`pps_long_114_reserved`
:   PPS 114 - 117 - reserved

`pps_long_118_reserved`
:   PPS 118 - 121 - reserved

`pps_long_122_reserved`
:   PPS 122- 125 - reserved

`pps_short_126_reserved`
:   PPS 126, 127 - reserved

**Description**

The VESA DSC standard defines picture parameter set (PPS) which display
stream compression encoders must communicate to decoders.
The PPS is encapsulated in 128 bytes (PPS 0 through PPS 127). The fields in
this structure are as per Table 4.1 in Vesa DSC specification v1.1/v1.2.
The PPS fields that span over more than a byte should be stored in Big Endian
format.

struct drm\_dsc\_pps\_infoframe
:   DSC infoframe carrying the Picture Parameter Set Metadata

**Definition**:

```
struct drm_dsc_pps_infoframe {
    struct dp_sdp_header pps_header;
    struct drm_dsc_picture_parameter_set pps_payload;
};
```

**Members**

`pps_header`
:   Header for PPS as per DP SDP header format of type
    [`struct dp_sdp_header`](#c.dp_sdp_header "dp_sdp_header")

`pps_payload`
:   PPS payload fields as per DSC specification Table 4-1
    as represented in [`struct drm_dsc_picture_parameter_set`](#c.drm_dsc_picture_parameter_set "drm_dsc_picture_parameter_set")

**Description**

This structure represents the DSC PPS infoframe required to send the Picture
Parameter Set metadata required before enabling VESA Display Stream
Compression. This is based on the DP Secondary Data Packet structure and
comprises of SDP Header as defined [`struct dp_sdp_header`](#c.dp_sdp_header "dp_sdp_header") in drm\_dp\_helper.h
and PPS payload defined in [`struct drm_dsc_picture_parameter_set`](#c.drm_dsc_picture_parameter_set "drm_dsc_picture_parameter_set").

void drm\_dsc\_dp\_pps\_header\_init(struct [dp\_sdp\_header](#c.dp_sdp_header "dp_sdp_header") \*pps\_header)
:   Initializes the PPS Header for DisplayPort as per the DP 1.4 spec.

**Parameters**

`struct dp_sdp_header *pps_header`
:   Secondary data packet header for DSC Picture
    Parameter Set as defined in [`struct dp_sdp_header`](#c.dp_sdp_header "dp_sdp_header")

**Description**

DP 1.4 spec defines the secondary data packet for sending the
picture parameter infoframes from the source to the sink.
This function populates the SDP header defined in
[`struct dp_sdp_header`](#c.dp_sdp_header "dp_sdp_header").

int drm\_dsc\_dp\_rc\_buffer\_size(u8 rc\_buffer\_block\_size, u8 rc\_buffer\_size)
:   get rc buffer size in bytes

**Parameters**

`u8 rc_buffer_block_size`
:   block size code, according to DPCD offset 62h

`u8 rc_buffer_size`
:   number of blocks - 1, according to DPCD offset 63h

**Return**

buffer size in bytes, or 0 on invalid input

void drm\_dsc\_pps\_payload\_pack(struct [drm\_dsc\_picture\_parameter\_set](#c.drm_dsc_picture_parameter_set "drm_dsc_picture_parameter_set") \*pps\_payload, const struct [drm\_dsc\_config](#c.drm_dsc_config "drm_dsc_config") \*dsc\_cfg)
:   Populates the DSC PPS

**Parameters**

`struct drm_dsc_picture_parameter_set *pps_payload`
:   Bitwise struct for DSC Picture Parameter Set. This is defined
    by [`struct drm_dsc_picture_parameter_set`](#c.drm_dsc_picture_parameter_set "drm_dsc_picture_parameter_set")

`const struct drm_dsc_config *dsc_cfg`
:   DSC Configuration data filled by driver as defined by
    [`struct drm_dsc_config`](#c.drm_dsc_config "drm_dsc_config")

**Description**

DSC source device sends a picture parameter set (PPS) containing the
information required by the sink to decode the compressed frame. Driver
populates the DSC PPS `struct using` the DSC configuration parameters in
the order expected by the DSC Display Sink device. For the DSC, the sink
device expects the PPS payload in big endian format for fields
that span more than 1 byte.

void drm\_dsc\_set\_const\_params(struct [drm\_dsc\_config](#c.drm_dsc_config "drm_dsc_config") \*vdsc\_cfg)
:   Set DSC parameters considered typically constant across operation modes

**Parameters**

`struct drm_dsc_config *vdsc_cfg`
:   DSC Configuration data partially filled by driver

void drm\_dsc\_set\_rc\_buf\_thresh(struct [drm\_dsc\_config](#c.drm_dsc_config "drm_dsc_config") \*vdsc\_cfg)
:   Set thresholds for the RC model in accordance with the DSC 1.2 specification.

**Parameters**

`struct drm_dsc_config *vdsc_cfg`
:   DSC Configuration data partially filled by driver

int drm\_dsc\_setup\_rc\_params(struct [drm\_dsc\_config](#c.drm_dsc_config "drm_dsc_config") \*vdsc\_cfg, enum drm\_dsc\_params\_type type)
:   Set parameters and limits for RC model in accordance with the DSC 1.1 or 1.2 specification and DSC C Model Required bits\_per\_pixel and bits\_per\_component to be set before calling this function.

**Parameters**

`struct drm_dsc_config *vdsc_cfg`
:   DSC Configuration data partially filled by driver

`enum drm_dsc_params_type type`
:   operating mode and standard to follow

**Return**

0 or -error code in case of an error

int drm\_dsc\_compute\_rc\_parameters(struct [drm\_dsc\_config](#c.drm_dsc_config "drm_dsc_config") \*vdsc\_cfg)
:   Write rate control parameters to the dsc configuration defined in [`struct drm_dsc_config`](#c.drm_dsc_config "drm_dsc_config") in accordance with the DSC 1.2 specification. Some configuration fields must be present beforehand.

**Parameters**

`struct drm_dsc_config *vdsc_cfg`
:   DSC Configuration data partially filled by driver

u32 drm\_dsc\_get\_bpp\_int(const struct [drm\_dsc\_config](#c.drm_dsc_config "drm_dsc_config") \*vdsc\_cfg)
:   Get integer bits per pixel value for the given DRM DSC config

**Parameters**

`const struct drm_dsc_config *vdsc_cfg`
:   Pointer to DRM DSC config struct

**Return**

Integer BPP value

u8 drm\_dsc\_initial\_scale\_value(const struct [drm\_dsc\_config](#c.drm_dsc_config "drm_dsc_config") \*dsc)
:   Calculate the initial scale value for the given DSC config

**Parameters**

`const struct drm_dsc_config *dsc`
:   Pointer to DRM DSC config struct

**Return**

Calculated initial scale value

u32 drm\_dsc\_flatness\_det\_thresh(const struct [drm\_dsc\_config](#c.drm_dsc_config "drm_dsc_config") \*dsc)
:   Calculate the flatness\_det\_thresh for the given DSC config

**Parameters**

`const struct drm_dsc_config *dsc`
:   Pointer to DRM DSC config struct

**Return**

Calculated flatness det thresh value

void drm\_dsc\_dump\_config(struct [drm\_printer](drm-internals.html#c.drm_printer "drm_printer") \*p, int indent, const struct [drm\_dsc\_config](#c.drm_dsc_config "drm_dsc_config") \*cfg)
:   Dump the provided DSC configuration

**Parameters**

`struct drm_printer *p`
:   The printer used for output

`int indent`
:   Tab indentation level (max 5)

`const struct drm_dsc_config *cfg`
:   DSC configuration to print

**Description**

Print the provided DSC configuration in **cfg**.

## Output Probing Helper Functions Reference

This library provides some helper code for output probing. It provides an
implementation of the core [`drm_connector_funcs.fill_modes`](drm-kms.html#c.drm_connector_funcs "drm_connector_funcs") interface with
[`drm_helper_probe_single_connector_modes()`](#c.drm_helper_probe_single_connector_modes "drm_helper_probe_single_connector_modes").

It also provides support for polling connectors with a work item and for
generic hotplug interrupt handling where the driver doesn’t or cannot keep
track of a per-connector hpd interrupt.

This helper library can be used independently of the modeset helper library.
Drivers can also overwrite different parts e.g. use their own hotplug
handling code to avoid probing unrelated outputs.

The probe helpers share the function table structures with other display
helper libraries. See [`struct drm_connector_helper_funcs`](#c.drm_connector_helper_funcs "drm_connector_helper_funcs") for the details.

void drm\_kms\_helper\_poll\_enable(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   re-enable output polling.

**Parameters**

`struct drm_device *dev`
:   drm\_device

**Description**

This function re-enables the output polling work, after it has been
temporarily disabled using [`drm_kms_helper_poll_disable()`](#c.drm_kms_helper_poll_disable "drm_kms_helper_poll_disable"), for example over
suspend/resume.

Drivers can call this helper from their device resume implementation. It is
not an error to call this even when output polling isn’t enabled.

If device polling was never initialized before, this call will trigger a
warning and return.

Note that calls to enable and disable polling must be strictly ordered, which
is automatically the case when they’re only call from suspend/resume
callbacks.

void drm\_kms\_helper\_poll\_reschedule(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   reschedule the output polling work

**Parameters**

`struct drm_device *dev`
:   drm\_device

**Description**

This function reschedules the output polling work, after polling for a
connector has been enabled.

Drivers must call this helper after enabling polling for a connector by
setting `DRM_CONNECTOR_POLL_CONNECT` / `DRM_CONNECTOR_POLL_DISCONNECT` flags
in drm\_connector::polled. Note that after disabling polling by clearing these
flags for a connector will stop the output polling work automatically if
the polling is disabled for all other connectors as well.

The function can be called only after polling has been enabled by calling
[`drm_kms_helper_poll_init()`](#c.drm_kms_helper_poll_init "drm_kms_helper_poll_init") / [`drm_kms_helper_poll_enable()`](#c.drm_kms_helper_poll_enable "drm_kms_helper_poll_enable").

int drm\_helper\_probe\_detect(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct [drm\_modeset\_acquire\_ctx](drm-kms.html#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx, bool force)
:   probe connector status

**Parameters**

`struct drm_connector *connector`
:   connector to probe

`struct drm_modeset_acquire_ctx *ctx`
:   acquire\_ctx, or NULL to let this function handle locking.

`bool force`
:   Whether destructive probe operations should be performed.

**Description**

This function calls the detect callbacks of the connector.
This function returns [`drm_connector_status`](drm-kms.html#c.drm_connector_status "drm_connector_status"), or
if **ctx** is set, it might also return -EDEADLK.

int drm\_helper\_probe\_single\_connector\_modes(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, uint32\_t maxX, uint32\_t maxY)
:   get complete set of display modes

**Parameters**

`struct drm_connector *connector`
:   connector to probe

`uint32_t maxX`
:   max width for modes

`uint32_t maxY`
:   max height for modes

**Description**

Based on the helper callbacks implemented by **connector** in struct
[`drm_connector_helper_funcs`](#c.drm_connector_helper_funcs "drm_connector_helper_funcs") try to detect all valid modes. Modes will first
be added to the connector’s probed\_modes list, then culled (based on validity
and the **maxX**, **maxY** parameters) and put into the normal modes list.

Intended to be used as a generic implementation of the
[`drm_connector_funcs.fill_modes()`](drm-kms.html#c.drm_connector_funcs "drm_connector_funcs") vfunc for drivers that use the CRTC helpers
for output mode filtering and detection.

The basic procedure is as follows

1. All modes currently on the connector’s modes list are marked as stale
2. New modes are added to the connector’s probed\_modes list with
   [`drm_mode_probed_add()`](drm-kms.html#c.drm_mode_probed_add "drm_mode_probed_add"). New modes start their life with status as OK.
   Modes are added from a single source using the following priority order.

   * [`drm_connector_helper_funcs.get_modes`](#c.drm_connector_helper_funcs "drm_connector_helper_funcs") vfunc
   * if the connector status is connector\_status\_connected, standard
     VESA DMT modes up to 1024x768 are automatically added
     ([`drm_add_modes_noedid()`](#c.drm_add_modes_noedid "drm_add_modes_noedid"))

   Finally modes specified via the kernel command line (video=...) are
   added in addition to what the earlier probes produced
   (`drm_helper_probe_add_cmdline_mode()`). These modes are generated
   using the VESA GTF/CVT formulas.
3. Modes are moved from the probed\_modes list to the modes list. Potential
   duplicates are merged together (see [`drm_connector_list_update()`](drm-kms.html#c.drm_connector_list_update "drm_connector_list_update")).
   After this step the probed\_modes list will be empty again.
4. Any non-stale mode on the modes list then undergoes validation

   * `drm_mode_validate_basic()` performs basic sanity checks
   * [`drm_mode_validate_size()`](drm-kms.html#c.drm_mode_validate_size "drm_mode_validate_size") filters out modes larger than **maxX** and **maxY**
     (if specified)
   * `drm_mode_validate_flag()` checks the modes against basic connector
     capabilities (interlace\_allowed,doublescan\_allowed,stereo\_allowed)
   * the optional [`drm_connector_helper_funcs.mode_valid`](#c.drm_connector_helper_funcs "drm_connector_helper_funcs") or
     [`drm_connector_helper_funcs.mode_valid_ctx`](#c.drm_connector_helper_funcs "drm_connector_helper_funcs") helpers can perform driver
     and/or sink specific checks
   * the optional [`drm_crtc_helper_funcs.mode_valid`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs"),
     [`drm_bridge_funcs.mode_valid`](#c.drm_bridge_funcs "drm_bridge_funcs") and [`drm_encoder_helper_funcs.mode_valid`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs")
     helpers can perform driver and/or source specific checks which are also
     enforced by the modeset/atomic helpers
5. Any mode whose status is not OK is pruned from the connector’s modes list,
   accompanied by a debug message indicating the reason for the mode’s
   rejection (see [`drm_mode_prune_invalid()`](drm-kms.html#c.drm_mode_prune_invalid "drm_mode_prune_invalid")).

**Return**

The number of modes found on **connector**.

void drm\_kms\_helper\_hotplug\_event(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   fire off KMS hotplug events

**Parameters**

`struct drm_device *dev`
:   drm\_device whose connector state changed

**Description**

This function fires off the uevent for userspace and also calls the
client hotplug function, which is most commonly used to inform the fbdev
emulation code and allow it to update the fbcon output configuration.

Drivers should call this from their hotplug handling code when a change is
detected. Note that this function does not do any output detection of its
own, like [`drm_helper_hpd_irq_event()`](#c.drm_helper_hpd_irq_event "drm_helper_hpd_irq_event") does - this is assumed to be done by the
driver already.

This function must be called from process context with no mode
setting locks held.

If only a single connector has changed, consider calling
[`drm_kms_helper_connector_hotplug_event()`](#c.drm_kms_helper_connector_hotplug_event "drm_kms_helper_connector_hotplug_event") instead.

void drm\_kms\_helper\_connector\_hotplug\_event(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   fire off a KMS connector hotplug event

**Parameters**

`struct drm_connector *connector`
:   drm\_connector which has changed

**Description**

This is the same as [`drm_kms_helper_hotplug_event()`](#c.drm_kms_helper_hotplug_event "drm_kms_helper_hotplug_event"), except it fires a more
fine-grained uevent for a single connector.

bool drm\_kms\_helper\_is\_poll\_worker(void)
:   is `current` task an output poll worker?

**Parameters**

`void`
:   no arguments

**Description**

Determine if `current` task is an output poll worker. This can be used
to select distinct code paths for output polling versus other contexts.

One use case is to avoid a deadlock between the output poll worker and
the autosuspend worker wherein the latter waits for polling to finish
upon calling [`drm_kms_helper_poll_disable()`](#c.drm_kms_helper_poll_disable "drm_kms_helper_poll_disable"), while the former waits for
runtime suspend to finish upon calling `pm_runtime_get_sync()` in a
connector ->detect hook.

void drm\_kms\_helper\_poll\_disable(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   disable output polling

**Parameters**

`struct drm_device *dev`
:   drm\_device

**Description**

This function disables the output polling work.

Drivers can call this helper from their device suspend implementation. It is
not an error to call this even when output polling isn’t enabled or already
disabled. Polling is re-enabled by calling [`drm_kms_helper_poll_enable()`](#c.drm_kms_helper_poll_enable "drm_kms_helper_poll_enable").

If however, the polling was never initialized, this call will trigger a
warning and return.

Note that calls to enable and disable polling must be strictly ordered, which
is automatically the case when they’re only call from suspend/resume
callbacks.

void drm\_kms\_helper\_poll\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   initialize and enable output polling

**Parameters**

`struct drm_device *dev`
:   drm\_device

**Description**

This function initializes and then also enables output polling support for
**dev**. Drivers which do not have reliable hotplug support in hardware can use
this helper infrastructure to regularly poll such connectors for changes in
their connection state.

Drivers can control which connectors are polled by setting the
DRM\_CONNECTOR\_POLL\_CONNECT and DRM\_CONNECTOR\_POLL\_DISCONNECT flags. On
connectors where probing live outputs can result in visual distortion drivers
should not set the DRM\_CONNECTOR\_POLL\_DISCONNECT flag to avoid this.
Connectors which have no flag or only DRM\_CONNECTOR\_POLL\_HPD set are
completely ignored by the polling logic.

Note that a connector can be both polled and probed from the hotplug handler,
in case the hotplug interrupt is known to be unreliable.

void drm\_kms\_helper\_poll\_fini(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   disable output polling and clean it up

**Parameters**

`struct drm_device *dev`
:   drm\_device

void drmm\_kms\_helper\_poll\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   initialize and enable output polling

**Parameters**

`struct drm_device *dev`
:   drm\_device

**Description**

This function initializes and then also enables output polling support for
**dev** similar to [`drm_kms_helper_poll_init()`](#c.drm_kms_helper_poll_init "drm_kms_helper_poll_init"). Polling will automatically be
cleaned up when the DRM device goes away.

See [`drm_kms_helper_poll_init()`](#c.drm_kms_helper_poll_init "drm_kms_helper_poll_init") for more information.

bool drm\_connector\_helper\_hpd\_irq\_event(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   hotplug processing

**Parameters**

`struct drm_connector *connector`
:   drm\_connector

**Description**

Drivers can use this helper function to run a detect cycle on a connector
which has the DRM\_CONNECTOR\_POLL\_HPD flag set in its `polled` member.

This helper function is useful for drivers which can track hotplug
interrupts for a single connector. Drivers that want to send a
hotplug event for all connectors or can’t track hotplug interrupts
per connector need to use [`drm_helper_hpd_irq_event()`](#c.drm_helper_hpd_irq_event "drm_helper_hpd_irq_event").

This function must be called from process context with no mode
setting locks held.

Note that a connector can be both polled and probed from the hotplug
handler, in case the hotplug interrupt is known to be unreliable.

**Return**

A boolean indicating whether the connector status changed or not

bool drm\_helper\_hpd\_irq\_event(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   hotplug processing

**Parameters**

`struct drm_device *dev`
:   drm\_device

**Description**

Drivers can use this helper function to run a detect cycle on all connectors
which have the DRM\_CONNECTOR\_POLL\_HPD flag set in their `polled` member. All
other connectors are ignored, which is useful to avoid reprobing fixed
panels.

This helper function is useful for drivers which can’t or don’t track hotplug
interrupts for each connector.

Drivers which support hotplug interrupts for each connector individually and
which have a more fine-grained detect logic can use
[`drm_connector_helper_hpd_irq_event()`](#c.drm_connector_helper_hpd_irq_event "drm_connector_helper_hpd_irq_event"). Alternatively, they should bypass this
code and directly call [`drm_kms_helper_hotplug_event()`](#c.drm_kms_helper_hotplug_event "drm_kms_helper_hotplug_event") in case the connector
state changed.

This function must be called from process context with no mode
setting locks held.

Note that a connector can be both polled and probed from the hotplug handler,
in case the hotplug interrupt is known to be unreliable.

**Return**

A boolean indicating whether the connector status changed or not

enum [drm\_mode\_status](drm-kms.html#c.drm_mode_status "drm_mode_status") drm\_crtc\_helper\_mode\_valid\_fixed(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, const struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*mode, const struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*fixed\_mode)
:   Validates a display mode

**Parameters**

`struct drm_crtc *crtc`
:   the crtc

`const struct drm_display_mode *mode`
:   the mode to validate

`const struct drm_display_mode *fixed_mode`
:   the display hardware’s mode

**Return**

MODE\_OK on success, or another mode-status code otherwise.

int drm\_connector\_helper\_get\_modes\_fixed(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, const struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*fixed\_mode)
:   Duplicates a display mode for a connector

**Parameters**

`struct drm_connector *connector`
:   the connector

`const struct drm_display_mode *fixed_mode`
:   the display hardware’s mode

**Description**

This function duplicates a display modes for a connector. Drivers for hardware
that only supports a single fixed mode can use this function in their connector’s
get\_modes helper.

**Return**

The number of created modes.

int drm\_connector\_helper\_get\_modes(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   Read EDID and update connector.

**Parameters**

`struct drm_connector *connector`
:   The connector

**Description**

Read the EDID using [`drm_edid_read()`](#c.drm_edid_read "drm_edid_read") (which requires that connector->ddc is
set), and update the connector using the EDID.

This can be used as the “default” connector helper .`get_modes()` hook if the
driver does not need any special processing. This is sets the example what
custom .`get_modes()` hooks should do regarding EDID read and connector update.

**Return**

Number of modes.

int drm\_connector\_helper\_tv\_get\_modes(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   Fills the modes availables to a TV connector

**Parameters**

`struct drm_connector *connector`
:   The connector

**Description**

Fills the available modes for a TV connector based on the supported
TV modes, and the default mode expressed by the kernel command line.

This can be used as the default TV connector helper .`get_modes()` hook
if the driver does not need any special processing.

**Return**

The number of modes added to the connector.

int drm\_connector\_helper\_detect\_from\_ddc(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct [drm\_modeset\_acquire\_ctx](drm-kms.html#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx, bool force)
:   Read EDID and detect connector status.

**Parameters**

`struct drm_connector *connector`
:   The connector

`struct drm_modeset_acquire_ctx *ctx`
:   Acquire context

`bool force`
:   Perform screen-destructive operations, if necessary

**Description**

Detects the connector status by reading the EDID using [`drm_probe_ddc()`](#c.drm_probe_ddc "drm_probe_ddc"),
which requires connector->ddc to be set. Returns connector\_status\_connected
on success or connector\_status\_disconnected on failure.

**Return**

The connector status as defined by [`enum drm_connector_status`](drm-kms.html#c.drm_connector_status "drm_connector_status").

## EDID Helper Functions Reference

const char \*drm\_edid\_decode\_mfg\_id(u16 mfg\_id, char vend[4])
:   Decode the manufacturer ID

**Parameters**

`u16 mfg_id`
:   The manufacturer ID

`char vend[4]`
:   A 4-byte buffer to store the 3-letter vendor string plus a ‘0’
    termination

drm\_edid\_encode\_panel\_id

`drm_edid_encode_panel_id (vend_chr_0, vend_chr_1, vend_chr_2, product_id)`

> Encode an ID for matching against [`drm_edid_get_panel_id()`](#c.drm_edid_get_panel_id "drm_edid_get_panel_id")

**Parameters**

`vend_chr_0`
:   First character of the vendor string.

`vend_chr_1`
:   Second character of the vendor string.

`vend_chr_2`
:   Third character of the vendor string.

`product_id`
:   The 16-bit product ID.

**Description**

This is a macro so that it can be calculated at compile time and used
as an initializer.

For instance:
:   drm\_edid\_encode\_panel\_id(‘B’, ‘O’, ‘E’, 0x2d08) => 0x09e52d08

**Return**

a 32-bit ID per panel.

void drm\_edid\_decode\_panel\_id(u32 panel\_id, char vend[4], u16 \*product\_id)
:   Decode a panel ID from [`drm_edid_encode_panel_id()`](#c.drm_edid_encode_panel_id "drm_edid_encode_panel_id")

**Parameters**

`u32 panel_id`
:   The panel ID to decode.

`char vend[4]`
:   A 4-byte buffer to store the 3-letter vendor string plus a ‘0’
    termination

`u16 *product_id`
:   The product ID will be returned here.

**Description**

For instance, after:
:   drm\_edid\_decode\_panel\_id(0x09e52d08, vend, `product_id`)

These will be true:
:   vend[0] = ‘B’
    vend[1] = ‘O’
    vend[2] = ‘E’
    vend[3] = ‘0’
    product\_id = 0x2d08

int drm\_edid\_header\_is\_valid(const void \*\_edid)
:   sanity check the header of the base EDID block

**Parameters**

`const void *_edid`
:   pointer to raw base EDID block

**Description**

Sanity check the header of the base EDID block.

**Return**

8 if the header is perfect, down to 0 if it’s totally wrong.

bool drm\_edid\_is\_valid(struct [edid](#c.drm_edid_is_valid "edid") \*edid)
:   sanity check EDID data

**Parameters**

`struct edid *edid`
:   EDID data

**Description**

Sanity-check an entire EDID record (including extensions)

**Return**

True if the EDID data is valid, false otherwise.

bool drm\_edid\_valid(const struct [drm\_edid](#c.drm_edid_valid "drm_edid") \*drm\_edid)
:   sanity check EDID data

**Parameters**

`const struct drm_edid *drm_edid`
:   EDID data

**Description**

Sanity check an EDID. Cross check block count against allocated size and
checksum the blocks.

**Return**

True if the EDID data is valid, false otherwise.

int drm\_edid\_override\_connector\_update(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   add modes from override/firmware EDID

**Parameters**

`struct drm_connector *connector`
:   connector we’re probing

**Description**

Add modes from the override/firmware EDID, if available. Only to be used from
[`drm_helper_probe_single_connector_modes()`](#c.drm_helper_probe_single_connector_modes "drm_helper_probe_single_connector_modes") as a fallback for when DDC probe
failed during [`drm_get_edid()`](#c.drm_get_edid "drm_get_edid") and caused the override/firmware EDID to be
skipped.

**Return**

The number of modes added or 0 if we couldn’t find any.

const struct edid \*drm\_edid\_raw(const struct [drm\_edid](#c.drm_edid_raw "drm_edid") \*drm\_edid)
:   Get a pointer to the raw EDID data.

**Parameters**

`const struct drm_edid *drm_edid`
:   drm\_edid container

**Description**

Get a pointer to the raw EDID data.

This is for transition only. Avoid using this like the plague.

**Return**

Pointer to raw EDID data.

const struct drm\_edid \*drm\_edid\_alloc(const void \*edid, size\_t size)
:   Allocate a new drm\_edid container

**Parameters**

`const void *edid`
:   Pointer to raw EDID data

`size_t size`
:   Size of memory allocated for EDID

**Description**

Allocate a new drm\_edid container. Do not calculate edid size from edid, pass
the actual size that has been allocated for the data. There is no validation
of the raw EDID data against the size, but at least the EDID base block must
fit in the buffer.

The returned pointer must be freed using [`drm_edid_free()`](#c.drm_edid_free "drm_edid_free").

**Return**

drm\_edid container, or NULL on errors

const struct [drm\_edid](#c.drm_edid_dup "drm_edid") \*drm\_edid\_dup(const struct [drm\_edid](#c.drm_edid_dup "drm_edid") \*drm\_edid)
:   Duplicate a drm\_edid container

**Parameters**

`const struct drm_edid *drm_edid`
:   EDID to duplicate

**Description**

The returned pointer must be freed using [`drm_edid_free()`](#c.drm_edid_free "drm_edid_free").

**Return**

drm\_edid container copy, or NULL on errors

void drm\_edid\_free(const struct [drm\_edid](#c.drm_edid_free "drm_edid") \*drm\_edid)
:   Free the drm\_edid container

**Parameters**

`const struct drm_edid *drm_edid`
:   EDID to free

bool drm\_probe\_ddc(struct i2c\_adapter \*adapter)
:   probe DDC presence

**Parameters**

`struct i2c_adapter *adapter`
:   I2C adapter to probe

**Return**

True on success, false on failure.

struct edid \*drm\_get\_edid(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct i2c\_adapter \*adapter)
:   get EDID data, if available

**Parameters**

`struct drm_connector *connector`
:   connector we’re probing

`struct i2c_adapter *adapter`
:   I2C adapter to use for DDC

**Description**

Poke the given I2C channel to grab EDID data if possible. If found,
attach it to the connector.

**Return**

Pointer to valid EDID or NULL if we couldn’t find any.

const struct drm\_edid \*drm\_edid\_read\_custom(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, read\_block\_fn read\_block, void \*context)
:   Read EDID data using given EDID block read function

**Parameters**

`struct drm_connector *connector`
:   Connector to use

`read_block_fn read_block`
:   EDID block read function

`void *context`
:   Private data passed to the block read function

**Description**

When the I2C adapter connected to the DDC bus is hidden behind a device that
exposes a different interface to read EDID blocks this function can be used
to get EDID data using a custom block read function.

As in the general case the DDC bus is accessible by the kernel at the I2C
level, drivers must make all reasonable efforts to expose it as an I2C
adapter and use [`drm_edid_read()`](#c.drm_edid_read "drm_edid_read") or [`drm_edid_read_ddc()`](#c.drm_edid_read_ddc "drm_edid_read_ddc") instead of abusing
this function.

The EDID may be overridden using debugfs override\_edid or firmware EDID
(`drm_edid_load_firmware()` and drm.edid\_firmware parameter), in this priority
order. Having either of them bypasses actual EDID reads.

The returned pointer must be freed using [`drm_edid_free()`](#c.drm_edid_free "drm_edid_free").

**Return**

Pointer to EDID, or NULL if probe/read failed.

const struct drm\_edid \*drm\_edid\_read\_ddc(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct i2c\_adapter \*adapter)
:   Read EDID data using given I2C adapter

**Parameters**

`struct drm_connector *connector`
:   Connector to use

`struct i2c_adapter *adapter`
:   I2C adapter to use for DDC

**Description**

Read EDID using the given I2C adapter.

The EDID may be overridden using debugfs override\_edid or firmware EDID
(`drm_edid_load_firmware()` and drm.edid\_firmware parameter), in this priority
order. Having either of them bypasses actual EDID reads.

Prefer initializing connector->ddc with [`drm_connector_init_with_ddc()`](drm-kms.html#c.drm_connector_init_with_ddc "drm_connector_init_with_ddc") and
using [`drm_edid_read()`](#c.drm_edid_read "drm_edid_read") instead of this function.

The returned pointer must be freed using [`drm_edid_free()`](#c.drm_edid_free "drm_edid_free").

**Return**

Pointer to EDID, or NULL if probe/read failed.

const struct drm\_edid \*drm\_edid\_read(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   Read EDID data using connector’s I2C adapter

**Parameters**

`struct drm_connector *connector`
:   Connector to use

**Description**

Read EDID using the connector’s I2C adapter.

The EDID may be overridden using debugfs override\_edid or firmware EDID
(`drm_edid_load_firmware()` and drm.edid\_firmware parameter), in this priority
order. Having either of them bypasses actual EDID reads.

The returned pointer must be freed using [`drm_edid_free()`](#c.drm_edid_free "drm_edid_free").

**Return**

Pointer to EDID, or NULL if probe/read failed.

void drm\_edid\_get\_product\_id(const struct [drm\_edid](#c.drm_edid_get_product_id "drm_edid") \*drm\_edid, struct drm\_edid\_product\_id \*id)
:   Get the vendor and product identification

**Parameters**

`const struct drm_edid *drm_edid`
:   EDID

`struct drm_edid_product_id *id`
:   Where to place the product id

void drm\_edid\_print\_product\_id(struct [drm\_printer](drm-internals.html#c.drm_printer "drm_printer") \*p, const struct drm\_edid\_product\_id \*id, bool raw)
:   Print decoded product id to printer

**Parameters**

`struct drm_printer *p`
:   drm printer

`const struct drm_edid_product_id *id`
:   EDID product id

`bool raw`
:   If true, also print the raw hex

**Description**

See VESA E-EDID 1.4 section 3.4.

u32 drm\_edid\_get\_panel\_id(const struct [drm\_edid](#c.drm_edid_get_panel_id "drm_edid") \*drm\_edid)
:   Get a panel’s ID from EDID

**Parameters**

`const struct drm_edid *drm_edid`
:   EDID that contains panel ID.

**Description**

This function uses the first block of the EDID of a panel and (assuming
that the EDID is valid) extracts the ID out of it. The ID is a 32-bit value
(16 bits of manufacturer ID and 16 bits of per-manufacturer ID) that’s
supposed to be different for each different modem of panel.

**Return**

A 32-bit ID that should be different for each make/model of panel.
See the functions [`drm_edid_encode_panel_id()`](#c.drm_edid_encode_panel_id "drm_edid_encode_panel_id") and
[`drm_edid_decode_panel_id()`](#c.drm_edid_decode_panel_id "drm_edid_decode_panel_id") for some details on the structure of this
ID. Return 0 if the EDID size is less than a base block.

const struct drm\_edid \*drm\_edid\_read\_base\_block(struct i2c\_adapter \*adapter)
:   Get a panel’s EDID base block

**Parameters**

`struct i2c_adapter *adapter`
:   I2C adapter to use for DDC

**Description**

This function returns the drm\_edid containing the first block of the EDID of
a panel.

This function is intended to be used during early probing on devices where
more than one panel might be present. Because of its intended use it must
assume that the EDID of the panel is correct, at least as far as the base
block is concerned (in other words, we don’t process any overrides here).

Caller should call [`drm_edid_free()`](#c.drm_edid_free "drm_edid_free") after use.

**NOTE**

it’s expected that this function and `drm_do_get_edid()` will both
be read the EDID, but there is no caching between them. Since we’re only
reading the first block, hopefully this extra overhead won’t be too big.

WARNING: Only use this function when the connector is unknown. For example,
during the early probe of panel. The EDID read from the function is temporary
and should be replaced by the full EDID returned from other drm\_edid\_read.

**Return**

Pointer to allocated EDID base block, or NULL on any failure.

struct edid \*drm\_get\_edid\_switcheroo(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct i2c\_adapter \*adapter)
:   get EDID data for a vga\_switcheroo output

**Parameters**

`struct drm_connector *connector`
:   connector we’re probing

`struct i2c_adapter *adapter`
:   I2C adapter to use for DDC

**Description**

Wrapper around [`drm_get_edid()`](#c.drm_get_edid "drm_get_edid") for laptops with dual GPUs using one set of
outputs. The wrapper adds the requisite vga\_switcheroo calls to temporarily
switch DDC to the GPU which is retrieving EDID.

**Return**

Pointer to valid EDID or `NULL` if we couldn’t find any.

const struct drm\_edid \*drm\_edid\_read\_switcheroo(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct i2c\_adapter \*adapter)
:   get EDID data for a vga\_switcheroo output

**Parameters**

`struct drm_connector *connector`
:   connector we’re probing

`struct i2c_adapter *adapter`
:   I2C adapter to use for DDC

**Description**

Wrapper around [`drm_edid_read_ddc()`](#c.drm_edid_read_ddc "drm_edid_read_ddc") for laptops with dual GPUs using one set
of outputs. The wrapper adds the requisite vga\_switcheroo calls to
temporarily switch DDC to the GPU which is retrieving EDID.

**Return**

Pointer to valid EDID or `NULL` if we couldn’t find any.

struct [edid](#c.drm_edid_duplicate "edid") \*drm\_edid\_duplicate(const struct [edid](#c.drm_edid_duplicate "edid") \*edid)
:   duplicate an EDID and the extensions

**Parameters**

`const struct edid *edid`
:   EDID to duplicate

**Return**

Pointer to duplicated EDID or NULL on allocation failure.

u8 drm\_match\_cea\_mode(const struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*to\_match)
:   look for a CEA mode matching given mode

**Parameters**

`const struct drm_display_mode *to_match`
:   display mode

**Return**

The CEA Video ID (VIC) of the mode or 0 if it isn’t a CEA-861
mode.

struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*drm\_display\_mode\_from\_cea\_vic(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, u8 video\_code)
:   return a mode for CEA VIC

**Parameters**

`struct drm_device *dev`
:   DRM device

`u8 video_code`
:   CEA VIC of the mode

**Description**

Creates a new mode matching the specified CEA VIC.

**Return**

A new drm\_display\_mode on success or NULL on failure

bool drm\_edid\_match(const struct [drm\_edid](#c.drm_edid_match "drm_edid") \*drm\_edid, const struct drm\_edid\_ident \*ident)
:   match drm\_edid with given identity

**Parameters**

`const struct drm_edid *drm_edid`
:   EDID

`const struct drm_edid_ident *ident`
:   the EDID identity to match with

**Description**

Check if the EDID matches with the given identity.

**Return**

True if the given identity matched with EDID, false otherwise.

void drm\_edid\_get\_monitor\_name(const struct [edid](#c.drm_edid_get_monitor_name "edid") \*edid, char \*name, int bufsize)
:   fetch the monitor name from the edid

**Parameters**

`const struct edid *edid`
:   monitor EDID information

`char *name`
:   pointer to a character array to hold the name of the monitor

`int bufsize`
:   The size of the name buffer (should be at least 14 chars.)

int drm\_edid\_to\_sad(const struct [edid](#c.drm_edid_to_sad "edid") \*edid, struct cea\_sad \*\*sads)
:   extracts SADs from EDID

**Parameters**

`const struct edid *edid`
:   EDID to parse

`struct cea_sad **sads`
:   pointer that will be set to the extracted SADs

**Description**

Looks for CEA EDID block and extracts SADs (Short Audio Descriptors) from it.

**Note**

The returned pointer needs to be freed using [`kfree()`](../core-api/mm-api.html#c.kfree "kfree").

**Return**

The number of found SADs or negative number on error.

int drm\_edid\_to\_speaker\_allocation(const struct [edid](#c.drm_edid_to_speaker_allocation "edid") \*edid, u8 \*\*sadb)
:   extracts Speaker Allocation Data Blocks from EDID

**Parameters**

`const struct edid *edid`
:   EDID to parse

`u8 **sadb`
:   pointer to the speaker block

**Description**

Looks for CEA EDID block and extracts the Speaker Allocation Data Block from it.

**Note**

The returned pointer needs to be freed using [`kfree()`](../core-api/mm-api.html#c.kfree "kfree").

**Return**

The number of found Speaker Allocation Blocks or negative number on
error.

int drm\_av\_sync\_delay(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, const struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*mode)
:   compute the HDMI/DP sink audio-video sync delay

**Parameters**

`struct drm_connector *connector`
:   connector associated with the HDMI/DP sink

`const struct drm_display_mode *mode`
:   the display mode

**Return**

The HDMI/DP sink’s audio-video sync delay in milliseconds or 0 if
the sink doesn’t support audio or video.

bool drm\_detect\_hdmi\_monitor(const struct [edid](#c.drm_detect_hdmi_monitor "edid") \*edid)
:   detect whether monitor is HDMI

**Parameters**

`const struct edid *edid`
:   monitor EDID information

**Description**

Parse the CEA extension according to CEA-861-B.

Drivers that have added the modes parsed from EDID to drm\_display\_info
should use [`drm_display_info.is_hdmi`](drm-kms.html#c.drm_display_info "drm_display_info") instead of calling this function.

**Return**

True if the monitor is HDMI, false if not or unknown.

bool drm\_detect\_monitor\_audio(const struct [edid](#c.drm_detect_monitor_audio "edid") \*edid)
:   check monitor audio capability

**Parameters**

`const struct edid *edid`
:   EDID block to scan

**Description**

Monitor should have CEA extension block.
If monitor has ‘basic audio’, but no CEA audio blocks, it’s ‘basic
audio’ only. If there is any audio extension block and supported
audio format, assume at least ‘basic audio’ support, even if ‘basic
audio’ is not defined in EDID.

**Return**

True if the monitor supports audio, false otherwise.

enum hdmi\_quantization\_range drm\_default\_rgb\_quant\_range(const struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*mode)
:   default RGB quantization range

**Parameters**

`const struct drm_display_mode *mode`
:   display mode

**Description**

Determine the default RGB quantization range for the mode,
as specified in CEA-861.

**Return**

The default RGB quantization range for the mode

int drm\_edid\_connector\_update(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, const struct [drm\_edid](#c.drm_edid_connector_update "drm_edid") \*drm\_edid)
:   Update connector information from EDID

**Parameters**

`struct drm_connector *connector`
:   Connector

`const struct drm_edid *drm_edid`
:   EDID

**Description**

Update the connector display info, ELD, HDR metadata, relevant properties,
etc. from the passed in EDID.

If EDID is NULL, reset the information.

Must be called before calling [`drm_edid_connector_add_modes()`](#c.drm_edid_connector_add_modes "drm_edid_connector_add_modes").

**Return**

0 on success, negative error on errors.

int drm\_edid\_connector\_add\_modes(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   Update probed modes from the EDID property

**Parameters**

`struct drm_connector *connector`
:   Connector

**Description**

Add the modes from the previously updated EDID property to the connector
probed modes list.

[`drm_edid_connector_update()`](#c.drm_edid_connector_update "drm_edid_connector_update") must have been called before this to update the
EDID property.

**Return**

The number of modes added, or 0 if we couldn’t find any.

int drm\_connector\_update\_edid\_property(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, const struct [edid](#c.drm_connector_update_edid_property "edid") \*edid)
:   update the edid property of a connector

**Parameters**

`struct drm_connector *connector`
:   drm connector

`const struct edid *edid`
:   new value of the edid property

**Description**

This function creates a new blob modeset object and assigns its id to the
connector’s edid property.
Since we also parse tile information from EDID’s displayID block, we also
set the connector’s tile property here. See [`drm_connector_set_tile_property()`](drm-kms.html#c.drm_connector_set_tile_property "drm_connector_set_tile_property")
for more details.

This function is deprecated. Use [`drm_edid_connector_update()`](#c.drm_edid_connector_update "drm_edid_connector_update") instead.

**Return**

Zero on success, negative errno on failure.

int drm\_add\_edid\_modes(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, struct [edid](#c.drm_add_edid_modes "edid") \*edid)
:   add modes from EDID data, if available

**Parameters**

`struct drm_connector *connector`
:   connector we’re probing

`struct edid *edid`
:   EDID data

**Description**

Add the specified modes to the connector’s mode list. Also fills out the
[`drm_display_info`](drm-kms.html#c.drm_display_info "drm_display_info") structure and ELD in **connector** with any information which
can be derived from the edid.

This function is deprecated. Use [`drm_edid_connector_add_modes()`](#c.drm_edid_connector_add_modes "drm_edid_connector_add_modes") instead.

**Return**

The number of modes added or 0 if we couldn’t find any.

int drm\_add\_modes\_noedid(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, unsigned int hdisplay, unsigned int vdisplay)
:   add modes for the connectors without EDID

**Parameters**

`struct drm_connector *connector`
:   connector we’re probing

`unsigned int hdisplay`
:   the horizontal display limit

`unsigned int vdisplay`
:   the vertical display limit

**Description**

Add the specified modes to the connector’s mode list. Only when the
hdisplay/vdisplay is not beyond the given limit, it will be added.

**Return**

The number of modes added or 0 if we couldn’t find any.

int drm\_hdmi\_avi\_infoframe\_from\_display\_mode(struct hdmi\_avi\_infoframe \*frame, const struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, const struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*mode)
:   fill an HDMI AVI infoframe with data from a DRM display mode

**Parameters**

`struct hdmi_avi_infoframe *frame`
:   HDMI AVI infoframe

`const struct drm_connector *connector`
:   the connector

`const struct drm_display_mode *mode`
:   DRM display mode

**Return**

0 on success or a negative error code on failure.

void drm\_hdmi\_avi\_infoframe\_quant\_range(struct hdmi\_avi\_infoframe \*frame, const struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, const struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*mode, enum hdmi\_quantization\_range rgb\_quant\_range)
:   fill the HDMI AVI infoframe quantization range information

**Parameters**

`struct hdmi_avi_infoframe *frame`
:   HDMI AVI infoframe

`const struct drm_connector *connector`
:   the connector

`const struct drm_display_mode *mode`
:   DRM display mode

`enum hdmi_quantization_range rgb_quant_range`
:   RGB quantization range (Q)

int drm\_hdmi\_vendor\_infoframe\_from\_display\_mode(struct hdmi\_vendor\_infoframe \*frame, const struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, const struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*mode)
:   fill an HDMI infoframe with data from a DRM display mode

**Parameters**

`struct hdmi_vendor_infoframe *frame`
:   HDMI vendor infoframe

`const struct drm_connector *connector`
:   the connector

`const struct drm_display_mode *mode`
:   DRM display mode

**Description**

Note that there’s is a need to send HDMI vendor infoframes only when using a
4k or stereoscopic 3D mode. So when giving any other mode as input this
function will return -EINVAL, error that can be safely ignored.

**Return**

0 on success or a negative error code on failure.

bool drm\_edid\_is\_digital(const struct [drm\_edid](#c.drm_edid_is_digital "drm_edid") \*drm\_edid)
:   is digital?

**Parameters**

`const struct drm_edid *drm_edid`
:   The EDID

**Description**

Return true if input is digital.

int drm\_eld\_mnl(const u8 \*eld)
:   Get ELD monitor name length in bytes.

**Parameters**

`const u8 *eld`
:   pointer to an eld memory structure with mnl set

const u8 \*drm\_eld\_sad(const u8 \*eld)
:   Get ELD SAD structures.

**Parameters**

`const u8 *eld`
:   pointer to an eld memory structure with sad\_count set

int drm\_eld\_sad\_count(const u8 \*eld)
:   Get ELD SAD count.

**Parameters**

`const u8 *eld`
:   pointer to an eld memory structure with sad\_count set

int drm\_eld\_calc\_baseline\_block\_size(const u8 \*eld)
:   Calculate baseline block size in bytes

**Parameters**

`const u8 *eld`
:   pointer to an eld memory structure with mnl and sad\_count set

**Description**

This is a helper for determining the payload size of the baseline block, in
bytes, for e.g. setting the Baseline\_ELD\_Len field in the ELD header block.

int drm\_eld\_size(const u8 \*eld)
:   Get ELD size in bytes

**Parameters**

`const u8 *eld`
:   pointer to a complete eld memory structure

**Description**

The returned value does not include the vendor block. It’s vendor specific,
and comprises of the remaining bytes in the ELD memory buffer after
[`drm_eld_size()`](#c.drm_eld_size "drm_eld_size") bytes of header and baseline block.

The returned value is guaranteed to be a multiple of 4.

u8 drm\_eld\_get\_spk\_alloc(const u8 \*eld)
:   Get speaker allocation

**Parameters**

`const u8 *eld`
:   pointer to an ELD memory structure

**Description**

The returned value is the speakers mask. User has to use `DRM_ELD_SPEAKER`
field definitions to identify speakers.

u8 drm\_eld\_get\_conn\_type(const u8 \*eld)
:   Get device type hdmi/dp connected

**Parameters**

`const u8 *eld`
:   pointer to an ELD memory structure

**Description**

The caller need to use `DRM_ELD_CONN_TYPE_HDMI` or `DRM_ELD_CONN_TYPE_DP` to
identify the display type connected.

int drm\_eld\_sad\_get(const u8 \*eld, int sad\_index, struct cea\_sad \*cta\_sad)
:   get SAD from ELD to `struct cea_sad`

**Parameters**

`const u8 *eld`
:   ELD buffer

`int sad_index`
:   SAD index

`struct cea_sad *cta_sad`
:   destination `struct cea_sad`

**Return**

0 on success, or negative on errors

int drm\_eld\_sad\_set(u8 \*eld, int sad\_index, const struct cea\_sad \*cta\_sad)
:   set SAD to ELD from `struct cea_sad`

**Parameters**

`u8 *eld`
:   ELD buffer

`int sad_index`
:   SAD index

`const struct cea_sad *cta_sad`
:   source `struct cea_sad`

**Return**

0 on success, or negative on errors

## SCDC Helper Functions Reference

Status and Control Data Channel (SCDC) is a mechanism introduced by the
HDMI 2.0 specification. It is a point-to-point protocol that allows the
HDMI source and HDMI sink to exchange data. The same I2C interface that
is used to access EDID serves as the transport mechanism for SCDC.

Note: The SCDC status is going to be lost when the display is
disconnected. This can happen physically when the user disconnects
the cable, but also when a display is switched on (such as waking up
a TV).

This is further complicated by the fact that, upon a disconnection /
reconnection, KMS won’t change the mode on its own. This means that
one can’t just rely on setting the SCDC status on enable, but also
has to track the connector status changes using interrupts and
restore the SCDC status. The typical solution for this is to trigger an
empty modeset in drm\_connector\_helper\_funcs.`detect_ctx()`, like what vc4 does
in `vc4_hdmi_reset_link()`.

int drm\_scdc\_readb(struct i2c\_adapter \*adapter, u8 offset, u8 \*value)
:   read a single byte from SCDC

**Parameters**

`struct i2c_adapter *adapter`
:   I2C adapter

`u8 offset`
:   offset of register to read

`u8 *value`
:   return location for the register value

**Description**

Reads a single byte from SCDC. This is a convenience wrapper around the
[`drm_scdc_read()`](#c.drm_scdc_read "drm_scdc_read") function.

**Return**

0 on success or a negative error code on failure.

int drm\_scdc\_writeb(struct i2c\_adapter \*adapter, u8 offset, u8 value)
:   write a single byte to SCDC

**Parameters**

`struct i2c_adapter *adapter`
:   I2C adapter

`u8 offset`
:   offset of register to read

`u8 value`
:   return location for the register value

**Description**

Writes a single byte to SCDC. This is a convenience wrapper around the
[`drm_scdc_write()`](#c.drm_scdc_write "drm_scdc_write") function.

**Return**

0 on success or a negative error code on failure.

ssize\_t drm\_scdc\_read(struct i2c\_adapter \*adapter, u8 offset, void \*buffer, size\_t size)
:   read a block of data from SCDC

**Parameters**

`struct i2c_adapter *adapter`
:   I2C controller

`u8 offset`
:   start offset of block to read

`void *buffer`
:   return location for the block to read

`size_t size`
:   size of the block to read

**Description**

Reads a block of data from SCDC, starting at a given offset.

**Return**

0 on success, negative error code on failure.

ssize\_t drm\_scdc\_write(struct i2c\_adapter \*adapter, u8 offset, const void \*buffer, size\_t size)
:   write a block of data to SCDC

**Parameters**

`struct i2c_adapter *adapter`
:   I2C controller

`u8 offset`
:   start offset of block to write

`const void *buffer`
:   block of data to write

`size_t size`
:   size of the block to write

**Description**

Writes a block of data to SCDC, starting at a given offset.

**Return**

0 on success, negative error code on failure.

bool drm\_scdc\_get\_scrambling\_status(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector)
:   what is status of scrambling?

**Parameters**

`struct drm_connector *connector`
:   connector

**Description**

Reads the scrambler status over SCDC, and checks the
scrambling status.

**Return**

True if the scrambling is enabled, false otherwise.

bool drm\_scdc\_set\_scrambling(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, bool enable)
:   enable scrambling

**Parameters**

`struct drm_connector *connector`
:   connector

`bool enable`
:   bool to indicate if scrambling is to be enabled/disabled

**Description**

Writes the TMDS config register over SCDC channel, and:
enables scrambling when enable = 1
disables scrambling when enable = 0

**Return**

True if scrambling is set/reset successfully, false otherwise.

bool drm\_scdc\_set\_high\_tmds\_clock\_ratio(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, bool set)
:   set TMDS clock ratio

**Parameters**

`struct drm_connector *connector`
:   connector

`bool set`
:   ret or reset the high clock ratio

**Description**

> TMDS clock ratio calculations go like this:
> :   TMDS character = 10 bit TMDS encoded value
>
>     TMDS character rate = The rate at which TMDS characters are
>     transmitted (Mcsc)
>
>     TMDS bit rate = 10x TMDS character rate
>
> As per the spec:
> :   TMDS clock rate for pixel clock < 340 MHz = 1x the character
>     rate = 1/10 pixel clock rate
>
>     TMDS clock rate for pixel clock > 340 MHz = 0.25x the character
>     rate = 1/40 pixel clock rate
>
> Writes to the TMDS config register over SCDC channel, and:
> :   sets TMDS clock ratio to 1/40 when set = 1
>
>     sets TMDS clock ratio to 1/10 when set = 0

**Return**

True if write is successful, false otherwise.

## HDMI Infoframes Helper Reference

Strictly speaking this is not a DRM helper library but generally usable
by any driver interfacing with HDMI outputs like v4l or alsa drivers.
But it nicely fits into the overall topic of mode setting helper
libraries and hence is also included here.

struct hdr\_sink\_metadata
:   HDR sink metadata

**Definition**:

```
struct hdr_sink_metadata {
    __u32 metadata_type;
    union {
        struct hdr_static_metadata hdmi_type1;
    };
};
```

**Members**

`metadata_type`
:   Static\_Metadata\_Descriptor\_ID.

`{unnamed_union}`
:   anonymous

`hdmi_type1`
:   HDR Metadata Infoframe.

**Description**

Metadata Information read from Sink’s EDID

union hdmi\_infoframe
:   overall `union of` all abstract infoframe representations

**Definition**:

```
union hdmi_infoframe {
    struct hdmi_any_infoframe any;
    struct hdmi_avi_infoframe avi;
    struct hdmi_spd_infoframe spd;
    union hdmi_vendor_any_infoframe vendor;
    struct hdmi_audio_infoframe audio;
    struct hdmi_drm_infoframe drm;
};
```

**Members**

`any`
:   generic infoframe

`avi`
:   avi infoframe

`spd`
:   spd infoframe

`vendor`
:   `union of` all vendor infoframes

`audio`
:   audio infoframe

`drm`
:   Dynamic Range and Mastering infoframe

**Description**

This is used by the generic pack function. This works since all infoframes
have the same header which also indicates which type of infoframe should be
packed.

void hdmi\_avi\_infoframe\_init(struct hdmi\_avi\_infoframe \*frame)
:   initialize an HDMI AVI infoframe

**Parameters**

`struct hdmi_avi_infoframe *frame`
:   HDMI AVI infoframe

int hdmi\_avi\_infoframe\_check(struct hdmi\_avi\_infoframe \*frame)
:   check a HDMI AVI infoframe

**Parameters**

`struct hdmi_avi_infoframe *frame`
:   HDMI AVI infoframe

**Description**

Validates that the infoframe is consistent and updates derived fields
(eg. length) based on other fields.

Returns 0 on success or a negative error code on failure.

ssize\_t hdmi\_avi\_infoframe\_pack\_only(const struct hdmi\_avi\_infoframe \*frame, void \*buffer, size\_t size)
:   write HDMI AVI infoframe to binary buffer

**Parameters**

`const struct hdmi_avi_infoframe *frame`
:   HDMI AVI infoframe

`void *buffer`
:   destination buffer

`size_t size`
:   size of buffer

**Description**

Packs the information contained in the **frame** structure into a binary
representation that can be written into the corresponding controller
registers. Also computes the checksum as required by section 5.3.5 of
the HDMI 1.4 specification.

Returns the number of bytes packed into the binary buffer or a negative
error code on failure.

ssize\_t hdmi\_avi\_infoframe\_pack(struct hdmi\_avi\_infoframe \*frame, void \*buffer, size\_t size)
:   check a HDMI AVI infoframe, and write it to binary buffer

**Parameters**

`struct hdmi_avi_infoframe *frame`
:   HDMI AVI infoframe

`void *buffer`
:   destination buffer

`size_t size`
:   size of buffer

**Description**

Validates that the infoframe is consistent and updates derived fields
(eg. length) based on other fields, after which it packs the information
contained in the **frame** structure into a binary representation that
can be written into the corresponding controller registers. This function
also computes the checksum as required by section 5.3.5 of the HDMI 1.4
specification.

Returns the number of bytes packed into the binary buffer or a negative
error code on failure.

int hdmi\_spd\_infoframe\_init(struct hdmi\_spd\_infoframe \*frame, const char \*vendor, const char \*product)
:   initialize an HDMI SPD infoframe

**Parameters**

`struct hdmi_spd_infoframe *frame`
:   HDMI SPD infoframe

`const char *vendor`
:   vendor string

`const char *product`
:   product string

**Description**

Returns 0 on success or a negative error code on failure.

int hdmi\_spd\_infoframe\_check(struct hdmi\_spd\_infoframe \*frame)
:   check a HDMI SPD infoframe

**Parameters**

`struct hdmi_spd_infoframe *frame`
:   HDMI SPD infoframe

**Description**

Validates that the infoframe is consistent and updates derived fields
(eg. length) based on other fields.

Returns 0 on success or a negative error code on failure.

ssize\_t hdmi\_spd\_infoframe\_pack\_only(const struct hdmi\_spd\_infoframe \*frame, void \*buffer, size\_t size)
:   write HDMI SPD infoframe to binary buffer

**Parameters**

`const struct hdmi_spd_infoframe *frame`
:   HDMI SPD infoframe

`void *buffer`
:   destination buffer

`size_t size`
:   size of buffer

**Description**

Packs the information contained in the **frame** structure into a binary
representation that can be written into the corresponding controller
registers. Also computes the checksum as required by section 5.3.5 of
the HDMI 1.4 specification.

Returns the number of bytes packed into the binary buffer or a negative
error code on failure.

ssize\_t hdmi\_spd\_infoframe\_pack(struct hdmi\_spd\_infoframe \*frame, void \*buffer, size\_t size)
:   check a HDMI SPD infoframe, and write it to binary buffer

**Parameters**

`struct hdmi_spd_infoframe *frame`
:   HDMI SPD infoframe

`void *buffer`
:   destination buffer

`size_t size`
:   size of buffer

**Description**

Validates that the infoframe is consistent and updates derived fields
(eg. length) based on other fields, after which it packs the information
contained in the **frame** structure into a binary representation that
can be written into the corresponding controller registers. This function
also computes the checksum as required by section 5.3.5 of the HDMI 1.4
specification.

Returns the number of bytes packed into the binary buffer or a negative
error code on failure.

int hdmi\_audio\_infoframe\_init(struct hdmi\_audio\_infoframe \*frame)
:   initialize an HDMI audio infoframe

**Parameters**

`struct hdmi_audio_infoframe *frame`
:   HDMI audio infoframe

**Description**

Returns 0 on success or a negative error code on failure.

int hdmi\_audio\_infoframe\_check(const struct hdmi\_audio\_infoframe \*frame)
:   check a HDMI audio infoframe

**Parameters**

`const struct hdmi_audio_infoframe *frame`
:   HDMI audio infoframe

**Description**

Validates that the infoframe is consistent and updates derived fields
(eg. length) based on other fields.

Returns 0 on success or a negative error code on failure.

ssize\_t hdmi\_audio\_infoframe\_pack\_only(const struct hdmi\_audio\_infoframe \*frame, void \*buffer, size\_t size)
:   write HDMI audio infoframe to binary buffer

**Parameters**

`const struct hdmi_audio_infoframe *frame`
:   HDMI audio infoframe

`void *buffer`
:   destination buffer

`size_t size`
:   size of buffer

**Description**

Packs the information contained in the **frame** structure into a binary
representation that can be written into the corresponding controller
registers. Also computes the checksum as required by section 5.3.5 of
the HDMI 1.4 specification.

Returns the number of bytes packed into the binary buffer or a negative
error code on failure.

ssize\_t hdmi\_audio\_infoframe\_pack(struct hdmi\_audio\_infoframe \*frame, void \*buffer, size\_t size)
:   check a HDMI Audio infoframe, and write it to binary buffer

**Parameters**

`struct hdmi_audio_infoframe *frame`
:   HDMI Audio infoframe

`void *buffer`
:   destination buffer

`size_t size`
:   size of buffer

**Description**

Validates that the infoframe is consistent and updates derived fields
(eg. length) based on other fields, after which it packs the information
contained in the **frame** structure into a binary representation that
can be written into the corresponding controller registers. This function
also computes the checksum as required by section 5.3.5 of the HDMI 1.4
specification.

Returns the number of bytes packed into the binary buffer or a negative
error code on failure.

ssize\_t hdmi\_audio\_infoframe\_pack\_for\_dp(const struct hdmi\_audio\_infoframe \*frame, struct [dp\_sdp](#c.dp_sdp "dp_sdp") \*sdp, u8 dp\_version)
:   Pack a HDMI Audio infoframe for DisplayPort

**Parameters**

`const struct hdmi_audio_infoframe *frame`
:   HDMI Audio infoframe

`struct dp_sdp *sdp`
:   Secondary data packet for DisplayPort.

`u8 dp_version`
:   DisplayPort version to be encoded in the header

**Description**

Packs a HDMI Audio Infoframe to be sent over DisplayPort. This function
fills the secondary data packet to be used for DisplayPort.

**Return**

Number of total written bytes or a negative errno on failure.

int hdmi\_vendor\_infoframe\_init(struct hdmi\_vendor\_infoframe \*frame)
:   initialize an HDMI vendor infoframe

**Parameters**

`struct hdmi_vendor_infoframe *frame`
:   HDMI vendor infoframe

**Description**

Returns 0 on success or a negative error code on failure.

int hdmi\_vendor\_infoframe\_check(struct hdmi\_vendor\_infoframe \*frame)
:   check a HDMI vendor infoframe

**Parameters**

`struct hdmi_vendor_infoframe *frame`
:   HDMI infoframe

**Description**

Validates that the infoframe is consistent and updates derived fields
(eg. length) based on other fields.

Returns 0 on success or a negative error code on failure.

ssize\_t hdmi\_vendor\_infoframe\_pack\_only(const struct hdmi\_vendor\_infoframe \*frame, void \*buffer, size\_t size)
:   write a HDMI vendor infoframe to binary buffer

**Parameters**

`const struct hdmi_vendor_infoframe *frame`
:   HDMI infoframe

`void *buffer`
:   destination buffer

`size_t size`
:   size of buffer

**Description**

Packs the information contained in the **frame** structure into a binary
representation that can be written into the corresponding controller
registers. Also computes the checksum as required by section 5.3.5 of
the HDMI 1.4 specification.

Returns the number of bytes packed into the binary buffer or a negative
error code on failure.

ssize\_t hdmi\_vendor\_infoframe\_pack(struct hdmi\_vendor\_infoframe \*frame, void \*buffer, size\_t size)
:   check a HDMI Vendor infoframe, and write it to binary buffer

**Parameters**

`struct hdmi_vendor_infoframe *frame`
:   HDMI Vendor infoframe

`void *buffer`
:   destination buffer

`size_t size`
:   size of buffer

**Description**

Validates that the infoframe is consistent and updates derived fields
(eg. length) based on other fields, after which it packs the information
contained in the **frame** structure into a binary representation that
can be written into the corresponding controller registers. This function
also computes the checksum as required by section 5.3.5 of the HDMI 1.4
specification.

Returns the number of bytes packed into the binary buffer or a negative
error code on failure.

int hdmi\_drm\_infoframe\_init(struct hdmi\_drm\_infoframe \*frame)
:   initialize an HDMI Dynaminc Range and mastering infoframe

**Parameters**

`struct hdmi_drm_infoframe *frame`
:   HDMI DRM infoframe

**Description**

Returns 0 on success or a negative error code on failure.

int hdmi\_drm\_infoframe\_check(struct hdmi\_drm\_infoframe \*frame)
:   check a HDMI DRM infoframe

**Parameters**

`struct hdmi_drm_infoframe *frame`
:   HDMI DRM infoframe

**Description**

Validates that the infoframe is consistent.
Returns 0 on success or a negative error code on failure.

ssize\_t hdmi\_drm\_infoframe\_pack\_only(const struct hdmi\_drm\_infoframe \*frame, void \*buffer, size\_t size)
:   write HDMI DRM infoframe to binary buffer

**Parameters**

`const struct hdmi_drm_infoframe *frame`
:   HDMI DRM infoframe

`void *buffer`
:   destination buffer

`size_t size`
:   size of buffer

**Description**

Packs the information contained in the **frame** structure into a binary
representation that can be written into the corresponding controller
registers. Also computes the checksum as required by section 5.3.5 of
the HDMI 1.4 specification.

Returns the number of bytes packed into the binary buffer or a negative
error code on failure.

ssize\_t hdmi\_drm\_infoframe\_pack(struct hdmi\_drm\_infoframe \*frame, void \*buffer, size\_t size)
:   check a HDMI DRM infoframe, and write it to binary buffer

**Parameters**

`struct hdmi_drm_infoframe *frame`
:   HDMI DRM infoframe

`void *buffer`
:   destination buffer

`size_t size`
:   size of buffer

**Description**

Validates that the infoframe is consistent and updates derived fields
(eg. length) based on other fields, after which it packs the information
contained in the **frame** structure into a binary representation that
can be written into the corresponding controller registers. This function
also computes the checksum as required by section 5.3.5 of the HDMI 1.4
specification.

Returns the number of bytes packed into the binary buffer or a negative
error code on failure.

ssize\_t hdmi\_infoframe\_pack\_only(const union [hdmi\_infoframe](#c.hdmi_infoframe "hdmi_infoframe") \*frame, void \*buffer, size\_t size)
:   write a HDMI infoframe to binary buffer

**Parameters**

`const union hdmi_infoframe *frame`
:   HDMI infoframe

`void *buffer`
:   destination buffer

`size_t size`
:   size of buffer

**Description**

Packs the information contained in the **frame** structure into a binary
representation that can be written into the corresponding controller
registers. Also computes the checksum as required by section 5.3.5 of
the HDMI 1.4 specification.

Returns the number of bytes packed into the binary buffer or a negative
error code on failure.

ssize\_t hdmi\_infoframe\_pack(union [hdmi\_infoframe](#c.hdmi_infoframe "hdmi_infoframe") \*frame, void \*buffer, size\_t size)
:   check a HDMI infoframe, and write it to binary buffer

**Parameters**

`union hdmi_infoframe *frame`
:   HDMI infoframe

`void *buffer`
:   destination buffer

`size_t size`
:   size of buffer

**Description**

Validates that the infoframe is consistent and updates derived fields
(eg. length) based on other fields, after which it packs the information
contained in the **frame** structure into a binary representation that
can be written into the corresponding controller registers. This function
also computes the checksum as required by section 5.3.5 of the HDMI 1.4
specification.

Returns the number of bytes packed into the binary buffer or a negative
error code on failure.

void hdmi\_infoframe\_log(const char \*level, struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, const union [hdmi\_infoframe](#c.hdmi_infoframe "hdmi_infoframe") \*frame)
:   log info of HDMI infoframe

**Parameters**

`const char *level`
:   logging level

`struct device *dev`
:   device

`const union hdmi_infoframe *frame`
:   HDMI infoframe

int hdmi\_drm\_infoframe\_unpack\_only(struct hdmi\_drm\_infoframe \*frame, const void \*buffer, size\_t size)
:   unpack binary buffer of CTA-861-G DRM infoframe DataBytes to a HDMI DRM infoframe

**Parameters**

`struct hdmi_drm_infoframe *frame`
:   HDMI DRM infoframe

`const void *buffer`
:   source buffer

`size_t size`
:   size of buffer

**Description**

Unpacks CTA-861-G DRM infoframe DataBytes contained in the binary **buffer**
into a structured **frame** of the HDMI Dynamic Range and Mastering (DRM)
infoframe.

Returns 0 on success or a negative error code on failure.

int hdmi\_infoframe\_unpack(union [hdmi\_infoframe](#c.hdmi_infoframe "hdmi_infoframe") \*frame, const void \*buffer, size\_t size)
:   unpack binary buffer to a HDMI infoframe

**Parameters**

`union hdmi_infoframe *frame`
:   HDMI infoframe

`const void *buffer`
:   source buffer

`size_t size`
:   size of buffer

**Description**

Unpacks the information contained in binary buffer **buffer** into a structured
**frame** of a HDMI infoframe.
Also verifies the checksum as required by section 5.3.5 of the HDMI 1.4
specification.

Returns 0 on success or a negative error code on failure.

## Rectangle Utilities Reference

Utility functions to help manage rectangular areas for
clipping, scaling, etc. calculations.

struct drm\_rect
:   two dimensional rectangle

**Definition**:

```
struct drm_rect {
    int x1, y1, x2, y2;
};
```

**Members**

`x1`
:   horizontal starting coordinate (inclusive)

`y1`
:   vertical starting coordinate (inclusive)

`x2`
:   horizontal ending coordinate (exclusive)

`y2`
:   vertical ending coordinate (exclusive)

**Description**

Note that this must match the layout of [`struct drm_mode_rect`](drm-uapi.html#c.drm_mode_rect "drm_mode_rect") or the damage
helpers like [`drm_atomic_helper_damage_iter_init()`](drm-kms.html#c.drm_atomic_helper_damage_iter_init "drm_atomic_helper_damage_iter_init") break.

DRM\_RECT\_INIT

`DRM_RECT_INIT (x, y, w, h)`

> initialize a rectangle from x/y/w/h

**Parameters**

`x`
:   x coordinate

`y`
:   y coordinate

`w`
:   width

`h`
:   height

**Return**

A new rectangle of the specified size.

DRM\_RECT\_FMT

`DRM_RECT_FMT`

> printf string for [`struct drm_rect`](#c.drm_rect "drm_rect")

DRM\_RECT\_ARG

`DRM_RECT_ARG (r)`

> printf arguments for [`struct drm_rect`](#c.drm_rect "drm_rect")

**Parameters**

`r`
:   rectangle struct

DRM\_RECT\_FP\_FMT

`DRM_RECT_FP_FMT`

> printf string for [`struct drm_rect`](#c.drm_rect "drm_rect") in 16.16 fixed point

DRM\_RECT\_FP\_ARG

`DRM_RECT_FP_ARG (r)`

> printf arguments for [`struct drm_rect`](#c.drm_rect "drm_rect") in 16.16 fixed point

**Parameters**

`r`
:   rectangle struct

**Description**

This is useful for e.g. printing plane source rectangles, which are in 16.16
fixed point.

void drm\_rect\_init(struct [drm\_rect](#c.drm_rect "drm_rect") \*r, int x, int y, int width, int height)
:   initialize the rectangle from x/y/w/h

**Parameters**

`struct drm_rect *r`
:   rectangle

`int x`
:   x coordinate

`int y`
:   y coordinate

`int width`
:   width

`int height`
:   height

void drm\_rect\_adjust\_size(struct [drm\_rect](#c.drm_rect "drm_rect") \*r, int dw, int dh)
:   adjust the size of the rectangle

**Parameters**

`struct drm_rect *r`
:   rectangle to be adjusted

`int dw`
:   horizontal adjustment

`int dh`
:   vertical adjustment

**Description**

Change the size of rectangle **r** by **dw** in the horizontal direction,
and by **dh** in the vertical direction, while keeping the center
of **r** stationary.

Positive **dw** and **dh** increase the size, negative values decrease it.

void drm\_rect\_translate(struct [drm\_rect](#c.drm_rect "drm_rect") \*r, int dx, int dy)
:   translate the rectangle

**Parameters**

`struct drm_rect *r`
:   rectangle to be translated

`int dx`
:   horizontal translation

`int dy`
:   vertical translation

**Description**

Move rectangle **r** by **dx** in the horizontal direction,
and by **dy** in the vertical direction.

void drm\_rect\_translate\_to(struct [drm\_rect](#c.drm_rect "drm_rect") \*r, int x, int y)
:   translate the rectangle to an absolute position

**Parameters**

`struct drm_rect *r`
:   rectangle to be translated

`int x`
:   horizontal position

`int y`
:   vertical position

**Description**

Move rectangle **r** to **x** in the horizontal direction,
and to **y** in the vertical direction.

void drm\_rect\_downscale(struct [drm\_rect](#c.drm_rect "drm_rect") \*r, int horz, int vert)
:   downscale a rectangle

**Parameters**

`struct drm_rect *r`
:   rectangle to be downscaled

`int horz`
:   horizontal downscale factor

`int vert`
:   vertical downscale factor

**Description**

Divide the coordinates of rectangle **r** by **horz** and **vert**.

int drm\_rect\_width(const struct [drm\_rect](#c.drm_rect "drm_rect") \*r)
:   determine the rectangle width

**Parameters**

`const struct drm_rect *r`
:   rectangle whose width is returned

**Return**

The width of the rectangle.

int drm\_rect\_height(const struct [drm\_rect](#c.drm_rect "drm_rect") \*r)
:   determine the rectangle height

**Parameters**

`const struct drm_rect *r`
:   rectangle whose height is returned

**Return**

The height of the rectangle.

bool drm\_rect\_visible(const struct [drm\_rect](#c.drm_rect "drm_rect") \*r)
:   determine if the rectangle is visible

**Parameters**

`const struct drm_rect *r`
:   rectangle whose visibility is returned

**Return**

`true` if the rectangle is visible, `false` otherwise.

bool drm\_rect\_equals(const struct [drm\_rect](#c.drm_rect "drm_rect") \*r1, const struct [drm\_rect](#c.drm_rect "drm_rect") \*r2)
:   determine if two rectangles are equal

**Parameters**

`const struct drm_rect *r1`
:   first rectangle

`const struct drm_rect *r2`
:   second rectangle

**Return**

`true` if the rectangles are equal, `false` otherwise.

void drm\_rect\_fp\_to\_int(struct [drm\_rect](#c.drm_rect "drm_rect") \*dst, const struct [drm\_rect](#c.drm_rect "drm_rect") \*src)
:   Convert a rect in 16.16 fixed point form to int form.

**Parameters**

`struct drm_rect *dst`
:   rect to be stored the converted value

`const struct drm_rect *src`
:   rect in 16.16 fixed point form

bool drm\_rect\_overlap(const struct [drm\_rect](#c.drm_rect "drm_rect") \*a, const struct [drm\_rect](#c.drm_rect "drm_rect") \*b)
:   Check if two rectangles overlap

**Parameters**

`const struct drm_rect *a`
:   first rectangle

`const struct drm_rect *b`
:   second rectangle

**Return**

`true` if the rectangles overlap, `false` otherwise.

bool drm\_rect\_intersect(struct [drm\_rect](#c.drm_rect "drm_rect") \*r1, const struct [drm\_rect](#c.drm_rect "drm_rect") \*r2)
:   intersect two rectangles

**Parameters**

`struct drm_rect *r1`
:   first rectangle

`const struct drm_rect *r2`
:   second rectangle

**Description**

Calculate the intersection of rectangles **r1** and **r2**.
**r1** will be overwritten with the intersection.

**Return**

`true` if rectangle **r1** is still visible after the operation,
`false` otherwise.

bool drm\_rect\_clip\_scaled(struct [drm\_rect](#c.drm_rect "drm_rect") \*src, struct [drm\_rect](#c.drm_rect "drm_rect") \*dst, const struct [drm\_rect](#c.drm_rect "drm_rect") \*clip)
:   perform a scaled clip operation

**Parameters**

`struct drm_rect *src`
:   source window rectangle

`struct drm_rect *dst`
:   destination window rectangle

`const struct drm_rect *clip`
:   clip rectangle

**Description**

Clip rectangle **dst** by rectangle **clip**. Clip rectangle **src** by
the corresponding amounts, retaining the vertical and horizontal scaling
factors from **src** to **dst**.

**Return**

`true` if rectangle **dst** is still visible after being clipped,
`false` otherwise.

int drm\_rect\_calc\_hscale(const struct [drm\_rect](#c.drm_rect "drm_rect") \*src, const struct [drm\_rect](#c.drm_rect "drm_rect") \*dst, int min\_hscale, int max\_hscale)
:   calculate the horizontal scaling factor

**Parameters**

`const struct drm_rect *src`
:   source window rectangle

`const struct drm_rect *dst`
:   destination window rectangle

`int min_hscale`
:   minimum allowed horizontal scaling factor

`int max_hscale`
:   maximum allowed horizontal scaling factor

**Description**

Calculate the horizontal scaling factor as
(**src** width) / (**dst** width).

If the scale is below 1 << 16, round down. If the scale is above
1 << 16, round up. This will calculate the scale with the most
pessimistic limit calculation.

**Return**

The horizontal scaling factor, or errno of out of limits.

int drm\_rect\_calc\_vscale(const struct [drm\_rect](#c.drm_rect "drm_rect") \*src, const struct [drm\_rect](#c.drm_rect "drm_rect") \*dst, int min\_vscale, int max\_vscale)
:   calculate the vertical scaling factor

**Parameters**

`const struct drm_rect *src`
:   source window rectangle

`const struct drm_rect *dst`
:   destination window rectangle

`int min_vscale`
:   minimum allowed vertical scaling factor

`int max_vscale`
:   maximum allowed vertical scaling factor

**Description**

Calculate the vertical scaling factor as
(**src** height) / (**dst** height).

If the scale is below 1 << 16, round down. If the scale is above
1 << 16, round up. This will calculate the scale with the most
pessimistic limit calculation.

**Return**

The vertical scaling factor, or errno of out of limits.

void drm\_rect\_debug\_print(const char \*prefix, const struct [drm\_rect](#c.drm_rect "drm_rect") \*r, bool fixed\_point)
:   print the rectangle information

**Parameters**

`const char *prefix`
:   prefix string

`const struct drm_rect *r`
:   rectangle to print

`bool fixed_point`
:   rectangle is in 16.16 fixed point format

void drm\_rect\_rotate(struct [drm\_rect](#c.drm_rect "drm_rect") \*r, int width, int height, unsigned int rotation)
:   Rotate the rectangle

**Parameters**

`struct drm_rect *r`
:   rectangle to be rotated

`int width`
:   Width of the coordinate space

`int height`
:   Height of the coordinate space

`unsigned int rotation`
:   Transformation to be applied

**Description**

Apply **rotation** to the coordinates of rectangle **r**.

**width** and **height** combined with **rotation** define
the location of the new origin.

**width** correcsponds to the horizontal and **height**
to the vertical axis of the untransformed coordinate
space.

void drm\_rect\_rotate\_inv(struct [drm\_rect](#c.drm_rect "drm_rect") \*r, int width, int height, unsigned int rotation)
:   Inverse rotate the rectangle

**Parameters**

`struct drm_rect *r`
:   rectangle to be rotated

`int width`
:   Width of the coordinate space

`int height`
:   Height of the coordinate space

`unsigned int rotation`
:   Transformation whose inverse is to be applied

**Description**

Apply the inverse of **rotation** to the coordinates
of rectangle **r**.

**width** and **height** combined with **rotation** define
the location of the new origin.

**width** correcsponds to the horizontal and **height**
to the vertical axis of the original untransformed
coordinate space, so that you never have to flip
them when doing a rotatation and its inverse.
That is, if you do

```
drm_rect_rotate(&r, width, height, rotation);
drm_rect_rotate_inv(&r, width, height, rotation);
```

you will always get back the original rectangle.

## Flip-work Helper Reference

Utility to queue up work to run from work-queue context after flip/vblank.
Typically this can be used to defer unref of framebuffer’s, cursor
bo’s, etc until after vblank. The APIs are all thread-safe. Moreover,
[`drm_flip_work_commit()`](#c.drm_flip_work_commit "drm_flip_work_commit") can be called in atomic context.

struct drm\_flip\_work
:   flip work queue

**Definition**:

```
struct drm_flip_work {
    const char *name;
    drm_flip_func_t func;
    struct work_struct worker;
    struct list_head queued;
    struct list_head commited;
    spinlock_t lock;
};
```

**Members**

`name`
:   debug name

`func`
:   callback fxn called for each committed item

`worker`
:   worker which calls **func**

`queued`
:   queued tasks

`commited`
:   commited tasks

`lock`
:   lock to access queued and commited lists

void drm\_flip\_work\_queue(struct [drm\_flip\_work](#c.drm_flip_work "drm_flip_work") \*work, void \*val)
:   queue work

**Parameters**

`struct drm_flip_work *work`
:   the flip-work

`void *val`
:   the value to queue

**Description**

Queues work, that will later be run (passed back to drm\_flip\_func\_t
func) on a work queue after [`drm_flip_work_commit()`](#c.drm_flip_work_commit "drm_flip_work_commit") is called.

void drm\_flip\_work\_commit(struct [drm\_flip\_work](#c.drm_flip_work "drm_flip_work") \*work, struct workqueue\_struct \*wq)
:   commit queued work

**Parameters**

`struct drm_flip_work *work`
:   the flip-work

`struct workqueue_struct *wq`
:   the work-queue to run the queued work on

**Description**

Trigger work previously queued by [`drm_flip_work_queue()`](#c.drm_flip_work_queue "drm_flip_work_queue") to run
on a workqueue. The typical usage would be to queue work (via
[`drm_flip_work_queue()`](#c.drm_flip_work_queue "drm_flip_work_queue")) at any point (from vblank irq and/or
prior), and then from vblank irq commit the queued work.

void drm\_flip\_work\_init(struct [drm\_flip\_work](#c.drm_flip_work "drm_flip_work") \*work, const char \*name, drm\_flip\_func\_t func)
:   initialize flip-work

**Parameters**

`struct drm_flip_work *work`
:   the flip-work to initialize

`const char *name`
:   debug name

`drm_flip_func_t func`
:   the callback work function

**Description**

Initializes/allocates resources for the flip-work

void drm\_flip\_work\_cleanup(struct [drm\_flip\_work](#c.drm_flip_work "drm_flip_work") \*work)
:   cleans up flip-work

**Parameters**

`struct drm_flip_work *work`
:   the flip-work to cleanup

**Description**

Destroy resources allocated for the flip-work

## Auxiliary Modeset Helpers

This helper library contains various one-off functions which don’t really fit
anywhere else in the DRM modeset helper library.

void drm\_helper\_move\_panel\_connectors\_to\_head(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   move panels to the front in the connector list

**Parameters**

`struct drm_device *dev`
:   drm device to operate on

**Description**

Some userspace presumes that the first connected connector is the main
display, where it’s supposed to display e.g. the login screen. For
laptops, this should be the main panel. Use this function to sort all
(eDP/LVDS/DSI) panels to the front of the connector list, instead of
painstakingly trying to initialize them in the right order.

void drm\_helper\_mode\_fill\_fb\_struct(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, const struct [drm\_format\_info](drm-kms.html#c.drm_format_info "drm_format_info") \*info, const struct [drm\_mode\_fb\_cmd2](drm-uapi.html#c.drm_mode_fb_cmd2 "drm_mode_fb_cmd2") \*mode\_cmd)
:   fill out framebuffer metadata

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_framebuffer *fb`
:   drm\_framebuffer object to fill out

`const struct drm_format_info *info`
:   pixel format information

`const struct drm_mode_fb_cmd2 *mode_cmd`
:   metadata from the userspace fb creation request

**Description**

This helper can be used in a drivers fb\_create callback to pre-fill the fb’s
metadata fields.

int drm\_crtc\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, const struct [drm\_crtc\_funcs](drm-kms.html#c.drm_crtc_funcs "drm_crtc_funcs") \*funcs)
:   Legacy CRTC initialization function

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_crtc *crtc`
:   CRTC object to init

`const struct drm_crtc_funcs *funcs`
:   callbacks for the new CRTC

**Description**

Initialize a CRTC object with a default helper-provided primary plane and no
cursor plane.

Note that we make some assumptions about hardware limitations that may not be
true for all hardware:

1. Primary plane cannot be repositioned.
2. Primary plane cannot be scaled.
3. Primary plane must cover the entire CRTC.
4. Subpixel positioning is not supported.
5. The primary plane must always be on if the CRTC is enabled.

This is purely a backwards compatibility helper for old drivers. Drivers
should instead implement their own primary plane. Atomic drivers must do so.

**Return**

Zero on success, error code on failure.

int drm\_mode\_config\_helper\_suspend(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   Modeset suspend helper

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

This helper function takes care of suspending the modeset side. It disables
output polling if initialized, suspends fbdev if used and finally calls
[`drm_atomic_helper_suspend()`](#c.drm_atomic_helper_suspend "drm_atomic_helper_suspend").
If suspending fails, fbdev and polling is re-enabled.

See also:
[`drm_kms_helper_poll_disable()`](#c.drm_kms_helper_poll_disable "drm_kms_helper_poll_disable") and `drm_client_dev_suspend()`.

**Return**

Zero on success, negative error code on error.

int drm\_mode\_config\_helper\_resume(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   Modeset resume helper

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

This helper function takes care of resuming the modeset side. It calls
[`drm_atomic_helper_resume()`](#c.drm_atomic_helper_resume "drm_atomic_helper_resume"), resumes fbdev if used and enables output polling
if initiaized.

See also:
`drm_client_dev_resume()` and [`drm_kms_helper_poll_enable()`](#c.drm_kms_helper_poll_enable "drm_kms_helper_poll_enable").

**Return**

Zero on success, negative error code on error.

## OF/DT Helpers

A set of helper functions to aid DRM drivers in parsing standard DT
properties.

uint32\_t drm\_of\_crtc\_port\_mask(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct device\_node \*port)
:   find the mask of a registered CRTC by port OF node

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct device_node *port`
:   port OF node

**Description**

Given a port OF node, return the possible mask of the corresponding
CRTC within a device’s list of CRTCs. Returns zero if not found.

uint32\_t drm\_of\_find\_possible\_crtcs(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct device\_node \*port)
:   find the possible CRTCs for an encoder port

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct device_node *port`
:   encoder port to scan for endpoints

**Description**

Scan all endpoints attached to a port, locate their attached CRTCs,
and generate the DRM mask of CRTCs which may be attached to this
encoder.

See <https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/graph.yaml>
for the bindings.

void drm\_of\_component\_match\_add(struct [device](../driver-api/infrastructure.html#c.device "device") \*master, struct component\_match \*\*matchptr, int (\*compare)(struct [device](../driver-api/infrastructure.html#c.device "device")\*, void\*), struct device\_node \*node)
:   Add a component helper OF node match rule

**Parameters**

`struct device *master`
:   master device

`struct component_match **matchptr`
:   component match pointer

`int (*compare)(struct device *, void *)`
:   compare function used for matching component

`struct device_node *node`
:   of\_node

int drm\_of\_component\_probe(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, int (\*compare\_of)(struct [device](../driver-api/infrastructure.html#c.device "device")\*, void\*), const struct [component\_master\_ops](../driver-api/component.html#c.component_master_ops "component_master_ops") \*m\_ops)
:   Generic probe function for a component based master

**Parameters**

`struct device *dev`
:   master device containing the OF node

`int (*compare_of)(struct device *, void *)`
:   compare function used for matching components

`const struct component_master_ops *m_ops`
:   component master ops to be used

**Description**

Parse the platform device OF node and bind all the components associated
with the master. Interface ports are added before the encoders in order to
satisfy their .bind requirements

See <https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/graph.yaml>
for the bindings.

Returns zero if successful, or one of the standard error codes if it fails.

int drm\_of\_find\_panel\_or\_bridge(const struct device\_node \*np, int port, int endpoint, struct [drm\_panel](#c.drm_panel "drm_panel") \*\*panel, struct [drm\_bridge](#c.drm_bridge "drm_bridge") \*\*bridge)
:   return connected panel or bridge device

**Parameters**

`const struct device_node *np`
:   device tree node containing encoder output ports

`int port`
:   port in the device tree node

`int endpoint`
:   endpoint in the device tree node

`struct drm_panel **panel`
:   pointer to hold returned drm\_panel

`struct drm_bridge **bridge`
:   pointer to hold returned drm\_bridge

**Description**

Given a DT node’s port and endpoint number, find the connected node and
return either the associated [`struct drm_panel`](#c.drm_panel "drm_panel") or drm\_bridge device. Either
**panel** or **bridge** must not be NULL.

This function is deprecated and should not be used in new drivers. Use
[`devm_drm_of_get_bridge()`](#c.devm_drm_of_get_bridge "devm_drm_of_get_bridge") instead.

Returns zero if successful, or one of the standard error codes if it fails.

int drm\_of\_lvds\_get\_dual\_link\_pixel\_order(const struct device\_node \*port1, const struct device\_node \*port2)
:   Get LVDS dual-link source pixel order

**Parameters**

`const struct device_node *port1`
:   First DT port node of the Dual-link LVDS source

`const struct device_node *port2`
:   Second DT port node of the Dual-link LVDS source

**Description**

An LVDS dual-link connection is made of two links, with even pixels
transitting on one link, and odd pixels on the other link. This function
returns, for two ports of an LVDS dual-link source, which port shall transmit
the even and odd pixels, based on the requirements of the connected sink.

The pixel order is determined from the dual-lvds-even-pixels and
dual-lvds-odd-pixels properties in the sink’s DT port nodes. If those
properties are not present, or if their usage is not valid, this function
returns -EINVAL.

If either port is not connected, this function returns -EPIPE.

**port1** and **port2** are typically DT sibling nodes, but may have different
parents when, for instance, two separate LVDS encoders carry the even and odd
pixels.

**Return**

* DRM\_LVDS\_DUAL\_LINK\_EVEN\_ODD\_PIXELS - **port1** carries even pixels and **port2**
  carries odd pixels
* DRM\_LVDS\_DUAL\_LINK\_ODD\_EVEN\_PIXELS - **port1** carries odd pixels and **port2**
  carries even pixels
* -EINVAL - **port1** and **port2** are not connected to a dual-link LVDS sink, or
  the sink configuration is invalid
* -EPIPE - when **port1** or **port2** are not connected

int drm\_of\_lvds\_get\_dual\_link\_pixel\_order\_sink(struct device\_node \*port1, struct device\_node \*port2)
:   Get LVDS dual-link sink pixel order

**Parameters**

`struct device_node *port1`
:   First DT port node of the Dual-link LVDS sink

`struct device_node *port2`
:   Second DT port node of the Dual-link LVDS sink

**Description**

An LVDS dual-link connection is made of two links, with even pixels
transitting on one link, and odd pixels on the other link. This function
returns, for two ports of an LVDS dual-link sink, which port shall transmit
the even and odd pixels, based on the requirements of the sink.

The pixel order is determined from the dual-lvds-even-pixels and
dual-lvds-odd-pixels properties in the sink’s DT port nodes. If those
properties are not present, or if their usage is not valid, this function
returns -EINVAL.

If either port is not connected, this function returns -EPIPE.

**port1** and **port2** are typically DT sibling nodes, but may have different
parents when, for instance, two separate LVDS decoders receive the even and
odd pixels.

**Return**

* DRM\_LVDS\_DUAL\_LINK\_EVEN\_ODD\_PIXELS - **port1** receives even pixels and **port2**
  receives odd pixels
* DRM\_LVDS\_DUAL\_LINK\_ODD\_EVEN\_PIXELS - **port1** receives odd pixels and **port2**
  receives even pixels
* -EINVAL - **port1** or **port2** are NULL
* -EPIPE - when **port1** or **port2** are not connected

int drm\_of\_lvds\_get\_data\_mapping(const struct device\_node \*port)
:   Get LVDS data mapping

**Parameters**

`const struct device_node *port`
:   DT port node of the LVDS source or sink

**Description**

Convert DT “data-mapping” property string value into media bus format value.

**Return**

* MEDIA\_BUS\_FMT\_RGB666\_1X7X3\_SPWG - data-mapping is “jeida-18”
* MEDIA\_BUS\_FMT\_RGB888\_1X7X4\_JEIDA - data-mapping is “jeida-24”
* MEDIA\_BUS\_FMT\_RGB101010\_1X7X5\_JEIDA - data-mapping is “jeida-30”
* MEDIA\_BUS\_FMT\_RGB888\_1X7X4\_SPWG - data-mapping is “vesa-24”
* MEDIA\_BUS\_FMT\_RGB101010\_1X7X5\_SPWG - data-mapping is “vesa-30”
* -EINVAL - the “data-mapping” property is unsupported
* -ENODEV - the “data-mapping” property is missing

int drm\_of\_get\_data\_lanes\_count(const struct device\_node \*endpoint, const unsigned int min, const unsigned int max)
:   Get DSI/(e)DP data lane count

**Parameters**

`const struct device_node *endpoint`
:   DT endpoint node of the DSI/(e)DP source or sink

`const unsigned int min`
:   minimum supported number of data lanes

`const unsigned int max`
:   maximum supported number of data lanes

**Description**

Count DT “data-lanes” property elements and check for validity.

**Return**

* min..max - positive integer count of “data-lanes” elements
* -ve - the “data-lanes” property is missing or invalid
* -EINVAL - the “data-lanes” property is unsupported

int drm\_of\_get\_data\_lanes\_count\_ep(const struct device\_node \*port, int port\_reg, int reg, const unsigned int min, const unsigned int max)
:   Get DSI/(e)DP data lane count by endpoint

**Parameters**

`const struct device_node *port`
:   DT port node of the DSI/(e)DP source or sink

`int port_reg`
:   identifier (value of reg property) of the parent port node

`int reg`
:   identifier (value of reg property) of the endpoint node

`const unsigned int min`
:   minimum supported number of data lanes

`const unsigned int max`
:   maximum supported number of data lanes

**Description**

Count DT “data-lanes” property elements and check for validity.
This variant uses endpoint specifier.

**Return**

* min..max - positive integer count of “data-lanes” elements
* -EINVAL - the “data-mapping” property is unsupported
* -ENODEV - the “data-mapping” property is missing

struct [mipi\_dsi\_host](#c.mipi_dsi_host "mipi_dsi_host") \*drm\_of\_get\_dsi\_bus(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev)
:   find the DSI bus for a given device

**Parameters**

`struct device *dev`
:   parent device of display (SPI, I2C)

**Description**

Gets parent DSI bus for a DSI device controlled through a bus other
than MIPI-DCS (SPI, I2C, etc.) using the Device Tree.

This function assumes that the device’s port\*\*0\*\* is the DSI input.

Returns pointer to mipi\_dsi\_host if successful, -EINVAL if the
request is unsupported, -EPROBE\_DEFER if the DSI host is found but
not available, or -ENODEV otherwise.

## Legacy Plane Helper Reference

This helper library contains helpers to implement primary plane support on
top of the normal CRTC configuration interface.
Since the legacy [`drm_mode_config_funcs.set_config`](drm-kms.html#c.drm_mode_config_funcs "drm_mode_config_funcs") interface ties the primary
plane together with the CRTC state this does not allow userspace to disable
the primary plane itself. The default primary plane only expose XRBG8888 and
ARGB8888 as valid pixel formats for the attached framebuffer.

Drivers are highly recommended to implement proper support for primary
planes, and newly merged drivers must not rely upon these transitional
helpers.

The plane helpers share the function table structures with other helpers,
specifically also the atomic helpers. See [`struct drm_plane_helper_funcs`](#c.drm_plane_helper_funcs "drm_plane_helper_funcs") for
the details.

int drm\_plane\_helper\_update\_primary(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*fb, int crtc\_x, int crtc\_y, unsigned int crtc\_w, unsigned int crtc\_h, uint32\_t src\_x, uint32\_t src\_y, uint32\_t src\_w, uint32\_t src\_h, struct [drm\_modeset\_acquire\_ctx](drm-kms.html#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx)
:   Helper for updating primary planes

**Parameters**

`struct drm_plane *plane`
:   plane to update

`struct drm_crtc *crtc`
:   the plane’s new CRTC

`struct drm_framebuffer *fb`
:   the plane’s new framebuffer

`int crtc_x`
:   x coordinate within CRTC

`int crtc_y`
:   y coordinate within CRTC

`unsigned int crtc_w`
:   width coordinate within CRTC

`unsigned int crtc_h`
:   height coordinate within CRTC

`uint32_t src_x`
:   x coordinate within source

`uint32_t src_y`
:   y coordinate within source

`uint32_t src_w`
:   width coordinate within source

`uint32_t src_h`
:   height coordinate within source

`struct drm_modeset_acquire_ctx *ctx`
:   modeset locking context

**Description**

This helper validates the given parameters and updates the primary plane.

This function is only useful for non-atomic modesetting. Don’t use
it in new drivers.

**Return**

Zero on success, or an errno code otherwise.

int drm\_plane\_helper\_disable\_primary(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane, struct [drm\_modeset\_acquire\_ctx](drm-kms.html#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx)
:   Helper for disabling primary planes

**Parameters**

`struct drm_plane *plane`
:   plane to disable

`struct drm_modeset_acquire_ctx *ctx`
:   modeset locking context

**Description**

This helper returns an error when trying to disable the primary
plane.

This function is only useful for non-atomic modesetting. Don’t use
it in new drivers.

**Return**

An errno code.

void drm\_plane\_helper\_destroy(struct [drm\_plane](drm-kms.html#c.drm_plane "drm_plane") \*plane)
:   Helper for primary plane destruction

**Parameters**

`struct drm_plane *plane`
:   plane to destroy

**Description**

Provides a default plane destroy handler for primary planes. This handler
is called during CRTC destruction. We disable the primary plane, remove
it from the DRM plane list, and deallocate the plane structure.

## Legacy CRTC/Modeset Helper Functions Reference

The CRTC modeset helper library provides a default set\_config implementation
in [`drm_crtc_helper_set_config()`](#c.drm_crtc_helper_set_config "drm_crtc_helper_set_config"). Plus a few other convenience functions using
the same callbacks which drivers can use to e.g. restore the modeset
configuration on resume with [`drm_helper_resume_force_mode()`](#c.drm_helper_resume_force_mode "drm_helper_resume_force_mode").

Note that this helper library doesn’t track the current power state of CRTCs
and encoders. It can call callbacks like [`drm_encoder_helper_funcs.dpms`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") even
though the hardware is already in the desired state. This deficiency has been
fixed in the atomic helpers.

The driver callbacks are mostly compatible with the atomic modeset helpers,
except for the handling of the primary plane: Atomic helpers require that the
primary plane is implemented as a real standalone plane and not directly tied
to the CRTC state. For easier transition this library provides functions to
implement the old semantics required by the CRTC helpers using the new plane
and atomic helper callbacks.

Drivers are strongly urged to convert to the atomic helpers (by way of first
converting to the plane helpers). New drivers must not use these functions
but need to implement the atomic interface instead, potentially using the
atomic helpers for that.

These legacy modeset helpers use the same function table structures as
all other modesetting helpers. See the documentation for struct
[`drm_crtc_helper_funcs`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs"), [`struct drm_encoder_helper_funcs`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") and struct
[`drm_connector_helper_funcs`](#c.drm_connector_helper_funcs "drm_connector_helper_funcs").

bool drm\_helper\_encoder\_in\_use(struct [drm\_encoder](drm-kms.html#c.drm_encoder "drm_encoder") \*encoder)
:   check if a given encoder is in use

**Parameters**

`struct drm_encoder *encoder`
:   encoder to check

**Description**

Checks whether **encoder** is with the current mode setting output configuration
in use by any connector. This doesn’t mean that it is actually enabled since
the DPMS state is tracked separately.

**Return**

True if **encoder** is used, false otherwise.

bool drm\_helper\_crtc\_in\_use(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc)
:   check if a given CRTC is in a mode\_config

**Parameters**

`struct drm_crtc *crtc`
:   CRTC to check

**Description**

Checks whether **crtc** is with the current mode setting output configuration
in use by any connector. This doesn’t mean that it is actually enabled since
the DPMS state is tracked separately.

**Return**

True if **crtc** is used, false otherwise.

void drm\_helper\_disable\_unused\_functions(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   disable unused objects

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

This function walks through the entire mode setting configuration of **dev**. It
will remove any CRTC links of unused encoders and encoder links of
disconnected connectors. Then it will disable all unused encoders and CRTCs
either by calling their disable callback if available or by calling their
dpms callback with DRM\_MODE\_DPMS\_OFF.

**NOTE**

This function is part of the legacy modeset helper library and will cause
major confusion with atomic drivers. This is because atomic helpers guarantee
to never call ->`disable()` hooks on a disabled function, or ->`enable()` hooks
on an enabled functions. [`drm_helper_disable_unused_functions()`](#c.drm_helper_disable_unused_functions "drm_helper_disable_unused_functions") on the other
hand throws such guarantees into the wind and calls disable hooks
unconditionally on unused functions.

bool drm\_crtc\_helper\_set\_mode(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_display\_mode](drm-kms.html#c.drm_display_mode "drm_display_mode") \*mode, int x, int y, struct [drm\_framebuffer](drm-kms.html#c.drm_framebuffer "drm_framebuffer") \*old\_fb)
:   internal helper to set a mode

**Parameters**

`struct drm_crtc *crtc`
:   CRTC to program

`struct drm_display_mode *mode`
:   mode to use

`int x`
:   horizontal offset into the surface

`int y`
:   vertical offset into the surface

`struct drm_framebuffer *old_fb`
:   old framebuffer, for cleanup

**Description**

Try to set **mode** on **crtc**. Give **crtc** and its associated connectors a chance
to fixup or reject the mode prior to trying to set it. This is an internal
helper that drivers could e.g. use to update properties that require the
entire output pipe to be disabled and re-enabled in a new configuration. For
example for changing whether audio is enabled on a hdmi link or for changing
panel fitter or dither attributes. It is also called by the
[`drm_crtc_helper_set_config()`](#c.drm_crtc_helper_set_config "drm_crtc_helper_set_config") helper function to drive the mode setting
sequence.

**Return**

True if the mode was set successfully, false otherwise.

int drm\_crtc\_helper\_atomic\_check(struct [drm\_crtc](drm-kms.html#c.drm_crtc "drm_crtc") \*crtc, struct [drm\_atomic\_state](drm-kms.html#c.drm_atomic_state "drm_atomic_state") \*state)
:   Helper to check CRTC atomic-state

**Parameters**

`struct drm_crtc *crtc`
:   CRTC to check

`struct drm_atomic_state *state`
:   atomic state object

**Description**

Provides a default CRTC-state check handler for CRTCs that only have
one primary plane attached to it. This is often the case for the CRTC
of simple framebuffers.

**Return**

Zero on success, or an errno code otherwise.

int drm\_crtc\_helper\_set\_config(struct [drm\_mode\_set](drm-kms.html#c.drm_mode_set "drm_mode_set") \*set, struct [drm\_modeset\_acquire\_ctx](drm-kms.html#c.drm_modeset_acquire_ctx "drm_modeset_acquire_ctx") \*ctx)
:   set a new config from userspace

**Parameters**

`struct drm_mode_set *set`
:   mode set configuration

`struct drm_modeset_acquire_ctx *ctx`
:   lock acquire context, not used here

**Description**

The [`drm_crtc_helper_set_config()`](#c.drm_crtc_helper_set_config "drm_crtc_helper_set_config") helper function implements the of
[`drm_crtc_funcs.set_config`](drm-kms.html#c.drm_crtc_funcs "drm_crtc_funcs") callback for drivers using the legacy CRTC
helpers.

It first tries to locate the best encoder for each connector by calling the
connector **drm\_connector\_helper\_funcs.best\_encoder** helper operation.

After locating the appropriate encoders, the helper function will call the
mode\_fixup encoder and CRTC helper operations to adjust the requested mode,
or reject it completely in which case an error will be returned to the
application. If the new configuration after mode adjustment is identical to
the current configuration the helper function will return without performing
any other operation.

If the adjusted mode is identical to the current mode but changes to the
frame buffer need to be applied, the [`drm_crtc_helper_set_config()`](#c.drm_crtc_helper_set_config "drm_crtc_helper_set_config") function
will call the CRTC [`drm_crtc_helper_funcs.mode_set_base`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") helper operation.

If the adjusted mode differs from the current mode, or if the
->`mode_set_base()` helper operation is not provided, the helper function
performs a full mode set sequence by calling the ->`prepare()`, ->`mode_set()`
and ->`commit()` CRTC and encoder helper operations, in that order.
Alternatively it can also use the dpms and disable helper operations. For
details see [`struct drm_crtc_helper_funcs`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") and struct
[`drm_encoder_helper_funcs`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs").

This function is deprecated. New drivers must implement atomic modeset
support, for which this function is unsuitable. Instead drivers should use
[`drm_atomic_helper_set_config()`](#c.drm_atomic_helper_set_config "drm_atomic_helper_set_config").

**Return**

Returns 0 on success, negative errno numbers on failure.

int drm\_helper\_connector\_dpms(struct [drm\_connector](drm-kms.html#c.drm_connector "drm_connector") \*connector, int mode)
:   connector dpms helper implementation

**Parameters**

`struct drm_connector *connector`
:   affected connector

`int mode`
:   DPMS mode

**Description**

The [`drm_helper_connector_dpms()`](#c.drm_helper_connector_dpms "drm_helper_connector_dpms") helper function implements the
[`drm_connector_funcs.dpms`](drm-kms.html#c.drm_connector_funcs "drm_connector_funcs") callback for drivers using the legacy CRTC
helpers.

This is the main helper function provided by the CRTC helper framework for
implementing the DPMS connector attribute. It computes the new desired DPMS
state for all encoders and CRTCs in the output mesh and calls the
[`drm_crtc_helper_funcs.dpms`](#c.drm_crtc_helper_funcs "drm_crtc_helper_funcs") and [`drm_encoder_helper_funcs.dpms`](#c.drm_encoder_helper_funcs "drm_encoder_helper_funcs") callbacks
provided by the driver.

This function is deprecated. New drivers must implement atomic modeset
support, where DPMS is handled in the DRM core.

**Return**

Always returns 0.

void drm\_helper\_resume\_force\_mode(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   force-restore mode setting configuration

**Parameters**

`struct drm_device *dev`
:   drm\_device which should be restored

**Description**

Drivers which use the mode setting helpers can use this function to
force-restore the mode setting configuration e.g. on resume or when something
else might have trampled over the hw state (like some overzealous old BIOSen
tended to do).

This helper doesn’t provide a error return value since restoring the old
config should never fail due to resource allocation issues since the driver
has successfully set the restored configuration already. Hence this should
boil down to the equivalent of a few dpms on calls, which also don’t provide
an error code.

Drivers where simply restoring an old configuration again might fail (e.g.
due to slight differences in allocating shared resources when the
configuration is restored in a different order than when userspace set it up)
need to use their own restore logic.

This function is deprecated. New drivers should implement atomic mode-
setting and use the atomic suspend/resume helpers.

See also:
[`drm_atomic_helper_suspend()`](#c.drm_atomic_helper_suspend "drm_atomic_helper_suspend"), [`drm_atomic_helper_resume()`](#c.drm_atomic_helper_resume "drm_atomic_helper_resume")

int drm\_helper\_force\_disable\_all(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   Forcibly turn off all enabled CRTCs

**Parameters**

`struct drm_device *dev`
:   DRM device whose CRTCs to turn off

**Description**

Drivers may want to call this on unload to ensure that all displays are
unlit and the GPU is in a consistent, low power state. Takes modeset locks.

**Note**

This should only be used by non-atomic legacy drivers. For an atomic
version look at [`drm_atomic_helper_shutdown()`](#c.drm_atomic_helper_shutdown "drm_atomic_helper_shutdown").

**Return**

Zero on success, error code on failure.

## Privacy-screen class

This class allows non KMS drivers, from e.g. drivers/platform/x86 to
register a privacy-screen device, which the KMS drivers can then use
to implement the standard privacy-screen properties, see
[Standard Connector Properties](drm-kms.html#standard-connector-properties).

KMS drivers using a privacy-screen class device are advised to use the
[`drm_connector_attach_privacy_screen_provider()`](drm-kms.html#c.drm_connector_attach_privacy_screen_provider "drm_connector_attach_privacy_screen_provider") and
[`drm_connector_update_privacy_screen()`](drm-kms.html#c.drm_connector_update_privacy_screen "drm_connector_update_privacy_screen") helpers for dealing with this.

struct drm\_privacy\_screen\_ops
:   drm\_privacy\_screen operations

**Definition**:

```
struct drm_privacy_screen_ops {
    int (*set_sw_state)(struct drm_privacy_screen *priv, enum drm_privacy_screen_status sw_state);
    void (*get_hw_state)(struct drm_privacy_screen *priv);
};
```

**Members**

`set_sw_state`
:   Called to request a change of the privacy-screen
    state. The privacy-screen class code contains a check to avoid this
    getting called when the hw\_state reports the state is locked.
    It is the driver’s responsibility to update sw\_state and hw\_state.
    This is always called with the drm\_privacy\_screen’s lock held.

`get_hw_state`
:   Called to request that the driver gets the current
    privacy-screen state from the hardware and then updates sw\_state and
    hw\_state accordingly. This will be called by the core just before
    the privacy-screen is registered in sysfs.

**Description**

Defines the operations which the privacy-screen class code may call.
These functions should be implemented by the privacy-screen driver.

struct drm\_privacy\_screen
:   central privacy-screen structure

**Definition**:

```
struct drm_privacy_screen {
    struct device dev;
    struct mutex lock;
    struct list_head list;
    struct blocking_notifier_head notifier_head;
    const struct drm_privacy_screen_ops *ops;
    enum drm_privacy_screen_status sw_state;
    enum drm_privacy_screen_status hw_state;
    void *drvdata;
};
```

**Members**

`dev`
:   device used to register the privacy-screen in sysfs.

`lock`
:   mutex protection all fields in this struct.

`list`
:   privacy-screen devices list list-entry.

`notifier_head`
:   privacy-screen notifier head.

`ops`
:   [`struct drm_privacy_screen_ops`](#c.drm_privacy_screen_ops "drm_privacy_screen_ops") for this privacy-screen.
    This is NULL if the driver has unregistered the privacy-screen.

`sw_state`
:   The privacy-screen’s software state, see
    [Standard Connector Properties](drm-kms.html#standard-connector-properties)
    for more info.

`hw_state`
:   The privacy-screen’s hardware state, see
    [Standard Connector Properties](drm-kms.html#standard-connector-properties)
    for more info.

`drvdata`
:   Private data owned by the privacy screen provider

**Description**

Central privacy-screen structure, this contains the [`struct device`](../driver-api/infrastructure.html#c.device "device") used
to register the screen in sysfs, the screen’s state, ops, etc.

struct drm\_privacy\_screen\_lookup
:   static privacy-screen lookup list entry

**Definition**:

```
struct drm_privacy_screen_lookup {
    struct list_head list;
    const char *dev_id;
    const char *con_id;
    const char *provider;
};
```

**Members**

`list`
:   Lookup list list-entry.

`dev_id`
:   Consumer device name or NULL to match all devices.

`con_id`
:   Consumer connector name or NULL to match all connectors.

`provider`
:   [`dev_name()`](../driver-api/infrastructure.html#c.dev_name "dev_name") of the privacy\_screen provider.

**Description**

Used for the static lookup-list for mapping privacy-screen consumer
dev-connector pairs to a privacy-screen provider.

void drm\_privacy\_screen\_lookup\_add(struct [drm\_privacy\_screen\_lookup](#c.drm_privacy_screen_lookup "drm_privacy_screen_lookup") \*lookup)
:   add an entry to the static privacy-screen lookup list

**Parameters**

`struct drm_privacy_screen_lookup *lookup`
:   lookup list entry to add

**Description**

Add an entry to the static privacy-screen lookup list. Note the
`struct list_head` which is part of the [`struct drm_privacy_screen_lookup`](#c.drm_privacy_screen_lookup "drm_privacy_screen_lookup")
gets added to a list owned by the privacy-screen core. So the passed in
[`struct drm_privacy_screen_lookup`](#c.drm_privacy_screen_lookup "drm_privacy_screen_lookup") must not be free-ed until it is removed
from the lookup list by calling [`drm_privacy_screen_lookup_remove()`](#c.drm_privacy_screen_lookup_remove "drm_privacy_screen_lookup_remove").

void drm\_privacy\_screen\_lookup\_remove(struct [drm\_privacy\_screen\_lookup](#c.drm_privacy_screen_lookup "drm_privacy_screen_lookup") \*lookup)
:   remove an entry to the static privacy-screen lookup list

**Parameters**

`struct drm_privacy_screen_lookup *lookup`
:   lookup list entry to remove

**Description**

Remove an entry previously added with [`drm_privacy_screen_lookup_add()`](#c.drm_privacy_screen_lookup_add "drm_privacy_screen_lookup_add")
from the static privacy-screen lookup list.

struct [drm\_privacy\_screen](#c.drm_privacy_screen "drm_privacy_screen") \*drm\_privacy\_screen\_get(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, const char \*con\_id)
:   get a privacy-screen provider

**Parameters**

`struct device *dev`
:   consumer-device for which to get a privacy-screen provider

`const char *con_id`
:   (video)connector name for which to get a privacy-screen provider

**Description**

Get a privacy-screen provider for a privacy-screen attached to the
display described by the **dev** and **con\_id** parameters.

**Return**

* A pointer to a [`struct drm_privacy_screen`](#c.drm_privacy_screen "drm_privacy_screen") on success.
* ERR\_PTR(-ENODEV) if no matching privacy-screen is found
* ERR\_PTR(-EPROBE\_DEFER) if there is a matching privacy-screen,
  :   but it has not been registered yet.

void drm\_privacy\_screen\_put(struct [drm\_privacy\_screen](#c.drm_privacy_screen "drm_privacy_screen") \*priv)
:   release a privacy-screen reference

**Parameters**

`struct drm_privacy_screen *priv`
:   privacy screen reference to release

**Description**

Release a privacy-screen provider reference gotten through
[`drm_privacy_screen_get()`](#c.drm_privacy_screen_get "drm_privacy_screen_get"). May be called with a NULL or ERR\_PTR,
in which case it is a no-op.

int drm\_privacy\_screen\_set\_sw\_state(struct [drm\_privacy\_screen](#c.drm_privacy_screen "drm_privacy_screen") \*priv, enum [drm\_privacy\_screen\_status](drm-kms.html#c.drm_privacy_screen_status "drm_privacy_screen_status") sw\_state)
:   set a privacy-screen’s sw-state

**Parameters**

`struct drm_privacy_screen *priv`
:   privacy screen to set the sw-state for

`enum drm_privacy_screen_status sw_state`
:   new sw-state value to set

**Description**

Set the sw-state of a privacy screen. If the privacy-screen is not
in a locked hw-state, then the actual and hw-state of the privacy-screen
will be immediately updated to the new value. If the privacy-screen is
in a locked hw-state, then the new sw-state will be remembered as the
requested state to put the privacy-screen in when it becomes unlocked.

**Return**

0 on success, negative error code on failure.

void drm\_privacy\_screen\_get\_state(struct [drm\_privacy\_screen](#c.drm_privacy_screen "drm_privacy_screen") \*priv, enum [drm\_privacy\_screen\_status](drm-kms.html#c.drm_privacy_screen_status "drm_privacy_screen_status") \*sw\_state\_ret, enum [drm\_privacy\_screen\_status](drm-kms.html#c.drm_privacy_screen_status "drm_privacy_screen_status") \*hw\_state\_ret)
:   get privacy-screen’s current state

**Parameters**

`struct drm_privacy_screen *priv`
:   privacy screen to get the state for

`enum drm_privacy_screen_status *sw_state_ret`
:   address where to store the privacy-screens current sw-state

`enum drm_privacy_screen_status *hw_state_ret`
:   address where to store the privacy-screens current hw-state

**Description**

Get the current state of a privacy-screen, both the sw-state and the
hw-state.

int drm\_privacy\_screen\_register\_notifier(struct [drm\_privacy\_screen](#c.drm_privacy_screen "drm_privacy_screen") \*priv, struct notifier\_block \*nb)
:   register a notifier

**Parameters**

`struct drm_privacy_screen *priv`
:   Privacy screen to register the notifier with

`struct notifier_block *nb`
:   Notifier-block for the notifier to register

**Description**

Register a notifier with the privacy-screen to be notified of changes made
to the privacy-screen state from outside of the privacy-screen class.
E.g. the state may be changed by the hardware itself in response to a
hotkey press.

The notifier is called with no locks held. The new hw\_state and sw\_state
can be retrieved using the [`drm_privacy_screen_get_state()`](#c.drm_privacy_screen_get_state "drm_privacy_screen_get_state") function.
A pointer to the drm\_privacy\_screen’s `struct is` passed as the `void *data`
argument of the notifier\_block’s notifier\_call.

The notifier will NOT be called when changes are made through
[`drm_privacy_screen_set_sw_state()`](#c.drm_privacy_screen_set_sw_state "drm_privacy_screen_set_sw_state"). It is only called for external changes.

**Return**

0 on success, negative error code on failure.

int drm\_privacy\_screen\_unregister\_notifier(struct [drm\_privacy\_screen](#c.drm_privacy_screen "drm_privacy_screen") \*priv, struct notifier\_block \*nb)
:   unregister a notifier

**Parameters**

`struct drm_privacy_screen *priv`
:   Privacy screen to register the notifier with

`struct notifier_block *nb`
:   Notifier-block for the notifier to register

**Description**

Unregister a notifier registered with [`drm_privacy_screen_register_notifier()`](#c.drm_privacy_screen_register_notifier "drm_privacy_screen_register_notifier").

**Return**

0 on success, negative error code on failure.

struct [drm\_privacy\_screen](#c.drm_privacy_screen "drm_privacy_screen") \*drm\_privacy\_screen\_register(struct [device](../driver-api/infrastructure.html#c.device "device") \*parent, const struct [drm\_privacy\_screen\_ops](#c.drm_privacy_screen_ops "drm_privacy_screen_ops") \*ops, void \*data)
:   register a privacy-screen

**Parameters**

`struct device *parent`
:   parent-device for the privacy-screen

`const struct drm_privacy_screen_ops *ops`
:   [`struct drm_privacy_screen_ops`](#c.drm_privacy_screen_ops "drm_privacy_screen_ops") pointer with ops for the privacy-screen

`void *data`
:   Private data owned by the privacy screen provider

**Description**

Create and register a privacy-screen.

**Return**

* A pointer to the created privacy-screen on success.
* An ERR\_PTR(errno) on failure.

void drm\_privacy\_screen\_unregister(struct [drm\_privacy\_screen](#c.drm_privacy_screen "drm_privacy_screen") \*priv)
:   unregister privacy-screen

**Parameters**

`struct drm_privacy_screen *priv`
:   privacy-screen to unregister

**Description**

Unregister a privacy-screen registered with [`drm_privacy_screen_register()`](#c.drm_privacy_screen_register "drm_privacy_screen_register").
May be called with a NULL or ERR\_PTR, in which case it is a no-op.

void drm\_privacy\_screen\_call\_notifier\_chain(struct [drm\_privacy\_screen](#c.drm_privacy_screen "drm_privacy_screen") \*priv)
:   notify consumers of state change

**Parameters**

`struct drm_privacy_screen *priv`
:   Privacy screen to register the notifier with

**Description**

A privacy-screen provider driver can call this functions upon external
changes to the privacy-screen state. E.g. the state may be changed by the
hardware itself in response to a hotkey press.
This function must be called without holding the privacy-screen lock.
the driver must update sw\_state and hw\_state to reflect the new state before
calling this function.
The expected behavior from the driver upon receiving an external state
change event is: 1. Take the lock; 2. Update sw\_state and hw\_state;
3. Release the lock. 4. Call [`drm_privacy_screen_call_notifier_chain()`](#c.drm_privacy_screen_call_notifier_chain "drm_privacy_screen_call_notifier_chain").
