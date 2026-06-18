# CMA Debugfs Interface

> 출처(원문): https://docs.kernel.org/admin-guide/mm/cma_debugfs.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# CMA Debugfs Interface

The CMA debugfs interface is useful to retrieve basic information out of the
different CMA areas and to test allocation/release in each of the areas.

Each CMA area represents a directory under <debugfs>/cma/, represented by
its CMA name like below:

> <debugfs>/cma/<cma\_name>

The structure of the files created under that directory is as follows:

> * [RO] base\_pfn: The base PFN (Page Frame Number) of the CMA area.
>   :   This is the same as ranges/0/base\_pfn.
> * [RO] count: Amount of memory in the CMA area.
> * [RO] order\_per\_bit: Order of pages represented by one bit.
> * [RO] bitmap: The bitmap of allocated pages in the area.
>   :   This is the same as ranges/0/base\_pfn.
> * [RO] ranges/N/base\_pfn: The base PFN of contiguous range N
>   :   in the CMA area.
> * [RO] ranges/N/bitmap: The bit map of allocated pages in
>   :   range N in the CMA area.
> * [WO] alloc: Allocate N pages from that CMA area. For example:
>
>   ```
>   echo 5 > <debugfs>/cma/<cma_name>/alloc
>   ```

would try to allocate 5 pages from the ‘cma\_name’ area.

> * [WO] free: Free N pages from that CMA area, similar to the above.
