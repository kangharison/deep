# Linux SCSI Disk Driver (sd) Parameters

> 출처(원문): https://docs.kernel.org/scsi/sd-parameters.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Linux SCSI Disk Driver (sd) Parameters

## cache\_type (RW)

Enable/disable drive write & read cache.

| cache\_type string | WCE | RCD | Write cache | Read cache |
| --- | --- | --- | --- | --- |
| write through | 0 | 0 | off | on |
| none | 0 | 1 | off | off |
| write back | 1 | 0 | on | on |
| write back, no read (daft) | 1 | 1 | on | off |

To set cache type to “write back” and save this setting to the drive:

```
# echo "write back" > cache_type
```

To modify the caching mode without making the change persistent, prepend
“temporary “ to the cache type string. E.g.:

```
# echo "temporary write back" > cache_type
```
