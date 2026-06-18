# Generic Radix Page Table

> 출처(원문): https://docs.kernel.org/driver-api/generic_pt.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Generic Radix Page Table

Generic Radix Page Table is a set of functions and helpers to efficiently
parse radix style page tables typically seen in HW implementations. The
interface is built to deliver similar code generation as the mm’s pte/pmd/etc
system by fully inlining the exact code required to handle each table level.

Like the mm subsystem each format contributes its parsing implementation
under common names and the common code implements the required algorithms.

The system is divided into three logical levels:

> * The page table format and its manipulation functions
> * Generic helpers to give a consistent API regardless of underlying format
> * An algorithm implementation (e.g. IOMMU/DRM/KVM/MM)

Multiple implementations are supported. The intention is to have the generic
format code be re-usable for whatever specialized implementation is required.
The generic code is solely about the format of the radix tree; it does not
include memory allocation or higher level decisions that are left for the
implementation.

The generic framework supports a superset of functions across many HW
implementations:

> * Entries comprised of contiguous blocks of IO PTEs for larger page sizes
> * Multi-level tables, up to 6 levels. Runtime selected top level
> * Runtime variable table level size (ARM’s concatenated tables)
> * Expandable top level allowing dynamic sizing of table levels
> * Optional leaf entries at any level
> * 32-bit/64-bit virtual and output addresses, using every address bit
> * Dirty tracking
> * Sign extended addressing

Language used in Generic Page Table
:   VA
    :   The input address to the page table, often the virtual address.

    OA
    :   The output address from the page table, often the physical address.

    leaf
    :   An entry that results in an output address.

    start/end
    :   An half-open range, e.g. [0,0) refers to no VA.

    start/last
    :   An inclusive closed range, e.g. [0,0] refers to the VA 0

    common
    :   The generic page table container [`struct pt_common`](#c.pt_common "pt_common")

    level
    :   Level 0 is always a table of only leaves with no futher table pointers.
        Increasing levels increase the size of the table items. The least
        significant VA bits used to index page tables are used to index the Level
        0 table. The various labels for table levels used by HW descriptions are
        not used.

    top\_level
    :   The inclusive highest level of the table. A two-level table
        has a top level of 1.

    table
    :   A linear array of translation items for that level.

    index
    :   The position in a table of an element: item = table[index]

    item
    :   A single index in a table

    entry
    :   A single logical element in a table. If contiguous pages are not
        supported then item and entry are the same thing, otherwise entry refers
        to all the items that comprise a single contiguous translation.

    item/entry\_size
    :   The number of bytes of VA the table index translates for.
        If the item is a table entry then the next table covers
        this size. If the entry translates to an output address then the
        full OA is: OA | (VA % entry\_size)

    contig\_count
    :   The number of consecutive items fused into a single entry.
        item\_size \* contig\_count is the size of that entry’s translation.

    lg2
    :   Indicates the value is encoded as log2, i.e. 1<<x is the actual value.
        Normally the compiler is fine to optimize divide and mod with log2 values
        automatically when inlining, however if the values are not constant
        expressions it can’t. So we do it by hand; we want to avoid 64-bit
        divmod.

## Usage

Generic PT is structured as a multi-compilation system. Since each format
provides an API using a common set of names there can be only one format active
within a compilation unit. This design avoids function pointers around the low
level API.

Instead the function pointers can end up at the higher level API (i.e.
map/unmap, etc.) and the per-format code can be directly inlined into the
per-format compilation unit. For something like IOMMU each format will be
compiled into a per-format IOMMU operations kernel module.

For this to work the .c file for each compilation unit will include both the
format headers and the generic code for the implementation. For instance in an
implementation compilation unit the headers would normally be included as
follows:

generic\_pt/fmt/iommu\_amdv1.c:

```
#include <linux/generic_pt/common.h>
#include "defs_amdv1.h"
#include "../pt_defs.h"
#include "amdv1.h"
#include "../pt_common.h"
#include "../pt_iter.h"
#include "../iommu_pt.h"  /* The IOMMU implementation */
```

iommu\_pt.h includes definitions that will generate the operations functions for
map/unmap/etc. using the definitions provided by AMDv1. The resulting module
will have exported symbols named like `pt_iommu_amdv1_init()`.

Refer to drivers/iommu/generic\_pt/fmt/iommu\_template.h for an example of how the
IOMMU implementation uses multi-compilation to generate per-format ops structs
pointers.

The format code is written so that the common names arise from #defines to
distinct format specific names. This is intended to aid debuggability by
avoiding symbol clashes across all the different formats.

Exported symbols and other global names are mangled using a per-format string
via the `NS()` helper macro.

The format uses [`struct pt_common`](#c.pt_common "pt_common") as the top-level struct for the table,
and each format will have its own `struct pt_xxx` which embeds it to store
format-specific information.

The implementation will further wrap [`struct pt_common`](#c.pt_common "pt_common") in its own top-level
struct, such as `struct pt_iommu_amdv1`.

### Format functions at the struct pt\_common level

struct pt\_common
:   struct for all page table implementations

**Definition**:

```
struct pt_common {
    uintptr_t top_of_table;
    u8 max_oasz_lg2;
    u8 max_vasz_lg2;
    unsigned int features;
};
```

**Members**

`top_of_table`
:   Encodes the table top pointer and the top level in a
    single value. Must use READ\_ONCE/WRITE\_ONCE to access it. The lower
    bits of the aligned table pointer are used for the level.

`max_oasz_lg2`
:   Maximum number of bits the OA can contain. Upper bits
    must be zero. This may be less than what the page table format
    supports, but must not be more.

`max_vasz_lg2`
:   Maximum number of bits the VA can contain. Upper bits
    are 0 or 1 depending on [`pt_full_va_prefix()`](#c.pt_full_va_prefix "pt_full_va_prefix"). This may be less than
    what the page table format supports, but must not be more. When
    PT\_FEAT\_DYNAMIC\_TOP is set this reflects the maximum VA capability.

`features`
:   Bitmap of [`enum pt_features`](#c.pt_features "pt_features")

enum pt\_features
:   Features turned on in the table. Each symbol is a bit position.

**Constants**

`PT_FEAT_DMA_INCOHERENT`
:   Cache flush page table memory before
    assuming the HW can read it. Otherwise a SMP release is sufficient
    for HW to read it.

`PT_FEAT_FULL_VA`
:   The table can span the full VA range from 0 to
    PT\_VADDR\_MAX.

`PT_FEAT_DYNAMIC_TOP`
:   The table’s top level can be increased
    dynamically during map. This requires HW support for atomically
    setting both the table top pointer and the starting table level.

`PT_FEAT_SIGN_EXTEND`
:   The top most bit of the valid VA range sign
    extends up to the full pt\_vaddr\_t. This divides the page table into
    three VA ranges:

    ```
    0         -> 2^N - 1             Lower
    2^N       -> (MAX - 2^N - 1)     Non-Canonical
    MAX - 2^N -> MAX                 Upper
    ```

    In this mode pt\_common::max\_vasz\_lg2 includes the sign bit and the
    upper bits that don’t fall within the translation are just validated.

    If not set there is no sign extension and valid VA goes from 0 to 2^N
    - 1.

`PT_FEAT_FLUSH_RANGE`
:   IOTLB maintenance is done by flushing IOVA
    ranges which will clean out any walk cache or any IOPTE fully
    contained by the range. The optimization objective is to minimize the
    number of flushes even if ranges include IOVA gaps that do not need
    to be flushed.

`PT_FEAT_FLUSH_RANGE_NO_GAPS`
:   Like PT\_FEAT\_FLUSH\_RANGE except that
    the optimization objective is to only flush IOVA that has been
    changed. This mode is suitable for cases like hypervisor shadowing
    where flushing unchanged ranges may cause the hypervisor to reparse
    significant amount of page table.

void pt\_attr\_from\_entry(const struct pt\_state \*pts, struct pt\_write\_attrs \*attrs)
:   Convert the permission bits back to attrs

**Parameters**

`const struct pt_state *pts`
:   Entry to convert from

`struct pt_write_attrs *attrs`
:   Resulting attrs

**Description**

Fill in the attrs with the permission bits encoded in the current leaf entry.
The attrs should be usable with [`pt_install_leaf_entry()`](#c.pt_install_leaf_entry "pt_install_leaf_entry") to reconstruct the
same entry.

bool pt\_can\_have\_leaf(const struct pt\_state \*pts)
:   True if the current level can have an OA entry

**Parameters**

`const struct pt_state *pts`
:   The current level

**Description**

True if the current level can support [`pt_install_leaf_entry()`](#c.pt_install_leaf_entry "pt_install_leaf_entry"). A leaf
entry produce an OA.

bool pt\_can\_have\_table(const struct pt\_state \*pts)
:   True if the current level can have a lower table

**Parameters**

`const struct pt_state *pts`
:   The current level

**Description**

Every level except 0 is allowed to have a lower table.

void pt\_clear\_entries(struct pt\_state \*pts, unsigned int num\_contig\_lg2)
:   Make entries empty (non-present)

**Parameters**

`struct pt_state *pts`
:   Starting table index

`unsigned int num_contig_lg2`
:   Number of contiguous items to clear

**Description**

Clear a run of entries. A cleared entry will load back as PT\_ENTRY\_EMPTY
and does not have any effect on table walking. The starting index must be
aligned to num\_contig\_lg2.

bool pt\_entry\_make\_write\_dirty(struct pt\_state \*pts)
:   Make an entry dirty

**Parameters**

`struct pt_state *pts`
:   Table entry to change

**Description**

Make [`pt_entry_is_write_dirty()`](#c.pt_entry_is_write_dirty "pt_entry_is_write_dirty") return true for this entry. This can be called
asynchronously with any other table manipulation under a RCU lock and must
not corrupt the table.

void pt\_entry\_make\_write\_clean(struct pt\_state \*pts)
:   Make the entry write clean

**Parameters**

`struct pt_state *pts`
:   Table entry to change

**Description**

Modify the entry so that [`pt_entry_is_write_dirty()`](#c.pt_entry_is_write_dirty "pt_entry_is_write_dirty") == false. The HW will
eventually be notified of this change via a TLB flush, which is the point
that the HW must become synchronized. Any “write dirty” prior to the TLB
flush can be lost, but once the TLB flush completes all writes must make
their entries write dirty.

The format should alter the entry in a way that is compatible with any
concurrent update from HW. The entire contiguous entry is changed.

bool pt\_entry\_is\_write\_dirty(const struct pt\_state \*pts)
:   True if the entry has been written to

**Parameters**

`const struct pt_state *pts`
:   Entry to query

**Description**

“write dirty” means that the HW has written to the OA translated
by this entry. If the entry is contiguous then the consolidated
“write dirty” for all the items must be returned.

bool pt\_dirty\_supported(struct [pt\_common](#c.pt_common "pt_common") \*common)
:   True if the page table supports dirty tracking

**Parameters**

`struct pt_common *common`
:   Page table to query

unsigned int pt\_entry\_num\_contig\_lg2(const struct pt\_state \*pts)
:   Number of contiguous items for this leaf entry

**Parameters**

`const struct pt_state *pts`
:   Entry to query

**Description**

Return the number of contiguous items this leaf entry spans. If the entry
is single item it returns ilog2(1).

pt\_oaddr\_t pt\_entry\_oa(const struct pt\_state \*pts)
:   Output Address for this leaf entry

**Parameters**

`const struct pt_state *pts`
:   Entry to query

**Description**

Return the output address for the start of the entry. If the entry
is contiguous this returns the same value for each sub-item. I.e.:

```
log2_mod(pt_entry_oa(), pt_entry_oa_lg2sz()) == 0
```

See [`pt_item_oa()`](#c.pt_item_oa "pt_item_oa"). The format should implement one of these two functions
depending on how it stores the OAs in the table.

unsigned int pt\_entry\_oa\_lg2sz(const struct pt\_state \*pts)
:   Return the size of an OA entry

**Parameters**

`const struct pt_state *pts`
:   Entry to query

**Description**

If the entry is not contiguous this returns [`pt_table_item_lg2sz()`](#c.pt_table_item_lg2sz "pt_table_item_lg2sz"), otherwise
it returns the total VA/OA size of the entire contiguous entry.

pt\_oaddr\_t pt\_entry\_oa\_exact(const struct pt\_state \*pts)
:   Return the complete OA for an entry

**Parameters**

`const struct pt_state *pts`
:   Entry to query

**Description**

During iteration the first entry could have a VA with an offset from the
natural start of the entry. Return the exact OA including the pts’s VA
offset.

pt\_vaddr\_t pt\_full\_va\_prefix(const struct [pt\_common](#c.pt_common "pt_common") \*common)
:   The top bits of the VA

**Parameters**

`const struct pt_common *common`
:   Page table to query

**Description**

This is usually 0, but some formats have their VA space going downward from
PT\_VADDR\_MAX, and will return that instead. This value must always be
adjusted by [`struct pt_common`](#c.pt_common "pt_common") max\_vasz\_lg2.

bool pt\_has\_system\_page\_size(const struct [pt\_common](#c.pt_common "pt_common") \*common)
:   True if level 0 can install a PAGE\_SHIFT entry

**Parameters**

`const struct pt_common *common`
:   Page table to query

**Description**

If true the caller can use, at level 0, pt\_install\_leaf\_entry(PAGE\_SHIFT).
This is useful to create optimized paths for common cases of PAGE\_SIZE
mappings.

void pt\_install\_leaf\_entry(struct pt\_state \*pts, pt\_oaddr\_t oa, unsigned int oasz\_lg2, const struct pt\_write\_attrs \*attrs)
:   Write a leaf entry to the table

**Parameters**

`struct pt_state *pts`
:   Table index to change

`pt_oaddr_t oa`
:   Output Address for this leaf

`unsigned int oasz_lg2`
:   Size in VA/OA for this leaf

`const struct pt_write_attrs *attrs`
:   Attributes to modify the entry

**Description**

A leaf OA entry will return PT\_ENTRY\_OA from [`pt_load_entry()`](#c.pt_load_entry "pt_load_entry"). It translates
the VA indicated by pts to the given OA.

For a single item non-contiguous entry oasz\_lg2 is [`pt_table_item_lg2sz()`](#c.pt_table_item_lg2sz "pt_table_item_lg2sz").
For contiguous it is [`pt_table_item_lg2sz()`](#c.pt_table_item_lg2sz "pt_table_item_lg2sz") + num\_contig\_lg2.

This must not be called if [`pt_can_have_leaf()`](#c.pt_can_have_leaf "pt_can_have_leaf") == false. Contiguous sizes
not indicated by [`pt_possible_sizes()`](#c.pt_possible_sizes "pt_possible_sizes") must not be specified.

bool pt\_install\_table(struct pt\_state \*pts, pt\_oaddr\_t table\_pa, const struct pt\_write\_attrs \*attrs)
:   Write a table entry to the table

**Parameters**

`struct pt_state *pts`
:   Table index to change

`pt_oaddr_t table_pa`
:   CPU physical address of the lower table’s memory

`const struct pt_write_attrs *attrs`
:   Attributes to modify the table index

**Description**

A table entry will return PT\_ENTRY\_TABLE from [`pt_load_entry()`](#c.pt_load_entry "pt_load_entry"). The table\_pa
is the table at pts->level - 1. This is done by cmpxchg so pts must have the
current entry loaded. The pts is updated with the installed entry.

This must not be called if [`pt_can_have_table()`](#c.pt_can_have_table "pt_can_have_table") == false.

**Return**

true if the table was installed successfully.

pt\_oaddr\_t pt\_item\_oa(const struct pt\_state \*pts)
:   Output Address for this leaf item

**Parameters**

`const struct pt_state *pts`
:   Item to query

**Description**

Return the output address for this item. If the item is part of a contiguous
entry it returns the value of the OA for this individual sub item.

See [`pt_entry_oa()`](#c.pt_entry_oa "pt_entry_oa"). The format should implement one of these two functions
depending on how it stores the OA’s in the table.

enum pt\_entry\_type pt\_load\_entry\_raw(struct pt\_state \*pts)
:   Read from the location pts points at into the pts

**Parameters**

`struct pt_state *pts`
:   Table index to load

**Description**

Return the type of entry that was loaded. pts->entry will be filled in with
the entry’s content. See [`pt_load_entry()`](#c.pt_load_entry "pt_load_entry")

unsigned int pt\_max\_oa\_lg2(const struct [pt\_common](#c.pt_common "pt_common") \*common)
:   Return the maximum OA the table format can hold

**Parameters**

`const struct pt_common *common`
:   Page table to query

**Description**

The value oalog2\_to\_max\_int([`pt_max_oa_lg2()`](#c.pt_max_oa_lg2 "pt_max_oa_lg2")) is the MAX for the
OA. This is the absolute maximum address the table can hold. [`struct pt_common`](#c.pt_common "pt_common")
max\_oasz\_lg2 sets a lower dynamic maximum based on HW capability.

unsigned int pt\_num\_items\_lg2(const struct pt\_state \*pts)
:   Return the number of items in this table level

**Parameters**

`const struct pt_state *pts`
:   The current level

**Description**

The number of items in a table level defines the number of bits this level
decodes from the VA. This function is not called for the top level,
so it does not need to compute a special value for the top case. The
result for the top is based on pt\_common max\_vasz\_lg2.

The value is used as part of determining the table indexes via the
equation:

```
log2_mod(log2_div(VA, pt_table_item_lg2sz()), pt_num_items_lg2())
```

unsigned int pt\_pgsz\_lg2\_to\_level(struct [pt\_common](#c.pt_common "pt_common") \*common, unsigned int pgsize\_lg2)
:   Return the level that maps the page size

**Parameters**

`struct pt_common *common`
:   Page table to query

`unsigned int pgsize_lg2`
:   Log2 page size

**Description**

Returns the table level that will map the given page size. The page
size must be part of the [`pt_possible_sizes()`](#c.pt_possible_sizes "pt_possible_sizes") for some level.

pt\_vaddr\_t pt\_possible\_sizes(const struct pt\_state \*pts)
:   Return a bitmap of possible output sizes at this level

**Parameters**

`const struct pt_state *pts`
:   The current level

**Description**

Each level has a list of possible output sizes that can be installed as
leaf entries. If [`pt_can_have_leaf()`](#c.pt_can_have_leaf "pt_can_have_leaf") is false returns zero.

Otherwise the bit in position [`pt_table_item_lg2sz()`](#c.pt_table_item_lg2sz "pt_table_item_lg2sz") should be set indicating
that a non-contiguous single item leaf entry is supported. The following
[`pt_num_items_lg2()`](#c.pt_num_items_lg2 "pt_num_items_lg2") number of bits can be set indicating contiguous entries
are supported. Bit [`pt_table_item_lg2sz()`](#c.pt_table_item_lg2sz "pt_table_item_lg2sz") + [`pt_num_items_lg2()`](#c.pt_num_items_lg2 "pt_num_items_lg2") must not be
set, contiguous entries cannot span the entire table.

The OR of [`pt_possible_sizes()`](#c.pt_possible_sizes "pt_possible_sizes") of all levels is the typical bitmask of all
supported sizes in the entire table.

unsigned int pt\_table\_item\_lg2sz(const struct pt\_state \*pts)
:   Size of a single item entry in this table level

**Parameters**

`const struct pt_state *pts`
:   The current level

**Description**

The size of the item specifies how much VA and OA a single item occupies.

See [`pt_entry_oa_lg2sz()`](#c.pt_entry_oa_lg2sz "pt_entry_oa_lg2sz") for the same value including the effect of contiguous
entries.

unsigned int pt\_table\_oa\_lg2sz(const struct pt\_state \*pts)
:   Return the VA/OA size of the entire table

**Parameters**

`const struct pt_state *pts`
:   The current level

**Description**

Return the size of VA decoded by the entire table level.

pt\_oaddr\_t pt\_table\_pa(const struct pt\_state \*pts)
:   Return the CPU physical address of the table entry

**Parameters**

`const struct pt_state *pts`
:   Entry to query

**Description**

This is only ever called on PT\_ENTRY\_TABLE entries. Must return the same
value passed to [`pt_install_table()`](#c.pt_install_table "pt_install_table").

struct pt\_table\_p \*pt\_table\_ptr(const struct pt\_state \*pts)
:   Return a CPU pointer for a table item

**Parameters**

`const struct pt_state *pts`
:   Entry to query

**Description**

Same as [`pt_table_pa()`](#c.pt_table_pa "pt_table_pa") but returns a CPU pointer.

unsigned int pt\_max\_sw\_bit(struct [pt\_common](#c.pt_common "pt_common") \*common)
:   Return the maximum software bit usable for any level and entry

**Parameters**

`struct pt_common *common`
:   Page table

**Description**

The swbit can be passed as bitnr to the other sw\_bit functions.

bool pt\_test\_sw\_bit\_acquire(struct pt\_state \*pts, unsigned int bitnr)
:   Read a software bit in an item

**Parameters**

`struct pt_state *pts`
:   Entry to read

`unsigned int bitnr`
:   Bit to read

**Description**

Software bits are ignored by HW and can be used for any purpose by the
software. This does a test bit and acquire operation.

void pt\_set\_sw\_bit\_release(struct pt\_state \*pts, unsigned int bitnr)
:   Set a software bit in an item

**Parameters**

`struct pt_state *pts`
:   Entry to set

`unsigned int bitnr`
:   Bit to set

**Description**

Software bits are ignored by HW and can be used for any purpose by the
software. This does a set bit and release operation.

void pt\_load\_entry(struct pt\_state \*pts)
:   Read from the location pts points at into the pts

**Parameters**

`struct pt_state *pts`
:   Table index to load

**Description**

Set the type of entry that was loaded. pts->entry and pts->table\_lower
will be filled in with the entry’s content.

### Iteration Helpers

int pt\_check\_range(struct pt\_range \*range)
:   Validate the range can be iterated

**Parameters**

`struct pt_range *range`
:   Range to validate

**Description**

Check that VA and last\_va fall within the permitted range of VAs. If the
format is using PT\_FEAT\_SIGN\_EXTEND then this also checks the sign extension
is correct.

void pt\_index\_to\_va(struct pt\_state \*pts)
:   Update range->va to the current pts->index

**Parameters**

`struct pt_state *pts`
:   Iteration State

**Description**

Adjust range->va to match the current index. This is done in a lazy manner
since computing the VA takes several instructions and is rarely required.

bool pt\_entry\_fully\_covered(const struct pt\_state \*pts, unsigned int oasz\_lg2)
:   Check if the item or entry is entirely contained within pts->range

**Parameters**

`const struct pt_state *pts`
:   Iteration State

`unsigned int oasz_lg2`
:   The size of the item to check, [`pt_table_item_lg2sz()`](#c.pt_table_item_lg2sz "pt_table_item_lg2sz") or
    [`pt_entry_oa_lg2sz()`](#c.pt_entry_oa_lg2sz "pt_entry_oa_lg2sz")

**Return**

true if the item is fully enclosed by the pts->range.

unsigned int pt\_range\_to\_index(const struct pt\_state \*pts)
:   Starting index for an iteration

**Parameters**

`const struct pt_state *pts`
:   Iteration State

**Return**

the starting index for the iteration in pts.

unsigned int pt\_range\_to\_end\_index(const struct pt\_state \*pts)
:   Ending index iteration

**Parameters**

`const struct pt_state *pts`
:   Iteration State

**Return**

the last index for the iteration in pts.

void pt\_next\_entry(struct pt\_state \*pts)
:   Advance pts to the next entry

**Parameters**

`struct pt_state *pts`
:   Iteration State

**Description**

Update pts to go to the next index at this level. If pts is pointing at a
contiguous entry then the index may advance my more than one.

for\_each\_pt\_level\_entry

`for_each_pt_level_entry (pts)`

> For loop wrapper over entries in the range

**Parameters**

`pts`
:   Iteration State

**Description**

This is the basic iteration primitive. It iterates over all the entries in
pts->range that fall within the pts’s current table level. Each step does
pt\_load\_entry(pts).

enum pt\_entry\_type pt\_load\_single\_entry(struct pt\_state \*pts)
:   Version of [`pt_load_entry()`](#c.pt_load_entry "pt_load_entry") usable within a walker

**Parameters**

`struct pt_state *pts`
:   Iteration State

**Description**

Alternative to [`for_each_pt_level_entry()`](#c.for_each_pt_level_entry "for_each_pt_level_entry") if the walker function uses only a
single entry.

struct pt\_range pt\_top\_range(struct [pt\_common](#c.pt_common "pt_common") \*common)
:   Return a range that spans part of the top level

**Parameters**

`struct pt_common *common`
:   Table

**Description**

For PT\_FEAT\_SIGN\_EXTEND this will return the lower range, and cover half the
total page table. Otherwise it returns the entire page table.

struct pt\_range pt\_all\_range(struct [pt\_common](#c.pt_common "pt_common") \*common)
:   Return a range that spans the entire page table

**Parameters**

`struct pt_common *common`
:   Table

**Description**

The returned range spans the whole page table. Due to how PT\_FEAT\_SIGN\_EXTEND
is supported range->va and range->last\_va will be incorrect during the
iteration and must not be accessed.

struct pt\_range pt\_upper\_range(struct [pt\_common](#c.pt_common "pt_common") \*common)
:   Return a range that spans part of the top level

**Parameters**

`struct pt_common *common`
:   Table

**Description**

For PT\_FEAT\_SIGN\_EXTEND this will return the upper range, and cover half the
total page table. Otherwise it returns the entire page table.

struct pt\_range pt\_make\_range(struct [pt\_common](#c.pt_common "pt_common") \*common, pt\_vaddr\_t va, pt\_vaddr\_t last\_va)
:   Return a range that spans part of the table

**Parameters**

`struct pt_common *common`
:   Table

`pt_vaddr_t va`
:   Start address

`pt_vaddr_t last_va`
:   Last address

**Description**

The caller must validate the range with [`pt_check_range()`](#c.pt_check_range "pt_check_range") before using it.

struct pt\_state pt\_init(struct pt\_range \*range, unsigned int level, struct pt\_table\_p \*table)
:   Initialize a pt\_state on the stack

**Parameters**

`struct pt_range *range`
:   Range pointer to embed in the state

`unsigned int level`
:   Table level for the state

`struct pt_table_p *table`
:   Pointer to the table memory at level

**Description**

Helper to initialize the on-stack pt\_state from walker arguments.

struct pt\_state pt\_init\_top(struct pt\_range \*range)
:   Initialize a pt\_state on the stack

**Parameters**

`struct pt_range *range`
:   Range pointer to embed in the state

**Description**

The pt\_state points to the top most level.

int pt\_descend(struct pt\_state \*pts, void \*arg, pt\_level\_fn\_t fn)
:   Recursively invoke the walker for the lower level

**Parameters**

`struct pt_state *pts`
:   Iteration State

`void *arg`
:   Value to pass to the function

`pt_level_fn_t fn`
:   Walker function to call

**Description**

pts must point to a table item. Invoke fn as a walker on the table
pts points to.

int pt\_walk\_range(struct pt\_range \*range, pt\_level\_fn\_t fn, void \*arg)
:   Walk over a VA range

**Parameters**

`struct pt_range *range`
:   Range pointer

`pt_level_fn_t fn`
:   Walker function to call

`void *arg`
:   Value to pass to the function

**Description**

Walk over a VA range. The caller should have done a validity check, at
least calling [`pt_check_range()`](#c.pt_check_range "pt_check_range"), when building range. The walk will
start at the top most table.

struct pt\_range pt\_range\_slice(const struct pt\_state \*pts, unsigned int start\_index, unsigned int end\_index)
:   Return a range that spans indexes

**Parameters**

`const struct pt_state *pts`
:   Iteration State

`unsigned int start_index`
:   Starting index within pts

`unsigned int end_index`
:   Ending index within pts

**Description**

Create a range than spans an index range of the current table level
pt\_state points at.

unsigned int pt\_top\_memsize\_lg2(struct [pt\_common](#c.pt_common "pt_common") \*common, uintptr\_t top\_of\_table)

**Parameters**

`struct pt_common *common`
:   Table

`uintptr_t top_of_table`
:   Top of table value from `_pt_top_set()`

**Description**

Compute the allocation size of the top table. For PT\_FEAT\_DYNAMIC\_TOP this
will compute the top size assuming the table will grow.

unsigned int pt\_compute\_best\_pgsize(pt\_vaddr\_t pgsz\_bitmap, pt\_vaddr\_t va, pt\_vaddr\_t last\_va, pt\_oaddr\_t oa)
:   Determine the best page size for leaf entries

**Parameters**

`pt_vaddr_t pgsz_bitmap`
:   Permitted page sizes

`pt_vaddr_t va`
:   Starting virtual address for the leaf entry

`pt_vaddr_t last_va`
:   Last virtual address for the leaf entry, sets the max page size

`pt_oaddr_t oa`
:   Starting output address for the leaf entry

**Description**

Compute the largest page size for va, last\_va, and oa together and return it
in lg2. The largest page size depends on the format’s supported page sizes at
this level, and the relative alignment of the VA and OA addresses. 0 means
the OA cannot be stored with the provided pgsz\_bitmap.

PT\_MAKE\_LEVELS

`PT_MAKE_LEVELS (fn, do_fn)`

> Build an unwound walker

**Parameters**

`fn`
:   Name of the walker function

`do_fn`
:   Function to call at each level

**Description**

This builds a function call tree that can be fully inlined.
The caller must provide a function body in an \_\_always\_inline function:

```
static __always_inline int do_fn(struct pt_range *range, void *arg,
       unsigned int level, struct pt_table_p *table,
       pt_level_fn_t descend_fn)
```

An inline function will be created for each table level that calls do\_fn with
a compile time constant for level and a pointer to the next lower function.
This generates an optimally inlined walk where each of the functions sees a
constant level and can codegen the exact constants/etc for that level.

Note this can produce a lot of code!

### Writing a Format

It is best to start from a simple format that is similar to the target. x86\_64
is usually a good reference for something simple, and AMDv1 is something fairly
complete.

The required inline functions need to be implemented in the format header.
These should all follow the standard pattern of:

```
static inline pt_oaddr_t amdv1pt_entry_oa(const struct pt_state *pts)
{
       [..]
}
#define pt_entry_oa amdv1pt_entry_oa
```

where a uniquely named per-format inline function provides the implementation
and a define maps it to the generic name. This is intended to make debug symbols
work better. inline functions should always be used as the prototypes in
pt\_common.h will cause the compiler to validate the function signature to
prevent errors.

Review pt\_fmt\_defaults.h to understand some of the optional inlines.

Once the format compiles then it should be run through the generic page table
kunit test in kunit\_generic\_pt.h using kunit. For example:

```
$ tools/testing/kunit/kunit.py run --build_dir build_kunit_x86_64 --arch x86_64 --kunitconfig ./drivers/iommu/generic_pt/.kunitconfig amdv1_fmt_test.*
[...]
[11:15:08] Testing complete. Ran 9 tests: passed: 9
[11:15:09] Elapsed time: 3.137s total, 0.001s configuring, 2.368s building, 0.311s running
```

The generic tests are intended to prove out the format functions and give
clearer failures to speed up finding the problems. Once those pass then the
entire kunit suite should be run.

### IOMMU Invalidation Features

Invalidation is how the page table algorithms synchronize with a HW cache of the
page table memory, typically called the TLB (or IOTLB for IOMMU cases).

The TLB can store present PTEs, non-present PTEs and table pointers, depending
on its design. Every HW has its own approach on how to describe what has changed
to have changed items removed from the TLB.

#### PT\_FEAT\_FLUSH\_RANGE

PT\_FEAT\_FLUSH\_RANGE is the easiest scheme to understand. It tries to generate a
single range invalidation for each operation, over-invalidating if there are
gaps of VA that don’t need invalidation. This trades off impacted VA for number
of invalidation operations. It does not keep track of what is being invalidated;
however, if pages have to be freed then page table pointers have to be cleaned
from the walk cache. The range can start/end at any page boundary.

#### PT\_FEAT\_FLUSH\_RANGE\_NO\_GAPS

PT\_FEAT\_FLUSH\_RANGE\_NO\_GAPS is similar to PT\_FEAT\_FLUSH\_RANGE; however, it tries
to minimize the amount of impacted VA by issuing extra flush operations. This is
useful if the cost of processing VA is very high, for instance because a
hypervisor is processing the page table with a shadowing algorithm.
