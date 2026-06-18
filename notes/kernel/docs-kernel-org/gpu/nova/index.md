# nova NVIDIA GPU drivers

> 출처(원문): https://docs.kernel.org/gpu/nova/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# nova NVIDIA GPU drivers

The nova driver project consists out of two separate drivers nova-core and
nova-drm and intends to supersede the nouveau driver for NVIDIA GPUs based on
the GPU System Processor (GSP).

The following documents apply to both nova-core and nova-drm.

* [Guidelines](guidelines.html)

## nova-core

The nova-core driver is the core driver for NVIDIA GPUs based on GSP. nova-core,
as the 1st level driver, provides an abstraction around the GPUs hard- and
firmware interfaces providing a common base for 2nd level drivers, such as the
vGPU manager VFIO driver and the nova-drm driver.

* [Guidelines](core/guidelines.html)
* [Task List](core/todo.html)
* [VBIOS](core/vbios.html)
* [Device Initialization (devinit)](core/devinit.html)
* [FWSEC (Firmware Security)](core/fwsec.html)
* [Falcon (FAst Logic Controller)](core/falcon.html)
