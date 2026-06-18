# Global File System 2

> 출처(원문): https://docs.kernel.org/filesystems/gfs2/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Global File System 2

## Overview

GFS2 is a cluster file system. It allows a cluster of computers to
simultaneously use a block device that is shared between them (with FC,
iSCSI, NBD, etc). GFS2 reads and writes to the block device like a local
file system, but also uses a lock module to allow the computers coordinate
their I/O so file system consistency is maintained. One of the nifty
features of GFS2 is perfect consistency -- changes made to the file system
on one machine show up immediately on all other machines in the cluster.

GFS2 uses interchangeable inter-node locking mechanisms, the currently
supported mechanisms are:

> lock\_nolock
> :   * allows GFS2 to be used as a local file system
>
> lock\_dlm
> :   * uses the distributed lock manager (dlm) for inter-node locking.
>       The dlm is found at linux/fs/dlm/

lock\_dlm depends on user space cluster management systems found
at the URL above.

To use GFS2 as a local file system, no external clustering systems are
needed, simply:

```
$ mkfs -t gfs2 -p lock_nolock -j 1 /dev/block_device
$ mount -t gfs2 /dev/block_device /dir
```

The gfs2-utils package is required on all cluster nodes and, for lock\_dlm, you
will also need the dlm and corosync user space utilities configured as per the
documentation.

gfs2-utils can be found at <https://pagure.io/gfs2-utils>

GFS2 is not on-disk compatible with previous versions of GFS, but it
is pretty close.

The following man pages are available from gfs2-utils:

> |  |  |
> | --- | --- |
> | fsck.gfs2 | to repair a filesystem |
> | gfs2\_grow | to expand a filesystem online |
> | gfs2\_jadd | to add journals to a filesystem online |
> | tunegfs2 | to manipulate, examine and tune a filesystem |
> | gfs2\_convert | to convert a gfs filesystem to GFS2 in-place |
> | mkfs.gfs2 | to make a filesystem |

## Implementation Notes

* [Glock internal locking rules](glocks.html)
* [uevents and GFS2](uevents.html)
