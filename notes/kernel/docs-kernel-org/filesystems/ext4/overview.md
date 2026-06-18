# 2.High Level Design

> 출처(원문): https://docs.kernel.org/filesystems/ext4/overview.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2. High Level Design

An ext4 file system is split into a series of block groups. To reduce
performance difficulties due to fragmentation, the block allocator tries
very hard to keep each file’s blocks within the same group, thereby
reducing seek times. The size of a block group is specified in
`sb.s_blocks_per_group` blocks, though it can also calculated as 8 \*
`block_size_in_bytes`. With the default block size of 4KiB, each group
will contain 32,768 blocks, for a length of 128MiB. The number of block
groups is the size of the device divided by the size of a block group.

All fields in ext4 are written to disk in little-endian order. HOWEVER,
all fields in jbd2 (the journal) are written to disk in big-endian
order.

* [2.1. Blocks](blocks.html)
* [2.2. Block Groups](blockgroup.html)
  + [2.2.1. Layout](blockgroup.html#layout)
  + [2.2.2. Flexible Block Groups](blockgroup.html#flexible-block-groups)
  + [2.2.3. Meta Block Groups](blockgroup.html#meta-block-groups)
  + [2.2.4. Lazy Block Group Initialization](blockgroup.html#lazy-block-group-initialization)
* [2.3. Special inodes](special_inodes.html)
* [2.4. Block and Inode Allocation Policy](allocators.html)
* [2.5. Checksums](checksums.html)
* [2.6. Bigalloc](bigalloc.html)
* [2.7. Inline Data](inlinedata.html)
  + [2.7.1. Inline Directories](inlinedata.html#inline-directories)
* [2.8. Large Extended Attribute Values](eainode.html)
* [2.9. Verity files](verity.html)
* [2.10. Atomic Block Writes](atomic_writes.html)
  + [2.10.1. Introduction](atomic_writes.html#introduction)
  + [2.10.2. Requirements](atomic_writes.html#requirements)
  + [2.10.3. Multi-fsblock Implementation Details](atomic_writes.html#multi-fsblock-implementation-details)
  + [2.10.4. Handling Split Extents Across Leaf Blocks](atomic_writes.html#handling-split-extents-across-leaf-blocks)
  + [2.10.5. Handling Journal transactions](atomic_writes.html#handling-journal-transactions)
  + [2.10.6. How to](atomic_writes.html#how-to)
    - [2.10.6.1. Creating Filesystems with Atomic Write Support](atomic_writes.html#creating-filesystems-with-atomic-write-support)
    - [2.10.6.2. Application Interface](atomic_writes.html#application-interface)
  + [2.10.7. Hardware Support](atomic_writes.html#hardware-support)
  + [2.10.8. See Also](atomic_writes.html#see-also)
