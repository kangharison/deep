# TODO list

> 출처(원문): https://docs.kernel.org/gpu/todo.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# TODO list

This section contains a list of smaller janitorial tasks in the kernel DRM
graphics subsystem useful as newbie projects. Or for slow rainy days.

## Difficulty

To make it easier task are categorized into different levels:

Starter: Good tasks to get started with the DRM subsystem.

Intermediate: Tasks which need some experience with working in the DRM
subsystem, or some specific GPU/display graphics knowledge. For debugging issue
it’s good to have the relevant hardware (or a virtual driver set up) available
for testing.

Advanced: Tricky tasks that need fairly good understanding of the DRM subsystem
and graphics topics. Generally need the relevant hardware for development and
testing.

Expert: Only attempt these if you’ve successfully completed some tricky
refactorings already and are an expert in the specific area

### Subsystem-wide refactorings

## Open-code drm\_simple\_encoder\_init()

The helper `drm_simple_encoder_init()` was supposed to simplify encoder
initialization. Instead it only added an intermediate layer between atomic
modesetting and the DRM driver.

The task here is to remove `drm_simple_encoder_init()`. Search for a driver
that calls `drm_simple_encoder_init()` and inline the helper. The driver will
also need its own instance of drm\_encoder\_funcs.

Contact: Thomas Zimmermann, respective driver maintainer

Level: Easy

## Replace struct drm\_simple\_display\_pipe with regular atomic helpers

The data type `struct drm_simple_display_pipe` and its helpers were supposed
to simplify driver development. Instead they only added an intermediate layer
between atomic modesetting and the DRM driver.

There are still drivers that use drm\_simple\_display\_pipe. The task here is to
convert them to use regular atomic helpers. Search for a driver that calls
`drm_simple_display_pipe_init()` and inline all helpers from drm\_simple\_kms\_helper.c
into the driver, such that no simple-KMS interfaces are required. Please also
rename all inlined fucntions according to driver conventions.

Contact: Thomas Zimmermann, respective driver maintainer

Level: Easy

## Remove custom dumb\_map\_offset implementations

All GEM based drivers should be using [`drm_gem_create_mmap_offset()`](drm-mm.html#c.drm_gem_create_mmap_offset "drm_gem_create_mmap_offset") instead.
Audit each individual driver, make sure it’ll work with the generic
implementation (there’s lots of outdated locking leftovers in various
implementations), and then remove it.

Contact: Simona Vetter, respective driver maintainers

Level: Intermediate

## Convert existing KMS drivers to atomic modesetting

3.19 has the atomic modeset interfaces and helpers, so drivers can now be
converted over. Modern compositors like Wayland or Surfaceflinger on Android
really want an atomic modeset interface, so this is all about the bright
future.

There is a conversion guide for atomic [[1]](#id4) and all you need is a GPU for a
non-converted driver. The “Atomic mode setting design overview” series [[2]](#id5)
[[3]](#id6) at LWN.net can also be helpful.

As part of this drivers also need to convert to universal plane (which means
exposing primary & cursor as proper plane objects). But that’s much easier to
do by directly using the new atomic helper driver callbacks.

> [[1](#id1)]
>
> <https://blog.ffwll.ch/2014/11/atomic-modeset-support-for-kms-drivers.html>
>
>
> [[2](#id2)]
>
> <https://lwn.net/Articles/653071/>
>
>
> [[3](#id3)]
>
> <https://lwn.net/Articles/653466/>

Contact: Simona Vetter, respective driver maintainers

Level: Advanced

## Clean up the clipped coordination confusion around planes

We have a helper to get this right with `drm_plane_helper_check_update()`, but
it’s not consistently used. This should be fixed, preferably in the atomic
helpers (and drivers then moved over to clipped coordinates). Probably the
helper should also be moved from drm\_plane\_helper.c to the atomic helpers, to
avoid confusion - the other helpers in that file are all deprecated legacy
helpers.

Contact: Ville Syrjälä, Simona Vetter, driver maintainers

Level: Advanced

## Improve plane atomic\_check helpers

Aside from the clipped coordinates right above there’s a few suboptimal things
with the current helpers:

* drm\_plane\_helper\_funcs->atomic\_check gets called for enabled or disabled
  planes. At best this seems to confuse drivers, worst it means they blow up
  when the plane is disabled without the CRTC. The only special handling is
  resetting values in the plane state structures, which instead should be moved
  into the drm\_plane\_funcs->atomic\_duplicate\_state functions.
* Once that’s done, helpers could stop calling ->atomic\_check for disabled
  planes.
* Then we could go through all the drivers and remove the more-or-less confused
  checks for plane\_state->fb and plane\_state->crtc.

Contact: Simona Vetter

Level: Advanced

## Convert early atomic drivers to async commit helpers

For the first year the atomic modeset helpers didn’t support asynchronous /
nonblocking commits, and every driver had to hand-roll them. This is fixed
now, but there’s still a pile of existing drivers that easily could be
converted over to the new infrastructure.

One issue with the helpers is that they require that drivers handle completion
events for atomic commits correctly. But fixing these bugs is good anyway.

Somewhat related is the legacy\_cursor\_update hack, which should be replaced with
the new atomic\_async\_check/commit functionality in the helpers in drivers that
still look at that flag.

Contact: Simona Vetter, respective driver maintainers

Level: Advanced

## Rename drm\_atomic\_state

The KMS framework uses two slightly different definitions for the `state`
concept. For a given object (plane, CRTC, encoder, etc., so
`drm_$OBJECT_state`), the state is the entire state of that object. However,
at the device level, `drm_atomic_state` refers to a state update for a
limited number of objects.

The state isn’t the entire device state, but only the full state of some
objects in that device. This is confusing to newcomers, and
`drm_atomic_state` should be renamed to something clearer like
`drm_atomic_commit`.

In addition to renaming the structure itself, it would also imply renaming some
related functions (`drm_atomic_state_alloc`, `drm_atomic_state_get`,
`drm_atomic_state_put`, `drm_atomic_state_init`,
`__drm_atomic_state_free`, etc.).

Contact: Maxime Ripard <[mripard@kernel.org](mailto:mripard%40kernel.org)>

Level: Advanced

## Fallout from atomic KMS

`drm_atomic_helper.c` provides a batch of functions which implement legacy
IOCTLs on top of the new atomic driver interface. Which is really nice for
gradual conversion of drivers, but unfortunately the semantic mismatches are
a bit too severe. So there’s some follow-up work to adjust the function
interfaces to fix these issues:

* atomic needs the lock acquire context. At the moment that’s passed around
  implicitly with some horrible hacks, and it’s also allocate with
  `GFP_NOFAIL` behind the scenes. All legacy paths need to start allocating
  the acquire context explicitly on stack and then also pass it down into
  drivers explicitly so that the legacy-on-atomic functions can use them.

  Except for some driver code this is done. This task should be finished by
  adding WARN\_ON(!drm\_drv\_uses\_atomic\_modeset) in [`drm_modeset_lock_all()`](drm-kms.html#c.drm_modeset_lock_all "drm_modeset_lock_all").
* A bunch of the vtable hooks are now in the wrong place: DRM has a split
  between core vfunc tables (named `drm_foo_funcs`), which are used to
  implement the userspace ABI. And then there’s the optional hooks for the
  helper libraries (name `drm_foo_helper_funcs`), which are purely for
  internal use. Some of these hooks should be move from `_funcs` to
  `_helper_funcs` since they are not part of the core ABI. There’s a
  `FIXME` comment in the kerneldoc for each such case in `drm_crtc.h`.

Contact: Simona Vetter

Level: Intermediate

## Move Buffer Object Locking to dma\_resv\_lock()

Many drivers have their own per-object locking scheme, usually using
[`mutex_lock()`](../kernel-hacking/locking.html#c.mutex_lock "mutex_lock"). This causes all kinds of trouble for buffer sharing, since
depending which driver is the exporter and importer, the locking hierarchy is
reversed.

To solve this we need one standard per-object locking mechanism, which is
[`dma_resv_lock()`](../driver-api/dma-buf.html#c.dma_resv_lock "dma_resv_lock"). This lock needs to be called as the outermost lock, with all
other driver specific per-object locks removed. The problem is that rolling out
the actual change to the locking contract is a flag day, due to [`struct dma_buf`](../driver-api/dma-buf.html#c.dma_buf "dma_buf")
buffer sharing.

Level: Expert

## Convert logging to drm\_\* functions with drm\_device parameter

For drivers which could have multiple instances, it is necessary to
differentiate between which is which in the logs. Since DRM\_INFO/WARN/ERROR
don’t do this, drivers used dev\_info/warn/err to make this differentiation. We
now have drm\_\* variants of the drm print functions, so we can start to convert
those drivers back to using drm-formatted specific log messages.

Before you start this conversion please contact the relevant maintainers to make
sure your work will be merged - not everyone agrees that the DRM dmesg macros
are better.

Contact: Sean Paul, Maintainer of the driver you plan to convert

Level: Starter

## Convert drivers to use simple modeset suspend/resume

Most drivers (except i915 and nouveau) that use
drm\_atomic\_helper\_suspend/`resume()` can probably be converted to use
drm\_mode\_config\_helper\_suspend/`resume()`. Also there’s still open-coded version
of the atomic suspend/resume code in older atomic modeset drivers.

Contact: Maintainer of the driver you plan to convert

Level: Intermediate

## Reimplement functions in drm\_fbdev\_fb\_ops without fbdev

A number of callback functions in drm\_fbdev\_fb\_ops could benefit from
being rewritten without dependencies on the fbdev module. Some of the
helpers could further benefit from using [`struct iosys_map`](../driver-api/device-io.html#c.iosys_map "iosys_map") instead of
raw pointers.

Contact: Thomas Zimmermann <[tzimmermann@suse.de](mailto:tzimmermann%40suse.de)>, Simona Vetter

Level: Advanced

## Benchmark and optimize blitting and format-conversion function

Drawing to display memory quickly is crucial for many applications’
performance.

On at least x86-64, `sys_imageblit()` is significantly slower than
`cfb_imageblit()`, even though both use the same blitting algorithm and
the latter is written for I/O memory. It turns out that `cfb_imageblit()`
uses movl instructions, while sys\_imageblit apparently does not. This
seems to be a problem with gcc’s optimizer. DRM’s format-conversion
helpers might be subject to similar issues.

Benchmark and optimize fbdev’s `sys_()` helpers and DRM’s format-conversion
helpers. In cases that can be further optimized, maybe implement a different
algorithm. For micro-optimizations, use movl/movq instructions explicitly.
That might possibly require architecture-specific helpers (e.g., `storel()`
`storeq()`).

Contact: Thomas Zimmermann <[tzimmermann@suse.de](mailto:tzimmermann%40suse.de)>

Level: Intermediate

## drm\_framebuffer\_funcs and drm\_mode\_config\_funcs.fb\_create cleanup

A lot more drivers could be switched over to the drm\_gem\_framebuffer helpers.
Various hold-ups:

* Need to switch over to the generic dirty tracking code using
  drm\_atomic\_helper\_dirtyfb first (e.g. qxl).
* Need to switch to `drm_fbdev_generic_setup()`, otherwise a lot of the custom fb
  setup code can’t be deleted.
* Need to switch to [`drm_gem_fb_create()`](drm-kms-helpers.html#c.drm_gem_fb_create "drm_gem_fb_create"), as now [`drm_gem_fb_create()`](drm-kms-helpers.html#c.drm_gem_fb_create "drm_gem_fb_create") checks for
  valid formats for atomic drivers.
* Many drivers subclass drm\_framebuffer, we’d need a embedding compatible
  version of the varios drm\_gem\_fb\_create functions. Maybe called
  drm\_gem\_fb\_create/\_with\_dirty/\_with\_funcs as needed.

Contact: Simona Vetter

Level: Intermediate

## Generic fbdev defio support

The defio support code in the fbdev core has some very specific requirements,
which means drivers need to have a special framebuffer for fbdev. The main
issue is that it uses some fields in `struct page` itself, which breaks shmem
gem objects (and other things). To support defio, affected drivers require
the use of a shadow buffer, which may add CPU and memory overhead.

Possible solution would be to write our own defio mmap code in the drm fbdev
emulation. It would need to fully wrap the existing mmap ops, forwarding
everything after it has done the write-protect/mkwrite trickery:

* In the drm\_fbdev\_fb\_mmap helper, if we need defio, change the
  default page prots to write-protected with something like this:

  ```
  vma->vm_page_prot = pgprot_wrprotect(vma->vm_page_prot);
  ```
* Set the mkwrite and fsync callbacks with similar implementions to the core
  fbdev defio stuff. These should all work on plain ptes, they don’t actually
  require a `struct page`. uff. These should all work on plain ptes, they don’t
  actually require a `struct page`.
* Track the dirty pages in a separate structure (bitfield with one bit per page
  should work) to avoid clobbering `struct page`.

Might be good to also have some igt testcases for this.

Contact: Simona Vetter, Noralf Tronnes

Level: Advanced

## connector register/unregister fixes

* For most connectors it’s a no-op to call drm\_connector\_register/unregister
  directly from driver code, drm\_dev\_register/unregister take care of this
  already. We can remove all of them.
* For dp drivers it’s a bit more a mess, since we need the connector to be
  registered when calling drm\_dp\_aux\_register. Fix this by instead calling
  drm\_dp\_aux\_init, and moving the actual registering into a late\_register
  callback as recommended in the kerneldoc.

Level: Intermediate

## Remove load/unload callbacks

The load/unload callbacks in struct &drm\_driver are very much midlayers, plus
for historical reasons they get the ordering wrong (and we can’t fix that)
between setting up the &drm\_driver structure and calling [`drm_dev_register()`](drm-internals.html#c.drm_dev_register "drm_dev_register").

* Rework drivers to no longer use the load/unload callbacks, directly coding the
  load/unload sequence into the driver’s probe function.
* Once all drivers are converted, remove the load/unload callbacks.

Contact: Simona Vetter

Level: Intermediate

## Replace drm\_detect\_hdmi\_monitor() with drm\_display\_info.is\_hdmi

Once EDID is parsed, the monitor HDMI support information is available through
drm\_display\_info.is\_hdmi. Many drivers still call [`drm_detect_hdmi_monitor()`](drm-kms-helpers.html#c.drm_detect_hdmi_monitor "drm_detect_hdmi_monitor") to
retrieve the same information, which is less efficient.

Audit each individual driver calling [`drm_detect_hdmi_monitor()`](drm-kms-helpers.html#c.drm_detect_hdmi_monitor "drm_detect_hdmi_monitor") and switch to
drm\_display\_info.is\_hdmi if applicable.

Contact: Laurent Pinchart, respective driver maintainers

Level: Intermediate

## Consolidate custom driver modeset properties

Before atomic modeset took place, many drivers where creating their own
properties. Among other things, atomic brought the requirement that custom,
driver specific properties should not be used.

For this task, we aim to introduce core helpers or reuse the existing ones
if available:

A quick, unconfirmed, examples list.

Introduce core helpers:
- audio (amdgpu, intel, gma500, radeon)
- brightness, contrast, etc (armada, nouveau) - overlay only (?)
- broadcast rgb (gma500, intel)
- colorkey (armada, nouveau, rcar) - overlay only (?)
- dither (amdgpu, nouveau, radeon) - varies across drivers
- underscan family (amdgpu, radeon, nouveau)

Already in core:
- colorspace (sti)
- tv format names, enhancements (gma500, intel)
- tv overscan, margins, etc. (gma500, intel)
- zorder (omapdrm) - same as zpos (?)

Contact: Emil Velikov, respective driver maintainers

Level: Intermediate

## Use struct iosys\_map throughout codebase

Pointers to shared device memory are stored in [`struct iosys_map`](../driver-api/device-io.html#c.iosys_map "iosys_map"). Each
instance knows whether it refers to system or I/O memory. Most of the DRM-wide
interface have been converted to use [`struct iosys_map`](../driver-api/device-io.html#c.iosys_map "iosys_map"), but implementations
often still use raw pointers.

The task is to use [`struct iosys_map`](../driver-api/device-io.html#c.iosys_map "iosys_map") where it makes sense.

* Memory managers should use [`struct iosys_map`](../driver-api/device-io.html#c.iosys_map "iosys_map") for dma-buf-imported buffers.
* TTM might benefit from using [`struct iosys_map`](../driver-api/device-io.html#c.iosys_map "iosys_map") internally.
* Framebuffer copying and blitting helpers should operate on [`struct iosys_map`](../driver-api/device-io.html#c.iosys_map "iosys_map").

Contact: Thomas Zimmermann <[tzimmermann@suse.de](mailto:tzimmermann%40suse.de)>, Christian König, Simona Vetter

Level: Intermediate

## Review all drivers for setting struct drm\_mode\_config.{max\_width,max\_height} correctly

The values in [`struct drm_mode_config`](drm-kms.html#c.drm_mode_config "drm_mode_config").{max\_width,max\_height} describe the
maximum supported framebuffer size. It’s the virtual screen size, but many
drivers treat it like limitations of the physical resolution.

The maximum width depends on the hardware’s maximum scanline pitch. The
maximum height depends on the amount of addressable video memory. Review all
drivers to initialize the fields to the correct values.

Contact: Thomas Zimmermann <[tzimmermann@suse.de](mailto:tzimmermann%40suse.de)>

Level: Intermediate

## Request memory regions in all fbdev drivers

Old/ancient fbdev drivers do not request their memory properly.
Go through these drivers and add code to request the memory regions
that the driver uses. This requires adding calls to `request_mem_region()`,
[`pci_request_region()`](../driver-api/pci/pci.html#c.pci_request_region "pci_request_region") or similar functions. Use helpers for managed cleanup
where possible. Problematic areas include hardware that has exclusive ranges
like VGA. VGA16fb does not request the range as it is expected.
Drivers are pretty bad at doing this and there used to be conflicts among
DRM and fbdev drivers. Still, it’s the correct thing to do.

Contact: Thomas Zimmermann <[tzimmermann@suse.de](mailto:tzimmermann%40suse.de)>

Level: Starter

## Remove driver dependencies on FB\_DEVICE

A number of fbdev drivers provide attributes via sysfs and therefore depend
on CONFIG\_FB\_DEVICE to be selected. Review each driver and attempt to make
any dependencies on CONFIG\_FB\_DEVICE optional. At the minimum, the respective
code in the driver could be conditionalized via ifdef CONFIG\_FB\_DEVICE. Not
all drivers might be able to drop CONFIG\_FB\_DEVICE.

Contact: Thomas Zimmermann <[tzimmermann@suse.de](mailto:tzimmermann%40suse.de)>

Level: Starter

## Remove disable/unprepare in remove/shutdown in panel-simple and panel-edp

As of [commit d2aacaf07395](https://git.kernel.org/torvalds/c/d2aacaf07395) (“drm/panel: Check for already prepared/enabled in
drm\_panel”), we have a check in the drm\_panel core to make sure nobody
double-calls prepare/enable/disable/unprepare. Eventually that should probably
be turned into a `WARN_ON()` or somehow made louder.

At the moment, we expect that we may still encounter the warnings in the
drm\_panel core when using panel-simple and panel-edp. Since those panel
drivers are used with a lot of different DRM modeset drivers they still
make an extra effort to disable/unprepare the panel themsevles at shutdown
time. Specifically we could still encounter those warnings if the panel
driver gets `shutdown()` \_before\_ the DRM modeset driver and the DRM modeset
driver properly calls [`drm_atomic_helper_shutdown()`](drm-kms-helpers.html#c.drm_atomic_helper_shutdown "drm_atomic_helper_shutdown") in its own `shutdown()`
callback. Warnings could be avoided in such a case by using something like
device links to ensure that the panel gets `shutdown()` after the DRM modeset
driver.

Once all DRM modeset drivers are known to shutdown properly, the extra
calls to disable/unprepare in remove/shutdown in panel-simple and panel-edp
should be removed and this TODO item marked complete.

Contact: Douglas Anderson <[dianders@chromium.org](mailto:dianders%40chromium.org)>

Level: Intermediate

## Transition away from using deprecated MIPI DSI functions

There are many functions defined in `drm_mipi_dsi.c` which have been
deprecated. Each deprecated function was deprecated in favor of its multi
variant (e.g. [`mipi_dsi_generic_write()`](drm-kms-helpers.html#c.mipi_dsi_generic_write "mipi_dsi_generic_write") and [`mipi_dsi_generic_write_multi()`](drm-kms-helpers.html#c.mipi_dsi_generic_write_multi "mipi_dsi_generic_write_multi")).
The multi variant of a function includes improved error handling and logic
which makes it more convenient to make several calls in a row, as most MIPI
drivers do.

Drivers should be updated to use undeprecated functions. Once all usages of the
deprecated MIPI DSI functions have been removed, their definitions may be
removed from `drm_mipi_dsi.c`.

Contact: Douglas Anderson <[dianders@chromium.org](mailto:dianders%40chromium.org)>

Level: Starter

## Remove devm\_drm\_put\_bridge()

Due to how the panel bridge handles the drm\_bridge object lifetime, special
care must be taken to dispose of the drm\_bridge object when the
panel\_bridge is removed. This is currently managed using
[`devm_drm_put_bridge()`](drm-kms-helpers.html#c.devm_drm_put_bridge "devm_drm_put_bridge"), but that is an unsafe, temporary workaround. To fix
that, the DRM panel lifetime needs to be reworked. After the rework is
done, remove [`devm_drm_put_bridge()`](drm-kms-helpers.html#c.devm_drm_put_bridge "devm_drm_put_bridge") and the TODO in
[`drm_panel_bridge_remove()`](drm-kms-helpers.html#c.drm_panel_bridge_remove "drm_panel_bridge_remove").

Contact: Maxime Ripard <[mripard@kernel.org](mailto:mripard%40kernel.org)>,
:   Luca Ceresoli <[luca.ceresoli@bootlin.com](mailto:luca.ceresoli%40bootlin.com)>

Level: Intermediate

## Convert users of of\_drm\_find\_bridge() to of\_drm\_find\_and\_get\_bridge()

Taking a [`struct drm_bridge`](drm-kms-helpers.html#c.drm_bridge "drm_bridge") pointer requires getting a reference and putting
it after disposing of the pointer. Most functions returning a [`struct
drm_bridge`](drm-kms-helpers.html#c.drm_bridge "drm_bridge") pointer already call [`drm_bridge_get()`](drm-kms-helpers.html#c.drm_bridge_get "drm_bridge_get") to increment the refcount
and their users have been updated to call [`drm_bridge_put()`](drm-kms-helpers.html#c.drm_bridge_put "drm_bridge_put") when
appropriate. [`of_drm_find_bridge()`](drm-kms-helpers.html#c.of_drm_find_bridge "of_drm_find_bridge") does not get a reference and it has been
deprecated in favor of [`of_drm_find_and_get_bridge()`](drm-kms-helpers.html#c.of_drm_find_and_get_bridge "of_drm_find_and_get_bridge") which does, but some
users still need to be converted.

Contact: Maxime Ripard <[mripard@kernel.org](mailto:mripard%40kernel.org)>,
:   Luca Ceresoli <[luca.ceresoli@bootlin.com](mailto:luca.ceresoli%40bootlin.com)>

Level: Intermediate

### Core refactorings

## Make panic handling work

This is a really varied tasks with lots of little bits and pieces:

* The panic path can’t be tested currently, leading to constant breaking. The
  main issue here is that panics can be triggered from hardirq contexts and
  hence all panic related callback can run in hardirq context. It would be
  awesome if we could test at least the fbdev helper code and driver code by
  e.g. trigger calls through drm debugfs files. hardirq context could be
  achieved by using an IPI to the local processor.
* There’s a massive confusion of different panic handlers. DRM fbdev emulation
  helpers had their own (long removed), but on top of that the fbcon code itself
  also has one. We need to make sure that they stop fighting over each other.
  This is worked around by checking `oops_in_progress` at various entry points
  into the DRM fbdev emulation helpers. A much cleaner approach here would be to
  switch fbcon to the [threaded printk support](https://lwn.net/Articles/800946/).
* `drm_can_sleep()` is a mess. It hides real bugs in normal operations and
  isn’t a full solution for panic paths. We need to make sure that it only
  returns true if there’s a panic going on for real, and fix up all the
  fallout.
* The panic handler must never sleep, which also means it can’t ever
  `mutex_lock()`. Also it can’t grab any other lock unconditionally, not
  even spinlocks (because NMI and hardirq can panic too). We need to either
  make sure to not call such paths, or trylock everything. Really tricky.
* A clean solution would be an entirely separate panic output support in KMS,
  bypassing the current fbcon support. See [[PATCH v2 0/3] drm: Add panic handling](https://lore.kernel.org/dri-devel/20190311174218.51899-1-noralf@tronnes.org/).
* Encoding the actual oops and preceding dmesg in a QR might help with the
  dread “important stuff scrolled away” problem. See [[RFC][PATCH] Oops messages
  transfer using QR codes](https://lore.kernel.org/lkml/1446217392-11981-1-git-send-email-alexandru.murtaza@intel.com/)
  for some example code that could be reused.

Contact: Simona Vetter

Level: Advanced

## Clean up the debugfs support

There’s a bunch of issues with it:

* Convert drivers to support the [`drm_debugfs_add_files()`](drm-uapi.html#c.drm_debugfs_add_files "drm_debugfs_add_files") function instead of
  the [`drm_debugfs_create_files()`](drm-uapi.html#c.drm_debugfs_create_files "drm_debugfs_create_files") function.
* Improve late-register debugfs by rolling out the same debugfs pre-register
  infrastructure for connector and crtc too. That way, the drivers won’t need to
  split their setup code into init and register anymore.
* We probably want to have some support for debugfs files on crtc/connectors and
  maybe other kms objects directly in core. There’s even drm\_print support in
  the funcs for these objects to dump kms state, so it’s all there. And then the
  ->`show()` functions should obviously give you a pointer to the right object.
* The drm\_driver->debugfs\_init hooks we have is just an artifact of the old
  midlayered load sequence. DRM debugfs should work more like sysfs, where you
  can create properties/files for an object anytime you want, and the core
  takes care of publishing/unpuplishing all the files at register/unregister
  time. Drivers shouldn’t need to worry about these technicalities, and fixing
  this (together with the drm\_minor->drm\_device move) would allow us to remove
  debugfs\_init.

Contact: Simona Vetter

Level: Intermediate

## Object lifetime fixes

There’s two related issues here

* Cleanup up the various ->destroy callbacks, which often are all the same
  simple code.
* Lots of drivers erroneously allocate DRM modeset objects using devm\_kzalloc,
  which results in use-after free issues on driver unload. This can be serious
  trouble even for drivers for hardware integrated on the SoC due to
  EPROBE\_DEFERRED backoff.

Both these problems can be solved by switching over to [`drmm_kzalloc()`](drm-internals.html#c.drmm_kzalloc "drmm_kzalloc"), and the
various convenience wrappers provided, e.g. [`drmm_crtc_alloc_with_planes()`](drm-kms.html#c.drmm_crtc_alloc_with_planes "drmm_crtc_alloc_with_planes"),
[`drmm_universal_plane_alloc()`](drm-kms.html#c.drmm_universal_plane_alloc "drmm_universal_plane_alloc"), ... and so on.

Contact: Simona Vetter

Level: Intermediate

## Remove automatic page mapping from dma-buf importing

When importing dma-bufs, the dma-buf and PRIME frameworks automatically map
imported pages into the importer’s DMA area. [`drm_gem_prime_fd_to_handle()`](drm-mm.html#c.drm_gem_prime_fd_to_handle "drm_gem_prime_fd_to_handle") and
[`drm_gem_prime_handle_to_fd()`](drm-mm.html#c.drm_gem_prime_handle_to_fd "drm_gem_prime_handle_to_fd") require that importers call [`dma_buf_attach()`](../driver-api/dma-buf.html#c.dma_buf_attach "dma_buf_attach")
even if they never do actual device DMA, but only CPU access through
[`dma_buf_vmap()`](../driver-api/dma-buf.html#c.dma_buf_vmap "dma_buf_vmap"). This is a problem for USB devices, which do not support DMA
operations.

To fix the issue, automatic page mappings should be removed from the
buffer-sharing code. Fixing this is a bit more involved, since the import/export
cache is also tied to &drm\_gem\_object.import\_attach. Meanwhile we paper over
this problem for USB devices by fishing out the USB host controller device, as
long as that supports DMA. Otherwise importing can still needlessly fail.

Contact: Thomas Zimmermann <[tzimmermann@suse.de](mailto:tzimmermann%40suse.de)>, Simona Vetter

Level: Advanced

## Implement a new DUMB\_CREATE2 ioctl

The current DUMB\_CREATE ioctl is not well defined. Instead of a pixel and
framebuffer format, it only accepts a color mode of vague semantics. Assuming
a linear framebuffer, the color mode gives an idea of the supported pixel
format. But userspace effectively has to guess the correct values. It really
only works reliably with framebuffers in XRGB8888. Userspace has begun to
workaround these limitations by computing arbitrary format’s buffer sizes and
calculating their sizes in terms of XRGB8888 pixels.

One possible solution is a new ioctl DUMB\_CREATE2. It should accept a DRM
format and a format modifier to resolve the color mode’s ambiguity. As
framebuffers can be multi-planar, the new ioctl has to return the buffer size,
pitch and GEM handle for each individual color plane.

In the first step, the new ioctl can be limited to the current features of
the existing DUMB\_CREATE. Individual drivers can then be extended to support
multi-planar formats. Rockchip might require this and would be a good candidate.

It might also be helpful to userspace to query information about the size of
a potential buffer, if allocated. Userspace would supply geometry and format;
the kernel would return minimal allocation sizes and scanline pitch. There is
interest to allocate that memory from another device and provide it to the
DRM driver (say via dma-buf).

Another requested feature is the ability to allocate a buffer by size, without
format. Accelators use this for their buffer allocation and it could likely be
generalized.

In addition to the kernel implementation, there must be user-space support
for the new ioctl. There’s code in Mesa that might be able to use the new
call.

Contact: Thomas Zimmermann <[tzimmermann@suse.de](mailto:tzimmermann%40suse.de)>

Level: Advanced

### Better Testing

## Add unit tests using the Kernel Unit Testing (KUnit) framework

The [KUnit](https://www.kernel.org/doc/html/latest/dev-tools/kunit/index.html)
provides a common framework for unit tests within the Linux kernel. Having a
test suite would allow to identify regressions earlier.

A good candidate for the first unit tests are the format-conversion helpers in
`drm_format_helper.c`.

Contact: Javier Martinez Canillas <[javierm@redhat.com](mailto:javierm%40redhat.com)>

Level: Intermediate

## Clean up and document former selftests suites

Some KUnit test suites (drm\_buddy, drm\_cmdline\_parser, drm\_damage\_helper,
drm\_format, drm\_framebuffer, drm\_dp\_mst\_helper, drm\_mm, drm\_plane\_helper and
drm\_rect) are former selftests suites that have been converted over when KUnit
was first introduced.

These suites were fairly undocumented, and with different goals than what unit
tests can be. Trying to identify what each test in these suites actually test
for, whether that makes sense for a unit test, and either remove it if it
doesn’t or document it if it does would be of great help.

Contact: Maxime Ripard <[mripard@kernel.org](mailto:mripard%40kernel.org)>

Level: Intermediate

## Enable trinity for DRM

And fix up the fallout. Should be really interesting ...

Level: Advanced

## Make KMS tests in i-g-t generic

The i915 driver team maintains an extensive testsuite for the i915 DRM driver,
including tons of testcases for corner-cases in the modesetting API. It would
be awesome if those tests (at least the ones not relying on Intel-specific GEM
features) could be made to run on any KMS driver.

Basic work to run i-g-t tests on non-i915 is done, what’s now missing is mass-
converting things over. For modeset tests we also first need a bit of
infrastructure to use dumb buffers for untiled buffers, to be able to run all
the non-i915 specific modeset tests.

Level: Advanced

## Extend virtual test driver (VKMS)

See the documentation of [VKMS](vkms.html#vkms) for more details. This is an ideal
internship task, since it only requires a virtual machine and can be sized to
fit the available time.

Level: See details

## Backlight Refactoring

Backlight drivers have a triple enable/disable state, which is a bit overkill.
Plan to fix this:

1. Roll out [`backlight_enable()`](backlight.html#c.backlight_enable "backlight_enable") and [`backlight_disable()`](backlight.html#c.backlight_disable "backlight_disable") helpers everywhere. This
   has started already.
2. In all, only look at one of the three status bits set by the above helpers.
3. Remove the other two status bits.

Contact: Simona Vetter

Level: Intermediate

### Driver Specific

## AMD DC Display Driver

AMD DC is the display driver for AMD devices starting with Vega. There has been
a bunch of progress cleaning it up but there’s still plenty of work to be done.

See drivers/gpu/drm/amd/display/TODO for tasks.

Contact: Harry Wentland, Alex Deucher

### Bootsplash

There is support in place now for writing internal DRM clients making it
possible to pick up the bootsplash work that was rejected because it was written
for fbdev.

* [v6,8/8] drm/client: Hack: Add bootsplash example
  <https://patchwork.freedesktop.org/patch/306579/>
* [RFC PATCH v2 00/13] Kernel based bootsplash
  <https://lore.kernel.org/r/20171213194755.3409-1-mstaudt@suse.de>

Contact: Sam Ravnborg

Level: Advanced

### Brightness handling on devices with multiple internal panels

On x86/ACPI devices there can be multiple backlight firmware interfaces:
(ACPI) video, vendor specific and others. As well as direct/native (PWM)
register programming by the KMS driver.

To deal with this backlight drivers used on x86/ACPI call
`acpi_video_get_backlight_type()` which has heuristics (+quirks) to select
which backlight interface to use; and backlight drivers which do not match
the returned type will not register themselves, so that only one backlight
device gets registered (in a single GPU setup, see below).

At the moment this more or less assumes that there will only
be 1 (internal) panel on a system.

On systems with 2 panels this may be a problem, depending on
what interface `acpi_video_get_backlight_type()` selects:

1. native: in this case the KMS driver is expected to know which backlight
   device belongs to which output so everything should just work.
2. video: this does support controlling multiple backlights, but some work
   will need to be done to get the output <-> backlight device mapping

The above assumes both panels will require the same backlight interface type.
Things will break on systems with multiple panels where the 2 panels need
a different type of control. E.g. one panel needs ACPI video backlight control,
where as the other is using native backlight control. Currently in this case
only one of the 2 required backlight devices will get registered, based on
the `acpi_video_get_backlight_type()` return value.

If this (theoretical) case ever shows up, then supporting this will need some
work. A possible solution here would be to pass a device and connector-name
to `acpi_video_get_backlight_type()` so that it can deal with this.

Note in a way we already have a case where userspace sees 2 panels,
in dual GPU laptop setups with a mux. On those systems we may see
either 2 native backlight devices; or 2 native backlight devices.

Userspace already has code to deal with this by detecting if the related
panel is active (iow which way the mux between the GPU and the panels
points) and then uses that backlight device. Userspace here very much
assumes a single panel though. It picks only 1 of the 2 backlight devices
and then only uses that one.

Note that all userspace code (that I know off) is currently hardcoded
to assume a single panel.

Before the recent changes to not register multiple (e.g. video + native)
/sys/class/backlight devices for a single panel (on a single GPU laptop),
userspace would see multiple backlight devices all controlling the same
backlight.

To deal with this userspace had to always picks one preferred device under
/sys/class/backlight and will ignore the others. So to support brightness
control on multiple panels userspace will need to be updated too.

There are plans to allow brightness control through the KMS API by adding
a “display brightness” property to drm\_connector objects for panels. This
solves a number of issues with the /sys/class/backlight API, including not
being able to map a sysfs backlight device to a specific connector. Any
userspace changes to add support for brightness control on devices with
multiple panels really should build on top of this new KMS property.

Contact: Hans de Goede

Level: Advanced

### Buffer age or other damage accumulation algorithm for buffer damage

Drivers that do per-buffer uploads, need a buffer damage handling (rather than
frame damage like drivers that do per-plane or per-CRTC uploads), but there is
no support to get the buffer age or any other damage accumulation algorithm.

For this reason, the damage helpers just fallback to a full plane update if the
framebuffer attached to a plane has changed since the last page-flip. Drivers
set &drm\_plane\_state.ignore\_damage\_clips to true as indication to
[`drm_atomic_helper_damage_iter_init()`](drm-kms.html#c.drm_atomic_helper_damage_iter_init "drm_atomic_helper_damage_iter_init") and [`drm_atomic_helper_damage_iter_next()`](drm-kms.html#c.drm_atomic_helper_damage_iter_next "drm_atomic_helper_damage_iter_next")
helpers that the damage clips should be ignored.

This should be improved to get damage tracking properly working on drivers that
do per-buffer uploads.

More information about damage tracking and references to learning materials can
be found in [Damage Tracking Properties](drm-kms.html#damage-tracking-properties).

Contact: Javier Martinez Canillas <[javierm@redhat.com](mailto:javierm%40redhat.com)>

Level: Advanced

### Querying errors from drm\_syncobj

The drm\_syncobj container can be used by driver independent code to signal
complection of submission.

One minor feature still missing is a generic DRM IOCTL to query the error
status of binary and timeline drm\_syncobj.

This should probably be improved by implementing the necessary kernel interface
and adding support for that in the userspace stack.

Contact: Christian König

Level: Starter

### DRM GPU Scheduler

## Provide a universal successor for drm\_sched\_resubmit\_jobs()

[`drm_sched_resubmit_jobs()`](drm-mm.html#c.drm_sched_resubmit_jobs "drm_sched_resubmit_jobs") is deprecated. Main reason being that it leads to
reinitializing dma\_fences. See that function’s docu for details. The better
approach for valid resubmissions by amdgpu and Xe is (apparently) to figure out
which job (and, through association: which entity) caused the hang. Then, the
job’s buffer data, together with all other jobs’ buffer data currently in the
same hardware ring, must be invalidated. This can for example be done by
overwriting it. amdgpu currently determines which jobs are in the ring and need
to be overwritten by keeping copies of the job. Xe obtains that information by
directly accessing drm\_sched’s pending\_list.

Tasks:

1. implement scheduler functionality through which the driver can obtain the
   information which *broken* jobs are currently in the hardware ring.
2. Such infrastructure would then typically be used in
   drm\_sched\_backend\_ops.`timedout_job()`. Document that.
3. Port a driver as first user.
4. Document the new alternative in the docu of deprecated
   [`drm_sched_resubmit_jobs()`](drm-mm.html#c.drm_sched_resubmit_jobs "drm_sched_resubmit_jobs").

Contact: Christian König <[christian.koenig@amd.com](mailto:christian.koenig%40amd.com)>
:   Philipp Stanner <[phasta@kernel.org](mailto:phasta%40kernel.org)>

Level: Advanced

## Add locking for runqueues

There is an old FIXME by Sima in include/drm/gpu\_scheduler.h. It details that
[`struct drm_sched_rq`](drm-mm.html#c.drm_sched_rq "drm_sched_rq") is read at many places without any locks, not even with a
READ\_ONCE. At XDC 2025 no one could really tell why that is the case, whether
locks are needed and whether they could be added. (But for real, that should
probably be locked!). Check whether it’s possible to add locks everywhere, and
do so if yes.

Contact: Philipp Stanner <[phasta@kernel.org](mailto:phasta%40kernel.org)>

Level: Intermediate

### Outside DRM

## Convert fbdev drivers to DRM

There are plenty of fbdev drivers for older hardware. Some hardware has
become obsolete, but some still provides good(-enough) framebuffers. The
drivers that are still useful should be converted to DRM and afterwards
removed from fbdev.

Very simple fbdev drivers can best be converted by starting with a new
DRM driver. Simple KMS helpers and SHMEM should be able to handle any
existing hardware. The new driver’s call-back functions are filled from
existing fbdev code.

More complex fbdev drivers can be refactored step-by-step into a DRM
driver with the help of the DRM fbconv helpers [[4]](#id10). These helpers provide
the transition layer between the DRM core infrastructure and the fbdev
driver interface. Create a new DRM driver on top of the fbconv helpers,
copy over the fbdev driver, and hook it up to the DRM code. Examples for
several fbdev drivers are available in Thomas Zimmermann’s fbconv tree
[[4]](#id10), as well as a tutorial of this process [[5]](#id11). The result is a primitive
DRM driver that can run X11 and Weston.

> [4]
> ([1](#id7),[2](#id8))
>
> <https://gitlab.freedesktop.org/tzimmermann/linux/tree/fbconv>
>
>
> [[5](#id9)]
>
> <https://gitlab.freedesktop.org/tzimmermann/linux/blob/fbconv/drivers/gpu/drm/drm_fbconv_helper.c>

Contact: Thomas Zimmermann <[tzimmermann@suse.de](mailto:tzimmermann%40suse.de)>

Level: Advanced
