# 3.Global Structures

> 출처(원문): https://docs.kernel.org/filesystems/ext4/globals.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3. Global Structures

The filesystem is sharded into a number of block groups, each of which
have static metadata at fixed locations.

* [3.1. Super Block](super.html)
* [3.2. Block Group Descriptors](group_descr.html)
* [3.3. Block and inode Bitmaps](bitmaps.html)
* [3.4. Inode Table](inode_table.html)
* [3.5. Multiple Mount Protection](mmp.html)
* [3.6. Journal (jbd2)](journal.html)
  + [3.6.1. Layout](journal.html#layout)
  + [3.6.2. External Journal](journal.html#external-journal)
  + [3.6.3. Block Header](journal.html#block-header)
  + [3.6.4. Super Block](journal.html#super-block)
  + [3.6.5. Descriptor Block](journal.html#descriptor-block)
  + [3.6.6. Data Block](journal.html#data-block)
  + [3.6.7. Revocation Block](journal.html#revocation-block)
  + [3.6.8. Commit Block](journal.html#commit-block)
  + [3.6.9. Fast commits](journal.html#fast-commits)
  + [3.6.10. Fast Commit Replay Idempotence](journal.html#fast-commit-replay-idempotence)
  + [3.6.11. Journal Checkpoint](journal.html#journal-checkpoint)
* [3.7. Orphan file](orphan.html)
