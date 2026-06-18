# Kernel clients

> 출처(원문): https://docs.kernel.org/gpu/drm-client.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel clients

This library provides support for clients running in the kernel like fbdev and bootsplash.

GEM drivers which provide a GEM based dumb buffer with a virtual address are supported.

struct drm\_client\_funcs
:   DRM client callbacks

**Definition**:

```
struct drm_client_funcs {
    struct module *owner;
    void (*free)(struct drm_client_dev *client);
    void (*unregister)(struct drm_client_dev *client);
    int (*restore)(struct drm_client_dev *client, bool force);
    int (*hotplug)(struct drm_client_dev *client);
    int (*suspend)(struct drm_client_dev *client);
    int (*resume)(struct drm_client_dev *client);
};
```

**Members**

`owner`
:   The module owner

`free`
:   Called when the client gets unregistered. Implementations should
    release all client-specific data and free the memory.

    This callback is optional.

`unregister`
:   Called when [`drm_device`](drm-internals.html#c.drm_device "drm_device") is unregistered. The client should respond by
    releasing its resources using [`drm_client_release()`](#c.drm_client_release "drm_client_release").

    This callback is optional.

`restore`
:   Called on `drm_lastclose()`. The first client instance in the list that
    returns zero gets the privilege to restore and no more clients are
    called. This callback is not called after **unregister** has been called.

    Note that the core does not guarantee exclusion against concurrent
    [`drm_open()`](drm-internals.html#c.drm_open "drm_open"). Clients need to ensure this themselves, for example by
    using `drm_master_internal_acquire()` and `drm_master_internal_release()`.

    If the caller passes force, the client should ignore any present DRM
    master and restore the display anyway.

    This callback is optional.

`hotplug`
:   Called on [`drm_kms_helper_hotplug_event()`](drm-kms-helpers.html#c.drm_kms_helper_hotplug_event "drm_kms_helper_hotplug_event").
    This callback is not called after **unregister** has been called.

    This callback is optional.

`suspend`
:   Called when suspending the device.

    This callback is optional.

`resume`
:   Called when resuming the device from suspend.

    This callback is optional.

struct drm\_client\_dev
:   DRM client instance

**Definition**:

```
struct drm_client_dev {
    struct drm_device *dev;
    const char *name;
    struct list_head list;
    const struct drm_client_funcs *funcs;
    struct drm_file *file;
    struct mutex modeset_mutex;
    struct drm_mode_set *modesets;
    bool suspended;
    bool hotplug_pending;
    bool hotplug_failed;
};
```

**Members**

`dev`
:   DRM device

`name`
:   Name of the client.

`list`
:   List of all clients of a DRM device, linked into
    [`drm_device.clientlist`](drm-internals.html#c.drm_device "drm_device"). Protected by [`drm_device.clientlist_mutex`](drm-internals.html#c.drm_device "drm_device").

`funcs`
:   DRM client functions (optional)

`file`
:   DRM file

`modeset_mutex`
:   Protects **modesets**.

`modesets`
:   CRTC configurations

`suspended`
:   The client has been suspended.

`hotplug_pending`
:   A hotplug event has been received while the client was suspended.
    Try again on resume.

`hotplug_failed`
:   Set by client hotplug helpers if the hotplugging failed
    before. It is usually not tried again.

struct drm\_client\_buffer
:   DRM client buffer

**Definition**:

```
struct drm_client_buffer {
    struct drm_client_dev *client;
    struct drm_gem_object *gem;
    struct iosys_map map;
    struct drm_framebuffer *fb;
};
```

**Members**

`client`
:   DRM client

`gem`
:   GEM object backing this buffer

    FIXME: The DRM framebuffer holds a reference on its GEM
    buffer objects. Do not use this field in new code and
    update existing users.

`map`
:   Virtual address for the buffer

`fb`
:   DRM framebuffer

drm\_client\_for\_each\_modeset

`drm_client_for_each_modeset (modeset, client)`

> Iterate over client modesets

**Parameters**

`modeset`
:   [`drm_mode_set`](drm-kms.html#c.drm_mode_set "drm_mode_set") loop cursor

`client`
:   DRM client

drm\_client\_for\_each\_connector\_iter

`drm_client_for_each_connector_iter (connector, iter)`

> connector\_list iterator macro

**Parameters**

`connector`
:   [`struct drm_connector`](drm-kms.html#c.drm_connector "drm_connector") pointer used as cursor

`iter`
:   [`struct drm_connector_list_iter`](drm-kms.html#c.drm_connector_list_iter "drm_connector_list_iter")

**Description**

This iterates the connectors that are useable for internal clients (excludes
writeback connectors).

For more info see [`drm_for_each_connector_iter()`](drm-kms.html#c.drm_for_each_connector_iter "drm_for_each_connector_iter").

int drm\_client\_init(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev, struct [drm\_client\_dev](#c.drm_client_dev "drm_client_dev") \*client, const char \*name, const struct [drm\_client\_funcs](#c.drm_client_funcs "drm_client_funcs") \*funcs)
:   Initialise a DRM client

**Parameters**

`struct drm_device *dev`
:   DRM device

`struct drm_client_dev *client`
:   DRM client

`const char *name`
:   Client name

`const struct drm_client_funcs *funcs`
:   DRM client functions (optional)

**Description**

This initialises the client and opens a [`drm_file`](drm-internals.html#c.drm_file "drm_file").
Use [`drm_client_register()`](#c.drm_client_register "drm_client_register") to complete the process.
The caller needs to hold a reference on **dev** before calling this function.
The client is freed when the [`drm_device`](drm-internals.html#c.drm_device "drm_device") is unregistered. See [`drm_client_release()`](#c.drm_client_release "drm_client_release").

**Return**

Zero on success or negative error code on failure.

void drm\_client\_register(struct [drm\_client\_dev](#c.drm_client_dev "drm_client_dev") \*client)
:   Register client

**Parameters**

`struct drm_client_dev *client`
:   DRM client

**Description**

Add the client to the [`drm_device`](drm-internals.html#c.drm_device "drm_device") client list to activate its callbacks.
**client** must be initialized by a call to [`drm_client_init()`](#c.drm_client_init "drm_client_init"). After
[`drm_client_register()`](#c.drm_client_register "drm_client_register") it is no longer permissible to call [`drm_client_release()`](#c.drm_client_release "drm_client_release")
directly (outside the unregister callback), instead cleanup will happen
automatically on driver unload.

Registering a client generates a hotplug event that allows the client
to set up its display from pre-existing outputs. The client must have
initialized its state to able to handle the hotplug event successfully.

void drm\_client\_release(struct [drm\_client\_dev](#c.drm_client_dev "drm_client_dev") \*client)
:   Release DRM client resources

**Parameters**

`struct drm_client_dev *client`
:   DRM client

**Description**

Releases resources by closing the [`drm_file`](drm-internals.html#c.drm_file "drm_file") that was opened by [`drm_client_init()`](#c.drm_client_init "drm_client_init").
It is called automatically if the [`drm_client_funcs.unregister`](#c.drm_client_funcs "drm_client_funcs") callback is \_not\_ set.

This function should only be called from the unregister callback. An exception
is fbdev which cannot free the buffer if userspace has open file descriptors.

**Note**

Clients cannot initiate a release by themselves. This is done to keep the code simple.
The driver has to be unloaded before the client can be unloaded.

void drm\_client\_buffer\_delete(struct [drm\_client\_buffer](#c.drm_client_buffer "drm_client_buffer") \*buffer)
:   Delete a client buffer

**Parameters**

`struct drm_client_buffer *buffer`
:   DRM client buffer

int drm\_client\_buffer\_vmap\_local(struct [drm\_client\_buffer](#c.drm_client_buffer "drm_client_buffer") \*buffer, struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*map\_copy)
:   Map DRM client buffer into address space

**Parameters**

`struct drm_client_buffer *buffer`
:   DRM client buffer

`struct iosys_map *map_copy`
:   Returns the mapped memory’s address

**Description**

This function maps a client buffer into kernel address space. If the
buffer is already mapped, it returns the existing mapping’s address.

Client buffer mappings are not ref’counted. Each call to
[`drm_client_buffer_vmap_local()`](#c.drm_client_buffer_vmap_local "drm_client_buffer_vmap_local") should be closely followed by a call to
[`drm_client_buffer_vunmap_local()`](#c.drm_client_buffer_vunmap_local "drm_client_buffer_vunmap_local"). See [`drm_client_buffer_vmap()`](#c.drm_client_buffer_vmap "drm_client_buffer_vmap") for
long-term mappings.

The returned address is a copy of the internal value. In contrast to
other vmap interfaces, you don’t need it for the client’s vunmap
function. So you can modify it at will during blit and draw operations.

**Return**

0 on success, or a negative errno code otherwise.

void drm\_client\_buffer\_vunmap\_local(struct [drm\_client\_buffer](#c.drm_client_buffer "drm_client_buffer") \*buffer)
:   Unmap DRM client buffer

**Parameters**

`struct drm_client_buffer *buffer`
:   DRM client buffer

**Description**

This function removes a client buffer’s memory mapping established
with [`drm_client_buffer_vunmap_local()`](#c.drm_client_buffer_vunmap_local "drm_client_buffer_vunmap_local"). Calling this function is only
required by clients that manage their buffer mappings by themselves.

int drm\_client\_buffer\_vmap(struct [drm\_client\_buffer](#c.drm_client_buffer "drm_client_buffer") \*buffer, struct [iosys\_map](../driver-api/device-io.html#c.iosys_map "iosys_map") \*map\_copy)
:   Map DRM client buffer into address space

**Parameters**

`struct drm_client_buffer *buffer`
:   DRM client buffer

`struct iosys_map *map_copy`
:   Returns the mapped memory’s address

**Description**

This function maps a client buffer into kernel address space. If the
buffer is already mapped, it returns the existing mapping’s address.

Client buffer mappings are not ref’counted. Each call to
[`drm_client_buffer_vmap()`](#c.drm_client_buffer_vmap "drm_client_buffer_vmap") should be followed by a call to
[`drm_client_buffer_vunmap()`](#c.drm_client_buffer_vunmap "drm_client_buffer_vunmap"); or the client buffer should be mapped
throughout its lifetime.

The returned address is a copy of the internal value. In contrast to
other vmap interfaces, you don’t need it for the client’s vunmap
function. So you can modify it at will during blit and draw operations.

**Return**

0 on success, or a negative errno code otherwise.

void drm\_client\_buffer\_vunmap(struct [drm\_client\_buffer](#c.drm_client_buffer "drm_client_buffer") \*buffer)
:   Unmap DRM client buffer

**Parameters**

`struct drm_client_buffer *buffer`
:   DRM client buffer

**Description**

This function removes a client buffer’s memory mapping. Calling this
function is only required by clients that manage their buffer mappings
by themselves.

struct [drm\_client\_buffer](#c.drm_client_buffer "drm_client_buffer") \*drm\_client\_buffer\_create\_dumb(struct [drm\_client\_dev](#c.drm_client_dev "drm_client_dev") \*client, u32 width, u32 height, u32 format)
:   Create a client buffer backed by a dumb buffer

**Parameters**

`struct drm_client_dev *client`
:   DRM client

`u32 width`
:   Framebuffer width

`u32 height`
:   Framebuffer height

`u32 format`
:   Buffer format

**Description**

This function creates a [`drm_client_buffer`](#c.drm_client_buffer "drm_client_buffer") which consists of a
[`drm_framebuffer`](drm-kms.html#c.drm_framebuffer "drm_framebuffer") backed by a dumb buffer.
Call [`drm_client_buffer_delete()`](#c.drm_client_buffer_delete "drm_client_buffer_delete") to free the buffer.

**Return**

Pointer to a client buffer or an error pointer on failure.

int drm\_client\_buffer\_flush(struct [drm\_client\_buffer](#c.drm_client_buffer "drm_client_buffer") \*buffer, struct [drm\_rect](drm-kms-helpers.html#c.drm_rect "drm_rect") \*rect)
:   Manually flush client buffer

**Parameters**

`struct drm_client_buffer *buffer`
:   DRM client buffer

`struct drm_rect *rect`
:   Damage rectangle (if NULL flushes all)

**Description**

This calls [`drm_framebuffer_funcs->dirty`](drm-kms.html#c.drm_framebuffer_funcs "drm_framebuffer_funcs") (if present) to flush buffer changes
for drivers that need it.

**Return**

Zero on success or negative error code on failure.

int drm\_client\_modeset\_probe(struct [drm\_client\_dev](#c.drm_client_dev "drm_client_dev") \*client, unsigned int width, unsigned int height)
:   Probe for displays

**Parameters**

`struct drm_client_dev *client`
:   DRM client

`unsigned int width`
:   Maximum display mode width (optional)

`unsigned int height`
:   Maximum display mode height (optional)

**Description**

This function sets up display pipelines for enabled connectors and stores the
config in the client’s modeset array.

**Return**

Zero on success or negative error code on failure.

bool drm\_client\_rotation(struct [drm\_mode\_set](drm-kms.html#c.drm_mode_set "drm_mode_set") \*modeset, unsigned int \*rotation)
:   Check the initial rotation value

**Parameters**

`struct drm_mode_set *modeset`
:   DRM modeset

`unsigned int *rotation`
:   Returned rotation value

**Description**

This function checks if the primary plane in **modeset** can hw rotate
to match the rotation needed on its connector.

**Note**

Currently only 0 and 180 degrees are supported.

**Return**

True if the plane can do the rotation, false otherwise.

int drm\_client\_modeset\_check(struct [drm\_client\_dev](#c.drm_client_dev "drm_client_dev") \*client)
:   Check modeset configuration

**Parameters**

`struct drm_client_dev *client`
:   DRM client

**Description**

Check modeset configuration.

**Return**

Zero on success or negative error code on failure.

int drm\_client\_modeset\_commit\_locked(struct [drm\_client\_dev](#c.drm_client_dev "drm_client_dev") \*client)
:   Force commit CRTC configuration

**Parameters**

`struct drm_client_dev *client`
:   DRM client

**Description**

Commit modeset configuration to crtcs without checking if there is a DRM
master. The assumption is that the caller already holds an internal DRM
master reference acquired with `drm_master_internal_acquire()`.

**Return**

Zero on success or negative error code on failure.

int drm\_client\_modeset\_commit(struct [drm\_client\_dev](#c.drm_client_dev "drm_client_dev") \*client)
:   Commit CRTC configuration

**Parameters**

`struct drm_client_dev *client`
:   DRM client

**Description**

Commit modeset configuration to crtcs.

**Return**

Zero on success or negative error code on failure.

int drm\_client\_modeset\_dpms(struct [drm\_client\_dev](#c.drm_client_dev "drm_client_dev") \*client, int mode)
:   Set DPMS mode

**Parameters**

`struct drm_client_dev *client`
:   DRM client

`int mode`
:   DPMS mode

**Note**

For atomic drivers **mode** is reduced to on/off.

**Return**

Zero on success or negative error code on failure.

int drm\_client\_modeset\_wait\_for\_vblank(struct [drm\_client\_dev](#c.drm_client_dev "drm_client_dev") \*client, unsigned int crtc\_index)
:   Wait for the next VBLANK to occur

**Parameters**

`struct drm_client_dev *client`
:   DRM client

`unsigned int crtc_index`
:   The ndex of the CRTC to wait on

**Description**

Block the caller until the given CRTC has seen a VBLANK. Do nothing
if the CRTC is disabled. If there’s another DRM master present, fail
with -EBUSY.

**Return**

0 on success, or negative error code otherwise.

void drm\_client\_dev\_unregister(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   Unregister clients

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

This function releases all clients by calling each client’s
[`drm_client_funcs.unregister`](#c.drm_client_funcs "drm_client_funcs") callback. The callback function
is responsibe for releaseing all resources including the client
itself.

The helper [`drm_dev_unregister()`](drm-internals.html#c.drm_dev_unregister "drm_dev_unregister") calls this function. Drivers
that use it don’t need to call this function themselves.

void drm\_client\_dev\_hotplug(struct [drm\_device](drm-internals.html#c.drm_device "drm_device") \*dev)
:   Send hotplug event to clients

**Parameters**

`struct drm_device *dev`
:   DRM device

**Description**

This function calls the [`drm_client_funcs.hotplug`](#c.drm_client_funcs "drm_client_funcs") callback on the attached clients.

[`drm_kms_helper_hotplug_event()`](drm-kms-helpers.html#c.drm_kms_helper_hotplug_event "drm_kms_helper_hotplug_event") calls this function, so drivers that use it
don’t need to call this function themselves.
