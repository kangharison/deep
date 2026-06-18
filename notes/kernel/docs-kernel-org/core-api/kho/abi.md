# Kexec Handover ABI

> 출처(원문): https://docs.kernel.org/core-api/kho/abi.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Kexec Handover ABI

## Core Kexec Handover ABI

Kexec Handover uses the ABI defined below for passing preserved data from
one kernel to the next.
The ABI uses Flattened Device Tree (FDT) format. The first kernel creates an
FDT which is then passed to the next kernel during a kexec handover.

This interface is a contract. Any modification to the FDT structure, node
properties, compatible string, or the layout of the data structures
referenced here constitutes a breaking change. Such changes require
incrementing the version number in KHO\_FDT\_COMPATIBLE to prevent a new kernel
from misinterpreting data from an older kernel. Changes are allowed provided
the compatibility version is incremented. However, backward/forward
compatibility is only guaranteed for kernels supporting the same ABI version.

FDT Structure Overview:
:   The FDT serves as a central registry for physical addresses of preserved
    data structures. The first kernel populates this FDT with references to
    memory regions and other metadata that need to persist across the kexec
    transition. The subsequent kernel then parses this FDT to locate and
    restore the preserved data.:

    ```
    / {
        compatible = "kho-v3";

        preserved-memory-map = <0x...>;

        <subnode-name-1> {
            preserved-data = <0x...>;
            blob-size = <0x...>;
        };

        <subnode-name-2> {
            preserved-data = <0x...>;
            blob-size = <0x...>;
        };
              ... ...
        <subnode-name-N> {
            preserved-data = <0x...>;
            blob-size = <0x...>;
        };
    };
    ```

    Root KHO Node (/):
    :   * compatible: “kho-v3”

          Indentifies the overall KHO ABI version.
        * preserved-memory-map: u64

          Physical memory address pointing to the root of the
          preserved memory map data structure.

    Subnodes (<subnode-name-N>):
    :   Subnodes can also be added to the root node to
        describe other preserved data blobs. The <subnode-name-N>
        is provided by the subsystem that uses KHO for preserving its
        data.

        * preserved-data: u64

          Physical address pointing to a subnode data blob that is also
          being preserved.
        * blob-size: u64

          Size in bytes of the preserved data blob. This is needed because
          blobs may use arbitrary formats (not just FDT), so the size
          cannot be determined from the blob content alone.

## vmalloc preservation ABI

The Kexec Handover ABI for preserving vmalloc’ed memory is defined by
a set of structures and helper macros. The layout of these structures is a
stable contract between kernels and is versioned by the KHO\_FDT\_COMPATIBLE
string.

The preservation is managed through a main descriptor `struct kho_vmalloc`,
which points to a linked list of `struct kho_vmalloc_chunk` structures. These
chunks contain the physical addresses of the preserved pages, allowing the
next kernel to reconstruct the vmalloc area with the same content and layout.
Helper macros are also defined for storing and loading pointers within
these structures.

## memblock preservation ABI

Memblock can serialize its current memory reservations created with
reserve\_mem command line option across kexec through KHO.
The post-KHO kernel can then consume these reservations and they are
guaranteed to have the same physical address.

The state is serialized using Flattened Device Tree (FDT) format. Any
modification to the FDT structure, node properties, or the compatible
strings constitutes a breaking change. Such changes require incrementing the
version number in the relevant \_COMPATIBLE string to prevent a new kernel
from misinterpreting data from an old kernel.

Changes are allowed provided the compatibility version is incremented.
However, backward/forward compatibility is only guaranteed for kernels
supporting the same ABI version.

FDT Structure Overview:
:   The entire memblock state is encapsulated within a single KHO entry named
    “memblock”.
    This entry contains an FDT with the following layout:

    ```
    / {
            compatible = "memblock-v1";

            n1 {
                    compatible = "reserve-mem-v1";
                    start = <0xc06b 0x4000000>;
                    size = <0x04 0x00>;
            };
    };
    ```

Main memblock node (/):

> * compatible: “memblock-v1”
>   Identifies the overall memblock ABI version.

reserved\_mem node:
:   These nodes describe all reserve\_mem regions. The node name is the name
    defined by the user for a reserve\_mem region.

    * compatible: “reserve-mem-v1”

      Identifies the ABI version of reserve\_mem descriptions
    * start: u64

      Physical address of the reserved memory region.
    * size: u64

      size in bytes of the reserved memory region.

## KHO persistent memory tracker ABI

KHO tracks preserved memory using a radix tree data structure. Each node of
the tree is exactly a single page. The leaf nodes are bitmaps where each set
bit is a preserved page of any order. The intermediate nodes are tables of
physical addresses that point to a lower level node.

The tree hierarchy is shown below:

```
root
+-------------------+
|     Level 5       | (struct kho_radix_node)
+-------------------+
  |
  v
+-------------------+
|     Level 4       | (struct kho_radix_node)
+-------------------+
  |
  | ... (intermediate levels)
  |
  v
+-------------------+
|      Level 0      | (struct kho_radix_leaf)
+-------------------+
```

The tree is traversed using a key that encodes the page’s physical address
(pa) and its order into a single unsigned long value. The encoded key value
is composed of two parts: the ‘order bit’ in the upper part and the
‘shifted physical address’ in the lower part.:

```
+------------+-----------------------------+--------------------------+
| Page Order | Order Bit                   | Shifted Physical Address |
+------------+-----------------------------+--------------------------+
| 0          | ...000100 ... (at bit 52)   | pa >> (PAGE_SHIFT + 0)   |
| 1          | ...000010 ... (at bit 51)   | pa >> (PAGE_SHIFT + 1)   |
| 2          | ...000001 ... (at bit 50)   | pa >> (PAGE_SHIFT + 2)   |
| ...        | ...                         | ...                      |
+------------+-----------------------------+--------------------------+
```

Shifted Physical Address:
The ‘shifted physical address’ is the physical address normalized for its
order. It effectively represents the PFN shifted right by the order.

Order Bit:
The ‘order bit’ encodes the page order by setting a single bit at a
specific position. The position of this bit itself represents the order.

For instance, on a 64-bit system with 4KB pages (PAGE\_SHIFT = 12), the
maximum range for the shifted physical address (for order 0) is 52 bits
(64 - 12). This address occupies bits [0-51]. For order 0, the order bit is
set at position 52.

The following diagram illustrates how the encoded key value is split into
indices for the tree levels, with PAGE\_SIZE of 4KB:

```
     63:60   59:51    50:42    41:33    32:24    23:15         14:0
+---------+--------+--------+--------+--------+--------+-----------------+
|    0    |  Lv 5  |  Lv 4  |  Lv 3  |  Lv 2  |  Lv 1  |  Lv 0 (bitmap)  |
+---------+--------+--------+--------+--------+--------+-----------------+
```

The radix tree stores pages of all orders in a single 6-level hierarchy. It
efficiently shares higher tree levels, especially due to common zero top
address bits, allowing a single, efficient algorithm to manage all
pages. This bitmap approach also offers memory efficiency; for example, a
512KB bitmap can cover a 16GB memory range for 0-order pages with PAGE\_SIZE =
4KB.

The data structures defined here are part of the KHO ABI. Any modification
to these structures that breaks backward compatibility must be accompanied by
an update to the “compatible” string. This ensures that a newer kernel can
correctly interpret the data passed by an older kernel.

## See Also

* [Kexec Handover Usage](../../admin-guide/mm/kho.html)
