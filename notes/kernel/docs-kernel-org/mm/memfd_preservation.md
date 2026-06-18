# Memfd Preservation via LUO

> 출처(원문): https://docs.kernel.org/mm/memfd_preservation.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Memfd Preservation via LUO

## Overview

Memory file descriptors (memfd) can be preserved over a kexec using the Live
Update Orchestrator (LUO) file preservation. This allows userspace to
transfer its memory contents to the next kernel after a kexec.

The preservation is not intended to be transparent. Only select properties of
the file are preserved. All others are reset to default. The preserved
properties are described below.

Note

The LUO API is not stabilized yet, so the preserved properties of a memfd
are also not stable and are subject to backwards incompatible changes.

Note

Currently a memfd backed by Hugetlb is not supported. Memfds created
with `MFD_HUGETLB` will be rejected.

## Preserved Properties

The following properties of the memfd are preserved across kexec:

File Contents
:   All data stored in the file is preserved.

File Size
:   The size of the file is preserved. Holes in the file are filled by
    allocating pages for them during preservation.

File Position
:   The current file position is preserved, allowing applications to continue
    reading/writing from their last position.

File Status Flags
:   memfds are always opened with `O_RDWR` and `O_LARGEFILE`. This property
    is maintained.

Seals
:   File seals set on the memfd are preserved and re-applied on restore.
    Only seals known to this LUO version (see `MEMFD_LUO_ALL_SEALS`) may
    be present; preservation fails with `-EOPNOTSUPP` otherwise.

## Non-Preserved Properties

All properties which are not preserved must be assumed to be reset to
default. This section describes some of those properties which may be more of
note.

`FD_CLOEXEC` flag
:   A memfd can be created with the `MFD_CLOEXEC` flag that sets the
    `FD_CLOEXEC` on the file. This flag is not preserved and must be set
    again after restore via `fcntl()`.

## Memfd Preservation ABI

MEMFD\_LUO\_FOLIO\_DIRTY

`MEMFD_LUO_FOLIO_DIRTY`

> > The folio is dirty.
>
> **Description**
>
> This flag indicates the folio contains data from user. A non-dirty folio is
> one that was allocated (say using fallocate(2)) but not written to.

MEMFD\_LUO\_FOLIO\_UPTODATE

`MEMFD_LUO_FOLIO_UPTODATE`

> > The folio is up-to-date.
>
> **Description**
>
> An up-to-date folio has been zeroed out. shmem zeroes out folios on first
> use. This flag tracks which folios need zeroing.

struct memfd\_luo\_folio\_ser
:   Serialized state of a single folio.

**Definition**:

```
struct memfd_luo_folio_ser {
    u64 pfn:52;
    u64 flags:12;
    u64 index;
};
```

**Members**

`pfn`
:   The page frame number of the folio.

`flags`
:   Flags to describe the state of the folio.

`index`
:   The page offset (pgoff\_t) of the folio within the original file.

struct memfd\_luo\_ser
:   Main serialization structure for a memfd.

**Definition**:

```
struct memfd_luo_ser {
    u64 pos;
    u64 size;
    u32 seals;
    u32 flags;
    u64 nr_folios;
    struct kho_vmalloc folios;
};
```

**Members**

`pos`
:   The file’s current position (f\_pos).

`size`
:   The total size of the file in bytes (i\_size).

`seals`
:   The seals present on the memfd. The seals are uABI so it is safe
    to directly use them in the ABI.

`flags`
:   Flags for the file. Unused flag bits must be set to 0.

`nr_folios`
:   Number of folios in the folios array.

`folios`
:   KHO vmalloc descriptor pointing to the array of
    [`struct memfd_luo_folio_ser`](#c.memfd_luo_folio_ser "memfd_luo_folio_ser").

## See Also

* [Live Update Orchestrator](../core-api/liveupdate.html)
* [Kexec Handover Subsystem](../core-api/kho/index.html)
