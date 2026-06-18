# Kernel driver i2c-via

> 출처(원문): https://docs.kernel.org/i2c/busses/i2c-via.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kernel driver i2c-via

Supported adapters:
:   * VIA Technologies, InC. VT82C586B
      Datasheet: Publicly available at the VIA website

Author: Kyösti Mälkki <[kmalkki@cc.hut.fi](mailto:kmalkki%40cc.hut.fi)>

## Description

i2c-via is an i2c bus driver for motherboards with VIA chipset.

The following VIA pci chipsets are supported:
:   * MVP3, VP3, VP2/97, VPX/97
    * others with South bridge VT82C586B

Your `lspci` listing must show this

```
Bridge: VIA Technologies, Inc. VT82C586B ACPI (rev 10)
```

## Problems?

> Q:
> :   You have VT82C586B on the motherboard, but not in the listing.
>
> A:
> :   Go to your BIOS setup, section PCI devices or similar.
>     Turn USB support on, and try again.
>
> Q:
> :   No error messages, but still i2c doesn’t seem to work.
>
> A:
> :   This can happen. This driver uses the pins VIA recommends in their
>     datasheets, but there are several ways the motherboard manufacturer
>     can actually wire the lines.
