# drm/amd/display - Display Core (DC)

> 출처(원문): https://docs.kernel.org/gpu/amdgpu/display/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# drm/amd/display - Display Core (DC)

AMD display engine is partially shared with other operating systems; for this
reason, our Display Core Driver is divided into two pieces:

1. **Display Core (DC)** contains the OS-agnostic components. Things like
   hardware programming and resource management are handled here.
2. **Display Manager (DM)** contains the OS-dependent components. Hooks to the
   amdgpu base driver and DRM are implemented here. For example, you can check
   display/amdgpu\_dm/ folder.

## DC Code validation

Maintaining the same code base across multiple OSes requires a lot of
synchronization effort between repositories and exhaustive validation. In the
DC case, we maintain a tree to centralize code from different parts. The shared
repository has integration tests with our Internal Linux CI farm, and we run a
comprehensive set of IGT tests in various AMD GPUs/APUs (mostly recent dGPUs
and APUs). Our CI also checks ARM64/32, PPC64/32, and x86\_64/32 compilation
with DCN enabled and disabled.

When we upstream a new feature or some patches, we pack them in a patchset with
the prefix **DC Patches for <DATE>**, which is created based on the latest
[amd-staging-drm-next](https://gitlab.freedesktop.org/agd5f/linux). All of
those patches are under a DC version tested as follows:

* Ensure that every patch compiles and the entire series pass our set of IGT
  test in different hardware.
* Prepare a branch with those patches for our validation team. If there is an
  error, a developer will debug as fast as possible; usually, a simple bisect
  in the series is enough to point to a bad change, and two possible actions
  emerge: fix the issue or drop the patch. If it is not an easy fix, the bad
  patch is dropped.
* Finally, developers wait a few days for community feedback before we merge
  the series.

It is good to stress that the test phase is something that we take extremely
seriously, and we never merge anything that fails our validation. Follows an
overview of our test set:

1. Manual test
   :   * Multiple Hotplugs with DP and HDMI.
       * Stress test with multiple display configuration changes via the user interface.
       * Validate VRR behaviour.
       * Check PSR.
       * Validate MPO when playing video.
       * Test more than two displays connected at the same time.
       * Check suspend/resume.
       * Validate FPO.
       * Check MST.
2. Automated test
   :   * IGT tests in a farm with GPUs and APUs that support DCN and DCE.
       * Compilation validation with the latest GCC and Clang from LTS distro.
       * Cross-compilation for PowerPC 64/32, ARM 64/32, and x86 32.

In terms of test setup for CI and manual tests, we usually use:

1. The latest Ubuntu LTS.
2. In terms of userspace, we only use fully updated open-source components
   provided by the distribution official package manager.
3. Regarding IGT, we use the latest code from the upstream.
4. Most of the manual tests are conducted in the GNome but we also use KDE.

Notice that someone from our test team will always reply to the cover letter
with the test report.

## DC Information

The display pipe is responsible for “scanning out” a rendered frame from the
GPU memory (also called VRAM, FrameBuffer, etc.) to a display. In other words,
it would:

1. Read frame information from memory;
2. Perform required transformation;
3. Send pixel data to sink devices.

If you want to learn more about our driver details, take a look at the below
table of content:

* [AMDgpu Display Manager](display-manager.html)
  + [Lifecycle](display-manager.html#lifecycle)
  + [Interrupts](display-manager.html#interrupts)
  + [Atomic Implementation](display-manager.html#atomic-implementation)
  + [Color Management Properties](display-manager.html#color-management-properties)
    - [drm\_crtc Properties](display-manager.html#drm-crtc-properties)
    - [AMD Private Color Management on drm\_plane](display-manager.html#amd-private-color-management-on-drm-plane)
    - [AMD plane color pipeline](display-manager.html#amd-plane-color-pipeline)
    - [DC Color Capabilities between DCN generations](display-manager.html#dc-color-capabilities-between-dcn-generations)
  + [Blend Mode Properties](display-manager.html#blend-mode-properties)
    - [Blend configuration flow](display-manager.html#blend-configuration-flow)
* [Display Core Next (DCN)](dcn-overview.html)
  + [Front End and Back End](dcn-overview.html#front-end-and-back-end)
  + [Data Flow](dcn-overview.html#data-flow)
  + [AMD Hardware Pipeline](dcn-overview.html#amd-hardware-pipeline)
  + [Global Sync](dcn-overview.html#global-sync)
* [DCN Blocks](dcn-blocks.html)
  + [DCHUBBUB](dcn-blocks.html#dchubbub)
  + [HUBP](dcn-blocks.html#hubp)
  + [DPP](dcn-blocks.html#dpp)
  + [MPC](dcn-blocks.html#mpc)
  + [OPP](dcn-blocks.html#opp)
  + [DIO](dcn-blocks.html#dio)
* [DC Programming Model](programming-model-dcn.html)
  + [Overview](programming-model-dcn.html#overview)
  + [Display Driver Architecture Overview](programming-model-dcn.html#display-driver-architecture-overview)
    - [Basic Objects](programming-model-dcn.html#basic-objects)
    - [Basic Operations](programming-model-dcn.html#basic-operations)
* [Multiplane Overlay (MPO)](mpo-overview.html)
  + [Plane Restrictions](mpo-overview.html#plane-restrictions)
  + [Cursor Restrictions](mpo-overview.html#cursor-restrictions)
  + [Use Cases](mpo-overview.html#use-cases)
    - [Picture-in-Picture (PIP) playback - Underlay strategy](mpo-overview.html#picture-in-picture-pip-playback-underlay-strategy)
    - [Multiple Display MPO](mpo-overview.html#multiple-display-mpo)
      * [Limitations](mpo-overview.html#limitations)
* [Display Core Debug tools](dc-debug.html)
  + [Narrow down display issues](dc-debug.html#narrow-down-display-issues)
    - [DC dmesg important messages](dc-debug.html#dc-dmesg-important-messages)
    - [Avoid loading display core](dc-debug.html#avoid-loading-display-core)
    - [Display flickering](dc-debug.html#display-flickering)
    - [Display artifacts](dc-debug.html#display-artifacts)
  + [Disabling/Enabling specific features](dc-debug.html#disabling-enabling-specific-features)
  + [DC Visual Confirmation](dc-debug.html#dc-visual-confirmation)
    - [Multiple Planes Debug](dc-debug.html#multiple-planes-debug)
    - [Pipe Split Debug](dc-debug.html#pipe-split-debug)
  + [DTN Debug](dc-debug.html#dtn-debug)
  + [Collect Firmware information](dc-debug.html#collect-firmware-information)
  + [DMUB Firmware Debug](dc-debug.html#dmub-firmware-debug)
    - [Trace Groups](dc-debug.html#trace-groups)
* [AMDGPU - Display Contributions](display-contributing.html)
  + [Gitlab issues](display-contributing.html#gitlab-issues)
  + [IGT](display-contributing.html#igt)
  + [Compilation](display-contributing.html#compilation)
    - [Fix compilation warnings](display-contributing.html#fix-compilation-warnings)
    - [Fix compilation issues when using um architecture](display-contributing.html#fix-compilation-issues-when-using-um-architecture)
  + [Code Refactor](display-contributing.html#code-refactor)
    - [Add prefix to DC functions to improve the debug with ftrace](display-contributing.html#add-prefix-to-dc-functions-to-improve-the-debug-with-ftrace)
    - [Reduce code duplication](display-contributing.html#reduce-code-duplication)
    - [Make atomic\_commit\_[check|tail] more readable](display-contributing.html#make-atomic-commit-check-tail-more-readable)
  + [Documentation](display-contributing.html#documentation)
    - [Expand kernel-doc](display-contributing.html#expand-kernel-doc)
  + [Beyond AMDGPU](display-contributing.html#beyond-amdgpu)
    - [Enable underlay](display-contributing.html#enable-underlay)
    - [Adaptive Backlight Modulation (ABM)](display-contributing.html#adaptive-backlight-modulation-abm)
    - [HDR & Color management & VRR](display-contributing.html#hdr-color-management-vrr)
* [DC Glossary](dc-glossary.html)
