# Slab Allocation

> 출처(원문): https://docs.kernel.org/mm/slab.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Slab Allocation

## Functions and structures

slab\_folio

`slab_folio (s)`

> The folio allocated for a slab

**Parameters**

`s`
:   The slab.

**Description**

Slabs are allocated as folios that contain the individual objects and are
using some fields in the first `struct page` of the folio - those fields are
now accessed by `struct slab`. It is occasionally necessary to convert back to
a folio in order to communicate with the rest of the mm. Please use this
helper function instead of casting yourself, as the implementation may change
in the future.

struct slab \*page\_slab(const struct [page](#c.page_slab "page") \*page)
:   Converts from `struct page` to its slab.

**Parameters**

`const struct page *page`
:   A page which may or may not belong to a slab.

**Return**

The slab which contains this page or NULL if the page does
not belong to a slab. This includes pages returned from large kmalloc.

slab\_page

`slab_page (s)`

> The first `struct page` allocated for a slab

**Parameters**

`s`
:   The slab.

**Description**

A convenience wrapper for converting slab to the first `struct page` of the
underlying folio, to communicate with code not yet converted to folio or
`struct slab`.

enum slab\_flags
:   How the slab flags bits are used.

**Constants**

`SL_locked`
:   Is locked with `slab_lock()`

`SL_partial`
:   On the per-node partial list

`SL_pfmemalloc`
:   Was allocated from PF\_MEMALLOC reserves

**Description**

The slab flags share space with the page flags but some bits have
different interpretations. The high bits are used for information
like zone/node/section.
