# VMCOREINFO

> 출처(원문): https://docs.kernel.org/admin-guide/kdump/vmcoreinfo.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# VMCOREINFO

## What is it?

VMCOREINFO is a special ELF note section. It contains various
information from the kernel like structure size, page size, symbol
values, field offsets, etc. These data are packed into an ELF note
section and used by user-space tools like crash and makedumpfile to
analyze a kernel’s memory layout.

## Common variables

### init\_uts\_ns.name.release

The version of the Linux kernel. Used to find the corresponding source
code from which the kernel has been built. For example, crash uses it to
find the corresponding vmlinux in order to process vmcore.

### PAGE\_SIZE

The size of a page. It is the smallest unit of data used by the memory
management facilities. It is usually 4096 bytes of size and a page is
aligned on 4096 bytes. Used for computing page addresses.

### init\_uts\_ns

The UTS namespace which is used to isolate two specific elements of the
system that relate to the uname(2) system call. It is named after the
data structure used to store information returned by the uname(2) system
call.

User-space tools can get the kernel name, host name, kernel release
number, kernel version, architecture name and OS type from it.

### (uts\_namespace, name)

Offset of the name’s member. Crash Utility and Makedumpfile get
the start address of the init\_uts\_ns.name from this.

### node\_online\_map

An array node\_states[N\_ONLINE] which represents the set of online nodes
in a system, one bit position per node number. Used to keep track of
which nodes are in the system and online.

### swapper\_pg\_dir

The global page directory pointer of the kernel. Used to translate
virtual to physical addresses.

### \_stext

Defines the beginning of the text section. In general, \_stext indicates
the kernel start address. Used to convert a virtual address from the
direct kernel map to a physical address.

### VMALLOC\_START

Stores the base address of vmalloc area. makedumpfile gets this value
since is necessary for vmalloc translation.

### mem\_map

Physical addresses are translated to `struct pages` by treating them as
an index into the mem\_map array. Right-shifting a physical address
PAGE\_SHIFT bits converts it into a page frame number which is an index
into that mem\_map array.

Used to map an address to the corresponding `struct page`.

### contig\_page\_data

Makedumpfile gets the pglist\_data structure from this symbol, which is
used to describe the memory layout.

User-space tools use this to exclude free pages when dumping memory.

### mem\_section|(mem\_section, NR\_SECTION\_ROOTS)|(mem\_section, section\_mem\_map)

The address of the mem\_section array, its length, structure size, and
the section\_mem\_map offset.

It exists in the sparse memory mapping model, and it is also somewhat
similar to the mem\_map variable, both of them are used to translate an
address.

### MAX\_PHYSMEM\_BITS

Defines the maximum supported physical address space memory.

### page

The size of a page structure. `struct page` is an important data structure
and it is widely used to compute contiguous memory.

### pglist\_data

The size of a pglist\_data structure. This value is used to check if the
pglist\_data structure is valid. It is also used for checking the memory
type.

### zone

The size of a zone structure. This value is used to check if the zone
structure has been found. It is also used for excluding free pages.

### free\_area

The size of a free\_area structure. It indicates whether the free\_area
structure is valid or not. Useful when excluding free pages.

### list\_head

The size of a list\_head structure. Used when iterating lists in a
post-mortem analysis session.

### nodemask\_t

The size of a nodemask\_t type. Used to compute the number of online
nodes.

### (page, flags|\_refcount|mapping|lru|\_mapcount|private|compound\_order|compound\_info)

User-space tools compute their values based on the offset of these
variables. The variables are used when excluding unnecessary pages.

### (pglist\_data, node\_zones|nr\_zones|node\_mem\_map|node\_start\_pfn|node\_spanned\_pages|node\_id)

On NUMA machines, each NUMA node has a pg\_data\_t to describe its memory
layout. On UMA machines there is a single pglist\_data which describes the
whole memory.

These values are used to check the memory type and to compute the
virtual address for memory map.

### (zone, free\_area|vm\_stat|spanned\_pages)

Each node is divided into a number of blocks called zones which
represent ranges within memory. A zone is described by a structure zone.

User-space tools compute required values based on the offset of these
variables.

### (free\_area, free\_list)

Offset of the free\_list’s member. This value is used to compute the number
of free pages.

Each zone has a free\_area structure array called free\_area[NR\_PAGE\_ORDERS].
The free\_list represents a linked list of free page blocks.

### (list\_head, next|prev)

Offsets of the list\_head’s members. list\_head is used to define a
circular linked list. User-space tools need these in order to traverse
lists.

### (vmap\_area, va\_start|list)

Offsets of the vmap\_area’s members. They carry vmalloc-specific
information. Makedumpfile gets the start address of the vmalloc region
from this.

### (zone.free\_area, NR\_PAGE\_ORDERS)

Free areas descriptor. User-space tools use this value to iterate the
free\_area ranges. NR\_PAGE\_ORDERS is used by the zone buddy allocator.

### prb

A pointer to the printk ringbuffer (`struct printk_ringbuffer`). This
may be pointing to the static boot ringbuffer or the dynamically
allocated ringbuffer, depending on when the core dump occurred.
Used by user-space tools to read the active kernel log buffer.

### printk\_rb\_static

A pointer to the static boot printk ringbuffer. If @prb has a
different value, this is useful for viewing the initial boot messages,
which may have been overwritten in the dynamically allocated
ringbuffer.

### clear\_seq

The sequence number of the [`printk()`](../../core-api/printk-basics.html#c.printk "printk") record after the last clear
command. It indicates the first record after the last
SYSLOG\_ACTION\_CLEAR, like issued by ‘dmesg -c’. Used by user-space
tools to dump a subset of the dmesg log.

### printk\_ringbuffer

The size of a printk\_ringbuffer structure. This structure contains all
information required for accessing the various components of the
kernel log buffer.

### (printk\_ringbuffer, desc\_ring|text\_data\_ring|dict\_data\_ring|fail)

Offsets for the various components of the printk ringbuffer. Used by
user-space tools to view the kernel log buffer without requiring the
declaration of the structure.

### prb\_desc\_ring

The size of the prb\_desc\_ring structure. This structure contains
information about the set of record descriptors.

### (prb\_desc\_ring, count\_bits|descs|head\_id|tail\_id)

Offsets for the fields describing the set of record descriptors. Used
by user-space tools to be able to traverse the descriptors without
requiring the declaration of the structure.

### prb\_desc

The size of the prb\_desc structure. This structure contains
information about a single record descriptor.

### (prb\_desc, info|state\_var|text\_blk\_lpos|dict\_blk\_lpos)

Offsets for the fields describing a record descriptors. Used by
user-space tools to be able to read descriptors without requiring
the declaration of the structure.

### prb\_data\_blk\_lpos

The size of the prb\_data\_blk\_lpos structure. This structure contains
information about where the text or dictionary data (data block) is
located within the respective data ring.

### (prb\_data\_blk\_lpos, begin|next)

Offsets for the fields describing the location of a data block. Used
by user-space tools to be able to locate data blocks without
requiring the declaration of the structure.

### printk\_info

The size of the printk\_info structure. This structure contains all
the meta-data for a record.

### (printk\_info, seq|ts\_nsec|text\_len|dict\_len|caller\_id)

Offsets for the fields providing the meta-data for a record. Used by
user-space tools to be able to read the information without requiring
the declaration of the structure.

### prb\_data\_ring

The size of the prb\_data\_ring structure. This structure contains
information about a set of data blocks.

### (prb\_data\_ring, size\_bits|data|head\_lpos|tail\_lpos)

Offsets for the fields describing a set of data blocks. Used by
user-space tools to be able to access the data blocks without
requiring the declaration of the structure.

### atomic\_long\_t

The size of the atomic\_long\_t structure. Used by user-space tools to
be able to copy the full structure, regardless of its
architecture-specific implementation.

### (atomic\_long\_t, counter)

Offset for the long value of an atomic\_long\_t variable. Used by
user-space tools to access the long value without requiring the
architecture-specific declaration.

### (free\_area.free\_list, MIGRATE\_TYPES)

The number of migrate types for pages. The free\_list is described by the
array. Used by tools to compute the number of free pages.

### NR\_FREE\_PAGES

On linux-2.6.21 or later, the number of free pages is in
vm\_stat[NR\_FREE\_PAGES]. Used to get the number of free pages.

### PG\_lru|PG\_private|PG\_swapcache|PG\_swapbacked|PG\_hwpoison|PG\_head\_mask

Page attributes. These flags are used to filter various unnecessary for
dumping pages.

### PAGE\_SLAB\_MAPCOUNT\_VALUE|PAGE\_BUDDY\_MAPCOUNT\_VALUE|PAGE\_OFFLINE\_MAPCOUNT\_VALUE|PAGE\_HUGETLB\_MAPCOUNT\_VALUE|PAGE\_UNACCEPTED\_MAPCOUNT\_VALUE

More page attributes. These flags are used to filter various unnecessary for
dumping pages.

## x86\_64

### phys\_base

Used to convert the virtual address of an exported kernel symbol to its
corresponding physical address.

### init\_top\_pgt

Used to walk through the whole page table and convert virtual addresses
to physical addresses. The init\_top\_pgt is somewhat similar to
swapper\_pg\_dir, but it is only used in x86\_64.

### pgtable\_l5\_enabled

User-space tools need to know whether the crash kernel was in 5-level
paging mode.

### node\_data

This is a `struct pglist_data` array and stores all NUMA nodes
information. Makedumpfile gets the pglist\_data structure from it.

### (node\_data, MAX\_NUMNODES)

The maximum number of nodes in system.

### KERNELOFFSET

The kernel randomization offset. Used to compute the page offset. If
KASLR is disabled, this value is zero.

### KERNEL\_IMAGE\_SIZE

Currently unused by Makedumpfile. Used to compute the module virtual
address by Crash.

### sme\_mask

AMD-specific with SME support: it indicates the secure memory encryption
mask. Makedumpfile tools need to know whether the crash kernel was
encrypted. If SME is enabled in the first kernel, the crash kernel’s
page table entries (pgd/pud/pmd/pte) contain the memory encryption
mask. This is used to remove the SME mask and obtain the true physical
address.

Currently, sme\_mask stores the value of the C-bit position. If needed,
additional SME-relevant info can be placed in that variable.

For example:

```
[ misc                ][ enc bit  ][ other misc SME info       ]
0000_0000_0000_0000_1000_0000_0000_0000_0000_0000_..._0000
63   59   55   51   47   43   39   35   31   27   ... 3
```

## x86\_32

### X86\_PAE

Denotes whether physical address extensions are enabled. It has the cost
of a higher page table lookup overhead, and also consumes more page
table space per process. Used to check whether PAE was enabled in the
crash kernel when converting virtual addresses to physical addresses.

## ARM64

### VA\_BITS

The maximum number of bits for virtual addresses. Used to compute the
virtual memory ranges.

### kimage\_voffset

The offset between the kernel virtual and physical mappings. Used to
translate virtual to physical addresses.

### PHYS\_OFFSET

Indicates the physical address of the start of memory. Similar to
kimage\_voffset, which is used to translate virtual to physical
addresses.

### KERNELOFFSET

The kernel randomization offset. Used to compute the page offset. If
KASLR is disabled, this value is zero.

### KERNELPACMASK

The mask to extract the Pointer Authentication Code from a kernel virtual
address.

### TCR\_EL1.T1SZ

Indicates the size offset of the memory region addressed by TTBR1\_EL1.
The region size is 2^(64-T1SZ) bytes.

TTBR1\_EL1 is the table base address register specified by ARMv8-A
architecture which is used to lookup the page-tables for the Virtual
addresses in the higher VA range (refer to ARMv8 ARM document for
more details).

### MODULES\_VADDR|MODULES\_END|VMALLOC\_START|VMALLOC\_END|VMEMMAP\_START|VMEMMAP\_END

Used to get the correct ranges:
:   MODULES\_VADDR ~ MODULES\_END-1 : Kernel module space.
    VMALLOC\_START ~ VMALLOC\_END-1 : [`vmalloc()`](../../core-api/mm-api.html#c.vmalloc "vmalloc") / [`ioremap()`](../../driver-api/device-io.html#c.ioremap "ioremap") space.
    VMEMMAP\_START ~ VMEMMAP\_END-1 : vmemmap region, used for `struct page` array.

## arm

### ARM\_LPAE

It indicates whether the crash kernel supports large physical address
extensions. Used to translate virtual to physical addresses.

## s390

### lowcore\_ptr

An array with a pointer to the lowcore of every CPU. Used to print the
psw and all registers information.

### high\_memory

Used to get the vmalloc\_start address from the high\_memory symbol.

### (lowcore\_ptr, NR\_CPUS)

The maximum number of CPUs.

## powerpc

### node\_data|(node\_data, MAX\_NUMNODES)

See above.

### contig\_page\_data

See above.

### vmemmap\_list

The vmemmap\_list maintains the entire vmemmap physical mapping. Used
to get vmemmap list count and populated vmemmap regions info. If the
vmemmap address translation information is stored in the crash kernel,
it is used to translate vmemmap kernel virtual addresses.

### mmu\_vmemmap\_psize

The size of a page. Used to translate virtual to physical addresses.

### mmu\_psize\_defs

Page size definitions, i.e. 4k, 64k, or 16M.

Used to make vtop translations.

### vmemmap\_backing|(vmemmap\_backing, list)|(vmemmap\_backing, phys)|(vmemmap\_backing, virt\_addr)

The vmemmap virtual address space management does not have a traditional
page table to track which virtual `struct pages` are backed by a physical
mapping. The virtual to physical mappings are tracked in a simple linked
list format.

User-space tools need to know the offset of list, phys and virt\_addr
when computing the count of vmemmap regions.

### mmu\_psize\_def|(mmu\_psize\_def, shift)

The size of a `struct mmu_psize_def` and the offset of mmu\_psize\_def’s
member.

Used in vtop translations.

## sh

### node\_data|(node\_data, MAX\_NUMNODES)

See above.

### X2TLB

Indicates whether the crashed kernel enabled SH extended mode.

## RISCV64

### VA\_BITS

The maximum number of bits for virtual addresses. Used to compute the
virtual memory ranges.

### PAGE\_OFFSET

Indicates the virtual kernel start address of the direct-mapped RAM region.

### phys\_ram\_base

Indicates the start physical RAM address.

### MODULES\_VADDR|MODULES\_END|VMALLOC\_START|VMALLOC\_END|VMEMMAP\_START|VMEMMAP\_END|KERNEL\_LINK\_ADDR

Used to get the correct ranges:

> * MODULES\_VADDR ~ MODULES\_END : Kernel module space.
> * VMALLOC\_START ~ VMALLOC\_END : [`vmalloc()`](../../core-api/mm-api.html#c.vmalloc "vmalloc") / [`ioremap()`](../../driver-api/device-io.html#c.ioremap "ioremap") space.
> * VMEMMAP\_START ~ VMEMMAP\_END : vmemmap space, used for `struct page` array.
> * KERNEL\_LINK\_ADDR : start address of Kernel link and BPF

### va\_kernel\_pa\_offset

Indicates the offset between the kernel virtual and physical mappings.
Used to translate virtual to physical addresses.
