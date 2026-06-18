# 3.29.V4L2 JPEG header related functions and data structures

> 출처(원문): https://docs.kernel.org/driver-api/media/v4l2-jpeg.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.29. V4L2 JPEG header related functions and data structures

struct v4l2\_jpeg\_reference
:   reference into the JPEG buffer

> **Definition**:
>
> ```
> struct v4l2_jpeg_reference {
>       u8 *start;
>       size_t length;
> };
> ```

**Members**

`start`
:   pointer to the start of the referenced segment or table

`length`
:   size of the referenced segment or table

**Description**

Wnen referencing marker segments, start points right after the marker code,
and length is the size of the segment parameters, excluding the marker code.

struct v4l2\_jpeg\_frame\_component\_spec
:   frame component-specification

> **Definition**:
>
> ```
> struct v4l2_jpeg_frame_component_spec {
>       u8 component_identifier;
>       u8 horizontal_sampling_factor;
>       u8 vertical_sampling_factor;
>       u8 quantization_table_selector;
> };
> ```

**Members**

`component_identifier`
:   C[i]

`horizontal_sampling_factor`
:   H[i]

`vertical_sampling_factor`
:   V[i]

`quantization_table_selector`
:   quantization table destination selector Tq[i]

struct v4l2\_jpeg\_frame\_header
:   JPEG frame header

> **Definition**:
>
> ```
> struct v4l2_jpeg_frame_header {
>       u16 height;
>       u16 width;
>       u8 precision;
>       u8 num_components;
>       struct v4l2_jpeg_frame_component_spec component[V4L2_JPEG_MAX_COMPONENTS];
>       enum v4l2_jpeg_chroma_subsampling subsampling;
> };
> ```

**Members**

`height`
:   Y

`width`
:   X

`precision`
:   P

`num_components`
:   Nf

`component`
:   component-specification, see v4l2\_jpeg\_frame\_component\_spec

`subsampling`
:   decoded subsampling from component-specification

struct v4l2\_jpeg\_scan\_component\_spec
:   scan component-specification

> **Definition**:
>
> ```
> struct v4l2_jpeg_scan_component_spec {
>       u8 component_selector;
>       u8 dc_entropy_coding_table_selector;
>       u8 ac_entropy_coding_table_selector;
> };
> ```

**Members**

`component_selector`
:   Cs[j]

`dc_entropy_coding_table_selector`
:   Td[j]

`ac_entropy_coding_table_selector`
:   Ta[j]

struct v4l2\_jpeg\_scan\_header
:   JPEG scan header

> **Definition**:
>
> ```
> struct v4l2_jpeg_scan_header {
>       u8 num_components;
>       struct v4l2_jpeg_scan_component_spec component[V4L2_JPEG_MAX_COMPONENTS];
> };
> ```

**Members**

`num_components`
:   Ns

`component`
:   component-specification, see v4l2\_jpeg\_scan\_component\_spec

enum v4l2\_jpeg\_app14\_tf
:   APP14 transform flag According to Rec. ITU-T T.872 (06/2012) 6.5.3 APP14 segment is for color encoding, it contains a transform flag, which may have values of 0, 1 and 2 and are interpreted as follows:

**Constants**

`V4L2_JPEG_APP14_TF_CMYK_RGB`
:   CMYK for images encoded with four components
    RGB for images encoded with three components

`V4L2_JPEG_APP14_TF_YCBCR`
:   an image encoded with three components using YCbCr

`V4L2_JPEG_APP14_TF_YCCK`
:   an image encoded with four components using YCCK

`V4L2_JPEG_APP14_TF_UNKNOWN`
:   indicate app14 is not present

struct v4l2\_jpeg\_header
:   parsed JPEG header

> **Definition**:
>
> ```
> struct v4l2_jpeg_header {
>       struct v4l2_jpeg_reference sof;
>       struct v4l2_jpeg_reference sos;
>       unsigned int num_dht;
>       struct v4l2_jpeg_reference dht[V4L2_JPEG_MAX_TABLES];
>       unsigned int num_dqt;
>       struct v4l2_jpeg_reference dqt[V4L2_JPEG_MAX_TABLES];
>       struct v4l2_jpeg_frame_header frame;
>       struct v4l2_jpeg_scan_header *scan;
>       struct v4l2_jpeg_reference *quantization_tables;
>       struct v4l2_jpeg_reference *huffman_tables;
>       u16 restart_interval;
>       size_t ecs_offset;
>       enum v4l2_jpeg_app14_tf app14_tf;
> };
> ```

**Members**

`sof`
:   pointer to frame header and size

`sos`
:   pointer to scan header and size

`num_dht`
:   number of entries in **dht**

`dht`
:   pointers to huffman tables and sizes

`num_dqt`
:   number of entries in **dqt**

`dqt`
:   pointers to quantization tables and sizes

`frame`
:   parsed frame header

`scan`
:   pointer to parsed scan header, optional

`quantization_tables`
:   references to four quantization tables, optional

`huffman_tables`
:   references to four Huffman tables in DC0, DC1, AC0, AC1
    order, optional

`restart_interval`
:   number of MCU per restart interval, Ri

`ecs_offset`
:   buffer offset in bytes to the entropy coded segment

`app14_tf`
:   transform flag from app14 data

**Description**

When this structure is passed to v4l2\_jpeg\_parse\_header, the optional scan,
quantization\_tables, and huffman\_tables pointers must be initialized to NULL
or point at valid memory.

int v4l2\_jpeg\_parse\_header(void \*buf, size\_t len, struct [v4l2\_jpeg\_header](#c.v4l2_jpeg_header "v4l2_jpeg_header") \*out)
:   locate marker segments and optionally parse headers

**Parameters**

`void *buf`
:   address of the JPEG buffer, should start with a SOI marker

`size_t len`
:   length of the JPEG buffer

`struct v4l2_jpeg_header *out`
:   returns marker segment positions and optionally parsed headers

**Description**

The out->scan\_header pointer must be initialized to NULL or point to a valid
v4l2\_jpeg\_scan\_header structure. The out->huffman\_tables and
out->quantization\_tables pointers must be initialized to NULL or point to a
valid array of 4 v4l2\_jpeg\_reference structures each.

Returns 0 or negative error if parsing failed.
