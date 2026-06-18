# Documentation for /proc/sys/sunrpc/

> 출처(원문): https://docs.kernel.org/admin-guide/sysctl/sunrpc.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Documentation for /proc/sys/sunrpc/

kernel version 2.2.10

Copyright (c) 1998, 1999, Rik van Riel <[riel@nl.linux.org](mailto:riel%40nl.linux.org)>

For general info and legal blurb, please look in [Documentation for /proc/sys](index.html).

---

This file contains the documentation for the sysctl files in
/proc/sys/sunrpc and is valid for Linux kernel version 2.2.

The files in this directory can be used to (re)set the debug
flags of the SUN Remote Procedure Call (RPC) subsystem in
the Linux kernel. This stuff is used for NFS, KNFSD and
maybe a few other things as well.

The files in there are used to control the debugging flags:
rpc\_debug, nfs\_debug, nfsd\_debug and nlm\_debug.

These flags are for kernel hackers only. You should read the
source code in net/sunrpc/ for more information.
