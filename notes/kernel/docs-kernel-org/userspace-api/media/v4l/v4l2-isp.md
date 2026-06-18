# 2.13.15.Generic V4L2 ISP formats

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/v4l2-isp.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.13.15. Generic V4L2 ISP formats

Generic ISP formats are metadata formats that define a mechanism to pass ISP
parameters and statistics between userspace and drivers in V4L2 buffers. They
are designed to allow extending them in a backward-compatible way.

## 2.13.15.1. ISP parameters

The generic ISP configuration parameters format is realized by a defining a
single C structure that contains a header, followed by a binary buffer where
userspace programs a variable number of ISP configuration data block, one for
each supported ISP feature.

The [`v4l2_isp_params_buffer`](#c.v4l2_isp_params_buffer "v4l2_isp_params_buffer") structure defines the buffer header which
is followed by a binary buffer of ISP configuration data. Userspace shall
correctly populate the buffer header with the generic parameters format version
and with the size (in bytes) of the binary data buffer where it will store the
ISP blocks configuration.

Each *ISP configuration block* is preceded by an header implemented by the
[`v4l2_isp_params_block_header`](#c.v4l2_isp_params_block_header "v4l2_isp_params_block_header") structure, followed by the configuration
parameters for that specific block, defined by the ISP driver specific data
types.

Userspace applications are responsible for correctly populating each block’s
header fields (type, flags and size) and the block-specific parameters.

### 2.13.15.1.1. ISP block enabling, disabling and configuration

When userspace wants to configure and enable an ISP block it shall fully
populate the block configuration and set the V4L2\_ISP\_PARAMS\_FL\_BLOCK\_ENABLE
bit in the block header’s flags field.

When userspace simply wants to disable an ISP block the
V4L2\_ISP\_PARAMS\_FL\_BLOCK\_DISABLE bit should be set in block header’s flags
field. Drivers accept a configuration parameters block with no additional
data after the header in this case.

If the configuration of an already active ISP block has to be updated,
userspace shall fully populate the ISP block parameters and omit setting the
V4L2\_ISP\_PARAMS\_FL\_BLOCK\_ENABLE and V4L2\_ISP\_PARAMS\_FL\_BLOCK\_DISABLE bits in the
header’s flags field.

Setting both the V4L2\_ISP\_PARAMS\_FL\_BLOCK\_ENABLE and
V4L2\_ISP\_PARAMS\_FL\_BLOCK\_DISABLE bits in the flags field is not allowed and
returns an error.

Extension to the parameters format can be implemented by adding new blocks
definition without invalidating the existing ones.

## 2.13.15.2. ISP statistics

Support for generic statistics format is not yet implemented in Video4Linux2.

## 2.13.15.3. V4L2 ISP uAPI data types

enum v4l2\_isp\_params\_version
:   V4L2 ISP parameters versioning

**Constants**

`V4L2_ISP_PARAMS_VERSION_V0`
:   First version of the V4L2 ISP parameters format
    (for compatibility)

`V4L2_ISP_PARAMS_VERSION_V1`
:   First version of the V4L2 ISP parameters format

**Description**

V0 and V1 are identical in order to support drivers compatible with the V4L2
ISP parameters format already upstreamed which use either 0 or 1 as their
versioning identifier. Both V0 and V1 refers to the first version of the
V4L2 ISP parameters format.

Future revisions of the V4L2 ISP parameters format should start from the
value of 2.

struct v4l2\_isp\_params\_block\_header
:   V4L2 extensible parameters block header

**Definition**:

```
struct v4l2_isp_params_block_header {
    __u16 type;
    __u16 flags;
    __u32 size;
};
```

**Members**

`type`
:   The parameters block type (driver-specific)

`flags`
:   A bitmask of block flags (driver-specific)

`size`
:   Size (in bytes) of the parameters block, including this header

**Description**

This structure represents the common part of all the ISP configuration
blocks. Each parameters block shall embed an instance of this structure type
as its first member, followed by the block-specific configuration data.

The **type** field is an ISP driver-specific value that identifies the block
type. The **size** field specifies the size of the parameters block.

The **flags** field is a bitmask of per-block flags V4L2\_PARAMS\_ISP\_FL\_\* and
driver-specific flags specified by the driver header.

struct v4l2\_isp\_params\_buffer
:   V4L2 extensible parameters configuration

**Definition**:

```
struct v4l2_isp_params_buffer {
    __u32 version;
    __u32 data_size;
    __u8 data[];
};
```

**Members**

`version`
:   The parameters buffer version (driver-specific)

`data_size`
:   The configuration data effective size, excluding this header

`data`
:   The configuration data

**Description**

This structure contains the configuration parameters of the ISP algorithms,
serialized by userspace into a data buffer. Each configuration parameter
block is represented by a block-specific structure which contains a
[`v4l2_isp_params_block_header`](#c.v4l2_isp_params_block_header "v4l2_isp_params_block_header") entry as first member. Userspace
populates the **data** buffer with configuration parameters for the blocks that
it intends to configure. As a consequence, the data buffer effective size
changes according to the number of ISP blocks that userspace intends to
configure and is set by userspace in the **data\_size** field.

The parameters buffer is versioned by the **version** field to allow modifying
and extending its definition. Userspace shall populate the **version** field to
inform the driver about the version it intends to use. The driver will parse
and handle the **data** buffer according to the data layout specific to the
indicated version and return an error if the desired version is not
supported.

For each ISP block that userspace wants to configure, a block-specific
structure is appended to the **data** buffer, one after the other without gaps
in between. Userspace shall populate the **data\_size** field with the effective
size, in bytes, of the **data** buffer.
