# Null block device driver

> 출처(원문): https://docs.kernel.org/block/null_blk.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Null block device driver

## Overview

The null block device (`/dev/nullb*`) is used for benchmarking the various
block-layer implementations. It emulates a block device of X gigabytes in size.
It does not execute any read/write operation, just mark them as complete in
the request queue. The following instances are possible:

> Multi-queue block-layer
>
> > * Request-based.
> > * Configurable submission queues per device.
>
> No block-layer (Known as bio-based)
>
> > * Bio-based. IO requests are submitted directly to the device driver.
> > * Directly accepts bio data structure and returns them.

All of them have a completion queue for each core in the system.

## Module parameters

queue\_mode=[0-2]: Default: 2-Multi-queue
:   Selects which block-layer the module should instantiate with.

    |  |  |
    | --- | --- |
    | 0 | Bio-based |
    | 1 | Single-queue (deprecated) |
    | 2 | Multi-queue |

home\_node=[0--nr\_nodes]: Default: NUMA\_NO\_NODE
:   Selects what CPU node the data structures are allocated from.

gb=[Size in GB]: Default: 250GB
:   The size of the device reported to the system.

bs=[Block size (in bytes)]: Default: 512 bytes
:   The block size reported to the system.

nr\_devices=[Number of devices]: Default: 1
:   Number of block devices instantiated. They are instantiated as /dev/nullb0,
    etc.

irqmode=[0-2]: Default: 1-Soft-irq
:   The completion mode used for completing IOs to the block-layer.

    |  |  |
    | --- | --- |
    | 0 | None. |
    | 1 | Soft-irq. Uses IPI to complete IOs across CPU nodes. Simulates the overhead when IOs are issued from another CPU node than the home the device is connected to. |
    | 2 | Timer: Waits a specific period (completion\_nsec) for each IO before completion. |

completion\_nsec=[ns]: Default: 10,000ns
:   Combined with irqmode=2 (timer). The time each completion event must wait.

submit\_queues=[1..nr\_cpus]: Default: 1
:   The number of submission queues attached to the device driver. If unset, it
    defaults to 1. For multi-queue, it is ignored when use\_per\_node\_hctx module
    parameter is 1.

hw\_queue\_depth=[0..qdepth]: Default: 64
:   The hardware queue depth of the device.

memory\_backed=[0/1]: Default: 0
:   Whether or not to use a memory buffer to respond to IO requests

    |  |  |
    | --- | --- |
    | 0 | Transfer no data in response to IO requests |
    | 1 | Use a memory buffer to respond to IO requests |

discard=[0/1]: Default: 0
:   Support discard operations (requires memory-backed null\_blk device).

    |  |  |
    | --- | --- |
    | 0 | Do not support discard operations |
    | 1 | Enable support for discard operations |

cache\_size=[Size in MB]: Default: 0
:   Cache size in MB for memory-backed device.

mbps=[Maximum bandwidth in MB/s]: Default: 0 (no limit)
:   Bandwidth limit for device performance.

### Multi-queue specific parameters

use\_per\_node\_hctx=[0/1]: Default: 0
:   Number of hardware context queues.

    |  |  |
    | --- | --- |
    | 0 | The number of submit queues are set to the value of the submit\_queues parameter. |
    | 1 | The multi-queue block layer is instantiated with a hardware dispatch queue for each CPU node in the system. |

no\_sched=[0/1]: Default: 0
:   Enable/disable the io scheduler.

    |  |  |
    | --- | --- |
    | 0 | nullb\* use default blk-mq io scheduler |
    | 1 | nullb\* doesn’t use io scheduler |

blocking=[0/1]: Default: 0
:   Blocking behavior of the request queue.

    |  |  |
    | --- | --- |
    | 0 | Register as a non-blocking blk-mq driver device. |
    | 1 | Register as a blocking blk-mq driver device, null\_blk will set the BLK\_MQ\_F\_BLOCKING flag, indicating that it sometimes/always needs to block in its ->`queue_rq()` function. |

shared\_tags=[0/1]: Default: 0
:   Sharing tags between devices.

    |  |  |
    | --- | --- |
    | 0 | Tag set is not shared. |
    | 1 | Tag set shared between devices for blk-mq. Only makes sense with nr\_devices > 1, otherwise there’s no tag set to share. |

zoned=[0/1]: Default: 0
:   Device is a random-access or a zoned block device.

    |  |  |
    | --- | --- |
    | 0 | Block device is exposed as a random-access block device. |
    | 1 | Block device is exposed as a host-managed zoned block device. Requires CONFIG\_BLK\_DEV\_ZONED. |

zone\_size=[MB]: Default: 256
:   Per zone size when exposed as a zoned block device. Must be a power of two.

zone\_nr\_conv=[nr\_conv]: Default: 0
:   The number of conventional zones to create when block device is zoned. If
    zone\_nr\_conv >= nr\_zones, it will be reduced to nr\_zones - 1.
