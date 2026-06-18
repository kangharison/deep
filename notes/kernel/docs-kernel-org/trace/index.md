# Linux Tracing Technologies Guide

> 출처(원문): https://docs.kernel.org/trace/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Linux Tracing Technologies Guide

Tracing in the Linux kernel is a powerful mechanism that allows
developers and system administrators to analyze and debug system
behavior. This guide provides documentation on various tracing
frameworks and tools available in the Linux kernel.

## Introduction to Tracing

This section provides an overview of Linux tracing mechanisms
and debugging approaches.

* [Using the tracer for debugging](debugging.html)
* [Using the Linux Kernel Tracepoints](tracepoints.html)
* [Notes on Analysing Behaviour Using Events and Tracepoints](tracepoint-analysis.html)
* [Tracefs ring-buffer memory mapping](ring-buffer-map.html)

## Core Tracing Frameworks

The following are the primary tracing frameworks integrated into
the Linux kernel.

* [ftrace - Function Tracer](ftrace.html)
* [Function Tracer Design](ftrace-design.html)
* [Using ftrace to hook to functions](ftrace-uses.html)
* [Kernel Probes (Kprobes)](kprobes.html)
* [Kprobe-based Event Tracing](kprobetrace.html)
* [Fprobe-based Event Tracing](fprobetrace.html)
* [Eprobe - Event-based Probe Tracing](eprobetrace.html)
* [Fprobe - Function entry/exit probe](fprobe.html)
* [Lockless Ring Buffer Design](ring-buffer-design.html)

## Event Tracing and Analysis

A detailed explanation of event tracing mechanisms and their
applications.

* [Event Tracing](events.html)
* [Subsystem Trace Points: kmem](events-kmem.html)
* [Subsystem Trace Points: power](events-power.html)
* [NMI Trace Events](events-nmi.html)
* [MSR Trace Events](events-msr.html)
* [Subsystem Trace Points: PCI](events-pci.html)
* [Subsystem Trace Points: PCI Controller](events-pci-controller.html)
* [Boot-time tracing](boottime-trace.html)
* [Event Histograms](histogram.html)
* [Histogram Design Notes](histogram-design.html)

## Hardware and Performance Tracing

This section covers tracing features that monitor hardware
interactions and system performance.

* [Intel(R) Trace Hub (TH)](intel_th.html)
* [System Trace Module](stm.html)
* [MIPI SyS-T over STP](sys-t.html)
* [CoreSight - ARM Hardware Trace](coresight/index.html)
* [Runtime Verification](rv/index.html)
* [HiSilicon PCIe Tune and Trace device](hisi-ptt.html)
* [In-kernel memory-mapped I/O tracing](mmiotrace.html)
* [Hardware Latency Detector](hwlat_detector.html)
* [OSNOISE Tracer](osnoise-tracer.html)
* [Timerlat tracer](timerlat-tracer.html)

## User-Space Tracing

These tools allow tracing user-space applications and
interactions.

* [user\_events: User-based Event Tracing](user_events.html)
* [Uprobe-tracer: Uprobe-based Event Tracing](uprobetracer.html)

## Remote Tracing

This section covers the framework to read compatible ring-buffers, written by
entities outside of the kernel (most likely firmware or hypervisor)

* [Tracing Remotes](remotes.html)

## Additional Resources

For more details, refer to the respective documentation of each
tracing tool and framework.
