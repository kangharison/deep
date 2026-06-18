# Network Block Device (TCP version)

> 출처(원문): https://docs.kernel.org/admin-guide/blockdev/nbd.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Network Block Device (TCP version)

## 1) Overview

What is it: With this compiled in the kernel (or as a module), Linux
can use a remote server as one of its block devices. So every time
the client computer wants to read, e.g., /dev/nb0, it sends a
request over TCP to the server, which will reply with the data read.
This can be used for stations with low disk space (or even diskless)
to borrow disk space from another computer.
Unlike NFS, it is possible to put any filesystem on it, etc.

For more information, or to download the nbd-client and nbd-server
tools, go to <https://github.com/NetworkBlockDevice/nbd>.

The nbd kernel module need only be installed on the client
system, as the nbd-server is completely in userspace. In fact,
the nbd-server has been successfully ported to other operating
systems, including Windows.

## A) NBD parameters

max\_part
:   Number of partitions per device (default: 0).

nbds\_max
:   Number of block devices that should be initialized (default: 16).
