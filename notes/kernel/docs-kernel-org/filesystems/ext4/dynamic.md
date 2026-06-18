# 4.Dynamic Structures

> 출처(원문): https://docs.kernel.org/filesystems/ext4/dynamic.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4. Dynamic Structures

Dynamic metadata are created on the fly when files and blocks are
allocated to files.

* [4.1. Index Nodes](inodes.html)
  + [4.1.1. Inode Size](inodes.html#inode-size)
  + [4.1.2. Finding an Inode](inodes.html#finding-an-inode)
  + [4.1.3. Inode Timestamps](inodes.html#inode-timestamps)
* [4.2. The Contents of inode.i\_block](ifork.html)
  + [4.2.1. Symbolic Links](ifork.html#symbolic-links)
  + [4.2.2. Direct/Indirect Block Addressing](ifork.html#direct-indirect-block-addressing)
  + [4.2.3. Extent Tree](ifork.html#extent-tree)
  + [4.2.4. Inline Data](ifork.html#inline-data)
* [4.3. Directory Entries](directory.html)
  + [4.3.1. Linear (Classic) Directories](directory.html#linear-classic-directories)
  + [4.3.2. Hash Tree Directories](directory.html#hash-tree-directories)
* [4.4. Extended Attributes](attributes.html)
  + [4.4.1. Attribute Name Indices](attributes.html#attribute-name-indices)
  + [4.4.2. POSIX ACLs](attributes.html#posix-acls)
