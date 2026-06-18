# dm-queue-length

> 출처(원문): https://docs.kernel.org/admin-guide/device-mapper/dm-queue-length.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# dm-queue-length

dm-queue-length is a path selector module for device-mapper targets,
which selects a path with the least number of in-flight I/Os.
The path selector name is ‘queue-length’.

Table parameters for each path: [<repeat\_count>]

```
<repeat_count>: The number of I/Os to dispatch using the selected
                path before switching to the next path.
                If not given, internal default is used. To check
                the default value, see the activated table.
```

Status for each path: <status> <fail-count> <in-flight>

```
<status>: 'A' if the path is active, 'F' if the path is failed.
<fail-count>: The number of path failures.
<in-flight>: The number of in-flight I/Os on the path.
```

## Algorithm

dm-queue-length increments/decrements ‘in-flight’ when an I/O is
dispatched/completed respectively.
dm-queue-length selects a path with the minimum ‘in-flight’.

## Examples

In case that 2 paths (sda and sdb) are used with repeat\_count == 128.

```
# echo "0 10 multipath 0 0 1 1 queue-length 0 2 1 8:0 128 8:16 128" \
  dmsetup create test
#
# dmsetup table
test: 0 10 multipath 0 0 1 1 queue-length 0 2 1 8:0 128 8:16 128
#
# dmsetup status
test: 0 10 multipath 2 0 0 0 1 1 E 0 2 1 8:0 A 0 0 8:16 A 0 0
```
