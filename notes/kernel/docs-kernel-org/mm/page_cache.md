# Page Cache

> 출처(원문): https://docs.kernel.org/mm/page_cache.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Page Cache

The page cache is the primary way that the user and the rest of the kernel
interact with filesystems. It can be bypassed (e.g. with O\_DIRECT),
but normal reads, writes and mmaps go through the page cache.

## Folios

The folio is the unit of memory management within the page cache.
Operations
