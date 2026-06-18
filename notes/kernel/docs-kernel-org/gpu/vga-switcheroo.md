# VGA Switcheroo

> 출처(원문): https://docs.kernel.org/gpu/vga-switcheroo.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# VGA Switcheroo

vga\_switcheroo is the Linux subsystem for laptop hybrid graphics.
These come in two flavors:

* muxed: Dual GPUs with a multiplexer chip to switch outputs between GPUs.
* muxless: Dual GPUs but only one of them is connected to outputs.
  The other one is merely used to offload rendering, its results
  are copied over PCIe into the framebuffer. On Linux this is
  supported with DRI PRIME.

Hybrid graphics started to appear in the late Naughties and were initially
all muxed. Newer laptops moved to a muxless architecture for cost reasons.
A notable exception is the MacBook Pro which continues to use a mux.
Muxes come with varying capabilities: Some switch only the panel, others
can also switch external displays. Some switch all display pins at once
while others can switch just the DDC lines. (To allow EDID probing
for the inactive GPU.) Also, muxes are often used to cut power to the
discrete GPU while it is not used.

DRM drivers register GPUs with vga\_switcheroo, these are henceforth called
clients. The mux is called the handler. Muxless machines also register a
handler to control the power state of the discrete GPU, its ->switchto
callback is a no-op for obvious reasons. The discrete GPU is often equipped
with an HDA controller for the HDMI/DP audio signal, this will also
register as a client so that vga\_switcheroo can take care of the correct
suspend/resume order when changing the discrete GPU’s power state. In total
there can thus be up to three clients: Two vga clients (GPUs) and one audio
client (on the discrete GPU). The code is mostly prepared to support
machines with more than two GPUs should they become available.

The GPU to which the outputs are currently switched is called the
active client in vga\_switcheroo parlance. The GPU not in use is the
inactive client. When the inactive client’s DRM driver is loaded,
it will be unable to probe the panel’s EDID and hence depends on
VBIOS to provide its display modes. If the VBIOS modes are bogus or
if there is no VBIOS at all (which is common on the MacBook Pro),
a client may alternatively request that the DDC lines are temporarily
switched to it, provided that the handler supports this. Switching
only the DDC lines and not the entire output avoids unnecessary
flickering.

## Modes of Use

### Manual switching and manual power control

In this mode of use, the file /sys/kernel/debug/vgaswitcheroo/switch
can be read to retrieve the current vga\_switcheroo state and commands
can be written to it to change the state. The file appears as soon as
two GPU drivers and one handler have registered with vga\_switcheroo.
The following commands are understood:

* OFF: Power off the device not in use.
* ON: Power on the device not in use.
* IGD: Switch to the integrated graphics device.
  Power on the integrated GPU if necessary, power off the discrete GPU.
  Prerequisite is that no user space processes (e.g. Xorg, alsactl)
  have opened device files of the GPUs or the audio client. If the
  switch fails, the user may invoke lsof(8) or fuser(1) on /dev/dri/
  and /dev/snd/controlC1 to identify processes blocking the switch.
* DIS: Switch to the discrete graphics device.
* DIGD: Delayed switch to the integrated graphics device.
  This will perform the switch once the last user space process has
  closed the device files of the GPUs and the audio client.
* DDIS: Delayed switch to the discrete graphics device.
* MIGD: Mux-only switch to the integrated graphics device.
  Does not remap console or change the power state of either gpu.
  If the integrated GPU is currently off, the screen will turn black.
  If it is on, the screen will show whatever happens to be in VRAM.
  Either way, the user has to blindly enter the command to switch back.
* MDIS: Mux-only switch to the discrete graphics device.

For GPUs whose power state is controlled by the driver’s runtime pm,
the ON and OFF commands are a no-op (see next section).

For muxless machines, the IGD/DIS, DIGD/DDIS and MIGD/MDIS commands
should not be used.

### Driver power control

In this mode of use, the discrete GPU automatically powers up and down at
the discretion of the driver’s runtime pm. On muxed machines, the user may
still influence the muxer state by way of the debugfs interface, however
the ON and OFF commands become a no-op for the discrete GPU.

This mode is the default on Nvidia HybridPower/Optimus and ATI PowerXpress.
Specifying nouveau.runpm=0, radeon.runpm=0 or amdgpu.runpm=0 on the kernel
command line disables it.

After the GPU has been suspended, the handler needs to be called to cut
power to the GPU. Likewise it needs to reinstate power before the GPU
can resume. This is achieved by [`vga_switcheroo_init_domain_pm_ops()`](#c.vga_switcheroo_init_domain_pm_ops "vga_switcheroo_init_domain_pm_ops"),
which augments the GPU’s suspend/resume functions by the requisite
calls to the handler.

When the audio device resumes, the GPU needs to be woken. This is achieved
by a PCI quirk which calls [`device_link_add()`](../driver-api/infrastructure.html#c.device_link_add "device_link_add") to declare a dependency on the
GPU. That way, the GPU is kept awake whenever and as long as the audio
device is in use.

On muxed machines, if the mux is initially switched to the discrete GPU,
the user ends up with a black screen when the GPU powers down after boot.
As a workaround, the mux is forced to the integrated GPU on runtime suspend,
cf. <https://bugs.freedesktop.org/show_bug.cgi?id=75917>

## API

### Public functions

int vga\_switcheroo\_register\_handler(const struct [vga\_switcheroo\_handler](#c.vga_switcheroo_handler "vga_switcheroo_handler") \*handler, enum [vga\_switcheroo\_handler\_flags\_t](#c.vga_switcheroo_handler_flags_t "vga_switcheroo_handler_flags_t") handler\_flags)
:   register handler

**Parameters**

`const struct vga_switcheroo_handler *handler`
:   handler callbacks

`enum vga_switcheroo_handler_flags_t handler_flags`
:   handler flags

**Description**

Register handler. Enable vga\_switcheroo if two vga clients have already
registered.

**Return**

0 on success, -EINVAL if a handler was already registered.

void vga\_switcheroo\_unregister\_handler(void)
:   unregister handler

**Parameters**

`void`
:   no arguments

**Description**

Unregister handler. Disable vga\_switcheroo.

enum [vga\_switcheroo\_handler\_flags\_t](#c.vga_switcheroo_handler_flags_t "vga_switcheroo_handler_flags_t") vga\_switcheroo\_handler\_flags(void)
:   obtain handler flags

**Parameters**

`void`
:   no arguments

**Description**

Helper for clients to obtain the handler flags bitmask.

**Return**

Handler flags. A value of 0 means that no handler is registered
or that the handler has no special capabilities.

int vga\_switcheroo\_register\_client(struct pci\_dev \*pdev, const struct [vga\_switcheroo\_client\_ops](#c.vga_switcheroo_client_ops "vga_switcheroo_client_ops") \*ops, bool driver\_power\_control)
:   register vga client

**Parameters**

`struct pci_dev *pdev`
:   client pci device

`const struct vga_switcheroo_client_ops *ops`
:   client callbacks

`bool driver_power_control`
:   whether power state is controlled by the driver’s
    runtime pm

**Description**

Register vga client (GPU). Enable vga\_switcheroo if another GPU and a
handler have already registered. The power state of the client is assumed
to be ON. Beforehand, [`vga_switcheroo_client_probe_defer()`](#c.vga_switcheroo_client_probe_defer "vga_switcheroo_client_probe_defer") shall be called
to ensure that all prerequisites are met.

**Return**

0 on success, -ENOMEM on memory allocation error.

int vga\_switcheroo\_register\_audio\_client(struct pci\_dev \*pdev, const struct [vga\_switcheroo\_client\_ops](#c.vga_switcheroo_client_ops "vga_switcheroo_client_ops") \*ops, struct pci\_dev \*vga\_dev)
:   register audio client

**Parameters**

`struct pci_dev *pdev`
:   client pci device

`const struct vga_switcheroo_client_ops *ops`
:   client callbacks

`struct pci_dev *vga_dev`
:   pci device which is bound to current audio client

**Description**

Register audio client (audio device on a GPU). The client is assumed
to use runtime PM. Beforehand, [`vga_switcheroo_client_probe_defer()`](#c.vga_switcheroo_client_probe_defer "vga_switcheroo_client_probe_defer")
shall be called to ensure that all prerequisites are met.

**Return**

0 on success, -ENOMEM on memory allocation error, -EINVAL on getting
client id error.

bool vga\_switcheroo\_client\_probe\_defer(struct pci\_dev \*pdev)
:   whether to defer probing a given client

**Parameters**

`struct pci_dev *pdev`
:   client pci device

**Description**

Determine whether any prerequisites are not fulfilled to probe a given
client. Drivers shall invoke this early on in their ->probe callback
and return `-EPROBE_DEFER` if it evaluates to `true`. Thou shalt not
register the client ere thou hast called this.

**Return**

`true` if probing should be deferred, otherwise `false`.

enum [vga\_switcheroo\_state](#c.vga_switcheroo_state "vga_switcheroo_state") vga\_switcheroo\_get\_client\_state(struct pci\_dev \*pdev)
:   obtain power state of a given client

**Parameters**

`struct pci_dev *pdev`
:   client pci device

**Description**

Obtain power state of a given client as seen from vga\_switcheroo.
The function is only called from hda\_intel.c.

**Return**

Power state.

void vga\_switcheroo\_unregister\_client(struct pci\_dev \*pdev)
:   unregister client

**Parameters**

`struct pci_dev *pdev`
:   client pci device

**Description**

Unregister client. Disable vga\_switcheroo if this is a vga client (GPU).

void vga\_switcheroo\_client\_fb\_set(struct pci\_dev \*pdev, struct fb\_info \*info)
:   set framebuffer of a given client

**Parameters**

`struct pci_dev *pdev`
:   client pci device

`struct fb_info *info`
:   framebuffer

**Description**

Set framebuffer of a given client. The console will be remapped to this
on switching.

int vga\_switcheroo\_lock\_ddc(struct pci\_dev \*pdev)
:   temporarily switch DDC lines to a given client

**Parameters**

`struct pci_dev *pdev`
:   client pci device

**Description**

Temporarily switch DDC lines to the client identified by **pdev**
(but leave the outputs otherwise switched to where they are).
This allows the inactive client to probe EDID. The DDC lines must
afterwards be switched back by calling [`vga_switcheroo_unlock_ddc()`](#c.vga_switcheroo_unlock_ddc "vga_switcheroo_unlock_ddc"),
even if this function returns an error.

**Return**

Previous DDC owner on success or a negative int on error.
Specifically, `-ENODEV` if no handler has registered or if the handler
does not support switching the DDC lines. Also, a negative value
returned by the handler is propagated back to the caller.
The return value has merely an informational purpose for any caller
which might be interested in it. It is acceptable to ignore the return
value and simply rely on the result of the subsequent EDID probe,
which will be `NULL` if DDC switching failed.

int vga\_switcheroo\_unlock\_ddc(struct pci\_dev \*pdev)
:   switch DDC lines back to previous owner

**Parameters**

`struct pci_dev *pdev`
:   client pci device

**Description**

Switch DDC lines back to the previous owner after calling
[`vga_switcheroo_lock_ddc()`](#c.vga_switcheroo_lock_ddc "vga_switcheroo_lock_ddc"). This must be called even if
[`vga_switcheroo_lock_ddc()`](#c.vga_switcheroo_lock_ddc "vga_switcheroo_lock_ddc") returned an error.

**Return**

Previous DDC owner on success (i.e. the client identifier of **pdev**)
or a negative int on error.
Specifically, `-ENODEV` if no handler has registered or if the handler
does not support switching the DDC lines. Also, a negative value
returned by the handler is propagated back to the caller.
Finally, invoking this function without calling [`vga_switcheroo_lock_ddc()`](#c.vga_switcheroo_lock_ddc "vga_switcheroo_lock_ddc")
first is not allowed and will result in `-EINVAL`.

int vga\_switcheroo\_process\_delayed\_switch(void)
:   helper for delayed switching

**Parameters**

`void`
:   no arguments

**Description**

Process a delayed switch if one is pending.

**Return**

0 on success. -EINVAL if no delayed switch is pending, if the client
has unregistered in the meantime or if there are other clients blocking the
switch. If the actual switch fails, an error is reported and 0 is returned.

int vga\_switcheroo\_init\_domain\_pm\_ops(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct [dev\_pm\_domain](../driver-api/pm/types.html#c.dev_pm_domain "dev_pm_domain") \*domain)
:   helper for driver power control

**Parameters**

`struct device *dev`
:   vga client device

`struct dev_pm_domain *domain`
:   power domain

**Description**

Helper for GPUs whose power state is controlled by the driver’s runtime pm.
After the GPU has been suspended, the handler needs to be called to cut
power to the GPU. Likewise it needs to reinstate power before the GPU
can resume. To this end, this helper augments the suspend/resume functions
by the requisite calls to the handler. It needs only be called on platforms
where the power switch is separate to the device being powered down.

### Public structures

struct vga\_switcheroo\_handler
:   handler callbacks

**Definition**:

```
struct vga_switcheroo_handler {
    int (*init)(void);
    int (*switchto)(enum vga_switcheroo_client_id id);
    int (*switch_ddc)(enum vga_switcheroo_client_id id);
    int (*power_state)(enum vga_switcheroo_client_id id, enum vga_switcheroo_state state);
    enum vga_switcheroo_client_id (*get_client_id)(struct pci_dev *pdev);
};
```

**Members**

`init`
:   initialize handler.
    Optional. This gets called when vga\_switcheroo is enabled, i.e. when
    two vga clients have registered. It allows the handler to perform
    some delayed initialization that depends on the existence of the
    vga clients. Currently only the radeon and amdgpu drivers use this.
    The return value is ignored

`switchto`
:   switch outputs to given client.
    Mandatory. For muxless machines this should be a no-op. Returning 0
    denotes success, anything else failure (in which case the switch is
    aborted)

`switch_ddc`
:   switch DDC lines to given client.
    Optional. Should return the previous DDC owner on success or a
    negative int on failure

`power_state`
:   cut or reinstate power of given client.
    Optional. The return value is ignored

`get_client_id`
:   determine if given pci device is integrated or discrete GPU.
    Mandatory

**Description**

Handler callbacks. The multiplexer itself. The **switchto** and **get\_client\_id**
methods are mandatory, all others may be set to NULL.

struct vga\_switcheroo\_client\_ops
:   client callbacks

**Definition**:

```
struct vga_switcheroo_client_ops {
    void (*set_gpu_state)(struct pci_dev *dev, enum vga_switcheroo_state);
    void (*reprobe)(struct pci_dev *dev);
    bool (*can_switch)(struct pci_dev *dev);
    void (*gpu_bound)(struct pci_dev *dev, enum vga_switcheroo_client_id);
};
```

**Members**

`set_gpu_state`
:   do the equivalent of suspend/resume for the card.
    Mandatory. This should not cut power to the discrete GPU,
    which is the job of the handler

`reprobe`
:   poll outputs.
    Optional. This gets called after waking the GPU and switching
    the outputs to it

`can_switch`
:   check if the device is in a position to switch now.
    Mandatory. The client should return false if a user space process
    has one of its device files open

`gpu_bound`
:   notify the client id to audio client when the GPU is bound.

**Description**

Client callbacks. A client can be either a GPU or an audio device on a GPU.
The **set\_gpu\_state** and **can\_switch** methods are mandatory, **reprobe** may be
set to NULL. For audio clients, the **reprobe** member is bogus.
OTOH, **gpu\_bound** is only for audio clients, and not used for GPU clients.

### Public constants

enum vga\_switcheroo\_handler\_flags\_t
:   handler flags bitmask

**Constants**

`VGA_SWITCHEROO_CAN_SWITCH_DDC`
:   whether the handler is able to switch the
    DDC lines separately. This signals to clients that they should call
    [`drm_get_edid_switcheroo()`](drm-kms-helpers.html#c.drm_get_edid_switcheroo "drm_get_edid_switcheroo") to probe the EDID

`VGA_SWITCHEROO_NEEDS_EDP_CONFIG`
:   whether the handler is unable to switch
    the AUX channel separately. This signals to clients that the active
    GPU needs to train the link and communicate the link parameters to the
    inactive GPU (mediated by vga\_switcheroo). The inactive GPU may then
    skip the AUX handshake and set up its output with these pre-calibrated
    values (DisplayPort specification v1.1a, section 2.5.3.3)

**Description**

Handler flags bitmask. Used by handlers to declare their capabilities upon
registering with vga\_switcheroo.

enum vga\_switcheroo\_client\_id
:   client identifier

**Constants**

`VGA_SWITCHEROO_UNKNOWN_ID`
:   initial identifier assigned to vga clients.
    Determining the id requires the handler, so GPUs are given their
    true id in a delayed fashion in `vga_switcheroo_enable()`

`VGA_SWITCHEROO_IGD`
:   integrated graphics device

`VGA_SWITCHEROO_DIS`
:   discrete graphics device

`VGA_SWITCHEROO_MAX_CLIENTS`
:   currently no more than two GPUs are supported

**Description**

Client identifier. Audio clients use the same identifier & 0x100.

enum vga\_switcheroo\_state
:   client power state

**Constants**

`VGA_SWITCHEROO_OFF`
:   off

`VGA_SWITCHEROO_ON`
:   on

`VGA_SWITCHEROO_NOT_FOUND`
:   client has not registered with vga\_switcheroo.
    Only used in [`vga_switcheroo_get_client_state()`](#c.vga_switcheroo_get_client_state "vga_switcheroo_get_client_state") which in turn is only
    called from hda\_intel.c

**Description**

Client power state.

### Private structures

struct vgasr\_priv
:   vga\_switcheroo private data

**Definition**:

```
struct vgasr_priv {
    bool active;
    bool delayed_switch_active;
    enum vga_switcheroo_client_id delayed_client_id;
    struct dentry *debugfs_root;
    int registered_clients;
    struct list_head clients;
    const struct vga_switcheroo_handler *handler;
    enum vga_switcheroo_handler_flags_t handler_flags;
    struct mutex mux_hw_lock;
    int old_ddc_owner;
};
```

**Members**

`active`
:   whether vga\_switcheroo is enabled.
    Prerequisite is the registration of two GPUs and a handler

`delayed_switch_active`
:   whether a delayed switch is pending

`delayed_client_id`
:   client to which a delayed switch is pending

`debugfs_root`
:   directory for vga\_switcheroo debugfs interface

`registered_clients`
:   number of registered GPUs
    (counting only vga clients, not audio clients)

`clients`
:   list of registered clients

`handler`
:   registered handler

`handler_flags`
:   flags of registered handler

`mux_hw_lock`
:   protects mux state
    (in particular while DDC lines are temporarily switched)

`old_ddc_owner`
:   client to which DDC lines will be switched back on unlock

**Description**

vga\_switcheroo private data. Currently only one vga\_switcheroo instance
per system is supported.

struct vga\_switcheroo\_client
:   registered client

**Definition**:

```
struct vga_switcheroo_client {
    struct pci_dev *pdev;
    struct fb_info *fb_info;
    enum vga_switcheroo_state pwr_state;
    const struct vga_switcheroo_client_ops *ops;
    enum vga_switcheroo_client_id id;
    bool active;
    bool driver_power_control;
    struct list_head list;
    struct pci_dev *vga_dev;
};
```

**Members**

`pdev`
:   client pci device

`fb_info`
:   framebuffer to which console is remapped on switching

`pwr_state`
:   current power state if manual power control is used.
    For driver power control, call `vga_switcheroo_pwr_state()`.

`ops`
:   client callbacks

`id`
:   client identifier. Determining the id requires the handler,
    so gpus are initially assigned VGA\_SWITCHEROO\_UNKNOWN\_ID
    and later given their true id in `vga_switcheroo_enable()`

`active`
:   whether the outputs are currently switched to this client

`driver_power_control`
:   whether power state is controlled by the driver’s
    runtime pm. If true, writing ON and OFF to the vga\_switcheroo debugfs
    interface is a no-op so as not to interfere with runtime pm

`list`
:   client list

`vga_dev`
:   pci device, indicate which GPU is bound to current audio client

**Description**

Registered client. A client can be either a GPU or an audio device on a GPU.
For audio clients, the **fb\_info** and **active** members are bogus. For GPU
clients, the **vga\_dev** is bogus.

## Handlers

### apple-gmux Handler

gmux is a microcontroller built into the MacBook Pro to support dual GPUs:
A [Lattice XP2](http://www.latticesemi.com/en/Products/FPGAandCPLD/LatticeXP2.aspx) on pre-retinas, a [Renesas R4F2113](http://www.renesas.com/products/mpumcu/h8s/h8s2100/h8s2113/index.jsp) on pre-T2 retinas.

On T2 Macbooks, the gmux is part of the T2 Coprocessor’s SMC. The SMC has
an I2C connection to a NXP PCAL6524 GPIO expander, which enables/disables
the voltage regulators of the discrete GPU, drives the display panel power,
and has a GPIO to switch the eDP mux. The Intel CPU can interact with
gmux through MMIO, similar to how the main SMC interface is controlled.

(The MacPro6,1 2013 also has a gmux, however it is unclear why since it has
dual GPUs but no built-in display.)

gmux is connected to the LPC bus of the southbridge. Its I/O ports are
accessed differently depending on the microcontroller: Driver functions
to access a pre-retina gmux are infixed `_pio_`, those for a pre-T2
retina gmux are infixed `_index_`, and those on T2 Macs are infixed
with `_mmio_`.

gmux is also connected to a GPIO pin of the southbridge and thereby is able
to trigger an ACPI GPE. ACPI name GMGP holds this GPIO pin’s number. On the
MBP5 2008/09 it’s GPIO pin 22 of the Nvidia MCP79, on following generations
it’s GPIO pin 6 of the Intel PCH, on MMIO gmux’s it’s pin 21.

The GPE merely signals that an interrupt occurred, the actual type of event
is identified by reading a gmux register.

In addition to the GMGP name, gmux’s ACPI device also has two methods GMSP
and GMLV. GMLV likely means “GMUX Level”, and reads the value of the GPIO,
while GMSP likely means “GMUX Set Polarity”, and seems to write to the GPIO’s
value. On newer Macbooks (This was introduced with or sometime before the
MacBookPro14,3), the ACPI GPE method differentiates between the OS type: On
Darwin, only a notification is signaled, whereas on other OSes, the GPIO’s
value is read and then inverted.

Because Linux masquerades as Darwin, it ends up in the notification-only code
path. On MMIO gmux’s, this seems to lead to us being unable to clear interrupts,
unless we call GMSP(0). Without this, there is a flood of status=0 interrupts
that can’t be cleared. This issue seems to be unique to MMIO gmux’s.

#### Graphics mux

On pre-retinas, the LVDS outputs of both GPUs feed into gmux which muxes
either of them to the panel. One of the tricks gmux has up its sleeve is
to lengthen the blanking interval of its output during a switch to
synchronize it with the GPU switched to. This allows for a flicker-free
switch that is imperceptible by the user ([US 8,687,007 B2](https://pimg-fpiw.uspto.gov/fdd/07/870/086/0.pdf)).

On retinas, muxing is no longer done by gmux itself, but by a separate
chip which is controlled by gmux. The chip is triple sourced, it is
either an [NXP CBTL06142](https://www.nxp.com/documents/data_sheet/CBTL06141.pdf), [TI HD3SS212](https://www.ti.com/lit/ds/symlink/hd3ss212.pdf) or [Pericom PI3VDP12412](https://www.pericom.com/assets/Datasheets/PI3VDP12412.pdf).
The panel is driven with eDP instead of LVDS since the pixel clock
required for retina resolution exceeds LVDS’ limits.

Pre-retinas are able to switch the panel’s DDC pins separately.
This is handled by a [TI SN74LV4066A](https://www.ti.com/lit/ds/symlink/sn74lv4066a.pdf) which is controlled by gmux.
The inactive GPU can thus probe the panel’s EDID without switching over
the entire panel. Retinas lack this functionality as the chips used for
eDP muxing are incapable of switching the AUX channel separately (see
the linked data sheets, Pericom would be capable but this is unused).
However the retina panel has the NO\_AUX\_HANDSHAKE\_LINK\_TRAINING bit set
in its DPCD, allowing the inactive GPU to skip the AUX handshake and
set up the output with link parameters pre-calibrated by the active GPU.

The external DP port is only fully switchable on the first two unibody
MacBook Pro generations, MBP5 2008/09 and MBP6 2010. This is done by an
[NXP CBTL06141](https://www.nxp.com/documents/data_sheet/CBTL06141.pdf) which is controlled by gmux. It’s the predecessor of the
eDP mux on retinas, the difference being support for 2.7 versus 5.4 Gbit/s.

The following MacBook Pro generations replaced the external DP port with a
combined DP/Thunderbolt port and lost the ability to switch it between GPUs,
connecting it either to the discrete GPU or the Thunderbolt controller.
Oddly enough, while the full port is no longer switchable, AUX and HPD
are still switchable by way of an [NXP CBTL03062](http://pdf.datasheetarchive.com/indexerfiles/Datasheets-SW16/DSASW00308511.pdf) (on pre-retinas
MBP8 2011 and MBP9 2012) or two [TI TS3DS10224](https://www.ti.com/lit/ds/symlink/ts3ds10224.pdf) (on pre-t2 retinas) under
the control of gmux. Since the integrated GPU is missing the main link,
external displays appear to it as phantoms which fail to link-train.

gmux receives the HPD signal of all display connectors and sends an
interrupt on hotplug. On generations which cannot switch external ports,
the discrete GPU can then be woken to drive the newly connected display.
The ability to switch AUX on these generations could be used to improve
reliability of hotplug detection by having the integrated GPU poll the
ports while the discrete GPU is asleep, but currently we do not make use
of this feature.

Our switching policy for the external port is that on those generations
which are able to switch it fully, the port is switched together with the
panel when IGD / DIS commands are issued to vga\_switcheroo. It is thus
possible to drive e.g. a beamer on battery power with the integrated GPU.
The user may manually switch to the discrete GPU if more performance is
needed.

On all newer generations, the external port can only be driven by the
discrete GPU. If a display is plugged in while the panel is switched to
the integrated GPU, *both* GPUs will be in use for maximum performance.
To decrease power consumption, the user may manually switch to the
discrete GPU, thereby suspending the integrated GPU.

gmux’ initial switch state on bootup is user configurable via the EFI
variable `gpu-power-prefs-fa4ce28d-b62f-4c99-9cc3-6815686e30f9` (5th byte,
1 = IGD, 0 = DIS). Based on this setting, the EFI firmware tells gmux to
switch the panel and the external DP connector and allocates a framebuffer
for the selected GPU.

#### Power control

gmux is able to cut power to the discrete GPU. It automatically takes care
of the correct sequence to tear down and bring up the power rails for
core voltage, VRAM and PCIe.

#### Backlight control

On single GPU MacBooks, the PWM signal for the backlight is generated by
the GPU. On dual GPU MacBook Pros by contrast, either GPU may be suspended
to conserve energy. Hence the PWM signal needs to be generated by a separate
backlight driver which is controlled by gmux. The earliest generation
MBP5 2008/09 uses a [TI LP8543](https://www.ti.com/lit/ds/symlink/lp8543.pdf) backlight driver. Newer models
use a [TI LP8545](https://www.ti.com/lit/ds/symlink/lp8545.pdf) or a TI LP8548.

#### Public functions

bool apple\_gmux\_detect(struct [pnp\_dev](#c.apple_gmux_detect "pnp_dev") \*pnp\_dev, enum apple\_gmux\_type \*type\_ret)
:   detect if gmux is built into the machine

**Parameters**

`struct pnp_dev *pnp_dev`
:   Device to probe or NULL to use the first matching device

`enum apple_gmux_type *type_ret`
:   Returns (by reference) the apple\_gmux\_type of the device

**Description**

Detect if a supported gmux device is present by actually probing it.
This avoids the false positives returned on some models by
[`apple_gmux_present()`](#c.apple_gmux_present "apple_gmux_present").

**Return**

`true` if a supported gmux ACPI device is detected and the kernel
was configured with CONFIG\_APPLE\_GMUX, `false` otherwise.

bool apple\_gmux\_present(void)
:   check if gmux ACPI device is present

**Parameters**

`void`
:   no arguments

**Description**

Drivers may use this to activate quirks specific to dual GPU MacBook Pros
and Mac Pros, e.g. for deferred probing, runtime pm and backlight.

**Return**

`true` if gmux ACPI device is present and the kernel was configured
with CONFIG\_APPLE\_GMUX, `false` otherwise.
