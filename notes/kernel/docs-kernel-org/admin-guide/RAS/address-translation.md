# Address translation

> 출처(원문): https://docs.kernel.org/admin-guide/RAS/address-translation.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Address translation

## x86 AMD

Zen-based AMD systems include a Data Fabric that manages the layout of
physical memory. Devices attached to the Fabric, like memory controllers,
I/O, etc., may not have a complete view of the system physical memory map.
These devices may provide a “normalized”, i.e. device physical, address
when reporting memory errors. Normalized addresses must be translated to
a system physical address for the kernel to action on the memory.

AMD Address Translation Library (CONFIG\_AMD\_ATL) provides translation for
this case.

Glossary of acronyms used in address translation for Zen-based systems

* CCM = Cache Coherent Moderator
* COD = Cluster-on-Die
* COH\_ST = Coherent Station
* DF = Data Fabric
