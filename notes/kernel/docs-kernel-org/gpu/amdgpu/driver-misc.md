# Misc AMDGPU driver information

> 출처(원문): https://docs.kernel.org/gpu/amdgpu/driver-misc.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Misc AMDGPU driver information

## GPU Product Information

Information about the GPU can be obtained on certain cards
via sysfs

### product\_name

The amdgpu driver provides a sysfs API for reporting the product name
for the device
The file product\_name is used for this and returns the product name
as returned from the FRU.
NOTE: This is only available for certain server cards

### product\_number

The amdgpu driver provides a sysfs API for reporting the part number
for the device
The file product\_number is used for this and returns the part number
as returned from the FRU.
NOTE: This is only available for certain server cards

### serial\_number

The amdgpu driver provides a sysfs API for reporting the serial number
for the device
The file serial\_number is used for this and returns the serial number
as returned from the FRU.
NOTE: This is only available for certain server cards

### fru\_id

The amdgpu driver provides a sysfs API for reporting FRU File Id
for the device.
The file fru\_id is used for this and returns the File Id value
as returned from the FRU.
NOTE: This is only available for certain server cards

### manufacturer

The amdgpu driver provides a sysfs API for reporting manufacturer name from
FRU information.
The file manufacturer returns the value as returned from the FRU.
NOTE: This is only available for certain server cards

### unique\_id

The amdgpu driver provides a sysfs API for providing a unique ID for the GPU
The file unique\_id is used for this.
This will provide a Unique ID that will persist from machine to machine

NOTE: This will only work for GFX9 and newer. This file will be absent
on unsupported ASICs (GFX8 and older)

### board\_info

The amdgpu driver provides a sysfs API for giving board related information.
It provides the form factor information in the format

> type : form factor

Possible form factor values

* “cem” - PCIE CEM card
* “oam” - Open Compute Accelerator Module
* “unknown” - Not known

## GPU Memory Usage Information

Various memory accounting can be accessed via sysfs

### mem\_info\_vram\_total

The amdgpu driver provides a sysfs API for reporting current total VRAM
available on the device
The file mem\_info\_vram\_total is used for this and returns the total
amount of VRAM in bytes

### mem\_info\_vram\_used

The amdgpu driver provides a sysfs API for reporting current total VRAM
available on the device
The file mem\_info\_vram\_used is used for this and returns the total
amount of currently used VRAM in bytes

### mem\_info\_vis\_vram\_total

The amdgpu driver provides a sysfs API for reporting current total
visible VRAM available on the device
The file mem\_info\_vis\_vram\_total is used for this and returns the total
amount of visible VRAM in bytes

### mem\_info\_vis\_vram\_used

The amdgpu driver provides a sysfs API for reporting current total of
used visible VRAM
The file mem\_info\_vis\_vram\_used is used for this and returns the total
amount of currently used visible VRAM in bytes

### mem\_info\_gtt\_total

The amdgpu driver provides a sysfs API for reporting current total size of
the GTT.
The file mem\_info\_gtt\_total is used for this, and returns the total size of
the GTT block, in bytes

### mem\_info\_gtt\_used

The amdgpu driver provides a sysfs API for reporting current total amount of
used GTT.
The file mem\_info\_gtt\_used is used for this, and returns the current used
size of the GTT block, in bytes

## PCIe Accounting Information

### pcie\_bw

The amdgpu driver provides a sysfs API for estimating how much data
has been received and sent by the GPU in the last second through PCIe.
The file pcie\_bw is used for this.
The Perf counters count the number of received and sent messages and return
those values, as well as the maximum payload size of a PCIe packet (mps).
Note that it is not possible to easily and quickly obtain the size of each
packet transmitted, so we output the max payload size (mps) to allow for
quick estimation of the PCIe bandwidth usage

### pcie\_replay\_count

The amdgpu driver provides a sysfs API for reporting the total number
of PCIe replays (NAKs).
The file pcie\_replay\_count is used for this and returns the total
number of replays as a sum of the NAKs generated and NAKs received.

## GPU SmartShift Information

GPU SmartShift information via sysfs

### smartshift\_apu\_power

The amdgpu driver provides a sysfs API for reporting APU power
shift in percentage if platform supports smartshift. Value 0 means that
there is no powershift and values between [1-100] means that the power
is shifted to APU, the percentage of boost is with respect to APU power
limit on the platform.

### smartshift\_dgpu\_power

The amdgpu driver provides a sysfs API for reporting dGPU power
shift in percentage if platform supports smartshift. Value 0 means that
there is no powershift and values between [1-100] means that the power is
shifted to dGPU, the percentage of boost is with respect to dGPU power
limit on the platform.

### smartshift\_bias

The amdgpu driver provides a sysfs API for reporting the
smartshift(SS2.0) bias level. The value ranges from -100 to 100
and the default is 0. -100 sets maximum preference to APU
and 100 sets max perference to dGPU.

## UMA Carveout

Some versions of Atom ROM expose available options for the VRAM carveout sizes,
and allow changes to the carveout size via the ATCS function code 0xA on supported
BIOS implementations.

For those platforms, users can use the following files under uma/ to set the
carveout size, in a way similar to what Windows users can do in the “Tuning”
tab in AMD Adrenalin.

Note that for BIOS implementations that don’t support this, these files will not
be created at all.

### uma/carveout\_options

This is a read-only file that lists all available UMA allocation
options and their corresponding indices. Example output:

```
$ cat uma/carveout_options
0: Minimum (512 MB)
1:  (1 GB)
2:  (2 GB)
3:  (4 GB)
4:  (6 GB)
5:  (8 GB)
6:  (12 GB)
7: Medium (16 GB)
8:  (24 GB)
9: High (32 GB)
```

### uma/carveout

This file is both readable and writable. When read, it shows the
index of the current setting. Writing a valid index to this file
allows users to change the UMA carveout size to the selected option
on the next boot.

The available options and their corresponding indices can be read
from the uma/carveout\_options file.
