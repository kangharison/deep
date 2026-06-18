# drm/vc4 Broadcom VC4 Graphics Driver

> 출처(원문): https://docs.kernel.org/gpu/vc4.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# drm/vc4 Broadcom VC4 Graphics Driver

The Broadcom VideoCore 4 (present in the Raspberry Pi) contains a
OpenGL ES 2.0-compatible 3D engine called V3D, and a highly
configurable display output pipeline that supports HDMI, DSI, DPI,
and Composite TV output.

The 3D engine also has an interface for submitting arbitrary
compute shader-style jobs using the same shader processor as is
used for vertex and fragment shaders in GLES 2.0. However, given
that the hardware isn’t able to expose any standard interfaces like
OpenGL compute shaders or OpenCL, it isn’t supported by this
driver.

## Display Hardware Handling

This section covers everything related to the display hardware including
the mode setting infrastructure, plane, sprite and cursor handling and
display, output probing and related topics.

### Pixel Valve (DRM CRTC)

In VC4, the Pixel Valve is what most closely corresponds to the
DRM’s concept of a CRTC. The PV generates video timings from the
encoder’s clock plus its configuration. It pulls scaled pixels from
the HVS at that timing, and feeds it to the encoder.

However, the DRM CRTC also collects the configuration of all the
DRM planes attached to it. As a result, the CRTC is also
responsible for writing the display list for the HVS channel that
the CRTC will use.

The 2835 has 3 different pixel valves. pv0 in the audio power
domain feeds DSI0 or DPI, while pv1 feeds DS1 or SMI. pv2 in the
image domain can feed either HDMI or the SDTV controller. The
pixel valve chooses from the CPRMAN clocks (HSM for HDMI, VEC for
SDTV, etc.) according to which output type is chosen in the mux.

For power management, the pixel valve’s registers are all clocked
by the AXI clock, while the timings and FIFOs make use of the
output-specific clock. Since the encoders also directly consume
the CPRMAN clocks, and know what timings they need, they are the
ones that set the clock.

### HVS

The Hardware Video Scaler (HVS) is the piece of hardware that does
translation, scaling, colorspace conversion, and compositing of
pixels stored in framebuffers into a FIFO of pixels going out to
the Pixel Valve (CRTC). It operates at the system clock rate (the
system audio clock gate, specifically), which is much higher than
the pixel clock rate.

There is a single global HVS, with multiple output FIFOs that can
be consumed by the PVs. This file just manages the resources for
the HVS, while the vc4\_crtc.c code actually drives HVS setup for
each CRTC.

### HVS planes

Each DRM plane is a layer of pixels being scanned out by the HVS.

At atomic modeset check time, we compute the HVS display element
state that would be necessary for displaying the plane (giving us a
chance to figure out if a plane configuration is invalid), then at
atomic flush time the CRTC will ask us to write our element state
into the region of the HVS that it has allocated for us.

### HDMI encoder

The HDMI core has a state machine and a PHY. On BCM2835, most of
the unit operates off of the HSM clock from CPRMAN. It also
internally uses the PLLH\_PIX clock for the PHY.

HDMI infoframes are kept within a small packet ram, where each
packet can be individually enabled for including in a frame.

HDMI audio is implemented entirely within the HDMI IP block. A
register in the HDMI encoder takes SPDIF frames from the DMA engine
and transfers them over an internal MAI (multi-channel audio
interconnect) bus to the encoder side for insertion into the video
blank regions.

The driver’s HDMI encoder does not yet support power management.
The HDMI encoder’s power domain and the HSM/pixel clocks are kept
continuously running, and only the HDMI logic and packet ram are
powered off/on at disable/enable time.

The driver does not yet support CEC control, though the HDMI
encoder block has CEC support.

### DSI encoder

BCM2835 contains two DSI modules, DSI0 and DSI1. DSI0 is a
single-lane DSI controller, while DSI1 is a more modern 4-lane DSI
controller.

Most Raspberry Pi boards expose DSI1 as their “DISPLAY” connector,
while the compute module brings both DSI0 and DSI1 out.

This driver has been tested for DSI1 video-mode display only
currently, with most of the information necessary for DSI0
hopefully present.

### DPI encoder

The VC4 DPI hardware supports MIPI DPI type 4 and Nokia ViSSI
signals. On BCM2835, these can be routed out to GPIO0-27 with the
ALT2 function.

### VEC (Composite TV out) encoder

The VEC encoder generates PAL or NTSC composite video output.

TV mode selection is done by an atomic property on the encoder,
because a drm\_mode\_modeinfo is insufficient to distinguish between
PAL and PAL-M or NTSC and NTSC-J.

## KUnit Tests

The VC4 Driver uses KUnit to perform driver-specific unit and
integration tests.

These tests are using a mock driver and can be ran using the
command below, on either arm or arm64 architectures,

```
$ ./tools/testing/kunit/kunit.py run \
        --kunitconfig=drivers/gpu/drm/vc4/tests/.kunitconfig \
        --cross_compile aarch64-linux-gnu- --arch arm64
```

Parts of the driver that are currently covered by tests are:
:   * The HVS to PixelValve dynamic FIFO assignment, for the BCM2835-7
      and BCM2711.

## Memory Management and 3D Command Submission

This section covers the GEM implementation in the vc4 driver.

### GPU buffer object (BO) management

The VC4 GPU architecture (both scanout and rendering) has direct
access to system memory with no MMU in between. To support it, we
use the GEM DMA helper functions to allocate contiguous ranges of
physical memory for our BOs.

Since the DMA allocator is very slow, we keep a cache of recently
freed BOs around so that the kernel’s allocation of objects for 3D
rendering can return quickly.

### V3D binner command list (BCL) validation

Since the VC4 has no IOMMU between it and system memory, a user
with access to execute command lists could escalate privilege by
overwriting system memory (drawing to it as a framebuffer) or
reading system memory it shouldn’t (reading it as a vertex buffer
or index buffer)

We validate binner command lists to ensure that all accesses are
within the bounds of the GEM objects referenced by the submitted
job. It explicitly whitelists packets, and looks at the offsets in
any address fields to make sure they’re contained within the BOs
they reference.

Note that because CL validation is already reading the
user-submitted CL and writing the validated copy out to the memory
that the GPU will actually read, this is also where GEM relocation
processing (turning BO references into actual addresses for the GPU
to use) happens.

### V3D render command list (RCL) generation

In the V3D hardware, render command lists are what load and store
tiles of a framebuffer and optionally call out to binner-generated
command lists to do the 3D drawing for that tile.

In the VC4 driver, render command list generation is performed by the
kernel instead of userspace. We do this because validating a
user-submitted command list is hard to get right and has high CPU overhead,
while the number of valid configurations for render command lists is
actually fairly low.

### Shader validator for VC4

Since the VC4 has no IOMMU between it and system memory, a user
with access to execute shaders could escalate privilege by
overwriting system memory (using the VPM write address register in
the general-purpose DMA mode) or reading system memory it shouldn’t
(reading it as a texture, uniform data, or direct-addressed TMU
lookup).

The shader validator walks over a shader’s BO, ensuring that its
accesses are appropriately bounded, and recording where texture
accesses are made so that we can do relocations for them in the
uniform stream.

Shader BO are immutable for their lifetimes (enforced by not
allowing mmaps, GEM prime export, or rendering to from a CL), so
this validation is only performed at BO creation time.

### V3D Interrupts

We have an interrupt status register (V3D\_INTCTL) which reports
interrupts, and where writing 1 bits clears those interrupts.
There are also a pair of interrupt registers
(V3D\_INTENA/V3D\_INTDIS) where writing a 1 to their bits enables or
disables that specific interrupt, and 0s written are ignored
(reading either one returns the set of enabled interrupts).

When we take a binning flush done interrupt, we need to submit the
next frame for binning and move the finished frame to the render
thread.

When we take a render frame interrupt, we need to wake the
processes waiting for some frame to be done, and get the next frame
submitted ASAP (so the hardware doesn’t sit idle when there’s work
to do).

When we take the binner out of memory interrupt, we need to
allocate some new memory and pass it to the binner so that the
current job can make progress.
