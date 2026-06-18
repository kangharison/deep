# AMDGPU RAS Support

> 출처(원문): https://docs.kernel.org/gpu/amdgpu/ras.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# AMDGPU RAS Support

The AMDGPU RAS interfaces are exposed via sysfs (for informational queries) and
debugfs (for error injection).

## RAS debugfs/sysfs Control and Error Injection Interfaces

The control interface accepts `struct ras_debug_if` which has two members.

First member: ras\_debug\_if::head or ras\_debug\_if::inject.

head is used to indicate which IP block will be under control.

head has four members, they are block, type, sub\_block\_index, name.
block: which IP will be under control.
type: what kind of error will be enabled/disabled/injected.
sub\_block\_index: some IPs have subcomponets. say, GFX, sDMA.
name: the name of IP.

inject has three more members than head, they are address, value and mask.
As their names indicate, inject operation will write the
value to the address.

The second member: `struct ras_debug_if`::op.
It has three kinds of operations.

* 0: disable RAS on the block. Take ::head as its data.
* 1: enable RAS on the block. Take ::head as its data.
* 2: inject errors on the block. Take ::inject as its data.

How to use the interface?

In a program

Copy the `struct ras_debug_if` in your code and initialize it.
Write the `struct to` the control interface.

From shell

```
echo "disable <block>" > /sys/kernel/debug/dri/<N>/ras/ras_ctrl
echo "enable  <block> <error>" > /sys/kernel/debug/dri/<N>/ras/ras_ctrl
echo "inject  <block> <error> <sub-block> <address> <value> <mask>" > /sys/kernel/debug/dri/<N>/ras/ras_ctrl
```

Where N, is the card which you want to affect.

“disable” requires only the block.
“enable” requires the block and error type.
“inject” requires the block, error type, address, and value.

The block is one of: umc, sdma, gfx, etc.
:   see ras\_block\_string[] for details

The error type is one of: ue, ce and poison where,
:   ue is multi-uncorrectable
    ce is single-correctable
    poison is poison

The sub-block is a the sub-block index, pass 0 if there is no sub-block.
The address and value are hexadecimal numbers, leading 0x is optional.
The mask means instance mask, is optional, default value is 0x1.

For instance,

```
echo inject umc ue 0x0 0x0 0x0 > /sys/kernel/debug/dri/0/ras/ras_ctrl
echo inject umc ce 0 0 0 3 > /sys/kernel/debug/dri/0/ras/ras_ctrl
echo disable umc > /sys/kernel/debug/dri/0/ras/ras_ctrl
```

How to check the result of the operation?

To check disable/enable, see “ras” features at,
/sys/class/drm/card[0/1/2...]/device/ras/features

To check inject, see the corresponding error count at,
/sys/class/drm/card[0/1/2...]/device/ras/[gfx|sdma|umc|...]\_err\_count

Note

Operations are only allowed on blocks which are supported.
Check the “ras” mask at /sys/module/amdgpu/parameters/ras\_mask
to see which blocks support RAS on a particular asic.

## RAS Reboot Behavior for Unrecoverable Errors

Normally when there is an uncorrectable error, the driver will reset
the GPU to recover. However, in the event of an unrecoverable error,
the driver provides an interface to reboot the system automatically
in that event.

The following file in debugfs provides that interface:
/sys/kernel/debug/dri/[0/1/2...]/ras/auto\_reboot

Usage:

```
echo true > .../ras/auto_reboot
```

## RAS Error Count sysfs Interface

It allows the user to read the error count for each IP block on the gpu through
/sys/class/drm/card[0/1/2...]/device/ras/[gfx/sdma/...]\_err\_count

It outputs the multiple lines which report the uncorrected (ue) and corrected
(ce) error counts.

The format of one line is below,

[ce|ue]: count

Example:

```
ue: 0
ce: 1
```

## RAS EEPROM debugfs Interface

Some boards contain an EEPROM which is used to persistently store a list of
bad pages which experiences ECC errors in vram. This interface provides
a way to reset the EEPROM, e.g., after testing error injection.

Usage:

```
echo 1 > ../ras/ras_eeprom_reset
```

will reset EEPROM table to 0 entries.

## RAS VRAM Bad Pages sysfs Interface

It allows user to read the bad pages of vram on the gpu through
/sys/class/drm/card[0/1/2...]/device/ras/gpu\_vram\_bad\_pages

It outputs multiple lines, and each line stands for one gpu page.

The format of one line is below,
gpu pfn : gpu page size : flags

gpu pfn and gpu page size are printed in hex format.
flags can be one of below character,

R: reserved, this gpu page is reserved and not able to use.

P: pending for reserve, this gpu page is marked as bad, will be reserved
in next window of page\_reserve.

F: unable to reserve. this gpu page can’t be reserved due to some reasons.

Examples:

```
0x00000001 : 0x00001000 : R
0x00000002 : 0x00001000 : P
```

## Sample Code

Sample code for testing error injection can be found here:
<https://cgit.freedesktop.org/mesa/drm/tree/tests/amdgpu/ras_tests.c>

This is part of the libdrm amdgpu unit tests which cover several areas of the GPU.
There are four sets of tests:

RAS Basic Test

The test verifies the RAS feature enabled status and makes sure the necessary sysfs and debugfs files
are present.

RAS Query Test

This test checks the RAS availability and enablement status for each supported IP block as well as
the error counts.

RAS Inject Test

This test injects errors for each IP.

RAS Disable Test

This test tests disabling of RAS features for each IP block.
