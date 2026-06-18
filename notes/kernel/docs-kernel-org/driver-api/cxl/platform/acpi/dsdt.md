# DSDT - Differentiated system Description Table

> 출처(원문): https://docs.kernel.org/driver-api/cxl/platform/acpi/dsdt.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# DSDT - Differentiated system Description Table

This table describes what peripherals a machine has.

This table’s UIDs for CXL devices - specifically host bridges, must be
consistent with the contents of the CEDT, otherwise the CXL driver will
fail to probe correctly.

Example Compute Express Link Host Bridge

```
Scope (_SB)
{
    Device (S0D0)
    {
        Name (_HID, "ACPI0016" /* Compute Express Link Host Bridge */)  // _HID: Hardware ID
        Name (_CID, Package (0x02)  // _CID: Compatible ID
        {
            EisaId ("PNP0A08") /* PCI Express Bus */,
            EisaId ("PNP0A03") /* PCI Bus */
        })
        ...
        Name (_UID, 0x05)  // _UID: Unique ID
        ...
  }
```
