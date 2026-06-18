# ext4 Data Structures and Algorithms

> 출처(원문): https://docs.kernel.org/filesystems/ext4/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# ext4 Data Structures and Algorithms

* [1. About this Book](about.html)
  + [1.1. License](about.html#license)
  + [1.2. Terminology](about.html#terminology)
  + [1.3. Other References](about.html#other-references)
* [2. High Level Design](overview.html)
  + [2.1. Blocks](blocks.html)
  + [2.2. Block Groups](blockgroup.html)
  + [2.3. Special inodes](special_inodes.html)
  + [2.4. Block and Inode Allocation Policy](allocators.html)
  + [2.5. Checksums](checksums.html)
  + [2.6. Bigalloc](bigalloc.html)
  + [2.7. Inline Data](inlinedata.html)
  + [2.8. Large Extended Attribute Values](eainode.html)
  + [2.9. Verity files](verity.html)
  + [2.10. Atomic Block Writes](atomic_writes.html)
* [3. Global Structures](globals.html)
  + [3.1. Super Block](super.html)
  + [3.2. Block Group Descriptors](group_descr.html)
  + [3.3. Block and inode Bitmaps](bitmaps.html)
  + [3.4. Inode Table](inode_table.html)
  + [3.5. Multiple Mount Protection](mmp.html)
  + [3.6. Journal (jbd2)](journal.html)
  + [3.7. Orphan file](orphan.html)
* [4. Dynamic Structures](dynamic.html)
  + [4.1. Index Nodes](inodes.html)
  + [4.2. The Contents of inode.i\_block](ifork.html)
  + [4.3. Directory Entries](directory.html)
  + [4.4. Extended Attributes](attributes.html)
