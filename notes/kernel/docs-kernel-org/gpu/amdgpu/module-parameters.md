# Module Parameters

> 출처(원문): https://docs.kernel.org/gpu/amdgpu/module-parameters.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Module Parameters

The amdgpu driver supports the following module parameters:

**vramlimit (int)**

Restrict the total amount of VRAM in MiB for testing. The default is 0 (Use full VRAM).

**vis\_vramlimit (int)**

Restrict the amount of CPU visible VRAM in MiB for testing. The default is 0 (Use full CPU visible VRAM).

**gartsize (uint)**

Restrict the size of GART (for kernel use) in Mib (32, 64, etc.) for testing.
The default is -1 (The size depends on asic).

**gttsize (int)**

Restrict the size of GTT domain (for userspace use) in MiB for testing.
The default is -1 (Use value specified by TTM).
This parameter is deprecated and will be removed in the future.

**moverate (int)**

Set maximum buffer migration rate in MB/s. The default is -1 (8 MB/s).

**audio (int)**

Set HDMI/DPAudio. Only affects non-DC display handling. The default is -1 (Enabled), set 0 to disabled it.

**disp\_priority (int)**

Set display Priority (1 = normal, 2 = high). Only affects non-DC display handling. The default is 0 (auto).

**hw\_i2c (int)**

To enable hw i2c engine. Only affects non-DC display handling. The default is 0 (Disabled).

**pcie\_gen2 (int)**

To disable PCIE Gen2/3 mode (0 = disable, 1 = enable). The default is -1 (auto, enabled).

**msi (int)**

To disable Message Signaled Interrupts (MSI) functionality (1 = enable, 0 = disable). The default is -1 (auto, enabled).

**svm\_default\_granularity (uint)**

Used in buffer migration and handling of recoverable page faults

**lockup\_timeout (string)**

Set GPU scheduler timeout value in ms.

The format can be [single value] for setting all timeouts at once or
[GFX,Compute,SDMA,Video] to set individual timeouts.
Negative values mean infinity.

By default(with no lockup\_timeout settings), the timeout for all queues is 2000.

**dpm (int)**

Override for dynamic power management setting
(0 = disable, 1 = enable)
The default is -1 (auto).

**fw\_load\_type (int)**

Set different firmware loading type for debugging, if supported.
Set to 0 to force direct loading if supported by the ASIC. Set
to -1 to select the default loading mode for the ASIC, as defined
by the driver. The default is -1 (auto).

**aspm (int)**

To disable ASPM (1 = enable, 0 = disable). The default is -1 (auto, enabled).

**runpm (int)**

Override for runtime power management control for dGPUs. The amdgpu driver can dynamically power down
the dGPUs when they are idle if supported. The default is -1 (auto enable).
Setting the value to 0 disables this functionality.
Setting the value to -2 is auto enabled with power down when displays are attached.

**ip\_block\_mask (uint)**

Override what IP blocks are enabled on the GPU. Each GPU is a collection of IP blocks (gfx, display, video, etc.).
Use this parameter to disable specific blocks. Note that the IP blocks do not have a fixed index. Some asics may not have
some IPs or may include multiple instances of an IP so the ordering various from asic to asic. See the driver output in
the kernel log for the list of IPs on the asic. The default is 0xffffffff (enable all blocks on a device).

**bapm (int)**

Bidirectional Application Power Management (BAPM) used to dynamically share TDP between CPU and GPU. Set value 0 to disable it.
The default -1 (auto, enabled)

**deep\_color (int)**

Set 1 to enable Deep Color support. Only affects non-DC display handling. The default is 0 (disabled).

**vm\_size (int)**

Override the size of the GPU’s per client virtual address space in GiB. The default is -1 (automatic for each asic).

**vm\_fragment\_size (int)**

Override VM fragment size in bits (4, 5, etc. 4 = 64K, 9 = 2M). The default is -1 (automatic for each asic).

**vm\_block\_size (int)**

Override VM page table size in bits (default depending on vm\_size and hw setup). The default is -1 (automatic for each asic).

**vm\_fault\_stop (int)**

Stop on VM fault for debugging (0 = never, 1 = print first, 2 = always). The default is 0 (No stop).

**vm\_update\_mode (int)**

Override VM update mode. VM updated by using CPU (0 = never, 1 = Graphics only, 2 = Compute only, 3 = Both). The default
is -1 (Only in large BAR(LB) systems Compute VM tables will be updated by CPU, otherwise 0, never).

**exp\_hw\_support (int)**

Enable experimental hw support (1 = enable). The default is 0 (disabled).

**dc (int)**

Disable/Enable Display Core driver for debugging (1 = enable, 0 = disable). The default is -1 (automatic for each asic).

**sched\_jobs (int)**

Override the max number of jobs supported in the sw queue. The default is 32.

**sched\_hw\_submission (int)**

Override the max number of HW submissions. The default is 2.

**ppfeaturemask (hexint)**

Override power features enabled. See `enum PP_FEATURE_MASK` in drivers/gpu/drm/amd/include/amd\_shared.h.
The default is the current set of stable power features.

**forcelongtraining (uint)**

Force long memory training in resume.
The default is zero, indicates short training in resume.

**pcie\_gen\_cap (uint)**

Override PCIE gen speed capabilities. See the CAIL flags in drivers/gpu/drm/amd/include/amd\_pcie.h.
The default is 0 (automatic for each asic).

**pcie\_lane\_cap (uint)**

Override PCIE lanes capabilities. See the CAIL flags in drivers/gpu/drm/amd/include/amd\_pcie.h.
The default is 0 (automatic for each asic).

**cg\_mask (ullong)**

Override Clockgating features enabled on GPU (0 = disable clock gating). See the AMD\_CG\_SUPPORT flags in
drivers/gpu/drm/amd/include/amd\_shared.h. The default is 0xffffffffffffffff (all enabled).

**pg\_mask (uint)**

Override Powergating features enabled on GPU (0 = disable power gating). See the AMD\_PG\_SUPPORT flags in
drivers/gpu/drm/amd/include/amd\_shared.h. The default is 0xffffffff (all enabled).

**sdma\_phase\_quantum (uint)**

Override SDMA context switch phase quantum (x 1K GPU clock cycles, 0 = no change). The default is 32.

**disable\_cu (charp)**

Set to disable CUs (It’s set like se.sh.cu,...). The default is NULL.

**virtual\_display (charp)**

Set to enable virtual display feature. This feature provides a virtual display hardware on headless boards
or in virtualized environments. It will be set like xxxx:xx:xx.x,x;xxxx:xx:xx.x,x. It’s the pci address of
the device, plus the number of crtcs to expose. E.g., 0000:26:00.0,4 would enable 4 virtual crtcs on the pci
device at 26:00.0. The default is NULL.

**lbpw (int)**

Override Load Balancing Per Watt (LBPW) support (1 = enable, 0 = disable). The default is -1 (auto, enabled).

**gpu\_recovery (int)**

Set to enable GPU recovery mechanism (1 = enable, 0 = disable). The default is -1 (auto, disabled except SRIOV).

**emu\_mode (int)**

Set value 1 to enable emulation mode. This is only needed when running on an emulator. The default is 0 (disabled).

**ras\_enable (int)**

Enable RAS features on the GPU (0 = disable, 1 = enable, -1 = auto (default))

**ras\_mask (uint)**

Mask of RAS features to enable (default 0xffffffff), only valid when ras\_enable == 1
See the flags in drivers/gpu/drm/amd/amdgpu/amdgpu\_ras.h

**timeout\_fatal\_disable (bool)**

Disable Watchdog timeout fatal error event

**timeout\_period (uint)**

Modify the watchdog timeout max\_cycles as (1 << period)

**si\_support (int)**

1 = enabled, 0 = disabled, -1 = default

SI (Southern Islands) are first generation GCN GPUs, supported by both
drivers: radeon (old) and amdgpu (new). This parameter controls whether
amdgpu should support SI.
By default, SI dedicated GPUs are supported by amdgpu.
Only relevant when CONFIG\_DRM\_AMDGPU\_SI is enabled to build SI support in amdgpu.
See also radeon.si\_support which should be disabled when amdgpu.si\_support is
enabled, and vice versa.

**cik\_support (int)**

1 = enabled, 0 = disabled, -1 = default

CIK (Sea Islands) are second generation GCN GPUs, supported by both
drivers: radeon (old) and amdgpu (new). This parameter controls whether
amdgpu should support CIK.
By default, CIK dedicated GPUs and APUs are supported by amdgpu.
Only relevant when CONFIG\_DRM\_AMDGPU\_CIK is enabled to build CIK support in amdgpu.
See also radeon.cik\_support which should be disabled when amdgpu.cik\_support is
enabled, and vice versa.

**smu\_memory\_pool\_size (uint)**

It is used to reserve gtt for smu debug usage, setting value 0 to disable it. The actual size is value \* 256MiB.
E.g. 0x1 = 256Mbyte, 0x2 = 512Mbyte, 0x4 = 1 Gbyte, 0x8 = 2GByte. The default is 0 (disabled).

**async\_gfx\_ring (int)**

It is used to enable gfx rings that could be configured with different prioritites or equal priorities

**mcbp (int)**

It is used to enable mid command buffer preemption. (0 = disabled, 1 = enabled, -1 auto (default))

**discovery (int)**

Allow driver to discover hardware IP information from IP Discovery table at the top of VRAM.
(-1 = auto (default), 0 = disabled, 1 = enabled, 2 = use ip\_discovery table from file)

**mes\_log\_enable (int)**

Enable Micro Engine Scheduler log. This is used to enable/disable MES internal log.
(0 = disabled (default), 1 = enabled)

**uni\_mes (int)**

Enable Unified Micro Engine Scheduler. This is a new engine pipe for unified scheduler.
(0 = disabled (default), 1 = enabled)

**noretry (int)**

Disable XNACK retry in the SQ by default on GFXv9 hardware. On ASICs that
do not support per-process XNACK this also disables retry page faults.
(0 = retry enabled, 1 = retry disabled, -1 auto (default))

**force\_asic\_type (int)**

A non negative value used to specify the asic type for all supported GPUs.

**use\_xgmi\_p2p (int)**

Enables/disables XGMI P2P interface (0 = disable, 1 = enable).

**sched\_policy (int)**

Set scheduling policy. Default is HWS(hardware scheduling) with over-subscription.
Setting 1 disables over-subscription. Setting 2 disables HWS and statically
assigns queues to HQDs.

**hws\_max\_conc\_proc (int)**

Maximum number of processes that HWS can schedule concurrently. The maximum is the
number of VMIDs assigned to the HWS, which is also the default.

**cwsr\_enable (int)**

CWSR(compute wave store and resume) allows the GPU to preempt shader execution in
the middle of a compute wave. Default is 1 to enable this feature. Setting 0
disables it.

**max\_num\_of\_queues\_per\_device (int)**

Maximum number of queues per device. Valid setting is between 1 and 4096. Default
is 4096.

**send\_sigterm (int)**

Send sigterm to HSA process on unhandled exceptions. Default is not to send sigterm
but just print errors on dmesg. Setting 1 enables sending sigterm.

**halt\_if\_hws\_hang (int)**

Halt if HWS hang is detected. Default value, 0, disables the halt on hang.
Setting 1 enables halt on hang.

**hws\_gws\_support(bool)**

Assume that HWS supports GWS barriers regardless of what firmware version
check says. Default value: false (rely on MEC2 firmware version check).

**queue\_preemption\_timeout\_ms (int)**

queue preemption timeout in ms (1 = Minimum, 9000 = default)

**debug\_evictions(bool)**

Enable extra debug messages to help determine the cause of evictions

**no\_system\_mem\_limit(bool)**

Disable system memory limit, to support multiple process shared memory

**no\_queue\_eviction\_on\_vm\_fault (int)**

If set, process queues will not be evicted on gpuvm fault. This is to keep the wavefront context for debugging (0 = queue eviction, 1 = no queue eviction). The default is 0 (queue eviction).

**mtype\_local (int)**

**pcie\_p2p (bool)**

Enable PCIe P2P (requires large-BAR). Default value: true (on)

**dcfeaturemask (uint)**

Override display features enabled. See [`enum DC_FEATURE_MASK`](driver-core.html#c.DC_FEATURE_MASK "DC_FEATURE_MASK") in drivers/gpu/drm/amd/include/amd\_shared.h.
The default is the current set of stable display features.

**dcdebugmask (uint)**

Display debug options. See [`enum DC_DEBUG_MASK`](driver-core.html#c.DC_DEBUG_MASK "DC_DEBUG_MASK") in drivers/gpu/drm/amd/include/amd\_shared.h.

**abmlevel (uint)**

Override the default ABM (Adaptive Backlight Management) level used for DC
enabled hardware. Requires DMCU to be supported and loaded.
Valid levels are 0-4. A value of 0 indicates that ABM should be disabled by
default. Values 1-4 control the maximum allowable brightness reduction via
the ABM algorithm, with 1 being the least reduction and 4 being the most
reduction.

Defaults to -1, or auto. Userspace can only override this level after
boot if it’s set to auto.

**damageclips (int)**

Enable or disable damage clips support. If damage clips support is disabled,
we will force full frame updates, irrespective of what user space sends to
us.

Defaults to -1 (where it is enabled unless a PSR-SU display is detected).

**tmz (int)**

Trusted Memory Zone (TMZ) is a method to protect data being written
to or read from memory.

The default value: 0 (off). TODO: change to auto till it is completed.

**freesync\_video (uint)**

Enable the optimization to adjust front porch timing to achieve seamless
mode change experience when setting a freesync supported mode for which full
modeset is not needed.

The Display Core will add a set of modes derived from the base FreeSync
video mode into the corresponding connector’s mode list based on commonly
used refresh rates and VRR range of the connected display, when users enable
this feature. From the userspace perspective, they can see a seamless mode
change experience when the change between different refresh rates under the
same resolution. Additionally, userspace applications such as Video playback
can read this modeset list and change the refresh rate based on the video
frame rate. Finally, the userspace can also derive an appropriate mode for a
particular refresh rate based on the FreeSync Mode and add it to the
connector’s mode list.

Note: This is an experimental feature.

The default value: 0 (off).

**reset\_method (int)**

GPU reset method (-1 = auto (default), 0 = legacy, 1 = mode0, 2 = mode1, 3 = mode2, 4 = baco)

**bad\_page\_threshold (int) Bad page threshold is specifies the**

threshold value of faulty pages detected by RAS ECC, which may
result in the GPU entering bad status when the number of total
faulty pages by ECC exceeds the threshold value.

**vcnfw\_log (int)**

Enable vcnfw log output for debugging, the default is disabled.

**sg\_display (int)**

Disable S/G (scatter/gather) display (i.e., display from system memory).
This option is only relevant on APUs. Set this option to 0 to disable
S/G display if you experience flickering or other issues under memory
pressure and report the issue.

**umsch\_mm (int)**

Enable Multi Media User Mode Scheduler. This is a HW scheduling engine for VCN and VPE.
(0 = disabled (default), 1 = enabled)

**umsch\_mm\_fwlog (int)**

Enable umschfw log output for debugging, the default is disabled.

**smu\_pptable\_id (int)**

Used to override pptable id. id = 0 use VBIOS pptable.
id > 0 use the soft pptable with specicfied id.

**partition\_mode (int)**

Used to override the default SPX mode.

**enforce\_isolation (int)**

enforce process isolation between graphics and compute.
(-1 = auto, 0 = disable, 1 = enable, 2 = enable legacy mode, 3 = enable without cleaner shader)

**modeset (int)**

Override nomodeset (1 = override, -1 = auto). The default is -1 (auto).

**seamless (int)**

Seamless boot will keep the image on the screen during the boot process.

**debug\_mask (uint)**

Debug options for amdgpu, work as a binary mask with the following options:

* 0x1: Debug VM handling
* 0x2: Enable simulating large-bar capability on non-large bar system. This
  limits the VRAM size reported to ROCm applications to the visible
  size, usually 256MB.
* 0x4: Disable GPU soft recovery, always do a full reset
* 0x8: Use VRAM for firmware loading
* 0x10: Enable ACA based RAS logging
* 0x20: Enable experimental resets
* 0x40: Disable ring resets
* 0x80: Use VRAM for SMU pool

**agp (int)**

Enable the AGP aperture. This provides an aperture in the GPU’s internal
address space for direct access to system memory. Note that these accesses
are non-snooped, so they are only used for access to uncached memory.

**wbrf (int)**

Enable Wifi RFI interference mitigation feature.
Due to electrical and mechanical constraints there may be likely interference of
relatively high-powered harmonics of the (G-)DDR memory clocks with local radio
module frequency bands used by Wifi 6/6e/7. To mitigate the possible RFI interference,
with this feature enabled, PMFW will use either “shadowed P-State” or “P-State” based
on active list of frequencies in-use (to be avoided) as part of initial setting or
P-state transition. However, there may be potential performance impact with this
feature enabled.
(0 = disabled, 1 = enabled, -1 = auto (default setting, will be enabled if supported))

**rebar (int)**

Allow BAR resizing. Disable this to prevent the driver from attempting
to resize the BAR if the GPU supports it and there is available MMIO space.
Note that this just prevents the driver from resizing the BAR. The BIOS
may have already resized the BAR at boot time.

**user\_queue (int)**

Enable user queues on systems that support user queues. Possible values:

* -1 = auto (ASIC specific default)
* 0 = user queues disabled
* 1 = user queues enabled and kernel queues enabled (if supported)
* 2 = user queues enabled and kernel queues disabled
