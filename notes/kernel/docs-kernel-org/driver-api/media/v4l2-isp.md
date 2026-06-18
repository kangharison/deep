# 3.30.V4L2 generic ISP parameters and statistics support

> 출처(원문): https://docs.kernel.org/driver-api/media/v4l2-isp.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.30. V4L2 generic ISP parameters and statistics support

## 3.30.1. Design rationale

ISP configuration parameters and statistics are processed and collected by
drivers and exchanged with userspace through data types that usually
reflect the ISP peripheral registers layout.

Each ISP driver defines its own metadata output format for parameters and
a metadata capture format for statistics. The buffer layout is realized by a
set of C structures that reflects the registers layout. The number and types
of C structures is fixed by the format definition and becomes part of the Linux
kernel uAPI/uABI interface.

Because of the hard requirement of backward compatibility when extending the
user API/ABI interface, modifying an ISP driver capture or output metadata
format after it has been accepted by mainline is very hard if not impossible.

It generally happens, in fact, that after the first accepted revision of an ISP
driver the buffers layout need to be modified, either to support new hardware
blocks, to fix bugs or to support different revisions of the hardware.

Each of these situations would require defining a new metadata format, making it
really hard to maintain and extend drivers and requiring userspace to use
the correct format depending on the kernel revision in use.

## 3.30.2. V4L2 ISP configuration parameters

For these reasons, Video4Linux2 defines generic types for ISP configuration
parameters and statistics. Drivers are still expected to define their own
formats for their metadata output and capture nodes, but the buffers layout can
be defined using the extensible and versioned types defined by
include/uapi/linux/media/v4l2-isp.h.

Drivers are expected to provide the definitions of their supported ISP blocks
and the expected maximum size of a buffer.

For driver developers a set of helper functions to assist them with validation
of the buffer received from userspace is available in
drivers/media/v4l2-core/v4l2-isp.c

## 3.30.3. V4L2 ISP support driver documentation

v4l2\_isp\_params\_buffer\_size

`v4l2_isp_params_buffer_size (max_params_size)`

> Calculate size of v4l2\_isp\_params\_buffer

**Parameters**

`max_params_size`
:   The total size of the ISP configuration blocks

**Description**

Users of the v4l2 extensible parameters will have differing sized data arrays
depending on their specific parameter buffers. Drivers and userspace will
need to be able to calculate the appropriate size of the `struct to`
accommodate all ISP configuration blocks provided by the platform.
This macro provides a convenient tool for the calculation.

int v4l2\_isp\_params\_validate\_buffer\_size(struct [device](../infrastructure.html#c.device "device") \*dev, struct [vb2\_buffer](v4l2-videobuf2.html#c.vb2_buffer "vb2_buffer") \*vb, size\_t max\_size)
:   Validate a V4L2 ISP buffer sizes

**Parameters**

`struct device *dev`
:   the driver’s device pointer

`struct vb2_buffer *vb`
:   the videobuf2 buffer

`size_t max_size`
:   the maximum allowed buffer size

**Description**

This function performs validation of the size of a V4L2 ISP parameters buffer
before the driver can access the actual data buffer content.

After the sizes validation, drivers should copy the buffer content to a
kernel-only memory area to prevent userspace from modifying it,
before completing validation using [`v4l2_isp_params_validate_buffer()`](#c.v4l2_isp_params_validate_buffer "v4l2_isp_params_validate_buffer").

The **vb** buffer as received from the vb2 .`buf_prepare()` operation is checked
against **max\_size** and it’s validated to be large enough to accommodate at
least one ISP configuration block.

struct v4l2\_isp\_params\_block\_type\_info
:   V4L2 ISP per-block-type info

> **Definition**:
>
> ```
> struct v4l2_isp_params_block_type_info {
>       size_t size;
> };
> ```

**Members**

`size`
:   the block type expected size

**Description**

The v4l2\_isp\_params\_block\_type\_info collects information of the ISP
configuration block types for validation purposes. It currently only contains
the expected block type size.

Drivers shall prepare a list of block type info, indexed by block type, one
for each supported ISP block type and correctly populate them with the
expected block type size.

int v4l2\_isp\_params\_validate\_buffer(struct [device](../infrastructure.html#c.device "device") \*dev, struct [vb2\_buffer](v4l2-videobuf2.html#c.vb2_buffer "vb2_buffer") \*vb, const struct [v4l2\_isp\_params\_buffer](../../userspace-api/media/v4l/v4l2-isp.html#c.v4l2_isp_params_buffer "v4l2_isp_params_buffer") \*buffer, const struct [v4l2\_isp\_params\_block\_type\_info](#c.v4l2_isp_params_block_type_info "v4l2_isp_params_block_type_info") \*type\_info, size\_t num\_block\_types)
:   Validate a V4L2 ISP parameters buffer

**Parameters**

`struct device *dev`
:   the driver’s device pointer

`struct vb2_buffer *vb`
:   the videobuf2 buffer

`const struct v4l2_isp_params_buffer *buffer`
:   the V4L2 ISP parameters buffer

`const struct v4l2_isp_params_block_type_info *type_info`
:   the array of per-block-type validation info

`size_t num_block_types`
:   the number of block types in the type\_info array

**Description**

This function completes the validation of a V4L2 ISP parameters buffer,
verifying each configuration block correctness before the driver can use
them to program the hardware.

Drivers should use this function after having validated the correctness of
the vb2 buffer sizes by using the [`v4l2_isp_params_validate_buffer_size()`](#c.v4l2_isp_params_validate_buffer_size "v4l2_isp_params_validate_buffer_size")
helper first. Once the buffer size has been validated, drivers should
perform a copy of the user provided buffer into a kernel-only memory buffer
to prevent userspace from modifying its content after it has been submitted
to the driver, and then call this function to complete validation.
