# VFS iomap Documentation

> 출처(원문): https://docs.kernel.org/filesystems/iomap/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# VFS iomap Documentation

* [1. Library Design](design.html)
  + [1.1. Introduction](design.html#introduction)
  + [1.2. Who Should Read This?](design.html#who-should-read-this)
  + [1.3. How Is This Better?](design.html#how-is-this-better)
  + [1.4. File Range Iterator](design.html#file-range-iterator)
  + [1.5. Preparing for File Operations](design.html#preparing-for-file-operations)
  + [1.6. Locking Hierarchy](design.html#locking-hierarchy)
  + [1.7. Bugs and Limitations](design.html#bugs-and-limitations)
* [2. Supported File Operations](operations.html)
  + [2.1. Buffered I/O](operations.html#buffered-i-o)
  + [2.2. Direct I/O](operations.html#direct-i-o)
  + [2.3. DAX I/O](operations.html#dax-i-o)
  + [2.4. Seeking Files](operations.html#seeking-files)
  + [2.5. Swap File Activation](operations.html#swap-file-activation)
  + [2.6. File Space Mapping Reporting](operations.html#file-space-mapping-reporting)
* [3. Porting Your Filesystem](porting.html)
  + [3.1. Why Convert?](porting.html#why-convert)
  + [3.2. How Do I Convert a Filesystem?](porting.html#how-do-i-convert-a-filesystem)
