# Netmem Support for Network Drivers

> 출처(원문): https://docs.kernel.org/networking/netmem.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Netmem Support for Network Drivers

This document outlines the requirements for network drivers to support netmem,
an abstract memory type that enables features like device memory TCP. By
supporting netmem, drivers can work with various underlying memory types
with little to no modification.

Benefits of Netmem :

* Flexibility: Netmem can be backed by different memory types (e.g., `struct
  page`, DMA-buf), allowing drivers to support various use cases such as device
  memory TCP.
* Future-proof: Drivers with netmem support are ready for upcoming
  features that rely on it.
* Simplified Development: Drivers interact with a consistent API,
  regardless of the underlying memory implementation.

## Driver RX Requirements

1. The driver must support page\_pool.
2. The driver must support the tcp-data-split ethtool option.
3. The driver must use the page\_pool netmem APIs for payload memory. The netmem
   APIs currently 1-to-1 correspond with page APIs. Conversion to netmem should
   be achievable by switching the page APIs to netmem APIs and tracking memory
   via netmem\_refs in the driver rather than `struct page` \* :

   * page\_pool\_alloc -> page\_pool\_alloc\_netmem
   * page\_pool\_get\_dma\_addr -> page\_pool\_get\_dma\_addr\_netmem
   * page\_pool\_put\_page -> page\_pool\_put\_netmem

   Not all page APIs have netmem equivalents at the moment. If your driver
   relies on a missing netmem API, feel free to add and propose to netdev@, or
   reach out to the maintainers and/or [almasrymina@google.com](mailto:almasrymina%40google.com) for help adding
   the netmem API.
4. The driver must use the following PP\_FLAGS:

   * PP\_FLAG\_DMA\_MAP: netmem is not dma-mappable by the driver. The driver
     must delegate the dma mapping to the page\_pool, which knows when
     dma-mapping is (or is not) appropriate.
   * PP\_FLAG\_DMA\_SYNC\_DEV: netmem dma addr is not necessarily dma-syncable
     by the driver. The driver must delegate the dma syncing to the page\_pool,
     which knows when dma-syncing is (or is not) appropriate.
   * PP\_FLAG\_ALLOW\_UNREADABLE\_NETMEM. The driver must specify this flag iff
     tcp-data-split is enabled.
5. The driver must not assume the netmem is readable and/or backed by pages.
   The netmem returned by the page\_pool may be unreadable, in which case
   `netmem_address()` will return NULL. The driver must correctly handle
   unreadable netmem, i.e. don’t attempt to handle its contents when
   `netmem_address()` is NULL.

   Ideally, drivers should not have to check the underlying netmem type via
   helpers like `netmem_is_net_iov()` or convert the netmem to any of its
   underlying types via `netmem_to_page()` or `netmem_to_net_iov()`. In most cases,
   netmem or page\_pool helpers that abstract this complexity are provided
   (and more can be added).
6. The driver must use `page_pool_dma_sync_netmem_for_cpu()` in lieu of
   `dma_sync_single_range_for_cpu()`. For some memory providers, dma\_syncing for
   CPU will be done by the page\_pool, for others (particularly dmabuf memory
   provider), dma syncing for CPU is the responsibility of the userspace using
   dmabuf APIs. The driver must delegate the entire dma-syncing operation to
   the page\_pool which will do it correctly.
7. Avoid implementing driver-specific recycling on top of the page\_pool. Drivers
   cannot hold onto a `struct page` to do their own recycling as the netmem may
   not be backed by a `struct page`. However, you may hold onto a page\_pool
   reference with `page_pool_fragment_netmem()` or `page_pool_ref_netmem()` for
   that purpose, but be mindful that some netmem types might have longer
   circulation times, such as when userspace holds a reference in zerocopy
   scenarios.

## Driver TX Requirements

1. The Driver must not pass the netmem dma\_addr to any of the dma-mapping APIs
   directly. This is because netmem dma\_addrs may come from a source like
   dma-buf that is not compatible with the dma-mapping APIs.

   Helpers like `netmem_dma_unmap_page_attrs()` & `netmem_dma_unmap_addr_set()`
   should be used in lieu of dma\_unmap\_page[\_attrs](), `dma_unmap_addr_set()`.
   The netmem variants will handle netmem dma\_addrs correctly regardless of the
   source, delegating to the dma-mapping APIs when appropriate.

   Not all dma-mapping APIs have netmem equivalents at the moment. If your
   driver relies on a missing netmem API, feel free to add and propose to
   netdev@, or reach out to the maintainers and/or [almasrymina@google.com](mailto:almasrymina%40google.com) for
   help adding the netmem API.
2. Driver should declare support by setting netdev->netmem\_tx = true
