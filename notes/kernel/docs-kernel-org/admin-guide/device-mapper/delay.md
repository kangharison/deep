# dm-delay

> 출처(원문): https://docs.kernel.org/admin-guide/device-mapper/delay.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# dm-delay

Device-Mapper’s “delay” target delays reads and/or writes
and/or flushes and optionally maps them to different devices.

Arguments:

```
<device> <offset> <delay> [<write_device> <write_offset> <write_delay>
                           [<flush_device> <flush_offset> <flush_delay>]]
```

Table line has to either have 3, 6 or 9 arguments:

3: apply offset and delay to read, write and flush operations on device

6: apply offset and delay to device, also apply write\_offset and write\_delay
:   to write and flush operations on optionally different write\_device with
    optionally different sector offset

9: same as 6 arguments plus define flush\_offset and flush\_delay explicitly
:   on/with optionally different flush\_device/flush\_offset.

Offsets are specified in sectors.

Delays are specified in milliseconds.

## Example scripts

::
:   #!/bin/sh
    #
    # Create mapped device named “delayed” delaying read, write and flush operations for 500ms.
    #
    dmsetup create delayed --table “0 blockdev --getsz $1 delay $1 0 500”

::
:   #!/bin/sh
    #
    # Create mapped device delaying write and flush operations for 400ms and
    # splitting reads to device $1 but writes and flushes to different device $2
    # to different offsets of 2048 and 4096 sectors respectively.
    #
    dmsetup create delayed --table “0 blockdev --getsz $1 delay $1 2048 0 $2 4096 400”

::
:   #!/bin/sh
    #
    # Create mapped device delaying reads for 50ms, writes for 100ms and flushes for 333ms
    # onto the same backing device at offset 0 sectors.
    #
    dmsetup create delayed --table “0 blockdev --getsz $1 delay $1 0 50 $2 0 100 $1 0 333”
