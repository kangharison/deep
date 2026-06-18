# dGPU firmware flashing

> 출처(원문): https://docs.kernel.org/gpu/amdgpu/flashing.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# dGPU firmware flashing

## IFWI

Flashing the dGPU integrated firmware image (IFWI) is supported by GPUs that
use the PSP to orchestrate the update (Navi3x or newer GPUs).
For supported GPUs, amdgpu will export a series of sysfs files that can be
used for the flash process.

The IFWI flash process is:

1. Ensure the IFWI image is intended for the dGPU on the system.
2. “Write” the IFWI image to the sysfs file psp\_vbflash. This will stage the IFWI in memory.
3. “Read” from the psp\_vbflash sysfs file to initiate the flash process.
4. Poll the psp\_vbflash\_status sysfs file to determine when the flash process completes.

## USB-C PD F/W

On GPUs that support flashing an updated USB-C PD firmware image, the process
is done using the usbc\_pd\_fw sysfs file.

* Reading the file will provide the current firmware version.
* Writing the name of a firmware payload stored in /lib/firmware/amdgpu to the sysfs file will initiate the flash process.

The firmware payload stored in /lib/firmware/amdgpu can be named any name
as long as it doesn’t conflict with other existing binaries that are used by
amdgpu.

## sysfs files

**usbc\_pd\_fw**

Reading from this file will retrieve the USB-C PD firmware version. Writing to
this file will trigger the update process.

**psp\_vbflash**

Writing to this file will stage an IFWI for update. Reading from this file
will trigger the update process.

**psp\_vbflash\_status**

The status of the flash process.
0: IFWI flash not complete.
1: IFWI flash complete.
