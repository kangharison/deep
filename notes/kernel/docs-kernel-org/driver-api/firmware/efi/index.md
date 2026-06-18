# Unified Extensible Firmware Interface (UEFI) Support

> 출처(원문): https://docs.kernel.org/driver-api/firmware/efi/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Unified Extensible Firmware Interface (UEFI) Support

## UEFI stub library functions

efi\_status\_t efi\_get\_memory\_map(struct efi\_boot\_memmap \*\*map, bool install\_cfg\_tbl)
:   get memory map

**Parameters**

`struct efi_boot_memmap **map`
:   pointer to memory map pointer to which to assign the
    newly allocated memory map

`bool install_cfg_tbl`
:   whether or not to install the boot memory map as a
    configuration table

**Description**

Retrieve the UEFI memory map. The allocated memory leaves room for
up to EFI\_MMAP\_NR\_SLACK\_SLOTS additional memory map entries.

**Return**

status code

efi\_status\_t efi\_allocate\_pages(unsigned long size, unsigned long \*addr, unsigned long max)
:   Allocate memory pages

**Parameters**

`unsigned long size`
:   minimum number of bytes to allocate

`unsigned long *addr`
:   On return the address of the first allocated page. The first
    allocated page has alignment EFI\_ALLOC\_ALIGN which is an
    architecture dependent multiple of the page size.

`unsigned long max`
:   the address that the last allocated memory page shall not
    exceed

**Description**

Allocate pages as EFI\_LOADER\_DATA. The allocated pages are aligned according
to EFI\_ALLOC\_ALIGN. The last allocated page will not exceed the address
given by **max**.

**Return**

status code

void efi\_free(unsigned long size, unsigned long addr)
:   free memory pages

**Parameters**

`unsigned long size`
:   size of the memory area to free in bytes

`unsigned long addr`
:   start of the memory area to free (must be EFI\_PAGE\_SIZE
    aligned)

**Description**

**size** is rounded up to a multiple of EFI\_ALLOC\_ALIGN which is an
architecture specific multiple of EFI\_PAGE\_SIZE. So this function should
only be used to return pages allocated with [`efi_allocate_pages()`](#c.efi_allocate_pages "efi_allocate_pages") or
[`efi_low_alloc_above()`](#c.efi_low_alloc_above "efi_low_alloc_above").

efi\_status\_t efi\_low\_alloc\_above(unsigned long size, unsigned long align, unsigned long \*addr, unsigned long min)
:   allocate pages at or above given address

**Parameters**

`unsigned long size`
:   size of the memory area to allocate

`unsigned long align`
:   minimum alignment of the allocated memory area. It should
    a power of two.

`unsigned long *addr`
:   on exit the address of the allocated memory

`unsigned long min`
:   minimum address to used for the memory allocation

**Description**

Allocate at the lowest possible address that is not below **min** as
EFI\_LOADER\_DATA. The allocated pages are aligned according to **align** but at
least EFI\_ALLOC\_ALIGN. The first allocated page will not below the address
given by **min**.

**Return**

status code

## UEFI Common Platform Error Record (CPER) functions

void cper\_print\_bits(const char \*pfx, unsigned int bits, const char \*const strs[], unsigned int strs\_size)
:   print strings for set bits

**Parameters**

`const char *pfx`
:   prefix for each line, including log level and prefix string

`unsigned int bits`
:   bit mask

`const char * const strs[]`
:   string array, indexed by bit position

`unsigned int strs_size`
:   size of the string array: **strs**

**Description**

For each set bit in **bits**, print the corresponding string in **strs**.
If the output length is longer than 80, multiple line will be
printed, with **pfx** is printed at the beginning of each line.

int cper\_bits\_to\_str(char \*buf, int buf\_size, unsigned long bits, const char \*const strs[], unsigned int strs\_size)
:   return a string for set bits

**Parameters**

`char *buf`
:   buffer to store the output string

`int buf_size`
:   size of the output string buffer

`unsigned long bits`
:   bit mask

`const char * const strs[]`
:   string array, indexed by bit position

`unsigned int strs_size`
:   size of the string array: **strs**

**Description**

Add to **buf** the bitmask in hexadecimal. Then, for each set bit in **bits**,
add the corresponding string describing the bit in **strs** to **buf**.

A typical example is:

```
const char * const bits[] = {
        "bit 3 name",
        "bit 4 name",
        "bit 5 name",
};
char str[120];
unsigned int bitmask = BIT(3) | BIT(5);
#define MASK GENMASK(5,3)

cper_bits_to_str(str, sizeof(str), FIELD_GET(MASK, bitmask),
                 bits, ARRAY_SIZE(bits));
```

The above code fills the string `str` with `bit 3 name|bit 5 name`.

**Return**

number of bytes stored or an error code if lower than zero.
