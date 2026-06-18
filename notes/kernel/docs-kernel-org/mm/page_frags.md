# Page fragments

> 출처(원문): https://docs.kernel.org/mm/page_frags.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Page fragments

A page fragment is an arbitrary-length arbitrary-offset area of memory
which resides within a 0 or higher order compound page. Multiple
fragments within that page are individually refcounted, in the page’s
reference counter.

The page\_frag functions, page\_frag\_alloc and page\_frag\_free, provide a
simple allocation framework for page fragments. This is used by the
network stack and network device drivers to provide a backing region of
memory for use as either an sk\_buff->head, or to be used in the “frags”
portion of skb\_shared\_info.

In order to make use of the page fragment APIs a backing page fragment
cache is needed. This provides a central point for the fragment allocation
and tracks allows multiple calls to make use of a cached page. The
advantage to doing this is that multiple calls to get\_page can be avoided
which can be expensive at allocation time. However due to the nature of
this caching it is required that any calls to the cache be protected by
either a per-cpu limitation, or a per-cpu limitation and forcing interrupts
to be disabled when executing the fragment allocation.

The network stack uses two separate caches per CPU to handle fragment
allocation. The netdev\_alloc\_cache is used by callers making use of the
netdev\_alloc\_frag and \_\_netdev\_alloc\_skb calls. The napi\_alloc\_cache is
used by callers of the \_\_napi\_alloc\_frag and napi\_alloc\_skb calls. The
main difference between these two calls is the context in which they may be
called. The “netdev” prefixed functions are usable in any context as these
functions will disable interrupts, while the “napi” prefixed functions are
only usable within the softirq context.

Many network device drivers use a similar methodology for allocating page
fragments, but the page fragments are cached at the ring or descriptor
level. In order to enable these cases it is necessary to provide a generic
way of tearing down a page cache. For this reason \_\_page\_frag\_cache\_drain
was implemented. It allows for freeing multiple references from a single
page via a single call. The advantage to doing this is that it allows for
cleaning up the multiple references that were added to a page in order to
avoid calling get\_page per allocation.

Alexander Duyck, Nov 29, 2016.
