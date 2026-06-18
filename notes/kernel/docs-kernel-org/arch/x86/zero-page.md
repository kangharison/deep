# 10.Zero Page

> 출처(원문): https://docs.kernel.org/arch/x86/zero-page.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 10. Zero Page

The additional fields in `struct boot_params` as a part of 32-bit boot
protocol of kernel. These should be filled by bootloader or 16-bit
real-mode setup code of the kernel. References/settings to it mainly
are in:

```
arch/x86/include/uapi/asm/bootparam.h
```

|  |  |  |  |
| --- | --- | --- | --- |
| Offset/Size | Proto | Name | Meaning |
| 000/040 | ALL | screen\_info | Text mode or frame buffer information (`struct screen_info`) |
| 040/014 | ALL | apm\_bios\_info | APM BIOS information (`struct apm_bios_info`) |
| 058/008 | ALL | tboot\_addr | Physical address of tboot shared page |
| 060/010 | ALL | ist\_info | Intel SpeedStep (IST) BIOS support information (`struct ist_info`) |
| 070/008 | ALL | acpi\_rsdp\_addr | Physical address of ACPI RSDP table |
| 080/010 | ALL | hd0\_info | hd0 disk parameter, OBSOLETE!! |
| 090/010 | ALL | hd1\_info | hd1 disk parameter, OBSOLETE!! |
| 0A0/010 | ALL | sys\_desc\_table | System description table (`struct sys_desc_table`), OBSOLETE!! |
| 0B0/010 | ALL | olpc\_ofw\_header | OLPC’s OpenFirmware CIF and friends |
| 0C0/004 | ALL | ext\_ramdisk\_image | ramdisk\_image high 32bits |
| 0C4/004 | ALL | ext\_ramdisk\_size | ramdisk\_size high 32bits |
| 0C8/004 | ALL | ext\_cmd\_line\_ptr | cmd\_line\_ptr high 32bits |
| 13C/004 | ALL | cc\_blob\_address | Physical address of Confidential Computing blob |
| 140/080 | ALL | edid\_info | Video mode setup (`struct edid_info`) |
| 1C0/020 | ALL | efi\_info | EFI 32 information (`struct efi_info`) |
| 1E0/004 | ALL | alt\_mem\_k | Alternative mem check, in KB |
| 1E4/004 | ALL | scratch | Scratch field for the kernel setup code |
| 1E8/001 | ALL | e820\_entries | Number of entries in e820\_table (below) |
| 1E9/001 | ALL | eddbuf\_entries | Number of entries in eddbuf (below) |
| 1EA/001 | ALL | edd\_mbr\_sig\_buf\_entries | Number of entries in edd\_mbr\_sig\_buffer (below) |
| 1EB/001 | ALL | kbd\_status | Numlock is enabled |
| 1EC/001 | ALL | secure\_boot | Secure boot is enabled in the firmware |
| 1EF/001 | ALL | sentinel | Used to detect broken bootloaders |
| 290/040 | ALL | edd\_mbr\_sig\_buffer | EDD MBR signatures |
| 2D0/A00 | ALL | e820\_table | E820 memory map table (array of `struct e820_entry`) |
| D00/1EC | ALL | eddbuf | EDD data (array of `struct edd_info`) |
