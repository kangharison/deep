# Distributed Replicated Block Device - DRBD

> 출처(원문): https://docs.kernel.org/admin-guide/blockdev/drbd/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Distributed Replicated Block Device - DRBD

## Description

> DRBD is a shared-nothing, synchronously replicated block device. It
> is designed to serve as a building block for high availability
> clusters and in this context, is a “drop-in” replacement for shared
> storage. Simplistically, you could see it as a network RAID 1.
>
> Please visit <https://www.drbd.org> to find out more.

* [kernel data structure for DRBD-9](data-structure-v9.html)
* [Data flows that Relate some functions, and write packets](figures.html)
* [Sub graphs of DRBD’s state transitions](figures.html#sub-graphs-of-drbd-s-state-transitions)
