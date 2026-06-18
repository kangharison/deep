# 3.4.Inode Table

> 출처(원문): https://docs.kernel.org/filesystems/ext4/inode_table.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.4. Inode Table

Inode tables are statically allocated at mkfs time. Each block group
descriptor points to the start of the table, and the superblock records
the number of inodes per group. See [inode documentation](inodes.html)
for more information on inode table layout.
