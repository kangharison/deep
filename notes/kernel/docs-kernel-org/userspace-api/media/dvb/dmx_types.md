# 3.1.Demux Data Types

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/dmx_types.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.1. Demux Data Types

enum dmx\_output
:   Output for the demux.

**Constants**

`DMX_OUT_DECODER`
:   Streaming directly to decoder.

`DMX_OUT_TAP`
:   Output going to a memory buffer (to be retrieved via the read command).
    Delivers the stream output to the demux device on which the ioctl
    is called.

`DMX_OUT_TS_TAP`
:   Output multiplexed into a new TS (to be retrieved by reading from the
    logical DVR device). Routes output to the logical DVR device
    `/dev/dvb/adapter?/dvr?`, which delivers a TS multiplexed from all
    filters for which **DMX\_OUT\_TS\_TAP** was specified.

`DMX_OUT_TSDEMUX_TAP`
:   Like **DMX\_OUT\_TS\_TAP** but retrieved from the DMX device.

enum dmx\_input
:   Input from the demux.

**Constants**

`DMX_IN_FRONTEND`
:   Input from a front-end device.

`DMX_IN_DVR`
:   Input from the logical DVR device.

enum dmx\_ts\_pes
:   type of the PES filter.

**Constants**

`DMX_PES_AUDIO0`
:   first audio PID. Also referred as **DMX\_PES\_AUDIO**.

`DMX_PES_VIDEO0`
:   first video PID. Also referred as **DMX\_PES\_VIDEO**.

`DMX_PES_TELETEXT0`
:   first teletext PID. Also referred as **DMX\_PES\_TELETEXT**.

`DMX_PES_SUBTITLE0`
:   first subtitle PID. Also referred as **DMX\_PES\_SUBTITLE**.

`DMX_PES_PCR0`
:   first Program Clock Reference PID.
    Also referred as **DMX\_PES\_PCR**.

`DMX_PES_AUDIO1`
:   second audio PID.

`DMX_PES_VIDEO1`
:   second video PID.

`DMX_PES_TELETEXT1`
:   second teletext PID.

`DMX_PES_SUBTITLE1`
:   second subtitle PID.

`DMX_PES_PCR1`
:   second Program Clock Reference PID.

`DMX_PES_AUDIO2`
:   third audio PID.

`DMX_PES_VIDEO2`
:   third video PID.

`DMX_PES_TELETEXT2`
:   third teletext PID.

`DMX_PES_SUBTITLE2`
:   third subtitle PID.

`DMX_PES_PCR2`
:   third Program Clock Reference PID.

`DMX_PES_AUDIO3`
:   fourth audio PID.

`DMX_PES_VIDEO3`
:   fourth video PID.

`DMX_PES_TELETEXT3`
:   fourth teletext PID.

`DMX_PES_SUBTITLE3`
:   fourth subtitle PID.

`DMX_PES_PCR3`
:   fourth Program Clock Reference PID.

`DMX_PES_OTHER`
:   any other PID.

struct dmx\_filter
:   Specifies a section header filter.

**Definition**:

```
struct dmx_filter {
    __u8 filter[DMX_FILTER_SIZE];
    __u8 mask[DMX_FILTER_SIZE];
    __u8 mode[DMX_FILTER_SIZE];
};
```

**Members**

`filter`
:   bit array with bits to be matched at the section header.

`mask`
:   bits that are valid at the filter bit array.

`mode`
:   mode of match: if bit is zero, it will match if equal (positive
    match); if bit is one, it will match if the bit is negated.

**Note**

All arrays in this `struct have` a size of DMX\_FILTER\_SIZE (16 bytes).

struct dmx\_sct\_filter\_params
:   Specifies a section filter.

**Definition**:

```
struct dmx_sct_filter_params {
    __u16 pid;
    struct dmx_filter filter;
    __u32 timeout;
    __u32 flags;
#define DMX_CHECK_CRC       1;
#define DMX_ONESHOT         2;
#define DMX_IMMEDIATE_START 4;
};
```

**Members**

`pid`
:   PID to be filtered.

`filter`
:   section header filter, as defined by [`struct dmx_filter`](#c.DTV.dmx.dmx_filter "dmx_filter").

`timeout`
:   maximum time to filter, in milliseconds.

`flags`
:   extra flags for the section filter.

**Description**

Carries the configuration for a MPEG-TS section filter.

The **flags** can be:

> * `DMX_CHECK_CRC` - only deliver sections where the CRC check succeeded;
> * `DMX_ONESHOT` - disable the section filter after one section
>   has been delivered;
> * `DMX_IMMEDIATE_START` - Start filter immediately without requiring a
>   [DMX\_START](dmx-start.html#dmx-start).

struct dmx\_pes\_filter\_params
:   Specifies Packetized Elementary Stream (PES) filter parameters.

**Definition**:

```
struct dmx_pes_filter_params {
    __u16 pid;
    enum dmx_input  input;
    enum dmx_output output;
    enum dmx_ts_pes pes_type;
    __u32 flags;
};
```

**Members**

`pid`
:   PID to be filtered.

`input`
:   Demux input, as specified by [`enum dmx_input`](#c.DTV.dmx.dmx_input "dmx_input").

`output`
:   Demux output, as specified by [`enum dmx_output`](#c.DTV.dmx.dmx_output "dmx_output").

`pes_type`
:   Type of the pes filter, as specified by `enum dmx_pes_type`.

`flags`
:   Demux PES flags.

struct dmx\_stc
:   Stores System Time Counter (STC) information.

**Definition**:

```
struct dmx_stc {
    unsigned int num;
    unsigned int base;
    __u64 stc;
};
```

**Members**

`num`
:   input data: number of the STC, from 0 to N.

`base`
:   output: divisor for STC to get 90 kHz clock.

`stc`
:   output: stc in **base** \* 90 kHz units.

enum dmx\_buffer\_flags
:   DMX memory-mapped buffer flags

**Constants**

`DMX_BUFFER_FLAG_HAD_CRC32_DISCARD`
:   Indicates that the Kernel discarded one or more frames due to wrong
    CRC32 checksum.

`DMX_BUFFER_FLAG_TEI`
:   Indicates that the Kernel has detected a Transport Error indicator
    (TEI) on a filtered pid.

`DMX_BUFFER_PKT_COUNTER_MISMATCH`
:   Indicates that the Kernel has detected a packet counter mismatch
    on a filtered pid.

`DMX_BUFFER_FLAG_DISCONTINUITY_DETECTED`
:   Indicates that the Kernel has detected one or more frame discontinuity.

`DMX_BUFFER_FLAG_DISCONTINUITY_INDICATOR`
:   Received at least one packet with a frame discontinuity indicator.

struct dmx\_buffer
:   dmx buffer info

**Definition**:

```
struct dmx_buffer {
    __u32 index;
    __u32 bytesused;
    __u32 offset;
    __u32 length;
    __u32 flags;
    __u32 count;
};
```

**Members**

`index`
:   id number of the buffer

`bytesused`
:   number of bytes occupied by data in the buffer (payload);

`offset`
:   for buffers with memory == DMX\_MEMORY\_MMAP;
    offset from the start of the device memory for this plane,
    (or a “cookie” that should be passed to [`mmap()`](dmx-mmap.html#c.DTV.dmx.mmap "DTV.dmx.mmap") as offset)

`length`
:   size in bytes of the buffer

`flags`
:   bit array of buffer flags as defined by [`enum dmx_buffer_flags`](#c.DTV.dmx.dmx_buffer_flags "dmx_buffer_flags").
    Filled only at [`DMX_DQBUF`](dmx-qbuf.html#c.DTV.dmx.DMX_DQBUF "DMX_DQBUF").

`count`
:   monotonic counter for filled buffers. Helps to identify
    data stream loses. Filled only at [`DMX_DQBUF`](dmx-qbuf.html#c.DTV.dmx.DMX_DQBUF "DMX_DQBUF").

**Description**

Contains data exchanged by application and driver using one of the streaming
I/O methods.

Please notice that, for [`DMX_QBUF`](dmx-qbuf.html#c.DTV.dmx.DMX_QBUF "DMX_QBUF"), only **index** should be filled.
On [`DMX_DQBUF`](dmx-qbuf.html#c.DTV.dmx.DMX_DQBUF "DMX_DQBUF") calls, all fields will be filled by the Kernel.

struct dmx\_requestbuffers
:   request dmx buffer information

**Definition**:

```
struct dmx_requestbuffers {
    __u32 count;
    __u32 size;
};
```

**Members**

`count`
:   number of requested buffers,

`size`
:   size in bytes of the requested buffer

**Description**

Contains data used for requesting a dmx buffer.
All reserved fields must be set to zero.

struct dmx\_exportbuffer
:   export of dmx buffer as DMABUF file descriptor

**Definition**:

```
struct dmx_exportbuffer {
    __u32 index;
    __u32 flags;
    __s32 fd;
};
```

**Members**

`index`
:   id number of the buffer

`flags`
:   flags for newly created file, currently only O\_CLOEXEC is
    supported, refer to manual of open syscall for more details

`fd`
:   file descriptor associated with DMABUF (set by driver)

**Description**

Contains data used for exporting a dmx buffer as DMABUF file descriptor.
The buffer is identified by a ‘cookie’ returned by DMX\_QUERYBUF
(identical to the cookie used to [`mmap()`](dmx-mmap.html#c.DTV.dmx.mmap "DTV.dmx.mmap") the buffer to userspace). All
reserved fields must be set to zero. The field reserved0 is expected to
become a structure ‘type’ allowing an alternative layout of the structure
content. Therefore this field should not be used for any other extensions.
