# Page Pool API

> 출처(원문): https://docs.kernel.org/networking/page_pool.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Page Pool API

The page\_pool allocator is optimized for recycling page or page fragment used
by skb packet and xdp frame.

Basic use involves replacing any [`alloc_pages()`](../core-api/mm-api.html#c.alloc_pages "alloc_pages") calls with `page_pool_alloc()`,
which allocate memory with or without page splitting depending on the
requested memory size.

If the driver knows that it always requires full pages or its allocations are
always smaller than half a page, it can use one of the more specific API
calls:

1. `page_pool_alloc_pages()`: allocate memory without page splitting when
driver knows that the memory it need is always bigger than half of the page
allocated from page pool. There is no cache line dirtying for ‘`struct page`’
when a page is recycled back to the page pool.

2. `page_pool_alloc_frag()`: allocate memory with page splitting when driver
knows that the memory it need is always smaller than or equal to half of the
page allocated from page pool. Page splitting enables memory saving and thus
avoids TLB/cache miss for data access, but there also is some cost to
implement page splitting, mainly some cache line dirtying/bouncing for
‘`struct page`’ and atomic operation for page->pp\_ref\_count.

The API keeps track of in-flight pages, in order to let API users know when
it is safe to free a page\_pool object, the API users must call
[`page_pool_put_page()`](#c.page_pool_put_page "page_pool_put_page") or [`page_pool_free_va()`](#c.page_pool_free_va "page_pool_free_va") to free the page\_pool object, or
attach the page\_pool object to a page\_pool-aware object like skbs marked with
`skb_mark_for_recycle()`.

[`page_pool_put_page()`](#c.page_pool_put_page "page_pool_put_page") may be called multiple times on the same page if a page
is split into multiple fragments. For the last fragment, it will either
recycle the page, or in case of page->\_refcount > 1, it will release the DMA
mapping and in-flight state accounting.

`dma_sync_single_range_for_device()` is only called for the last fragment when
page\_pool is created with PP\_FLAG\_DMA\_SYNC\_DEV flag, so it depends on the
last freed fragment to do the sync\_for\_device operation for all fragments in
the same page when a page is split. The API user must setup pool->p.max\_len
and pool->p.offset correctly and ensure that [`page_pool_put_page()`](#c.page_pool_put_page "page_pool_put_page") is called
with dma\_sync\_size being -1 for fragment API.

## Architecture overview

```
+------------------+
|       Driver     |
+------------------+
        ^
        |
        |
        |
        v
+--------------------------------------------+
|                request memory              |
+--------------------------------------------+
    ^                                  ^
    |                                  |
    | Pool empty                       | Pool has entries
    |                                  |
    v                                  v
+-----------------------+     +------------------------+
| alloc (and map) pages |     |  get page from cache   |
+-----------------------+     +------------------------+
                                ^                    ^
                                |                    |
                                | cache available    | No entries, refill
                                |                    | from ptr-ring
                                |                    |
                                v                    v
                      +-----------------+     +------------------+
                      |   Fast cache    |     |  ptr-ring cache  |
                      +-----------------+     +------------------+
```

## Monitoring

Information about page pools on the system can be accessed via the netdev
genetlink family (see Documentation/netlink/specs/netdev.yaml).

## API interface

The number of pools created **must** match the number of hardware queues
unless hardware restrictions make that impossible. This would otherwise beat the
purpose of page pool, which is allocate pages fast from cache without locking.
This lockless guarantee naturally comes from running under a NAPI softirq.
The protection doesn’t strictly have to be NAPI, any guarantee that allocating
a page will cause no race conditions is enough.

struct page\_pool \*page\_pool\_create(const struct [page\_pool\_params](#c.page_pool_params "page_pool_params") \*params)
:   create a page pool

**Parameters**

`const struct page_pool_params *params`
:   parameters, see [`struct page_pool_params`](#c.page_pool_params "page_pool_params")

struct page\_pool\_params
:   page pool parameters

**Definition**:

```
struct page_pool_params {
    struct {
        unsigned int    order;
        unsigned int    pool_size;
        int nid;
        struct device   *dev;
        struct napi_struct *napi;
        enum dma_data_direction dma_dir;
        unsigned int    max_len;
        unsigned int    offset;
    };
    struct {
        struct net_device *netdev;
        unsigned int queue_idx;
        unsigned int    flags;
    };
};
```

**Members**

`{unnamed_struct}`
:   anonymous

`order`
:   2^order pages on allocation

`pool_size`
:   size of the ptr\_ring

`nid`
:   NUMA node id to allocate from pages from

`dev`
:   device, for DMA pre-mapping purposes

`napi`
:   NAPI which is the sole consumer of pages, otherwise NULL

`dma_dir`
:   DMA mapping direction

`max_len`
:   max DMA sync memory size for PP\_FLAG\_DMA\_SYNC\_DEV

`offset`
:   DMA sync address offset for PP\_FLAG\_DMA\_SYNC\_DEV

`{unnamed_struct}`
:   anonymous

`netdev`
:   netdev this pool will serve (leave as NULL if none or multiple)

`queue_idx`
:   queue idx this page\_pool is being created for.

`flags`
:   PP\_FLAG\_DMA\_MAP, PP\_FLAG\_DMA\_SYNC\_DEV, PP\_FLAG\_SYSTEM\_POOL,
    PP\_FLAG\_ALLOW\_UNREADABLE\_NETMEM.

struct page \*page\_pool\_dev\_alloc\_pages(struct page\_pool \*pool)
:   allocate a page.

**Parameters**

`struct page_pool *pool`
:   pool from which to allocate

**Description**

Get a page from the page allocator or page\_pool caches.

struct page \*page\_pool\_dev\_alloc\_frag(struct page\_pool \*pool, unsigned int \*offset, unsigned int size)
:   allocate a page fragment.

**Parameters**

`struct page_pool *pool`
:   pool from which to allocate

`unsigned int *offset`
:   offset to the allocated page

`unsigned int size`
:   requested size

**Description**

Get a page fragment from the page allocator or page\_pool caches.

**Return**

allocated page fragment, otherwise return NULL.

struct page \*page\_pool\_dev\_alloc(struct page\_pool \*pool, unsigned int \*offset, unsigned int \*size)
:   allocate a page or a page fragment.

**Parameters**

`struct page_pool *pool`
:   pool from which to allocate

`unsigned int *offset`
:   offset to the allocated page

`unsigned int *size`
:   in as the requested size, out as the allocated size

**Description**

Get a page or a page fragment from the page allocator or page\_pool caches
depending on the requested size in order to allocate memory with least memory
utilization and performance penalty.

**Return**

allocated page or page fragment, otherwise return NULL.

void \*page\_pool\_dev\_alloc\_va(struct page\_pool \*pool, unsigned int \*size)
:   allocate a page or a page fragment and return its va.

**Parameters**

`struct page_pool *pool`
:   pool from which to allocate

`unsigned int *size`
:   in as the requested size, out as the allocated size

**Description**

This is just a thin wrapper around the `page_pool_alloc()` API, and
it returns va of the allocated page or page fragment.

**Return**

the va for the allocated page or page fragment, otherwise return NULL.

enum dma\_data\_direction page\_pool\_get\_dma\_dir(const struct page\_pool \*pool)
:   Retrieve the stored DMA direction.

**Parameters**

`const struct page_pool *pool`
:   pool from which page was allocated

**Description**

Get the stored dma direction. A driver might decide to store this locally
and avoid the extra cache line from page\_pool to determine the direction.

void page\_pool\_put\_page(struct page\_pool \*pool, struct [page](#c.page_pool_put_page "page") \*page, unsigned int dma\_sync\_size, bool allow\_direct)
:   release a reference to a page pool page

**Parameters**

`struct page_pool *pool`
:   pool from which page was allocated

`struct page *page`
:   page to release a reference on

`unsigned int dma_sync_size`
:   how much of the page may have been touched by the device

`bool allow_direct`
:   released by the consumer, allow lockless caching

**Description**

The outcome of this depends on the page refcnt. If the driver bumps
the refcnt > 1 this will unmap the page. If the page refcnt is 1
the allocator owns the page and will try to recycle it in one of the pool
caches. If PP\_FLAG\_DMA\_SYNC\_DEV is set, the page will be synced for\_device
using `dma_sync_single_range_for_device()`.

void page\_pool\_put\_full\_page(struct page\_pool \*pool, struct [page](#c.page_pool_put_full_page "page") \*page, bool allow\_direct)
:   release a reference on a page pool page

**Parameters**

`struct page_pool *pool`
:   pool from which page was allocated

`struct page *page`
:   page to release a reference on

`bool allow_direct`
:   released by the consumer, allow lockless caching

**Description**

Similar to [`page_pool_put_page()`](#c.page_pool_put_page "page_pool_put_page"), but will DMA sync the entire memory area
as configured in [`page_pool_params.max_len`](#c.page_pool_params "page_pool_params").

void page\_pool\_recycle\_direct(struct page\_pool \*pool, struct [page](#c.page_pool_recycle_direct "page") \*page)
:   release a reference on a page pool page

**Parameters**

`struct page_pool *pool`
:   pool from which page was allocated

`struct page *page`
:   page to release a reference on

**Description**

Similar to [`page_pool_put_full_page()`](#c.page_pool_put_full_page "page_pool_put_full_page") but caller must guarantee safe context
(e.g NAPI), since it will recycle the page directly into the pool fast cache.

void page\_pool\_free\_va(struct page\_pool \*pool, void \*va, bool allow\_direct)
:   free a va into the page\_pool

**Parameters**

`struct page_pool *pool`
:   pool from which va was allocated

`void *va`
:   va to be freed

`bool allow_direct`
:   freed by the consumer, allow lockless caching

**Description**

Free a va allocated from `page_pool_allo_va()`.

dma\_addr\_t page\_pool\_get\_dma\_addr(const struct [page](#c.page_pool_get_dma_addr "page") \*page)
:   Retrieve the stored DMA address.

**Parameters**

`const struct page *page`
:   page allocated from a page pool

**Description**

Fetch the DMA address of the page. The page pool to which the page belongs
must had been created with PP\_FLAG\_DMA\_MAP.

bool page\_pool\_get\_stats(const struct page\_pool \*pool, struct [page\_pool\_stats](#c.page_pool_stats "page_pool_stats") \*stats)
:   fetch page pool stats

**Parameters**

`const struct page_pool *pool`
:   pool from which page was allocated

`struct page_pool_stats *stats`
:   [`struct page_pool_stats`](#c.page_pool_stats "page_pool_stats") to fill in

**Description**

Retrieve statistics about the page\_pool. This API is only available
if the kernel has been configured with `CONFIG_PAGE_POOL_STATS=y`.
A pointer to a caller allocated [`struct page_pool_stats`](#c.page_pool_stats "page_pool_stats") structure
is passed to this API which is filled in. The caller can then report
those stats to the user (perhaps via ethtool, debugfs, etc.).

### DMA sync

Driver is always responsible for syncing the pages for the CPU.
Drivers may choose to take care of syncing for the device as well
or set the `PP_FLAG_DMA_SYNC_DEV` flag to request that pages
allocated from the page pool are already synced for the device.

If `PP_FLAG_DMA_SYNC_DEV` is set, the driver must inform the core what portion
of the buffer has to be synced. This allows the core to avoid syncing the entire
page when the drivers knows that the device only accessed a portion of the page.

Most drivers will reserve headroom in front of the frame. This part
of the buffer is not touched by the device, so to avoid syncing
it drivers can set the `offset` field in [`struct page_pool_params`](#c.page_pool_params "page_pool_params")
appropriately.

For pages recycled on the XDP xmit and skb paths the page pool will
use the `max_len` member of [`struct page_pool_params`](#c.page_pool_params "page_pool_params") to decide how
much of the page needs to be synced (starting at `offset`).
When directly freeing pages in the driver ([`page_pool_put_page()`](#c.page_pool_put_page "page_pool_put_page"))
the `dma_sync_size` argument specifies how much of the buffer needs
to be synced.

If in doubt set `offset` to 0, `max_len` to `PAGE_SIZE` and
pass -1 as `dma_sync_size`. That combination of arguments is always
correct.

Note that the syncing parameters are for the entire page.
This is important to remember when using fragments (`PP_FLAG_PAGE_FRAG`),
where allocated buffers may be smaller than a full page.
Unless the driver author really understands page pool internals
it’s recommended to always use `offset = 0`, `max_len = PAGE_SIZE`
with fragmented page pools.

### Stats API and structures

If the kernel is configured with `CONFIG_PAGE_POOL_STATS=y`, the API
[`page_pool_get_stats()`](#c.page_pool_get_stats "page_pool_get_stats") and structures described below are available.
It takes a pointer to a `struct page_pool` and a pointer to a [`struct
page_pool_stats`](#c.page_pool_stats "page_pool_stats") allocated by the caller.

Older drivers expose page pool statistics via ethtool or debugfs.
The same statistics are accessible via the netlink netdev family
in a driver-independent fashion.

struct page\_pool\_alloc\_stats
:   allocation statistics

**Definition**:

```
struct page_pool_alloc_stats {
    u64 fast;
    u64 slow;
    u64 slow_high_order;
    u64 empty;
    u64 refill;
    u64 waive;
};
```

**Members**

`fast`
:   successful fast path allocations

`slow`
:   slow path order-0 allocations

`slow_high_order`
:   slow path high order allocations

`empty`
:   ptr ring is empty, so a slow path allocation was forced

`refill`
:   an allocation which triggered a refill of the cache

`waive`
:   pages obtained from the ptr ring that cannot be added to
    the cache due to a NUMA mismatch

struct page\_pool\_recycle\_stats
:   recycling (freeing) statistics

**Definition**:

```
struct page_pool_recycle_stats {
    u64 cached;
    u64 cache_full;
    u64 ring;
    u64 ring_full;
    u64 released_refcnt;
};
```

**Members**

`cached`
:   recycling placed page in the page pool cache

`cache_full`
:   page pool cache was full

`ring`
:   page placed into the ptr ring

`ring_full`
:   page released from page pool because the ptr ring was full

`released_refcnt`
:   page released (and not recycled) because refcnt > 1

struct page\_pool\_stats
:   combined page pool use statistics

**Definition**:

```
struct page_pool_stats {
    struct page_pool_alloc_stats alloc_stats;
    struct page_pool_recycle_stats recycle_stats;
};
```

**Members**

`alloc_stats`
:   see [`struct page_pool_alloc_stats`](#c.page_pool_alloc_stats "page_pool_alloc_stats")

`recycle_stats`
:   see [`struct page_pool_recycle_stats`](#c.page_pool_recycle_stats "page_pool_recycle_stats")

**Description**

Wrapper struct for combining page pool stats with different storage
requirements.

## Coding examples

### Registration

```
/* Page pool registration */
struct page_pool_params pp_params = { 0 };
struct xdp_rxq_info xdp_rxq;
int err;

pp_params.order = 0;
/* internal DMA mapping in page_pool */
pp_params.flags = PP_FLAG_DMA_MAP;
pp_params.pool_size = DESC_NUM;
pp_params.nid = NUMA_NO_NODE;
pp_params.dev = priv->dev;
pp_params.napi = napi; /* only if locking is tied to NAPI */
pp_params.dma_dir = xdp_prog ? DMA_BIDIRECTIONAL : DMA_FROM_DEVICE;
page_pool = page_pool_create(&pp_params);

err = xdp_rxq_info_reg(&xdp_rxq, ndev, 0);
if (err)
    goto err_out;

err = xdp_rxq_info_reg_mem_model(&xdp_rxq, MEM_TYPE_PAGE_POOL, page_pool);
if (err)
    goto err_out;
```

### NAPI poller

```
/* NAPI Rx poller */
enum dma_data_direction dma_dir;

dma_dir = page_pool_get_dma_dir(dring->page_pool);
while (done < budget) {
    if (some error)
        page_pool_recycle_direct(page_pool, page);
    if (packet_is_xdp) {
        if XDP_DROP:
            page_pool_recycle_direct(page_pool, page);
    } else (packet_is_skb) {
        skb_mark_for_recycle(skb);
        new_page = page_pool_dev_alloc_pages(page_pool);
    }
}
```

### Stats

```
#ifdef CONFIG_PAGE_POOL_STATS
/* retrieve stats */
struct page_pool_stats stats = { 0 };
if (page_pool_get_stats(page_pool, &stats)) {
        /* perhaps the driver reports statistics with ethool */
        ethtool_print_allocation_stats(&stats.alloc_stats);
        ethtool_print_recycle_stats(&stats.recycle_stats);
}
#endif
```

### Driver unload

```
/* Driver unload */
page_pool_put_full_page(page_pool, page, false);
xdp_rxq_info_unreg(&xdp_rxq);
```
