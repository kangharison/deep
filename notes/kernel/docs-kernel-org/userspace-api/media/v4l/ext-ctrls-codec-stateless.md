# 1.16.Stateless Codec Control Reference

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/ext-ctrls-codec-stateless.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1.16. Stateless Codec Control Reference

The Stateless Codec control class is intended to support
stateless decoder and encoders (i.e. hardware accelerators).

These drivers are typically supported by the [Memory-to-memory Stateless Video Decoder Interface](dev-stateless-decoder.html#stateless-decoder),
and deal with parsed pixel formats such as V4L2\_PIX\_FMT\_H264\_SLICE.

## 1.16.1. Stateless Codec Control ID

`V4L2_CID_CODEC_STATELESS_CLASS (class)`
:   The Stateless Codec class descriptor.

`V4L2_CID_STATELESS_H264_SPS (struct)`
:   Specifies the sequence parameter set (as extracted from the
    bitstream) for the associated H264 slice data. This includes the
    necessary parameters for configuring a stateless hardware decoding
    pipeline for H264. The bitstream parameters are defined according
    to [ITU-T Rec. H.264 Specification (04/2017 Edition)](biblio.html#h264), section 7.4.2.1.1 “Sequence Parameter Set Data
    Semantics”. For further documentation, refer to the above
    specification, unless there is an explicit comment stating
    otherwise.

type v4l2\_ctrl\_h264\_sps

struct v4l2\_ctrl\_h264\_sps

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `profile_idc` |  |
| \_\_u8 | `constraint_set_flags` | See [Sequence Parameter Set Constraints Set Flags](#h264-sps-constraints-set-flags) |
| \_\_u8 | `level_idc` |  |
| \_\_u8 | `seq_parameter_set_id` |  |
| \_\_u8 | `chroma_format_idc` |  |
| \_\_u8 | `bit_depth_luma_minus8` |  |
| \_\_u8 | `bit_depth_chroma_minus8` |  |
| \_\_u8 | `log2_max_frame_num_minus4` |  |
| \_\_u8 | `pic_order_cnt_type` |  |
| \_\_u8 | `log2_max_pic_order_cnt_lsb_minus4` |  |
| \_\_u8 | `max_num_ref_frames` |  |
| \_\_u8 | `num_ref_frames_in_pic_order_cnt_cycle` |  |
| \_\_s32 | `offset_for_ref_frame[255]` |  |
| \_\_s32 | `offset_for_non_ref_pic` |  |
| \_\_s32 | `offset_for_top_to_bottom_field` |  |
| \_\_u16 | `pic_width_in_mbs_minus1` |  |
| \_\_u16 | `pic_height_in_map_units_minus1` |  |
| \_\_u32 | `flags` | See [Sequence Parameter Set Flags](#h264-sps-flags) |

`Sequence Parameter Set Constraints Set Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_H264_SPS_CONSTRAINT_SET0_FLAG` | 0x00000001 |  |
| `V4L2_H264_SPS_CONSTRAINT_SET1_FLAG` | 0x00000002 |  |
| `V4L2_H264_SPS_CONSTRAINT_SET2_FLAG` | 0x00000004 |  |
| `V4L2_H264_SPS_CONSTRAINT_SET3_FLAG` | 0x00000008 |  |
| `V4L2_H264_SPS_CONSTRAINT_SET4_FLAG` | 0x00000010 |  |
| `V4L2_H264_SPS_CONSTRAINT_SET5_FLAG` | 0x00000020 |  |

`Sequence Parameter Set Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_H264_SPS_FLAG_SEPARATE_COLOUR_PLANE` | 0x00000001 |  |
| `V4L2_H264_SPS_FLAG_QPPRIME_Y_ZERO_TRANSFORM_BYPASS` | 0x00000002 |  |
| `V4L2_H264_SPS_FLAG_DELTA_PIC_ORDER_ALWAYS_ZERO` | 0x00000004 |  |
| `V4L2_H264_SPS_FLAG_GAPS_IN_FRAME_NUM_VALUE_ALLOWED` | 0x00000008 |  |
| `V4L2_H264_SPS_FLAG_FRAME_MBS_ONLY` | 0x00000010 |  |
| `V4L2_H264_SPS_FLAG_MB_ADAPTIVE_FRAME_FIELD` | 0x00000020 |  |
| `V4L2_H264_SPS_FLAG_DIRECT_8X8_INFERENCE` | 0x00000040 |  |

`V4L2_CID_STATELESS_H264_PPS (struct)`
:   Specifies the picture parameter set (as extracted from the
    bitstream) for the associated H264 slice data. This includes the
    necessary parameters for configuring a stateless hardware decoding
    pipeline for H264. The bitstream parameters are defined according
    to [ITU-T Rec. H.264 Specification (04/2017 Edition)](biblio.html#h264), section 7.4.2.2 “Picture Parameter Set RBSP
    Semantics”. For further documentation, refer to the above
    specification, unless there is an explicit comment stating
    otherwise.

type v4l2\_ctrl\_h264\_pps

struct v4l2\_ctrl\_h264\_pps

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `pic_parameter_set_id` |  |
| \_\_u8 | `seq_parameter_set_id` |  |
| \_\_u8 | `num_slice_groups_minus1` |  |
| \_\_u8 | `num_ref_idx_l0_default_active_minus1` |  |
| \_\_u8 | `num_ref_idx_l1_default_active_minus1` |  |
| \_\_u8 | `weighted_bipred_idc` |  |
| \_\_s8 | `pic_init_qp_minus26` |  |
| \_\_s8 | `pic_init_qs_minus26` |  |
| \_\_s8 | `chroma_qp_index_offset` |  |
| \_\_s8 | `second_chroma_qp_index_offset` |  |
| \_\_u16 | `flags` | See [Picture Parameter Set Flags](#h264-pps-flags) |

`Picture Parameter Set Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_H264_PPS_FLAG_ENTROPY_CODING_MODE` | 0x0001 |  |
| `V4L2_H264_PPS_FLAG_BOTTOM_FIELD_PIC_ORDER_IN_FRAME_PRESENT` | 0x0002 |  |
| `V4L2_H264_PPS_FLAG_WEIGHTED_PRED` | 0x0004 |  |
| `V4L2_H264_PPS_FLAG_DEBLOCKING_FILTER_CONTROL_PRESENT` | 0x0008 |  |
| `V4L2_H264_PPS_FLAG_CONSTRAINED_INTRA_PRED` | 0x0010 |  |
| `V4L2_H264_PPS_FLAG_REDUNDANT_PIC_CNT_PRESENT` | 0x0020 |  |
| `V4L2_H264_PPS_FLAG_TRANSFORM_8X8_MODE` | 0x0040 |  |
| `V4L2_H264_PPS_FLAG_SCALING_MATRIX_PRESENT` | 0x0080 | `V4L2_CID_STATELESS_H264_SCALING_MATRIX` must be used for this picture. |

`V4L2_CID_STATELESS_H264_SCALING_MATRIX (struct)`
:   Specifies the scaling matrix (as extracted from the bitstream) for
    the associated H264 slice data. The bitstream parameters are
    defined according to [ITU-T Rec. H.264 Specification (04/2017 Edition)](biblio.html#h264), section 7.4.2.1.1.1 “Scaling
    List Semantics”. For further documentation, refer to the above
    specification, unless there is an explicit comment stating
    otherwise.

type v4l2\_ctrl\_h264\_scaling\_matrix

struct v4l2\_ctrl\_h264\_scaling\_matrix

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `scaling_list_4x4[6][16]` | Scaling matrix after applying the inverse scanning process. Expected list order is Intra Y, Intra Cb, Intra Cr, Inter Y, Inter Cb, Inter Cr. The values on each scaling list are expected in raster scan order. |
| \_\_u8 | `scaling_list_8x8[6][64]` | Scaling matrix after applying the inverse scanning process. Expected list order is Intra Y, Inter Y, Intra Cb, Inter Cb, Intra Cr, Inter Cr. The values on each scaling list are expected in raster scan order. |

`V4L2_CID_STATELESS_H264_SLICE_PARAMS (struct)`
:   Specifies the slice parameters (as extracted from the bitstream)
    for the associated H264 slice data. This includes the necessary
    parameters for configuring a stateless hardware decoding pipeline
    for H264. The bitstream parameters are defined according to
    [ITU-T Rec. H.264 Specification (04/2017 Edition)](biblio.html#h264), section 7.4.3 “Slice Header Semantics”. For further
    documentation, refer to the above specification, unless there is
    an explicit comment stating otherwise.

type v4l2\_ctrl\_h264\_slice\_params

struct v4l2\_ctrl\_h264\_slice\_params

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `header_bit_size` | Offset in bits to `slice_data()` from the beginning of this slice. |
| \_\_u32 | `first_mb_in_slice` |  |
| \_\_u8 | `slice_type` |  |
| \_\_u8 | `colour_plane_id` |  |
| \_\_u8 | `redundant_pic_cnt` |  |
| \_\_u8 | `cabac_init_idc` |  |
| \_\_s8 | `slice_qp_delta` |  |
| \_\_s8 | `slice_qs_delta` |  |
| \_\_u8 | `disable_deblocking_filter_idc` |  |
| \_\_s8 | `slice_alpha_c0_offset_div2` |  |
| \_\_s8 | `slice_beta_offset_div2` |  |
| \_\_u8 | `num_ref_idx_l0_active_minus1` | If num\_ref\_idx\_active\_override\_flag is not set, this field must be set to the value of num\_ref\_idx\_l0\_default\_active\_minus1 |
| \_\_u8 | `num_ref_idx_l1_active_minus1` | If num\_ref\_idx\_active\_override\_flag is not set, this field must be set to the value of num\_ref\_idx\_l1\_default\_active\_minus1 |
| \_\_u8 | `reserved` | Applications and drivers must set this to zero. |
| struct [`v4l2_h264_reference`](#c.V4L.v4l2_h264_reference "v4l2_h264_reference") | `ref_pic_list0[32]` | Reference picture list after applying the per-slice modifications |
| struct [`v4l2_h264_reference`](#c.V4L.v4l2_h264_reference "v4l2_h264_reference") | `ref_pic_list1[32]` | Reference picture list after applying the per-slice modifications |
| \_\_u32 | `flags` | See [Slice Parameter Flags](#h264-slice-flags) |

`Slice Parameter Set Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_H264_SLICE_FLAG_DIRECT_SPATIAL_MV_PRED` | 0x00000001 |  |
| `V4L2_H264_SLICE_FLAG_SP_FOR_SWITCH` | 0x00000002 |  |

`V4L2_CID_STATELESS_H264_PRED_WEIGHTS (struct)`
:   Prediction weight table defined according to [ITU-T Rec. H.264 Specification (04/2017 Edition)](biblio.html#h264),
    section 7.4.3.2 “Prediction Weight Table Semantics”.
    The prediction weight table must be passed by applications
    under the conditions explained in section 7.3.3 “Slice header
    syntax”.

type v4l2\_ctrl\_h264\_pred\_weights

struct v4l2\_ctrl\_h264\_pred\_weights

|  |  |  |
| --- | --- | --- |
| \_\_u16 | `luma_log2_weight_denom` |  |
| \_\_u16 | `chroma_log2_weight_denom` |  |
| struct [`v4l2_h264_weight_factors`](#c.V4L.v4l2_h264_weight_factors "v4l2_h264_weight_factors") | `weight_factors[2]` | The weight factors at index 0 are the weight factors for the reference list 0, the one at index 1 for the reference list 1. |

type v4l2\_h264\_weight\_factors

struct v4l2\_h264\_weight\_factors

|  |  |  |
| --- | --- | --- |
| \_\_s16 | `luma_weight[32]` |  |
| \_\_s16 | `luma_offset[32]` |  |
| \_\_s16 | `chroma_weight[32][2]` |  |
| \_\_s16 | `chroma_offset[32][2]` |  |

`Picture Reference`

type v4l2\_h264\_reference

struct v4l2\_h264\_reference

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `fields` | Specifies how the picture is referenced. See [Reference Fields](#h264-ref-fields) |
| \_\_u8 | `index` | Index into the [`v4l2_ctrl_h264_decode_params`](#c.V4L.v4l2_ctrl_h264_decode_params "v4l2_ctrl_h264_decode_params").dpb array. |

`Reference Fields`

|  |  |  |
| --- | --- | --- |
| `V4L2_H264_TOP_FIELD_REF` | 0x1 | The top field in field pair is used for short-term reference. |
| `V4L2_H264_BOTTOM_FIELD_REF` | 0x2 | The bottom field in field pair is used for short-term reference. |
| `V4L2_H264_FRAME_REF` | 0x3 | The frame (or the top/bottom fields, if it’s a field pair) is used for short-term reference. |

`V4L2_CID_STATELESS_H264_DECODE_PARAMS (struct)`
:   Specifies the decode parameters (as extracted from the bitstream)
    for the associated H264 slice data. This includes the necessary
    parameters for configuring a stateless hardware decoding pipeline
    for H264. The bitstream parameters are defined according to
    [ITU-T Rec. H.264 Specification (04/2017 Edition)](biblio.html#h264). For further documentation, refer to the above
    specification, unless there is an explicit comment stating
    otherwise.

type v4l2\_ctrl\_h264\_decode\_params

struct v4l2\_ctrl\_h264\_decode\_params

|  |  |  |
| --- | --- | --- |
| struct [`v4l2_h264_dpb_entry`](#c.V4L.v4l2_h264_dpb_entry "v4l2_h264_dpb_entry") | `dpb[16]` |  |
| \_\_u16 | `nal_ref_idc` | NAL reference ID value coming from the NAL Unit header |
| \_\_u16 | `frame_num` |  |
| \_\_s32 | `top_field_order_cnt` | Picture Order Count for the coded top field |
| \_\_s32 | `bottom_field_order_cnt` | Picture Order Count for the coded bottom field |
| \_\_u16 | `idr_pic_id` |  |
| \_\_u16 | `pic_order_cnt_lsb` |  |
| \_\_s32 | `delta_pic_order_cnt_bottom` |  |
| \_\_s32 | `delta_pic_order_cnt0` |  |
| \_\_s32 | `delta_pic_order_cnt1` |  |
| \_\_u32 | `dec_ref_pic_marking_bit_size` | Size in bits of the `dec_ref_pic_marking()` syntax element. |
| \_\_u32 | `pic_order_cnt_bit_size` | Combined size in bits of the picture order count related syntax elements: pic\_order\_cnt\_lsb, delta\_pic\_order\_cnt\_bottom, delta\_pic\_order\_cnt0, and delta\_pic\_order\_cnt1. |
| \_\_u32 | `slice_group_change_cycle` |  |
| \_\_u32 | `reserved` | Applications and drivers must set this to zero. |
| \_\_u32 | `flags` | See [Decode Parameters Flags](#h264-decode-params-flags) |

`Decode Parameters Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_H264_DECODE_PARAM_FLAG_IDR_PIC` | 0x00000001 | That picture is an IDR picture |
| `V4L2_H264_DECODE_PARAM_FLAG_FIELD_PIC` | 0x00000002 |  |
| `V4L2_H264_DECODE_PARAM_FLAG_BOTTOM_FIELD` | 0x00000004 |  |
| `V4L2_H264_DECODE_PARAM_FLAG_PFRAME` | 0x00000008 |  |
| `V4L2_H264_DECODE_PARAM_FLAG_BFRAME` | 0x00000010 |  |

type v4l2\_h264\_dpb\_entry

struct v4l2\_h264\_dpb\_entry

|  |  |  |
| --- | --- | --- |
| \_\_u64 | `reference_ts` | Timestamp of the V4L2 capture buffer to use as reference, used with B-coded and P-coded frames. The timestamp refers to the `timestamp` field in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer"). Use the `v4l2_timeval_to_ns()` function to convert the struct `timeval` in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer") to a \_\_u64. |
| \_\_u32 | `pic_num` | For short term references, this must match the derived value PicNum (8-28) and for long term references it must match the derived value LongTermPicNum (8-29). When decoding frames (as opposed to fields) pic\_num is the same as FrameNumWrap. |
| \_\_u16 | `frame_num` | For short term references, this must match the frame\_num value from the slice header syntax (the driver will wrap the value if needed). For long term references, this must be set to the value of long\_term\_frame\_idx described in the `dec_ref_pic_marking()` syntax. |
| \_\_u8 | `fields` | Specifies how the DPB entry is referenced. See [Reference Fields](#h264-ref-fields) |
| \_\_u8 | `reserved[5]` | Applications and drivers must set this to zero. |
| \_\_s32 | `top_field_order_cnt` |  |
| \_\_s32 | `bottom_field_order_cnt` |  |
| \_\_u32 | `flags` | See [DPB Entry Flags](#h264-dpb-flags) |

`DPB Entries Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_H264_DPB_ENTRY_FLAG_VALID` | 0x00000001 | The DPB entry is valid (non-empty) and should be considered. |
| `V4L2_H264_DPB_ENTRY_FLAG_ACTIVE` | 0x00000002 | The DPB entry is used for reference. |
| `V4L2_H264_DPB_ENTRY_FLAG_LONG_TERM` | 0x00000004 | The DPB entry is used for long-term reference. |
| `V4L2_H264_DPB_ENTRY_FLAG_FIELD` | 0x00000008 | The DPB entry is a single field or a complementary field pair. |

`V4L2_CID_STATELESS_H264_DECODE_MODE (enum)`
:   Specifies the decoding mode to use. Currently exposes slice-based and
    frame-based decoding but new modes might be added later on.
    This control is used as a modifier for V4L2\_PIX\_FMT\_H264\_SLICE
    pixel format. Applications that support V4L2\_PIX\_FMT\_H264\_SLICE
    are required to set this control in order to specify the decoding mode
    that is expected for the buffer.
    Drivers may expose a single or multiple decoding modes, depending
    on what they can support.

type v4l2\_stateless\_h264\_decode\_mode

|  |  |  |
| --- | --- | --- |
| `V4L2_STATELESS_H264_DECODE_MODE_SLICE_BASED` | 0 | Decoding is done at the slice granularity. The OUTPUT buffer must contain a single slice. When this mode is selected, the `V4L2_CID_STATELESS_H264_SLICE_PARAMS` control shall be set. When multiple slices compose a frame, use of `V4L2_BUF_CAP_SUPPORTS_M2M_HOLD_CAPTURE_BUF` flag is required. |
| `V4L2_STATELESS_H264_DECODE_MODE_FRAME_BASED` | 1 | Decoding is done at the frame granularity, The OUTPUT buffer must contain all slices needed to decode the frame. The OUTPUT buffer must also contain both fields. This mode will be supported by devices that parse the slice(s) header(s) in hardware. When this mode is selected, the `V4L2_CID_STATELESS_H264_SLICE_PARAMS` control shall not be set. |

`V4L2_CID_STATELESS_H264_START_CODE (enum)`
:   Specifies the H264 slice start code expected for each slice.
    This control is used as a modifier for V4L2\_PIX\_FMT\_H264\_SLICE
    pixel format. Applications that support V4L2\_PIX\_FMT\_H264\_SLICE
    are required to set this control in order to specify the start code
    that is expected for the buffer.
    Drivers may expose a single or multiple start codes, depending
    on what they can support.

type v4l2\_stateless\_h264\_start\_code

|  |  |  |
| --- | --- | --- |
| `V4L2_STATELESS_H264_START_CODE_NONE` | 0 | Selecting this value specifies that H264 slices are passed to the driver without any start code. The bitstream data should be according to [ITU-T Rec. H.264 Specification (04/2017 Edition)](biblio.html#h264) 7.3.1 NAL unit syntax, hence contains emulation prevention bytes when required. |
| `V4L2_STATELESS_H264_START_CODE_ANNEX_B` | 1 | Selecting this value specifies that H264 slices are expected to be prefixed by Annex B start codes. According to [ITU-T Rec. H.264 Specification (04/2017 Edition)](biblio.html#h264) valid start codes can be 3-bytes 0x000001 or 4-bytes 0x00000001. |

`V4L2_CID_STATELESS_FWHT_PARAMS (struct)`
:   Specifies the FWHT (Fast Walsh Hadamard Transform) parameters (as extracted
    from the bitstream) for the associated FWHT data. This includes the necessary
    parameters for configuring a stateless hardware decoding pipeline for FWHT.
    This codec is specific to the vicodec test driver.

type v4l2\_ctrl\_fwht\_params

struct v4l2\_ctrl\_fwht\_params

|  |  |  |
| --- | --- | --- |
| \_\_u64 | `backward_ref_ts` | Timestamp of the V4L2 capture buffer to use as backward reference, used with P-coded frames. The timestamp refers to the `timestamp` field in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer"). Use the `v4l2_timeval_to_ns()` function to convert the struct `timeval` in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer") to a \_\_u64. |
| \_\_u32 | `version` | The version of the codec. Set to `V4L2_FWHT_VERSION`. |
| \_\_u32 | `width` | The width of the frame. |
| \_\_u32 | `height` | The height of the frame. |
| \_\_u32 | `flags` | The flags of the frame, see [FWHT Flags](#fwht-flags). |
| \_\_u32 | `colorspace` | The colorspace of the frame, from enum [`v4l2_colorspace`](colorspaces-defs.html#c.V4L.v4l2_colorspace "v4l2_colorspace"). |
| \_\_u32 | `xfer_func` | The transfer function, from enum [`v4l2_xfer_func`](colorspaces-defs.html#c.V4L.v4l2_xfer_func "v4l2_xfer_func"). |
| \_\_u32 | `ycbcr_enc` | The Y’CbCr encoding, from enum [`v4l2_ycbcr_encoding`](colorspaces-defs.html#c.V4L.v4l2_ycbcr_encoding "v4l2_ycbcr_encoding"). |
| \_\_u32 | `quantization` | The quantization range, from enum [`v4l2_quantization`](colorspaces-defs.html#c.V4L.v4l2_quantization "v4l2_quantization"). |

## 1.16.2. FWHT Flags

|  |  |  |
| --- | --- | --- |
| `V4L2_FWHT_FL_IS_INTERLACED` | 0x00000001 | Set if this is an interlaced format. |
| `V4L2_FWHT_FL_IS_BOTTOM_FIRST` | 0x00000002 | Set if this is a bottom-first (NTSC) interlaced format. |
| `V4L2_FWHT_FL_IS_ALTERNATE` | 0x00000004 | Set if each ‘frame’ contains just one field. |
| `V4L2_FWHT_FL_IS_BOTTOM_FIELD` | 0x00000008 | If V4L2\_FWHT\_FL\_IS\_ALTERNATE was set, then this is set if this ‘frame’ is the bottom field, else it is the top field. |
| `V4L2_FWHT_FL_LUMA_IS_UNCOMPRESSED` | 0x00000010 | Set if the Y’ (luma) plane is uncompressed. |
| `V4L2_FWHT_FL_CB_IS_UNCOMPRESSED` | 0x00000020 | Set if the Cb plane is uncompressed. |
| `V4L2_FWHT_FL_CR_IS_UNCOMPRESSED` | 0x00000040 | Set if the Cr plane is uncompressed. |
| `V4L2_FWHT_FL_CHROMA_FULL_HEIGHT` | 0x00000080 | Set if the chroma plane has the same height as the luma plane, else the chroma plane is half the height of the luma plane. |
| `V4L2_FWHT_FL_CHROMA_FULL_WIDTH` | 0x00000100 | Set if the chroma plane has the same width as the luma plane, else the chroma plane is half the width of the luma plane. |
| `V4L2_FWHT_FL_ALPHA_IS_UNCOMPRESSED` | 0x00000200 | Set if the alpha plane is uncompressed. |
| `V4L2_FWHT_FL_I_FRAME` | 0x00000400 | Set if this is an I-frame. |
| `V4L2_FWHT_FL_COMPONENTS_NUM_MSK` | 0x00070000 | The number of color components minus one. |
| `V4L2_FWHT_FL_PIXENC_MSK` | 0x00180000 | The mask for the pixel encoding. |
| `V4L2_FWHT_FL_PIXENC_YUV` | 0x00080000 | Set if the pixel encoding is YUV. |
| `V4L2_FWHT_FL_PIXENC_RGB` | 0x00100000 | Set if the pixel encoding is RGB. |
| `V4L2_FWHT_FL_PIXENC_HSV` | 0x00180000 | Set if the pixel encoding is HSV. |

`V4L2_CID_STATELESS_VP8_FRAME (struct)`
:   Specifies the frame parameters for the associated VP8 parsed frame data.
    This includes the necessary parameters for
    configuring a stateless hardware decoding pipeline for VP8.
    The bitstream parameters are defined according to [VP8](biblio.html#vp8).

type v4l2\_ctrl\_vp8\_frame

struct v4l2\_ctrl\_vp8\_frame

|  |  |  |
| --- | --- | --- |
| struct [`v4l2_vp8_segment`](#c.V4L.v4l2_vp8_segment "v4l2_vp8_segment") | `segment` | Structure with segment-based adjustments metadata. |
| struct [`v4l2_vp8_loop_filter`](#c.V4L.v4l2_vp8_loop_filter "v4l2_vp8_loop_filter") | `lf` | Structure with loop filter level adjustments metadata. |
| struct [`v4l2_vp8_quantization`](#c.V4L.v4l2_vp8_quantization "v4l2_vp8_quantization") | `quant` | Structure with VP8 dequantization indices metadata. |
| struct [`v4l2_vp8_entropy`](#c.V4L.v4l2_vp8_entropy "v4l2_vp8_entropy") | `entropy` | Structure with VP8 entropy coder probabilities metadata. |
| struct [`v4l2_vp8_entropy_coder_state`](#c.V4L.v4l2_vp8_entropy_coder_state "v4l2_vp8_entropy_coder_state") | `coder_state` | Structure with VP8 entropy coder state. |
| \_\_u16 | `width` | The width of the frame. Must be set for all frames. |
| \_\_u16 | `height` | The height of the frame. Must be set for all frames. |
| \_\_u8 | `horizontal_scale` | Horizontal scaling factor. |
| \_\_u8 | `vertical_scale` | Vertical scaling factor. |
| \_\_u8 | `version` | Bitstream version. |
| \_\_u8 | `prob_skip_false` | Indicates the probability that the macroblock is not skipped. |
| \_\_u8 | `prob_intra` | Indicates the probability that a macroblock is intra-predicted. |
| \_\_u8 | `prob_last` | Indicates the probability that the last reference frame is used for inter-prediction |
| \_\_u8 | `prob_gf` | Indicates the probability that the golden reference frame is used for inter-prediction |
| \_\_u8 | `num_dct_parts` | Number of DCT coefficients partitions. Must be one of: 1, 2, 4, or 8. |
| \_\_u32 | `first_part_size` | Size of the first partition, i.e. the control partition. |
| \_\_u32 | `first_part_header_bits` | Size in bits of the first partition header portion. |
| \_\_u32 | `dct_part_sizes[8]` | DCT coefficients sizes. |
| \_\_u64 | `last_frame_ts` | Timestamp for the V4L2 capture buffer to use as last reference frame, used with inter-coded frames. The timestamp refers to the `timestamp` field in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer"). Use the `v4l2_timeval_to_ns()` function to convert the struct `timeval` in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer") to a \_\_u64. |
| \_\_u64 | `golden_frame_ts` | Timestamp for the V4L2 capture buffer to use as last reference frame, used with inter-coded frames. The timestamp refers to the `timestamp` field in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer"). Use the `v4l2_timeval_to_ns()` function to convert the struct `timeval` in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer") to a \_\_u64. |
| \_\_u64 | `alt_frame_ts` | Timestamp for the V4L2 capture buffer to use as alternate reference frame, used with inter-coded frames. The timestamp refers to the `timestamp` field in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer"). Use the `v4l2_timeval_to_ns()` function to convert the struct `timeval` in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer") to a \_\_u64. |
| \_\_u64 | `flags` | See [Frame Flags](#vp8-frame-flags) |

`Frame Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_VP8_FRAME_FLAG_KEY_FRAME` | 0x01 | Indicates if the frame is a key frame. |
| `V4L2_VP8_FRAME_FLAG_EXPERIMENTAL` | 0x02 | Experimental bitstream. |
| `V4L2_VP8_FRAME_FLAG_SHOW_FRAME` | 0x04 | Show frame flag, indicates if the frame is for display. |
| `V4L2_VP8_FRAME_FLAG_MB_NO_SKIP_COEFF` | 0x08 | Enable/disable skipping of macroblocks with no non-zero coefficients. |
| `V4L2_VP8_FRAME_FLAG_SIGN_BIAS_GOLDEN` | 0x10 | Sign of motion vectors when the golden frame is referenced. |
| `V4L2_VP8_FRAME_FLAG_SIGN_BIAS_ALT` | 0x20 | Sign of motion vectors when the alt frame is referenced. |

type v4l2\_vp8\_entropy\_coder\_state

struct v4l2\_vp8\_entropy\_coder\_state

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `range` | coder state value for “Range” |
| \_\_u8 | `value` | coder state value for “Value”- |
| \_\_u8 | `bit_count` | number of bits left. |
| \_\_u8 | `padding` | Applications and drivers must set this to zero. |

type v4l2\_vp8\_segment

struct v4l2\_vp8\_segment

|  |  |  |
| --- | --- | --- |
| \_\_s8 | `quant_update[4]` | Signed quantizer value update. |
| \_\_s8 | `lf_update[4]` | Signed loop filter level value update. |
| \_\_u8 | `segment_probs[3]` | Segment probabilities. |
| \_\_u8 | `padding` | Applications and drivers must set this to zero. |
| \_\_u32 | `flags` | See [Segment Flags](#vp8-segment-flags) |

`Segment Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_VP8_SEGMENT_FLAG_ENABLED` | 0x01 | Enable/disable segment-based adjustments. |
| `V4L2_VP8_SEGMENT_FLAG_UPDATE_MAP` | 0x02 | Indicates if the macroblock segmentation map is updated in this frame. |
| `V4L2_VP8_SEGMENT_FLAG_UPDATE_FEATURE_DATA` | 0x04 | Indicates if the segment feature data is updated in this frame. |
| `V4L2_VP8_SEGMENT_FLAG_DELTA_VALUE_MODE` | 0x08 | If is set, the segment feature data mode is delta-value. If cleared, it’s absolute-value. |

type v4l2\_vp8\_loop\_filter

struct v4l2\_vp8\_loop\_filter

|  |  |  |
| --- | --- | --- |
| \_\_s8 | `ref_frm_delta[4]` | Reference adjustment (signed) delta value. |
| \_\_s8 | `mb_mode_delta[4]` | Macroblock prediction mode adjustment (signed) delta value. |
| \_\_u8 | `sharpness_level` | Sharpness level |
| \_\_u8 | `level` | Filter level |
| \_\_u16 | `padding` | Applications and drivers must set this to zero. |
| \_\_u32 | `flags` | See [Loop Filter Flags](#vp8-loop-filter-flags) |

`Loop Filter Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_VP8_LF_ADJ_ENABLE` | 0x01 | Enable/disable macroblock-level loop filter adjustment. |
| `V4L2_VP8_LF_DELTA_UPDATE` | 0x02 | Indicates if the delta values used in an adjustment are updated. |
| `V4L2_VP8_LF_FILTER_TYPE_SIMPLE` | 0x04 | If set, indicates the filter type is simple. If cleared, the filter type is normal. |

type v4l2\_vp8\_quantization

struct v4l2\_vp8\_quantization

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `y_ac_qi` | Luma AC coefficient table index. |
| \_\_s8 | `y_dc_delta` | Luma DC delta value. |
| \_\_s8 | `y2_dc_delta` | Y2 block DC delta value. |
| \_\_s8 | `y2_ac_delta` | Y2 block AC delta value. |
| \_\_s8 | `uv_dc_delta` | Chroma DC delta value. |
| \_\_s8 | `uv_ac_delta` | Chroma AC delta value. |
| \_\_u16 | `padding` | Applications and drivers must set this to zero. |

type v4l2\_vp8\_entropy

struct v4l2\_vp8\_entropy

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `coeff_probs[4][8][3][11]` | Coefficient update probabilities. |
| \_\_u8 | `y_mode_probs[4]` | Luma mode update probabilities. |
| \_\_u8 | `uv_mode_probs[3]` | Chroma mode update probabilities. |
| \_\_u8 | `mv_probs[2][19]` | MV decoding update probabilities. |
| \_\_u8 | `padding[3]` | Applications and drivers must set this to zero. |

`V4L2_CID_STATELESS_MPEG2_SEQUENCE (struct)`
:   Specifies the sequence parameters (as extracted from the bitstream) for the
    associated MPEG-2 slice data. This includes fields matching the syntax
    elements from the sequence header and sequence extension parts of the
    bitstream as specified by [ISO 13818-2](biblio.html#mpeg2part2).

type v4l2\_ctrl\_mpeg2\_sequence

struct v4l2\_ctrl\_mpeg2\_sequence

|  |  |  |
| --- | --- | --- |
| \_\_u16 | `horizontal_size` | The width of the displayable part of the frame’s luminance component. |
| \_\_u16 | `vertical_size` | The height of the displayable part of the frame’s luminance component. |
| \_\_u32 | `vbv_buffer_size` | Used to calculate the required size of the video buffering verifier, defined (in bits) as: 16 \* 1024 \* vbv\_buffer\_size. |
| \_\_u16 | `profile_and_level_indication` | The current profile and level indication as extracted from the bitstream. |
| \_\_u8 | `chroma_format` | The chrominance sub-sampling format (1: 4:2:0, 2: 4:2:2, 3: 4:4:4). |
| \_\_u8 | `flags` | See [MPEG-2 Sequence Flags](#mpeg2-sequence-flags). |

`MPEG-2 Sequence Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_MPEG2_SEQ_FLAG_PROGRESSIVE` | 0x01 | Indication that all the frames for the sequence are progressive instead of interlaced. |

`V4L2_CID_STATELESS_MPEG2_PICTURE (struct)`
:   Specifies the picture parameters (as extracted from the bitstream) for the
    associated MPEG-2 slice data. This includes fields matching the syntax
    elements from the picture header and picture coding extension parts of the
    bitstream as specified by [ISO 13818-2](biblio.html#mpeg2part2).

type v4l2\_ctrl\_mpeg2\_picture

struct v4l2\_ctrl\_mpeg2\_picture

|  |  |  |
| --- | --- | --- |
| \_\_u64 | `backward_ref_ts` | Timestamp of the V4L2 capture buffer to use as backward reference, used with B-coded and P-coded frames. The timestamp refers to the `timestamp` field in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer"). Use the `v4l2_timeval_to_ns()` function to convert the struct `timeval` in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer") to a \_\_u64. |
| \_\_u64 | `forward_ref_ts` | Timestamp for the V4L2 capture buffer to use as forward reference, used with B-coded frames. The timestamp refers to the `timestamp` field in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer"). Use the `v4l2_timeval_to_ns()` function to convert the struct `timeval` in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer") to a \_\_u64. |
| \_\_u32 | `flags` | See [MPEG-2 Picture Flags](#mpeg2-picture-flags). |
| \_\_u8 | `f_code[2][2]` | Motion vector codes. |
| \_\_u8 | `picture_coding_type` | Picture coding type for the frame covered by the current slice (V4L2\_MPEG2\_PIC\_CODING\_TYPE\_I, V4L2\_MPEG2\_PIC\_CODING\_TYPE\_P or V4L2\_MPEG2\_PIC\_CODING\_TYPE\_B). |
| \_\_u8 | `picture_structure` | Picture structure (1: interlaced top field, 2: interlaced bottom field, 3: progressive frame). |
| \_\_u8 | `intra_dc_precision` | Precision of Discrete Cosine transform (0: 8 bits precision, 1: 9 bits precision, 2: 10 bits precision, 3: 11 bits precision). |
| \_\_u8 | `reserved[5]` | Applications and drivers must set this to zero. |

`MPEG-2 Picture Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_MPEG2_PIC_FLAG_TOP_FIELD_FIRST` | 0x00000001 | If set and it’s an interlaced stream, top field is output first. |
| `V4L2_MPEG2_PIC_FLAG_FRAME_PRED_DCT` | 0x00000002 | If set only frame-DCT and frame prediction are used. |
| `V4L2_MPEG2_PIC_FLAG_CONCEALMENT_MV` | 0x00000004 | If set motion vectors are coded for intra macroblocks. |
| `V4L2_MPEG2_PIC_FLAG_Q_SCALE_TYPE` | 0x00000008 | This flag affects the inverse quantization process. |
| `V4L2_MPEG2_PIC_FLAG_INTRA_VLC` | 0x00000010 | This flag affects the decoding of transform coefficient data. |
| `V4L2_MPEG2_PIC_FLAG_ALT_SCAN` | 0x00000020 | This flag affects the decoding of transform coefficient data. |
| `V4L2_MPEG2_PIC_FLAG_REPEAT_FIRST` | 0x00000040 | This flag affects the decoding process of progressive frames. |
| `V4L2_MPEG2_PIC_FLAG_PROGRESSIVE` | 0x00000080 | Indicates whether the current frame is progressive. |

`V4L2_CID_STATELESS_MPEG2_QUANTISATION (struct)`
:   Specifies quantisation matrices, in zigzag scanning order, for the
    associated MPEG-2 slice data. This control is initialized by the kernel
    to the matrices default values. If a bitstream transmits a user-defined
    quantisation matrices load, applications are expected to use this control.
    Applications are also expected to set the control loading the default
    values, if the quantisation matrices need to be reset, for instance on a
    sequence header. This process is specified by section 6.3.7.
    “Quant matrix extension” of the specification.

type v4l2\_ctrl\_mpeg2\_quantisation

struct v4l2\_ctrl\_mpeg2\_quantisation

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `intra_quantiser_matrix[64]` | The quantisation matrix coefficients for intra-coded frames, in zigzag scanning order. It is relevant for both luma and chroma components, although it can be superseded by the chroma-specific matrix for non-4:2:0 YUV formats. |
| \_\_u8 | `non_intra_quantiser_matrix[64]` | The quantisation matrix coefficients for non-intra-coded frames, in zigzag scanning order. It is relevant for both luma and chroma components, although it can be superseded by the chroma-specific matrix for non-4:2:0 YUV formats. |
| \_\_u8 | `chroma_intra_quantiser_matrix[64]` | The quantisation matrix coefficients for the chominance component of intra-coded frames, in zigzag scanning order. Only relevant for non-4:2:0 YUV formats. |
| \_\_u8 | `chroma_non_intra_quantiser_matrix[64]` | The quantisation matrix coefficients for the chrominance component of non-intra-coded frames, in zigzag scanning order. Only relevant for non-4:2:0 YUV formats. |

`V4L2_CID_STATELESS_VP9_COMPRESSED_HDR (struct)`
:   Stores VP9 probabilities updates as parsed from the current compressed frame
    header. A value of zero in an array element means no update of the relevant
    probability. Motion vector-related updates contain a new value or zero. All
    other updates contain values translated with inv\_map\_table[] (see 6.3.5 in
    [VP9](biblio.html#vp9)).

type v4l2\_ctrl\_vp9\_compressed\_hdr

struct v4l2\_ctrl\_vp9\_compressed\_hdr

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `tx_mode` | Specifies the TX mode. See [TX Mode](#vp9-tx-mode) for more details. |
| \_\_u8 | `tx8[2][1]` | TX 8x8 probabilities delta. |
| \_\_u8 | `tx16[2][2]` | TX 16x16 probabilities delta. |
| \_\_u8 | `tx32[2][3]` | TX 32x32 probabilities delta. |
| \_\_u8 | `coef[4][2][2][6][6][3]` | Coefficient probabilities delta. |
| \_\_u8 | `skip[3]` | Skip probabilities delta. |
| \_\_u8 | `inter_mode[7][3]` | Inter prediction mode probabilities delta. |
| \_\_u8 | `interp_filter[4][2]` | Interpolation filter probabilities delta. |
| \_\_u8 | `is_inter[4]` | Is inter-block probabilities delta. |
| \_\_u8 | `comp_mode[5]` | Compound prediction mode probabilities delta. |
| \_\_u8 | `single_ref[5][2]` | Single reference probabilities delta. |
| \_\_u8 | `comp_ref[5]` | Compound reference probabilities delta. |
| \_\_u8 | `y_mode[4][9]` | Y prediction mode probabilities delta. |
| \_\_u8 | `uv_mode[10][9]` | UV prediction mode probabilities delta. |
| \_\_u8 | `partition[16][3]` | Partition probabilities delta. |
| \_\_u8 | `mv.joint[3]` | Motion vector joint probabilities delta. |
| \_\_u8 | `mv.sign[2]` | Motion vector sign probabilities delta. |
| \_\_u8 | `mv.classes[2][10]` | Motion vector class probabilities delta. |
| \_\_u8 | `mv.class0_bit[2]` | Motion vector class0 bit probabilities delta. |
| \_\_u8 | `mv.bits[2][10]` | Motion vector bits probabilities delta. |
| \_\_u8 | `mv.class0_fr[2][2][3]` | Motion vector class0 fractional bit probabilities delta. |
| \_\_u8 | `mv.fr[2][3]` | Motion vector fractional bit probabilities delta. |
| \_\_u8 | `mv.class0_hp[2]` | Motion vector class0 high precision fractional bit probabilities delta. |
| \_\_u8 | `mv.hp[2]` | Motion vector high precision fractional bit probabilities delta. |

`TX Mode`

|  |  |  |
| --- | --- | --- |
| `V4L2_VP9_TX_MODE_ONLY_4X4` | 0 | Transform size is 4x4. |
| `V4L2_VP9_TX_MODE_ALLOW_8X8` | 1 | Transform size can be up to 8x8. |
| `V4L2_VP9_TX_MODE_ALLOW_16X16` | 2 | Transform size can be up to 16x16. |
| `V4L2_VP9_TX_MODE_ALLOW_32X32` | 3 | transform size can be up to 32x32. |
| `V4L2_VP9_TX_MODE_SELECT` | 4 | Bitstream contains the transform size for each block. |

See section ‘7.3.1 Tx mode semantics’ of the [VP9](biblio.html#vp9) specification for more details.

`V4L2_CID_STATELESS_VP9_FRAME (struct)`
:   Specifies the frame parameters for the associated VP9 frame decode request.
    This includes the necessary parameters for configuring a stateless hardware
    decoding pipeline for VP9. The bitstream parameters are defined according
    to [VP9](biblio.html#vp9).

type v4l2\_ctrl\_vp9\_frame

struct v4l2\_ctrl\_vp9\_frame

|  |  |  |
| --- | --- | --- |
| struct [`v4l2_vp9_loop_filter`](#c.V4L.v4l2_vp9_loop_filter "v4l2_vp9_loop_filter") | `lf` | Loop filter parameters. See struct [`v4l2_vp9_loop_filter`](#c.V4L.v4l2_vp9_loop_filter "v4l2_vp9_loop_filter") for more details. |
| struct [`v4l2_vp9_quantization`](#c.V4L.v4l2_vp9_quantization "v4l2_vp9_quantization") | `quant` | Quantization parameters. See [`v4l2_vp9_quantization`](#c.V4L.v4l2_vp9_quantization "v4l2_vp9_quantization") for more details. |
| struct [`v4l2_vp9_segmentation`](#c.V4L.v4l2_vp9_segmentation "v4l2_vp9_segmentation") | `seg` | Segmentation parameters. See [`v4l2_vp9_segmentation`](#c.V4L.v4l2_vp9_segmentation "v4l2_vp9_segmentation") for more details. |
| \_\_u32 | `flags` | Combination of V4L2\_VP9\_FRAME\_FLAG\_\* flags. See [Frame Flags](#vp9-frame-flags). |
| \_\_u16 | `compressed_header_size` | Compressed header size in bytes. |
| \_\_u16 | `uncompressed_header_size` | Uncompressed header size in bytes. |
| \_\_u16 | `frame_width_minus_1` | Add 1 to get the frame width expressed in pixels. See section 7.2.3 in [VP9](biblio.html#vp9). |
| \_\_u16 | `frame_height_minus_1` | Add 1 to get the frame height expressed in pixels. See section 7.2.3 in [VP9](biblio.html#vp9). |
| \_\_u16 | `render_width_minus_1` | Add 1 to get the expected render width expressed in pixels. This is not used during the decoding process but might be used by HW scalers to prepare a frame that’s ready for scanout. See section 7.2.4 in [VP9](biblio.html#vp9). |
| \_\_u16 | render\_height\_minus\_1 | Add 1 to get the expected render height expressed in pixels. This is not used during the decoding process but might be used by HW scalers to prepare a frame that’s ready for scanout. See section 7.2.4 in [VP9](biblio.html#vp9). |
| \_\_u64 | `last_frame_ts` | “last” reference buffer timestamp. The timestamp refers to the `timestamp` field in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer"). Use the `v4l2_timeval_to_ns()` function to convert the struct `timeval` in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer") to a \_\_u64. |
| \_\_u64 | `golden_frame_ts` | “golden” reference buffer timestamp. The timestamp refers to the `timestamp` field in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer"). Use the `v4l2_timeval_to_ns()` function to convert the struct `timeval` in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer") to a \_\_u64. |
| \_\_u64 | `alt_frame_ts` | “alt” reference buffer timestamp. The timestamp refers to the `timestamp` field in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer"). Use the `v4l2_timeval_to_ns()` function to convert the struct `timeval` in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer") to a \_\_u64. |
| \_\_u8 | `ref_frame_sign_bias` | a bitfield specifying whether the sign bias is set for a given reference frame. See [Reference Frame Sign Bias](#vp9-ref-frame-sign-bias) for more details. |
| \_\_u8 | `reset_frame_context` | specifies whether the frame context should be reset to default values. See [Reset Frame Context](#vp9-reset-frame-context) for more details. |
| \_\_u8 | `frame_context_idx` | Frame context that should be used/updated. |
| \_\_u8 | `profile` | VP9 profile. Can be 0, 1, 2 or 3. |
| \_\_u8 | `bit_depth` | Component depth in bits. Can be 8, 10 or 12. Note that not all profiles support 10 and/or 12 bits depths. |
| \_\_u8 | `interpolation_filter` | Specifies the filter selection used for performing inter prediction. See [Interpolation Filter](#vp9-interpolation-filter) for more details. |
| \_\_u8 | `tile_cols_log2` | Specifies the base 2 logarithm of the width of each tile (where the width is measured in units of 8x8 blocks). Shall be less than or equal to 6. |
| \_\_u8 | `tile_rows_log2` | Specifies the base 2 logarithm of the height of each tile (where the height is measured in units of 8x8 blocks). |
| \_\_u8 | `reference_mode` | Specifies the type of inter prediction to be used. See [Reference Mode](#vp9-reference-mode) for more details. Note that this is derived as part of the compressed header parsing process and for this reason should have been part of :c:type: v4l2\_ctrl\_vp9\_compressed\_hdr optional control. It is safe to set this value to zero if the driver does not require compressed headers. |
| \_\_u8 | `reserved[7]` | Applications and drivers must set this to zero. |

`Frame Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_VP9_FRAME_FLAG_KEY_FRAME` | 0x001 | The frame is a key frame. |
| `V4L2_VP9_FRAME_FLAG_SHOW_FRAME` | 0x002 | The frame should be displayed. |
| `V4L2_VP9_FRAME_FLAG_ERROR_RESILIENT` | 0x004 | The decoding should be error resilient. |
| `V4L2_VP9_FRAME_FLAG_INTRA_ONLY` | 0x008 | The frame does not reference other frames. |
| `V4L2_VP9_FRAME_FLAG_ALLOW_HIGH_PREC_MV` | 0x010 | The frame can use high precision motion vectors. |
| `V4L2_VP9_FRAME_FLAG_REFRESH_FRAME_CTX` | 0x020 | Frame context should be updated after decoding. |
| `V4L2_VP9_FRAME_FLAG_PARALLEL_DEC_MODE` | 0x040 | Parallel decoding is used. |
| `V4L2_VP9_FRAME_FLAG_X_SUBSAMPLING` | 0x080 | Vertical subsampling is enabled. |
| `V4L2_VP9_FRAME_FLAG_Y_SUBSAMPLING` | 0x100 | Horizontal subsampling is enabled. |
| `V4L2_VP9_FRAME_FLAG_COLOR_RANGE_FULL_SWING` | 0x200 | The full UV range is used. |

`Reference Frame Sign Bias`

|  |  |  |
| --- | --- | --- |
| `V4L2_VP9_SIGN_BIAS_LAST` | 0x1 | Sign bias is set for the last reference frame. |
| `V4L2_VP9_SIGN_BIAS_GOLDEN` | 0x2 | Sign bias is set for the golden reference frame. |
| `V4L2_VP9_SIGN_BIAS_ALT` | 0x2 | Sign bias is set for the alt reference frame. |

`Reset Frame Context`

|  |  |  |
| --- | --- | --- |
| `V4L2_VP9_RESET_FRAME_CTX_NONE` | 0 | Do not reset any frame context. |
| `V4L2_VP9_RESET_FRAME_CTX_SPEC` | 1 | Reset the frame context pointed to by [`v4l2_ctrl_vp9_frame`](#c.V4L.v4l2_ctrl_vp9_frame "v4l2_ctrl_vp9_frame").frame\_context\_idx. |
| `V4L2_VP9_RESET_FRAME_CTX_ALL` | 2 | Reset all frame contexts. |

See section ‘7.2 Uncompressed header semantics’ of the [VP9](biblio.html#vp9) specification
for more details.

`Interpolation Filter`

|  |  |  |
| --- | --- | --- |
| `V4L2_VP9_INTERP_FILTER_EIGHTTAP` | 0 | Eight tap filter. |
| `V4L2_VP9_INTERP_FILTER_EIGHTTAP_SMOOTH` | 1 | Eight tap smooth filter. |
| `V4L2_VP9_INTERP_FILTER_EIGHTTAP_SHARP` | 2 | Eeight tap sharp filter. |
| `V4L2_VP9_INTERP_FILTER_BILINEAR` | 3 | Bilinear filter. |
| `V4L2_VP9_INTERP_FILTER_SWITCHABLE` | 4 | Filter selection is signaled at the block level. |

See section ‘7.2.7 Interpolation filter semantics’ of the [VP9](biblio.html#vp9) specification
for more details.

`Reference Mode`

|  |  |  |
| --- | --- | --- |
| `V4L2_VP9_REFERENCE_MODE_SINGLE_REFERENCE` | 0 | Indicates that all the inter blocks use only a single reference frame to generate motion compensated prediction. |
| `V4L2_VP9_REFERENCE_MODE_COMPOUND_REFERENCE` | 1 | Requires all the inter blocks to use compound mode. Single reference frame prediction is not allowed. |
| `V4L2_VP9_REFERENCE_MODE_SELECT` | 2 | Allows each individual inter block to select between single and compound prediction modes. |

See section ‘7.3.6 Frame reference mode semantics’ of the [VP9](biblio.html#vp9) specification for more details.

type v4l2\_vp9\_segmentation

Encodes the quantization parameters. See section ‘7.2.10 Segmentation
params syntax’ of the [VP9](biblio.html#vp9) specification for more details.

struct v4l2\_vp9\_segmentation

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `feature_data[8][4]` | Data attached to each feature. Data entry is only valid if the feature is enabled. The array shall be indexed with segment number as the first dimension (0..7) and one of V4L2\_VP9\_SEG\_\* as the second dimension. See [Segment Feature IDs](#vp9-segment-feature). |
| \_\_u8 | `feature_enabled[8]` | Bitmask defining which features are enabled in each segment. The value for each segment is a combination of V4L2\_VP9\_SEGMENT\_FEATURE\_ENABLED(id) values where id is one of V4L2\_VP9\_SEG\_\*. See [Segment Feature IDs](#vp9-segment-feature). |
| \_\_u8 | `tree_probs[7]` | Specifies the probability values to be used when decoding a Segment-ID. See ‘5.15 Segmentation map’ section of [VP9](biblio.html#vp9) for more details. |
| \_\_u8 | `pred_probs[3]` | Specifies the probability values to be used when decoding a Predicted-Segment-ID. See ‘6.4.14 Get segment id syntax’ section of [VP9](biblio.html#vp9) for more details. |
| \_\_u8 | `flags` | Combination of V4L2\_VP9\_SEGMENTATION\_FLAG\_\* flags. See [Segmentation Flags](#vp9-segmentation-flags). |
| \_\_u8 | `reserved[5]` | Applications and drivers must set this to zero. |

`Segment feature IDs`

|  |  |  |
| --- | --- | --- |
| `V4L2_VP9_SEG_LVL_ALT_Q` | 0 | Quantizer segment feature. |
| `V4L2_VP9_SEG_LVL_ALT_L` | 1 | Loop filter segment feature. |
| `V4L2_VP9_SEG_LVL_REF_FRAME` | 2 | Reference frame segment feature. |
| `V4L2_VP9_SEG_LVL_SKIP` | 3 | Skip segment feature. |
| `V4L2_VP9_SEG_LVL_MAX` | 4 | Number of segment features. |

`Segmentation Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_VP9_SEGMENTATION_FLAG_ENABLED` | 0x01 | Indicates that this frame makes use of the segmentation tool. |
| `V4L2_VP9_SEGMENTATION_FLAG_UPDATE_MAP` | 0x02 | Indicates that the segmentation map should be updated during the decoding of this frame. |
| `V4L2_VP9_SEGMENTATION_FLAG_TEMPORAL_UPDATE` | 0x04 | Indicates that the updates to the segmentation map are coded relative to the existing segmentation map. |
| `V4L2_VP9_SEGMENTATION_FLAG_UPDATE_DATA` | 0x08 | Indicates that new parameters are about to be specified for each segment. |
| `V4L2_VP9_SEGMENTATION_FLAG_ABS_OR_DELTA_UPDATE` | 0x10 | Indicates that the segmentation parameters represent the actual values to be used. |

type v4l2\_vp9\_quantization

Encodes the quantization parameters. See section ‘7.2.9 Quantization params
syntax’ of the VP9 specification for more details.

struct v4l2\_vp9\_quantization

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `base_q_idx` | Indicates the base frame qindex. |
| \_\_s8 | `delta_q_y_dc` | Indicates the Y DC quantizer relative to base\_q\_idx. |
| \_\_s8 | `delta_q_uv_dc` | Indicates the UV DC quantizer relative to base\_q\_idx. |
| \_\_s8 | `delta_q_uv_ac` | Indicates the UV AC quantizer relative to base\_q\_idx. |
| \_\_u8 | `reserved[4]` | Applications and drivers must set this to zero. |

type v4l2\_vp9\_loop\_filter

This structure contains all loop filter related parameters. See sections
‘7.2.8 Loop filter semantics’ of the [VP9](biblio.html#vp9) specification for more details.

struct v4l2\_vp9\_loop\_filter

|  |  |  |
| --- | --- | --- |
| \_\_s8 | `ref_deltas[4]` | Contains the adjustment needed for the filter level based on the chosen reference frame. |
| \_\_s8 | `mode_deltas[2]` | Contains the adjustment needed for the filter level based on the chosen mode. |
| \_\_u8 | `level` | Indicates the loop filter strength. |
| \_\_u8 | `sharpness` | Indicates the sharpness level. |
| \_\_u8 | `flags` | Combination of V4L2\_VP9\_LOOP\_FILTER\_FLAG\_\* flags. See [Loop Filter Flags](#vp9-loop-filter-flags). |
| \_\_u8 | `reserved[7]` | Applications and drivers must set this to zero. |

`Loop Filter Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_VP9_LOOP_FILTER_FLAG_DELTA_ENABLED` | 0x1 | When set, the filter level depends on the mode and reference frame used to predict a block. |
| `V4L2_VP9_LOOP_FILTER_FLAG_DELTA_UPDATE` | 0x2 | When set, the bitstream contains additional syntax elements that specify which mode and reference frame deltas are to be updated. |

`V4L2_CID_STATELESS_HEVC_SPS (struct)`
:   Specifies the Sequence Parameter Set fields (as extracted from the
    bitstream) for the associated HEVC slice data.
    These bitstream parameters are defined according to [ITU H.265/HEVC](biblio.html#hevc).
    They are described in section 7.4.3.2 “Sequence parameter set RBSP
    semantics” of the specification.

type v4l2\_ctrl\_hevc\_sps

struct v4l2\_ctrl\_hevc\_sps

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `video_parameter_set_id` | Specifies the value of the vps\_video\_parameter\_set\_id of the active VPS as described in section “7.4.3.2.1 General sequence parameter set RBSP semantics” of H.265 specifications. |
| \_\_u8 | `seq_parameter_set_id` | Provides an identifier for the SPS for reference by other syntax elements as described in section “7.4.3.2.1 General sequence parameter set RBSP semantics” of H.265 specifications. |
| \_\_u16 | `pic_width_in_luma_samples` | Specifies the width of each decoded picture in units of luma samples. |
| \_\_u16 | `pic_height_in_luma_samples` | Specifies the height of each decoded picture in units of luma samples. |
| \_\_u8 | `bit_depth_luma_minus8` | This value plus 8 specifies the bit depth of the samples of the luma array. |
| \_\_u8 | `bit_depth_chroma_minus8` | This value plus 8 specifies the bit depth of the samples of the chroma arrays. |
| \_\_u8 | `log2_max_pic_order_cnt_lsb_minus4` | Specifies the value of the variable MaxPicOrderCntLsb. |
| \_\_u8 | `sps_max_dec_pic_buffering_minus1` | This value plus 1 specifies the maximum required size of the decoded picture buffer for the coded video sequence (CVS). |
| \_\_u8 | `sps_max_num_reorder_pics` | Indicates the maximum allowed number of pictures. |
| \_\_u8 | `sps_max_latency_increase_plus1` | Used to signal MaxLatencyPictures, which indicates the maximum number of pictures that can precede any picture in output order and follow that picture in decoding order. |
| \_\_u8 | `log2_min_luma_coding_block_size_minus3` | This value plus 3 specifies the minimum luma coding block size. |
| \_\_u8 | `log2_diff_max_min_luma_coding_block_size` | Specifies the difference between the maximum and minimum luma coding block size. |
| \_\_u8 | `log2_min_luma_transform_block_size_minus2` | This value plus 2 specifies the minimum luma transform block size. |
| \_\_u8 | `log2_diff_max_min_luma_transform_block_size` | Specifies the difference between the maximum and minimum luma transform block size. |
| \_\_u8 | `max_transform_hierarchy_depth_inter` | Specifies the maximum hierarchy depth for transform units of coding units coded in inter prediction mode. |
| \_\_u8 | `max_transform_hierarchy_depth_intra` | Specifies the maximum hierarchy depth for transform units of coding units coded in intra prediction mode. |
| \_\_u8 | `pcm_sample_bit_depth_luma_minus1` | This value plus 1 specifies the number of bits used to represent each of PCM sample values of the luma component. |
| \_\_u8 | `pcm_sample_bit_depth_chroma_minus1` | Specifies the number of bits used to represent each of PCM sample values of the chroma components. |
| \_\_u8 | `log2_min_pcm_luma_coding_block_size_minus3` | Plus 3 specifies the minimum size of coding blocks. |
| \_\_u8 | `log2_diff_max_min_pcm_luma_coding_block_size` | Specifies the difference between the maximum and minimum size of coding blocks. |
| \_\_u8 | `num_short_term_ref_pic_sets` | Specifies the number of `st_ref_pic_set()` syntax structures included in the SPS. |
| \_\_u8 | `num_long_term_ref_pics_sps` | Specifies the number of candidate long-term reference pictures that are specified in the SPS. |
| \_\_u8 | `chroma_format_idc` | Specifies the chroma sampling. |
| \_\_u8 | `sps_max_sub_layers_minus1` | This value plus 1 specifies the maximum number of temporal sub-layers. |
| \_\_u64 | `flags` | See [Sequence Parameter Set Flags](#hevc-sps-flags) |

`Sequence Parameter Set Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_HEVC_SPS_FLAG_SEPARATE_COLOUR_PLANE` | 0x00000001 |  |
| `V4L2_HEVC_SPS_FLAG_SCALING_LIST_ENABLED` | 0x00000002 |  |
| `V4L2_HEVC_SPS_FLAG_AMP_ENABLED` | 0x00000004 |  |
| `V4L2_HEVC_SPS_FLAG_SAMPLE_ADAPTIVE_OFFSET` | 0x00000008 |  |
| `V4L2_HEVC_SPS_FLAG_PCM_ENABLED` | 0x00000010 |  |
| `V4L2_HEVC_SPS_FLAG_PCM_LOOP_FILTER_DISABLED` | 0x00000020 |  |
| `V4L2_HEVC_SPS_FLAG_LONG_TERM_REF_PICS_PRESENT` | 0x00000040 |  |
| `V4L2_HEVC_SPS_FLAG_SPS_TEMPORAL_MVP_ENABLED` | 0x00000080 |  |
| `V4L2_HEVC_SPS_FLAG_STRONG_INTRA_SMOOTHING_ENABLED` | 0x00000100 |  |

`V4L2_CID_STATELESS_HEVC_PPS (struct)`
:   Specifies the Picture Parameter Set fields (as extracted from the
    bitstream) for the associated HEVC slice data.
    These bitstream parameters are defined according to [ITU H.265/HEVC](biblio.html#hevc).
    They are described in section 7.4.3.3 “Picture parameter set RBSP
    semantics” of the specification.

type v4l2\_ctrl\_hevc\_pps

struct v4l2\_ctrl\_hevc\_pps

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `pic_parameter_set_id` | Identifies the PPS for reference by other syntax elements. |
| \_\_u8 | `num_extra_slice_header_bits` | Specifies the number of extra slice header bits that are present in the slice header RBSP for coded pictures referring to the PPS. |
| \_\_u8 | `num_ref_idx_l0_default_active_minus1` | This value plus 1 specifies the inferred value of num\_ref\_idx\_l0\_active\_minus1. |
| \_\_u8 | `num_ref_idx_l1_default_active_minus1` | This value plus 1 specifies the inferred value of num\_ref\_idx\_l1\_active\_minus1. |
| \_\_s8 | `init_qp_minus26` | This value plus 26 specifies the initial value of SliceQp Y for each slice referring to the PPS. |
| \_\_u8 | `diff_cu_qp_delta_depth` | Specifies the difference between the luma coding tree block size and the minimum luma coding block size of coding units that convey cu\_qp\_delta\_abs and cu\_qp\_delta\_sign\_flag. |
| \_\_s8 | `pps_cb_qp_offset` | Specifies the offsets to the luma quantization parameter Cb. |
| \_\_s8 | `pps_cr_qp_offset` | Specifies the offsets to the luma quantization parameter Cr. |
| \_\_u8 | `num_tile_columns_minus1` | This value plus 1 specifies the number of tile columns partitioning the picture. |
| \_\_u8 | `num_tile_rows_minus1` | This value plus 1 specifies the number of tile rows partitioning the picture. |
| \_\_u8 | `column_width_minus1[20]` | This value plus 1 specifies the width of the i-th tile column in units of coding tree blocks. |
| \_\_u8 | `row_height_minus1[22]` | This value plus 1 specifies the height of the i-th tile row in units of coding tree blocks. |
| \_\_s8 | `pps_beta_offset_div2` | Specifies the default deblocking parameter offsets for beta divided by 2. |
| \_\_s8 | `pps_tc_offset_div2` | Specifies the default deblocking parameter offsets for tC divided by 2. |
| \_\_u8 | `log2_parallel_merge_level_minus2` | This value plus 2 specifies the value of the variable Log2ParMrgLevel. |
| \_\_u8 | `padding[4]` | Applications and drivers must set this to zero. |
| \_\_u64 | `flags` | See [Picture Parameter Set Flags](#hevc-pps-flags) |

`Picture Parameter Set Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_HEVC_PPS_FLAG_DEPENDENT_SLICE_SEGMENT_ENABLED` | 0x00000001 |  |
| `V4L2_HEVC_PPS_FLAG_OUTPUT_FLAG_PRESENT` | 0x00000002 |  |
| `V4L2_HEVC_PPS_FLAG_SIGN_DATA_HIDING_ENABLED` | 0x00000004 |  |
| `V4L2_HEVC_PPS_FLAG_CABAC_INIT_PRESENT` | 0x00000008 |  |
| `V4L2_HEVC_PPS_FLAG_CONSTRAINED_INTRA_PRED` | 0x00000010 |  |
| `V4L2_HEVC_PPS_FLAG_TRANSFORM_SKIP_ENABLED` | 0x00000020 |  |
| `V4L2_HEVC_PPS_FLAG_CU_QP_DELTA_ENABLED` | 0x00000040 |  |
| `V4L2_HEVC_PPS_FLAG_PPS_SLICE_CHROMA_QP_OFFSETS_PRESENT` | 0x00000080 |  |
| `V4L2_HEVC_PPS_FLAG_WEIGHTED_PRED` | 0x00000100 |  |
| `V4L2_HEVC_PPS_FLAG_WEIGHTED_BIPRED` | 0x00000200 |  |
| `V4L2_HEVC_PPS_FLAG_TRANSQUANT_BYPASS_ENABLED` | 0x00000400 |  |
| `V4L2_HEVC_PPS_FLAG_TILES_ENABLED` | 0x00000800 |  |
| `V4L2_HEVC_PPS_FLAG_ENTROPY_CODING_SYNC_ENABLED` | 0x00001000 |  |
| `V4L2_HEVC_PPS_FLAG_LOOP_FILTER_ACROSS_TILES_ENABLED` | 0x00002000 |  |
| `V4L2_HEVC_PPS_FLAG_PPS_LOOP_FILTER_ACROSS_SLICES_ENABLED` | 0x00004000 |  |
| `V4L2_HEVC_PPS_FLAG_DEBLOCKING_FILTER_OVERRIDE_ENABLED` | 0x00008000 |  |
| `V4L2_HEVC_PPS_FLAG_PPS_DISABLE_DEBLOCKING_FILTER` | 0x00010000 |  |
| `V4L2_HEVC_PPS_FLAG_LISTS_MODIFICATION_PRESENT` | 0x00020000 |  |
| `V4L2_HEVC_PPS_FLAG_SLICE_SEGMENT_HEADER_EXTENSION_PRESENT` | 0x00040000 |  |
| `V4L2_HEVC_PPS_FLAG_DEBLOCKING_FILTER_CONTROL_PRESENT` | 0x00080000 | Specifies the presence of deblocking filter control syntax elements in the PPS |
| `V4L2_HEVC_PPS_FLAG_UNIFORM_SPACING` | 0x00100000 | Specifies that tile column boundaries and likewise tile row boundaries are distributed uniformly across the picture |

`V4L2_CID_STATELESS_HEVC_SLICE_PARAMS (struct)`
:   Specifies various slice-specific parameters, especially from the NAL unit
    header, general slice segment header and weighted prediction parameter
    parts of the bitstream.
    These bitstream parameters are defined according to [ITU H.265/HEVC](biblio.html#hevc).
    They are described in section 7.4.7 “General slice segment header
    semantics” of the specification.
    This control is a dynamically sized 1-dimensional array,
    V4L2\_CTRL\_FLAG\_DYNAMIC\_ARRAY flag must be set when using it.

type v4l2\_ctrl\_hevc\_slice\_params

struct v4l2\_ctrl\_hevc\_slice\_params

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `bit_size` | Size (in bits) of the current slice data. |
| \_\_u32 | `data_byte_offset` | Offset (in byte) to the video data in the current slice data. |
| \_\_u32 | `num_entry_point_offsets` | Specifies the number of entry point offset syntax elements in the slice header. When the driver supports it, the `V4L2_CID_STATELESS_HEVC_ENTRY_POINT_OFFSETS` must be set. |
| \_\_u8 | `nal_unit_type` | Specifies the coding type of the slice (B, P or I). |
| \_\_u8 | `nuh_temporal_id_plus1` | Minus 1 specifies a temporal identifier for the NAL unit. |
| \_\_u8 | `slice_type` | (V4L2\_HEVC\_SLICE\_TYPE\_I, V4L2\_HEVC\_SLICE\_TYPE\_P or V4L2\_HEVC\_SLICE\_TYPE\_B). |
| \_\_u8 | `colour_plane_id` | Specifies the colour plane associated with the current slice. |
| \_\_s32 | `slice_pic_order_cnt` | Specifies the picture order count. |
| \_\_u8 | `num_ref_idx_l0_active_minus1` | This value plus 1 specifies the maximum reference index for reference picture list 0 that may be used to decode the slice. |
| \_\_u8 | `num_ref_idx_l1_active_minus1` | This value plus 1 specifies the maximum reference index for reference picture list 1 that may be used to decode the slice. |
| \_\_u8 | `collocated_ref_idx` | Specifies the reference index of the collocated picture used for temporal motion vector prediction. |
| \_\_u8 | `five_minus_max_num_merge_cand` | Specifies the maximum number of merging motion vector prediction candidates supported in the slice subtracted from 5. |
| \_\_s8 | `slice_qp_delta` | Specifies the initial value of QpY to be used for the coding blocks in the slice. |
| \_\_s8 | `slice_cb_qp_offset` | Specifies a difference to be added to the value of pps\_cb\_qp\_offset. |
| \_\_s8 | `slice_cr_qp_offset` | Specifies a difference to be added to the value of pps\_cr\_qp\_offset. |
| \_\_s8 | `slice_act_y_qp_offset` | Specifies the offset to the luma of quantization parameter qP derived in section 8.6.2 |
| \_\_s8 | `slice_act_cb_qp_offset` | Specifies the offset to the cb of quantization parameter qP derived in section 8.6.2 |
| \_\_s8 | `slice_act_cr_qp_offset` | Specifies the offset to the cr of quantization parameter qP derived in section 8.6.2 |
| \_\_s8 | `slice_beta_offset_div2` | Specifies the deblocking parameter offsets for beta divided by 2. |
| \_\_s8 | `slice_tc_offset_div2` | Specifies the deblocking parameter offsets for tC divided by 2. |
| \_\_u8 | `pic_struct` | Indicates whether a picture should be displayed as a frame or as one or more fields. |
| \_\_u32 | `slice_segment_addr` | Specifies the address of the first coding tree block in the slice segment. |
| \_\_u8 | `ref_idx_l0[V4L2_HEVC_DPB_ENTRIES_NUM_MAX]` | The list of L0 reference elements as indices in the DPB. |
| \_\_u8 | `ref_idx_l1[V4L2_HEVC_DPB_ENTRIES_NUM_MAX]` | The list of L1 reference elements as indices in the DPB. |
| \_\_u16 | `short_term_ref_pic_set_size` | Specifies the size, in bits, of the short-term reference picture set, described as `st_ref_pic_set()` in the specification, included in the slice header or SPS (section 7.3.6.1). |
| \_\_u16 | `long_term_ref_pic_set_size` | Specifies the size, in bits, of the long-term reference picture set include in the slice header or SPS. It is the number of bits in the conditional block if(long\_term\_ref\_pics\_present\_flag) in section 7.3.6.1 of the specification. |
| \_\_u8 | `padding` | Applications and drivers must set this to zero. |
| struct [`v4l2_hevc_pred_weight_table`](#c.V4L.v4l2_hevc_pred_weight_table "v4l2_hevc_pred_weight_table") | `pred_weight_table` | The prediction weight coefficients for inter-picture prediction. |
| \_\_u64 | `flags` | See [Slice Parameters Flags](#hevc-slice-params-flags) |

`Slice Parameters Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_HEVC_SLICE_PARAMS_FLAG_SLICE_SAO_LUMA` | 0x00000001 |  |
| `V4L2_HEVC_SLICE_PARAMS_FLAG_SLICE_SAO_CHROMA` | 0x00000002 |  |
| `V4L2_HEVC_SLICE_PARAMS_FLAG_SLICE_TEMPORAL_MVP_ENABLED` | 0x00000004 |  |
| `V4L2_HEVC_SLICE_PARAMS_FLAG_MVD_L1_ZERO` | 0x00000008 |  |
| `V4L2_HEVC_SLICE_PARAMS_FLAG_CABAC_INIT` | 0x00000010 |  |
| `V4L2_HEVC_SLICE_PARAMS_FLAG_COLLOCATED_FROM_L0` | 0x00000020 |  |
| `V4L2_HEVC_SLICE_PARAMS_FLAG_USE_INTEGER_MV` | 0x00000040 |  |
| `V4L2_HEVC_SLICE_PARAMS_FLAG_SLICE_DEBLOCKING_FILTER_DISABLED` | 0x00000080 |  |
| `V4L2_HEVC_SLICE_PARAMS_FLAG_SLICE_LOOP_FILTER_ACROSS_SLICES_ENABLED` | 0x00000100 |  |
| `V4L2_HEVC_SLICE_PARAMS_FLAG_DEPENDENT_SLICE_SEGMENT` | 0x00000200 |  |

`V4L2_CID_STATELESS_HEVC_ENTRY_POINT_OFFSETS (integer)`
:   Specifies entry point offsets in bytes.
    This control is a dynamically sized array. The number of entry point
    offsets is reported by the `elems` field.
    This bitstream parameter is defined according to [ITU H.265/HEVC](biblio.html#hevc).
    They are described in section 7.4.7.1 “General slice segment header
    semantics” of the specification.
    When multiple slices are submitted in a request, the length of
    this array must be the sum of num\_entry\_point\_offsets of all the
    slices in the request.

`V4L2_CID_STATELESS_HEVC_SCALING_MATRIX (struct)`
:   Specifies the HEVC scaling matrix parameters used for the scaling process
    for transform coefficients.
    These matrix and parameters are defined according to [ITU H.265/HEVC](biblio.html#hevc).
    They are described in section 7.4.5 “Scaling list data semantics” of
    the specification.

type v4l2\_ctrl\_hevc\_scaling\_matrix

struct v4l2\_ctrl\_hevc\_scaling\_matrix

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `scaling_list_4x4[6][16]` | Scaling list is used for the scaling process for transform coefficients. The values on each scaling list are expected in raster scan order. |
| \_\_u8 | `scaling_list_8x8[6][64]` | Scaling list is used for the scaling process for transform coefficients. The values on each scaling list are expected in raster scan order. |
| \_\_u8 | `scaling_list_16x16[6][64]` | Scaling list is used for the scaling process for transform coefficients. The values on each scaling list are expected in raster scan order. |
| \_\_u8 | `scaling_list_32x32[2][64]` | Scaling list is used for the scaling process for transform coefficients. The values on each scaling list are expected in raster scan order. |
| \_\_u8 | `scaling_list_dc_coef_16x16[6]` | Scaling list is used for the scaling process for transform coefficients. The values on each scaling list are expected in raster scan order. |
| \_\_u8 | `scaling_list_dc_coef_32x32[2]` | Scaling list is used for the scaling process for transform coefficients. The values on each scaling list are expected in raster scan order. |

type v4l2\_hevc\_dpb\_entry

struct v4l2\_hevc\_dpb\_entry

|  |  |  |
| --- | --- | --- |
| \_\_u64 | `timestamp` | Timestamp of the V4L2 capture buffer to use as reference, used with B-coded and P-coded frames. The timestamp refers to the `timestamp` field in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer"). Use the `v4l2_timeval_to_ns()` function to convert the struct `timeval` in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer") to a \_\_u64. |
| \_\_u8 | `flags` | Long term flag for the reference frame (V4L2\_HEVC\_DPB\_ENTRY\_LONG\_TERM\_REFERENCE). The flag is set as described in the ITU HEVC specification chapter “8.3.2 Decoding process for reference picture set”. |
| \_\_u8 | `field_pic` | Whether the reference is a field picture or a frame. See [HEVC dpb field pic Flags](#hevc-dpb-field-pic-flags) |
| \_\_s32 | `pic_order_cnt_val` | The picture order count of the current picture. |
| \_\_u8 | `padding[2]` | Applications and drivers must set this to zero. |

`HEVC dpb field pic Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_HEVC_SEI_PIC_STRUCT_FRAME` | 0 | (progressive) Frame |
| `V4L2_HEVC_SEI_PIC_STRUCT_TOP_FIELD` | 1 | Top field |
| `V4L2_HEVC_SEI_PIC_STRUCT_BOTTOM_FIELD` | 2 | Bottom field |
| `V4L2_HEVC_SEI_PIC_STRUCT_TOP_BOTTOM` | 3 | Top field, bottom field, in that order |
| `V4L2_HEVC_SEI_PIC_STRUCT_BOTTOM_TOP` | 4 | Bottom field, top field, in that order |
| `V4L2_HEVC_SEI_PIC_STRUCT_TOP_BOTTOM_TOP` | 5 | Top field, bottom field, top field repeated, in that order |
| `V4L2_HEVC_SEI_PIC_STRUCT_BOTTOM_TOP_BOTTOM` | 6 | Bottom field, top field, bottom field repeated, in that order |
| `V4L2_HEVC_SEI_PIC_STRUCT_FRAME_DOUBLING` | 7 | Frame doubling |
| `V4L2_HEVC_SEI_PIC_STRUCT_FRAME_TRIPLING` | 8 | Frame tripling |
| `V4L2_HEVC_SEI_PIC_STRUCT_TOP_PAIRED_PREVIOUS_BOTTOM` | 9 | Top field paired with previous bottom field in output order |
| `V4L2_HEVC_SEI_PIC_STRUCT_BOTTOM_PAIRED_PREVIOUS_TOP` | 10 | Bottom field paired with previous top field in output order |
| `V4L2_HEVC_SEI_PIC_STRUCT_TOP_PAIRED_NEXT_BOTTOM` | 11 | Top field paired with next bottom field in output order |
| `V4L2_HEVC_SEI_PIC_STRUCT_BOTTOM_PAIRED_NEXT_TOP` | 12 | Bottom field paired with next top field in output order |

type v4l2\_hevc\_pred\_weight\_table

struct v4l2\_hevc\_pred\_weight\_table

|  |  |  |
| --- | --- | --- |
| \_\_s8 | `delta_luma_weight_l0[V4L2_HEVC_DPB_ENTRIES_NUM_MAX]` | The difference of the weighting factor applied to the luma prediction value for list 0. |
| \_\_s8 | `luma_offset_l0[V4L2_HEVC_DPB_ENTRIES_NUM_MAX]` | The additive offset applied to the luma prediction value for list 0. |
| \_\_s8 | `delta_chroma_weight_l0[V4L2_HEVC_DPB_ENTRIES_NUM_MAX][2]` | The difference of the weighting factor applied to the chroma prediction value for list 0. |
| \_\_s8 | `chroma_offset_l0[V4L2_HEVC_DPB_ENTRIES_NUM_MAX][2]` | The difference of the additive offset applied to the chroma prediction values for list 0. |
| \_\_s8 | `delta_luma_weight_l1[V4L2_HEVC_DPB_ENTRIES_NUM_MAX]` | The difference of the weighting factor applied to the luma prediction value for list 1. |
| \_\_s8 | `luma_offset_l1[V4L2_HEVC_DPB_ENTRIES_NUM_MAX]` | The additive offset applied to the luma prediction value for list 1. |
| \_\_s8 | `delta_chroma_weight_l1[V4L2_HEVC_DPB_ENTRIES_NUM_MAX][2]` | The difference of the weighting factor applied to the chroma prediction value for list 1. |
| \_\_s8 | `chroma_offset_l1[V4L2_HEVC_DPB_ENTRIES_NUM_MAX][2]` | The difference of the additive offset applied to the chroma prediction values for list 1. |
| \_\_u8 | `luma_log2_weight_denom` | The base 2 logarithm of the denominator for all luma weighting factors. |
| \_\_s8 | `delta_chroma_log2_weight_denom` | The difference of the base 2 logarithm of the denominator for all chroma weighting factors. |
| \_\_u8 | `padding[6]` | Applications and drivers must set this to zero. |

`V4L2_CID_STATELESS_HEVC_DECODE_MODE (enum)`
:   Specifies the decoding mode to use. Currently exposes slice-based and
    frame-based decoding but new modes might be added later on.
    This control is used as a modifier for V4L2\_PIX\_FMT\_HEVC\_SLICE
    pixel format. Applications that support V4L2\_PIX\_FMT\_HEVC\_SLICE
    are required to set this control in order to specify the decoding mode
    that is expected for the buffer.
    Drivers may expose a single or multiple decoding modes, depending
    on what they can support.

type v4l2\_stateless\_hevc\_decode\_mode

|  |  |  |
| --- | --- | --- |
| `V4L2_STATELESS_HEVC_DECODE_MODE_SLICE_BASED` | 0 | Decoding is done at the slice granularity. The OUTPUT buffer must contain a single slice. |
| `V4L2_STATELESS_HEVC_DECODE_MODE_FRAME_BASED` | 1 | Decoding is done at the frame granularity. The OUTPUT buffer must contain all slices needed to decode the frame. |

`V4L2_CID_STATELESS_HEVC_START_CODE (enum)`
:   Specifies the HEVC slice start code expected for each slice.
    This control is used as a modifier for V4L2\_PIX\_FMT\_HEVC\_SLICE
    pixel format. Applications that support V4L2\_PIX\_FMT\_HEVC\_SLICE
    are required to set this control in order to specify the start code
    that is expected for the buffer.
    Drivers may expose a single or multiple start codes, depending
    on what they can support.

type v4l2\_stateless\_hevc\_start\_code

|  |  |  |
| --- | --- | --- |
| `V4L2_STATELESS_HEVC_START_CODE_NONE` | 0 | Selecting this value specifies that HEVC slices are passed to the driver without any start code. The bitstream data should be according to [ITU H.265/HEVC](biblio.html#hevc) 7.3.1.1 General NAL unit syntax, hence contains emulation prevention bytes when required. |
| `V4L2_STATELESS_HEVC_START_CODE_ANNEX_B` | 1 | Selecting this value specifies that HEVC slices are expected to be prefixed by Annex B start codes. According to [ITU H.265/HEVC](biblio.html#hevc) valid start codes can be 3-bytes 0x000001 or 4-bytes 0x00000001. |

`V4L2_CID_MPEG_VIDEO_BASELAYER_PRIORITY_ID (integer)`
:   Specifies a priority identifier for the NAL unit, which will be applied to
    the base layer. By default this value is set to 0 for the base layer,
    and the next layer will have the priority ID assigned as 1, 2, 3 and so on.
    The video encoder can’t decide the priority id to be applied to a layer,
    so this has to come from client.
    This is applicable to H264 and valid Range is from 0 to 63.
    Source Rec. ITU-T H.264 (06/2019); G.7.4.1.1, G.8.8.1.

`V4L2_CID_MPEG_VIDEO_LTR_COUNT (integer)`
:   Specifies the maximum number of Long Term Reference (LTR) frames at any
    given time that the encoder can keep.
    This is applicable to the H264 and HEVC encoders.

`V4L2_CID_MPEG_VIDEO_FRAME_LTR_INDEX (integer)`
:   After setting this control the frame that will be queued next
    will be marked as a Long Term Reference (LTR) frame
    and given this LTR index which ranges from 0 to LTR\_COUNT-1.
    This is applicable to the H264 and HEVC encoders.
    Source Rec. ITU-T H.264 (06/2019); Table 7.9

`V4L2_CID_MPEG_VIDEO_USE_LTR_FRAMES (bitmask)`
:   Specifies the Long Term Reference (LTR) frame(s) to be used for
    encoding the next frame queued after setting this control.
    This provides a bitmask which consists of bits [0, LTR\_COUNT-1].
    This is applicable to the H264 and HEVC encoders.

`V4L2_CID_STATELESS_HEVC_DECODE_PARAMS (struct)`
:   Specifies various decode parameters, especially the references picture order
    count (POC) for all the lists (short, long, before, current, after) and the
    number of entries for each of them.
    These parameters are defined according to [ITU H.265/HEVC](biblio.html#hevc).
    They are described in section 8.3 “Slice decoding process” of the
    specification.

type v4l2\_ctrl\_hevc\_decode\_params

struct v4l2\_ctrl\_hevc\_decode\_params

|  |  |  |
| --- | --- | --- |
| \_\_s32 | `pic_order_cnt_val` | PicOrderCntVal as described in section 8.3.1 “Decoding process for picture order count” of the specification. |
| \_\_u16 | `short_term_ref_pic_set_size` | Specifies the size, in bits, of the short-term reference picture set, of the first slice described as `st_ref_pic_set()` in the specification, included in the slice header or SPS (section 7.3.6.1). |
| \_\_u16 | `long_term_ref_pic_set_size` | Specifies the size, in bits, of the long-term reference picture set, of the first slice included in the slice header or SPS. It is the number of bits in the conditional block if(long\_term\_ref\_pics\_present\_flag) in section 7.3.6.1 of the specification. |
| \_\_u8 | `num_active_dpb_entries` | The number of entries in `dpb`. |
| \_\_u8 | `num_poc_st_curr_before` | The number of reference pictures in the short-term set that come before the current frame. |
| \_\_u8 | `num_poc_st_curr_after` | The number of reference pictures in the short-term set that come after the current frame. |
| \_\_u8 | `num_poc_lt_curr` | The number of reference pictures in the long-term set. |
| \_\_u8 | `poc_st_curr_before[V4L2_HEVC_DPB_ENTRIES_NUM_MAX]` | PocStCurrBefore as described in section 8.3.2 “Decoding process for reference picture set”: provides the index of the short term before references in DPB array. |
| \_\_u8 | `poc_st_curr_after[V4L2_HEVC_DPB_ENTRIES_NUM_MAX]` | PocStCurrAfter as described in section 8.3.2 “Decoding process for reference picture set”: provides the index of the short term after references in DPB array. |
| \_\_u8 | `poc_lt_curr[V4L2_HEVC_DPB_ENTRIES_NUM_MAX]` | PocLtCurr as described in section 8.3.2 “Decoding process for reference picture set”: provides the index of the long term references in DPB array. |
| \_\_u8 | `num_delta_pocs_of_ref_rps_idx` | When the short\_term\_ref\_pic\_set\_sps\_flag in the slice header is equal to 0, it is the same as the derived value NumDeltaPocs[RefRpsIdx]. It can be used to parse the RPS data in slice headers instead of skipping it with @short\_term\_ref\_pic\_set\_size. When the value of short\_term\_ref\_pic\_set\_sps\_flag in the slice header is equal to 1, num\_delta\_pocs\_of\_ref\_rps\_idx shall be set to 0. |
| struct [`v4l2_hevc_dpb_entry`](#c.V4L.v4l2_hevc_dpb_entry "v4l2_hevc_dpb_entry") | `dpb[V4L2_HEVC_DPB_ENTRIES_NUM_MAX]` | The decoded picture buffer, for meta-data about reference frames. |
| \_\_u64 | `flags` | See [Decode Parameters Flags](#hevc-decode-params-flags) |

`Decode Parameters Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_HEVC_DECODE_PARAM_FLAG_IRAP_PIC` | 0x00000001 |  |
| `V4L2_HEVC_DECODE_PARAM_FLAG_IDR_PIC` | 0x00000002 |  |
| `V4L2_HEVC_DECODE_PARAM_FLAG_NO_OUTPUT_OF_PRIOR` | 0x00000004 |  |

`V4L2_CID_STATELESS_HEVC_EXT_SPS_LT_RPS (struct)`
:   Subset of the [`v4l2_ctrl_hevc_sps`](#c.V4L.v4l2_ctrl_hevc_sps "v4l2_ctrl_hevc_sps") control.
    It extends it with the list of Long-term reference sets parameters.
    These parameters are defined according to [ITU H.265/HEVC](biblio.html#hevc).
    They are described in section 7.4.3.2.1 “General sequence parameter set
    RBSP semantics” of the specification.
    This control is a dynamically sized 1-dimensional array.
    The values in the array should be ignored when either
    num\_long\_term\_ref\_pics\_sps is 0 or the
    V4L2\_HEVC\_SPS\_FLAG\_LONG\_TERM\_REF\_PICS\_PRESENT flag is not set in
    [`v4l2_ctrl_hevc_sps`](#c.V4L.v4l2_ctrl_hevc_sps "v4l2_ctrl_hevc_sps").

type v4l2\_ctrl\_hevc\_ext\_sps\_lt\_rps

struct v4l2\_ctrl\_hevc\_ext\_sps\_lt\_rps

|  |  |  |
| --- | --- | --- |
| \_\_u16 | `lt_ref_pic_poc_lsb_sps` | Long term reference picture order count as described in section 7.4.3.2.1 “General sequence parameter set RBSP semantics” of the specification. |
| \_\_u16 | `flags` | See [Extended Long-Term RPS Flags](#hevc-ext-sps-lt-rps-flags) |

`Extended SPS Long-Term RPS Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_HEVC_EXT_SPS_LT_RPS_FLAG_USED_LT` | 0x00000001 | Specifies if the long-term reference picture is used 7.4.3.2.1 “General sequence parameter set RBSP semantics” of the specification. |

`V4L2_CID_STATELESS_HEVC_EXT_SPS_ST_RPS (struct)`
:   Subset of the [`v4l2_ctrl_hevc_sps`](#c.V4L.v4l2_ctrl_hevc_sps "v4l2_ctrl_hevc_sps") control.
    It extends it with the list of Short-term reference sets parameters.
    These parameters are defined according to [ITU H.265/HEVC](biblio.html#hevc).
    They are described in section 7.4.8 “Short-term reference picture set
    semantics” of the specification.
    This control is a dynamically sized 1-dimensional array.
    The values in the array should be ignored when
    num\_short\_term\_ref\_pic\_sets is 0.

type v4l2\_ctrl\_hevc\_ext\_sps\_st\_rps

struct v4l2\_ctrl\_hevc\_ext\_sps\_st\_rps

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `delta_idx_minus1` | Specifies the delta compare to the index. See details in section 7.4.8 “Short-term reference picture set semantics” of the specification. |
| \_\_u8 | `delta_rps_sign` | Sign of the delta as specified in section 7.4.8 “Short-term reference picture set semantics” of the specification. |
| \_\_u8 | `num_negative_pics` | Number of short-term RPS entries that have picture order count values less than the picture order count value of the current picture. |
| \_\_u8 | `num_positive_pics` | Number of short-term RPS entries that have picture order count values greater than the picture order count value of the current picture. |
| \_\_u32 | `used_by_curr_pic` | Bit i specifies if short-term RPS i is used by the current picture. |
| \_\_u32 | `use_delta_flag` | Bit i specifies if short-term RPS i is included in the short-term RPS entries. |
| \_\_u16 | `abs_delta_rps_minus1` | Absolute delta RPS as specified in section 7.4.8 “Short-term reference picture set semantics” of the specification. |
| \_\_u16 | `delta_poc_s0_minus1[16]` | Specifies the negative picture order count delta for the i-th entry in the short-term RPS. See details in section 7.4.8 “Short-term reference picture set semantics” of the specification. |
| \_\_u16 | `delta_poc_s1_minus1[16]` | Specifies the positive picture order count delta for the i-th entry in the short-term RPS. See details in section 7.4.8 “Short-term reference picture set semantics” of the specification. |
| \_\_u16 | `flags` | See [Extended Short-Term RPS Flags](#hevc-ext-sps-st-rps-flags) |

`Extended SPS Short-Term RPS Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_HEVC_EXT_SPS_ST_RPS_FLAG_INTER_REF_PIC_SET_PRED` | 0x00000001 | Specifies if the short-term RPS is predicted from another short term RPS. See details in section 7.4.8 “Short-term reference picture set semantics” of the specification. |

`V4L2_CID_STATELESS_AV1_SEQUENCE (struct)`
:   Represents an AV1 Sequence OBU (Open Bitstream Unit). See section 5.5
    “Sequence header OBU syntax” in [AV1](biblio.html#av1) for more details.

type v4l2\_ctrl\_av1\_sequence

struct v4l2\_ctrl\_av1\_sequence

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `flags` | See [AV1 Sequence Flags](#av1-sequence-flags). |
| \_\_u8 | `seq_profile` | Specifies the features that can be used in the coded video sequence. |
| \_\_u8 | `order_hint_bits` | Specifies the number of bits used for the order\_hint field at each frame. |
| \_\_u8 | `bit_depth` | the bit depth to use for the sequence as described in section 5.5.2 “Color config syntax” in [AV1](biblio.html#av1) for more details. |
| \_\_u8 | `reserved` | Applications and drivers must set this to zero. |
| \_\_u16 | `max_frame_width_minus_1` | Specifies the maximum frame width minus 1 for the frames represented by this sequence header. |
| \_\_u16 | `max_frame_height_minus_1` | Specifies the maximum frame height minus 1 for the frames represented by this sequence header. |

`AV1 Sequence Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_AV1_SEQUENCE_FLAG_STILL_PICTURE` | 0x00000001 | If set, specifies that the coded video sequence contains only one coded frame. If not set, specifies that the coded video sequence contains one or more coded frames. |
| `V4L2_AV1_SEQUENCE_FLAG_USE_128X128_SUPERBLOCK` | 0x00000002 | If set, indicates that superblocks contain 128x128 luma samples. When equal to 0, it indicates that superblocks contain 64x64 luma samples. The number of contained chroma samples depends on subsampling\_x and subsampling\_y. |
| `V4L2_AV1_SEQUENCE_FLAG_ENABLE_FILTER_INTRA` | 0x00000004 | If set, specifies that the use\_filter\_intra syntax element may be present. If not set, specifies that the use\_filter\_intra syntax element will not be present. |
| `V4L2_AV1_SEQUENCE_FLAG_ENABLE_INTRA_EDGE_FILTER` | 0x00000008 | Specifies whether the intra edge filtering process should be enabled. |
| `V4L2_AV1_SEQUENCE_FLAG_ENABLE_INTERINTRA_COMPOUND` | 0x00000010 | If set, specifies that the mode info for inter blocks may contain the syntax element interintra. If not set, specifies that the syntax element interintra will not be present. |
| `V4L2_AV1_SEQUENCE_FLAG_ENABLE_MASKED_COMPOUND` | 0x00000020 | If set, specifies that the mode info for inter blocks may contain the syntax element compound\_type. If not set, specifies that the syntax element compound\_type will not be present. |
| `V4L2_AV1_SEQUENCE_FLAG_ENABLE_WARPED_MOTION` | 0x00000040 | If set, indicates that the allow\_warped\_motion syntax element may be present. If not set, indicates that the allow\_warped\_motion syntax element will not be present. |
| `V4L2_AV1_SEQUENCE_FLAG_ENABLE_DUAL_FILTER` | 0x00000080 | If set, indicates that the inter prediction filter type may be specified independently in the horizontal and vertical directions. If the flag is equal to 0, only one filter type may be specified, which is then used in both directions. |
| `V4L2_AV1_SEQUENCE_FLAG_ENABLE_ORDER_HINT` | 0x00000100 | If set, indicates that tools based on the values of order hints may be used. If not set, indicates that tools based on order hints are disabled. |
| `V4L2_AV1_SEQUENCE_FLAG_ENABLE_JNT_COMP` | 0x00000200 | If set, indicates that the distance weights process may be used for inter prediction. |
| `V4L2_AV1_SEQUENCE_FLAG_ENABLE_REF_FRAME_MVS` | 0x00000400 | If set, indicates that the use\_ref\_frame\_mvs syntax element may be present. If not set, indicates that the use\_ref\_frame\_mvs syntax element will not be present. |
| `V4L2_AV1_SEQUENCE_FLAG_ENABLE_SUPERRES` | 0x00000800 | If set, specifies that the use\_superres syntax element will be present in the uncompressed header. If not set, specifies that the use\_superres syntax element will not be present (instead use\_superres will be set to 0 in the uncompressed header without being read). |
| `V4L2_AV1_SEQUENCE_FLAG_ENABLE_CDEF` | 0x00001000 | If set, specifies that cdef filtering may be enabled. If not set, specifies that cdef filtering is disabled. |
| `V4L2_AV1_SEQUENCE_FLAG_ENABLE_RESTORATION` | 0x00002000 | If set, specifies that loop restoration filtering may be enabled. If not set, specifies that loop restoration filtering is disabled. |
| `V4L2_AV1_SEQUENCE_FLAG_MONO_CHROME` | 0x00004000 | If set, indicates that the video does not contain U and V color planes. If not set, indicates that the video contains Y, U, and V color planes. |
| `V4L2_AV1_SEQUENCE_FLAG_COLOR_RANGE` | 0x00008000 | If set, signals full swing representation, i.e. “Full Range Quantization”. If not set, signals studio swing representation, i.e. “Limited Range Quantization”. |
| `V4L2_AV1_SEQUENCE_FLAG_SUBSAMPLING_X` | 0x00010000 | Specify the chroma subsampling format. |
| `V4L2_AV1_SEQUENCE_FLAG_SUBSAMPLING_Y` | 0x00020000 | Specify the chroma subsampling format. |
| `V4L2_AV1_SEQUENCE_FLAG_FILM_GRAIN_PARAMS_PRESENT` | 0x00040000 | Specifies whether film grain parameters are present in the coded video sequence. |
| `V4L2_AV1_SEQUENCE_FLAG_SEPARATE_UV_DELTA_Q` | 0x00080000 | If set, indicates that the U and V planes may have separate delta quantizer values. If not set, indicates that the U and V planes will share the same delta quantizer value. |

`V4L2_CID_STATELESS_AV1_TILE_GROUP_ENTRY (struct)`
:   Represents a single AV1 tile inside an AV1 Tile Group. Note that MiRowStart,
    MiRowEnd, MiColStart and MiColEnd can be retrieved from [`struct
    v4l2_av1_tile_info`](#c.V4L.v4l2_av1_tile_info "V4L.v4l2_av1_tile_info") in [`struct v4l2_ctrl_av1_frame`](#c.V4L.v4l2_ctrl_av1_frame "V4L.v4l2_ctrl_av1_frame") using tile\_row and
    tile\_col. See section 6.10.1 “General tile group OBU semantics” in
    [AV1](biblio.html#av1) for more details.

type v4l2\_ctrl\_av1\_tile\_group\_entry

struct v4l2\_ctrl\_av1\_tile\_group\_entry

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `tile_offset` | Offset from the OBU data, i.e. where the coded tile data actually starts. |
| \_\_u32 | `tile_size` | Specifies the size in bytes of the coded tile. Equivalent to “TileSize” in [AV1](biblio.html#av1). |
| \_\_u32 | `tile_row` | Specifies the row of the current tile. Equivalent to “TileRow” in [AV1](biblio.html#av1). |
| \_\_u32 | `tile_col` | Specifies the column of the current tile. Equivalent to “TileColumn” in [AV1](biblio.html#av1). |

type v4l2\_av1\_warp\_model
:   AV1 Warp Model as described in section 3 “Symbols and abbreviated terms” of
    [AV1](biblio.html#av1).

|  |  |  |
| --- | --- | --- |
| `V4L2_AV1_WARP_MODEL_IDENTITY` | 0 | Warp model is just an identity transform. |
| `V4L2_AV1_WARP_MODEL_TRANSLATION` | 1 | Warp model is a pure translation. |
| `V4L2_AV1_WARP_MODEL_ROTZOOM` | 2 | Warp model is a rotation + symmetric zoom + translation. |
| `V4L2_AV1_WARP_MODEL_AFFINE` | 3 | Warp model is a general affine transform. |

type v4l2\_av1\_reference\_frame

AV1 Reference Frames as described in section 6.10.24 “Ref frames semantics”
of [AV1](biblio.html#av1).

|  |  |  |
| --- | --- | --- |
| `V4L2_AV1_REF_INTRA_FRAME` | 0 | Intra Frame Reference. |
| `V4L2_AV1_REF_LAST_FRAME` | 1 | Last Frame Reference. |
| `V4L2_AV1_REF_LAST2_FRAME` | 2 | Last2 Frame Reference. |
| `V4L2_AV1_REF_LAST3_FRAME` | 3 | Last3 Frame Reference. |
| `V4L2_AV1_REF_GOLDEN_FRAME` | 4 | Golden Frame Reference. |
| `V4L2_AV1_REF_BWDREF_FRAME` | 5 | BWD Frame Reference. |
| `V4L2_AV1_REF_ALTREF2_FRAME` | 6 | ALTREF2 Frame Reference. |
| `V4L2_AV1_REF_ALTREF_FRAME` | 7 | ALTREF Frame Reference. |

type v4l2\_av1\_global\_motion

AV1 Global Motion parameters as described in section 6.8.17
“Global motion params semantics” of [AV1](biblio.html#av1).

struct v4l2\_av1\_global\_motion

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `flags[V4L2_AV1_TOTAL_REFS_PER_FRAME]` | A bitfield containing the flags per reference frame. See [AV1 Global Motion Flags](#av1-global-motion-flags) for more details. |
| enum [`v4l2_av1_warp_model`](#c.V4L.v4l2_av1_warp_model "v4l2_av1_warp_model") | `type[V4L2_AV1_TOTAL_REFS_PER_FRAME]` | The type of global motion transform used. |
| \_\_s32 | `params[V4L2_AV1_TOTAL_REFS_PER_FRAME][6]` | This field has the same meaning as “gm\_params” in [AV1](biblio.html#av1). |
| \_\_u8 | `invalid` | Bitfield indicating whether the global motion params are invalid for a given reference frame. See section 7.11.3.6 Setup shear process and the variable “warpValid”. Use V4L2\_AV1\_GLOBAL\_MOTION\_IS\_INVALID(ref) to create a suitable mask. |
| \_\_u8 | `reserved[3]` | Applications and drivers must set this to zero. |

`AV1 Global Motion Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_AV1_GLOBAL_MOTION_FLAG_IS_GLOBAL` | 0x00000001 | Specifies whether global motion parameters are present for a particular reference frame. |
| `V4L2_AV1_GLOBAL_MOTION_FLAG_IS_ROT_ZOOM` | 0x00000002 | Specifies whether a particular reference frame uses rotation and zoom global motion. |
| `V4L2_AV1_GLOBAL_MOTION_FLAG_IS_TRANSLATION` | 0x00000004 | Specifies whether a particular reference frame uses translation global motion |

type v4l2\_av1\_frame\_restoration\_type

AV1 Frame Restoration Type.

|  |  |  |
| --- | --- | --- |
| `V4L2_AV1_FRAME_RESTORE_NONE` | 0 | No filtering is applied. |
| `V4L2_AV1_FRAME_RESTORE_WIENER` | 1 | Wiener filter process is invoked. |
| `V4L2_AV1_FRAME_RESTORE_SGRPROJ` | 2 | Self guided filter process is invoked. |
| `V4L2_AV1_FRAME_RESTORE_SWITCHABLE` | 3 | Restoration filter is swichtable. |

type v4l2\_av1\_loop\_restoration

AV1 Loop Restoration as described in section 6.10.15 “Loop restoration params
semantics” of [AV1](biblio.html#av1).

struct v4l2\_av1\_loop\_restoration

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `flags` | See [AV1 Loop Restoration Flags](#av1-loop-restoration-flags). |
| \_\_u8 | `lr_unit_shift` | Specifies if the luma restoration size should be halved. |
| \_\_u8 | `lr_uv_shift` | Specifies if the chroma size should be half the luma size. |
| \_\_u8 | `reserved` | Applications and drivers must set this to zero. |
| [`v4l2_av1_frame_restoration_type`](#c.V4L.v4l2_av1_frame_restoration_type "v4l2_av1_frame_restoration_type") | `frame_restoration_type[V4L2_AV1_NUM_PLANES_MAX]` | Specifies the type of restoration used for each plane. |
| \_\_u8 | `loop_restoration_size[V4L2_AV1_MAX_NUM_PLANES]` | Specifies the size of loop restoration units in units of samples in the current plane. |

`AV1 Loop Restoration Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_AV1_LOOP_RESTORATION_FLAG_USES_LR` | 0x00000001 | Retains the same meaning as UsesLr in [AV1](biblio.html#av1). |
| `V4L2_AV1_LOOP_RESTORATION_FLAG_USES_CHROMA_LR` | 0x00000002 | Retains the same meaning as UsesChromaLr in [AV1](biblio.html#av1). |

type v4l2\_av1\_cdef

AV1 CDEF params semantics as described in section 6.10.14 “CDEF params
semantics” of [AV1](biblio.html#av1).

struct v4l2\_av1\_cdef

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `damping_minus_3` | Controls the amount of damping in the deringing filter. |
| \_\_u8 | `bits` | Specifies the number of bits needed to specify which CDEF filter to apply. |
| \_\_u8 | `y_pri_strength[V4L2_AV1_CDEF_MAX]` | Specifies the strength of the primary filter. |
| \_\_u8 | `y_sec_strength[V4L2_AV1_CDEF_MAX]` | Specifies the strength of the secondary filter. |
| \_\_u8 | `uv_pri_strength[V4L2_AV1_CDEF_MAX]` | Specifies the strength of the primary filter. |
| \_\_u8 | `uv_sec_strength[V4L2_AV1_CDEF_MAX]` | Specifies the strength of the secondary filter. |

type v4l2\_av1\_segment\_feature

AV1 segment features as described in section 3 “Symbols and abbreviated terms”
of [AV1](biblio.html#av1).

|  |  |  |
| --- | --- | --- |
| `V4L2_AV1_SEG_LVL_ALT_Q` | 0 | Index for quantizer segment feature. |
| `V4L2_AV1_SEG_LVL_ALT_LF_Y_V` | 1 | Index for vertical luma loop filter segment feature. |
| `V4L2_AV1_SEG_LVL_REF_FRAME` | 5 | Index for reference frame segment feature. |
| `V4L2_AV1_SEG_LVL_REF_SKIP` | 6 | Index for skip segment feature. |
| `V4L2_AV1_SEG_LVL_REF_GLOBALMV` | 7 | Index for global mv feature. |
| `V4L2_AV1_SEG_LVL_MAX` | 8 | Number of segment features. |

type v4l2\_av1\_segmentation

AV1 Segmentation params as defined in section 6.8.13 “Segmentation params
semantics” of [AV1](biblio.html#av1).

struct v4l2\_av1\_segmentation

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `flags` | See [AV1 Segmentation Flags](#av1-segmentation-flags) |
| \_\_u8 | `last_active_seg_id` | Indicates the highest numbered segment id that has some enabled feature. This is used when decoding the segment id to only decode choices corresponding to used segments. |
| \_\_u8 | `feature_enabled[V4L2_AV1_MAX_SEGMENTS]` | Bitmask defining which features are enabled in each segment. Use V4L2\_AV1\_SEGMENT\_FEATURE\_ENABLED to build a suitable mask. |
| \_\_u16 | `feature_data[V4L2_AV1_MAX_SEGMENTS][V4L2_AV1_SEG_LVL_MAX]` | Data attached to each feature. Data entry is only valid if the feature is enabled. |

`AV1 Segmentation Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_AV1_SEGMENTATION_FLAG_ENABLED` | 0x00000001 | If set, indicates that this frame makes use of the segmentation tool. If not set, indicates that the frame does not use segmentation. |
| `V4L2_AV1_SEGMENTATION_FLAG_UPDATE_MAP` | 0x00000002 | If set, indicates that the segmentation map are updated during the decoding of this frame. If not set, indicates that the segmentation map from the previous frame is used. |
| `V4L2_AV1_SEGMENTATION_FLAG_TEMPORAL_UPDATE` | 0x00000004 | If set, indicates that the updates to the segmentation map are coded relative to the existing segmentation map. If not set, indicates that the new segmentation map is coded without reference to the existing segmentation map. |
| `V4L2_AV1_SEGMENTATION_FLAG_UPDATE_DATA` | 0x00000008 | If set, indicates that the updates to the segmentation map are coded relative to the existing segmentation map. If not set, indicates that the new segmentation map is coded without reference to the existing segmentation map. |
| `V4L2_AV1_SEGMENTATION_FLAG_SEG_ID_PRE_SKIP` | 0x00000010 | If set, indicates that the segment id will be read before the skip syntax element. If not set, indicates that the skip syntax element will be read first. |

type v4l2\_av1\_loop\_filter

AV1 Loop filter params as defined in section 6.8.10 “Loop filter semantics” of
[AV1](biblio.html#av1).

struct v4l2\_av1\_loop\_filter

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `flags` | See [AV1 Loop Filter flags](#av1-loop-filter-flags) for more details. |
| \_\_u8 | `level[4]` | An array containing loop filter strength values. Different loop filter strength values from the array are used depending on the image plane being filtered, and the edge direction (vertical or horizontal) being filtered. |
| \_\_u8 | `sharpness` | indicates the sharpness level. The loop\_filter\_level and loop\_filter\_sharpness together determine when a block edge is filtered, and by how much the filtering can change the sample values. The loop filter process is described in section 7.14 of [AV1](biblio.html#av1). |
| \_\_u8 | `ref_deltas[V4L2_AV1_TOTAL_REFS_PER_FRAME]` | contains the adjustment needed for the filter level based on the chosen reference frame. If this syntax element is not present, it maintains its previous value. |
| \_\_u8 | `mode_deltas[2]` | contains the adjustment needed for the filter level based on the chosen mode. If this syntax element is not present, it maintains its previous value. |
| \_\_u8 | `delta_lf_res` | specifies the left shift which should be applied to decoded loop filter delta values. |

`AV1 Loop Filter Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_AV1_LOOP_FILTER_FLAG_DELTA_ENABLED` | 0x00000001 | If set, means that the filter level depends on the mode and reference frame used to predict a block. If not set, means that the filter level does not depend on the mode and reference frame. |
| `V4L2_AV1_LOOP_FILTER_FLAG_DELTA_UPDATE` | 0x00000002 | If set, means that additional syntax elements are present that specify which mode and reference frame deltas are to be updated. If not set, means that these syntax elements are not present. |
| `V4L2_AV1_LOOP_FILTER_FLAG_DELTA_LF_PRESENT` | 0x00000004 | Specifies whether loop filter delta values are present |
| `V4L2_AV1_LOOP_FILTER_FLAG_DELTA_LF_MULTI` | 0x00000008 | A value equal to 1 specifies that separate loop filter deltas are sent for horizontal luma edges, vertical luma edges, the U edges, and the V edges. A value of delta\_lf\_multi equal to 0 specifies that the same loop filter delta is used for all edges. |

type v4l2\_av1\_quantization

AV1 Quantization params as defined in section 6.8.11 “Quantization params
semantics” of [AV1](biblio.html#av1).

struct v4l2\_av1\_quantization

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `flags` | See [AV1 Loop Filter flags](#av1-quantization-flags) for more details. |
| \_\_u8 | `base_q_idx` | Indicates the base frame qindex. This is used for Y AC coefficients and as the base value for the other quantizers. |
| \_\_u8 | `delta_q_y_dc` | Indicates the Y DC quantizer relative to base\_q\_idx. |
| \_\_u8 | `delta_q_u_dc` | Indicates the U DC quantizer relative to base\_q\_idx. |
| \_\_u8 | `delta_q_u_ac` | Indicates the U AC quantizer relative to base\_q\_idx. |
| \_\_u8 | `delta_q_v_dc` | Indicates the V DC quantizer relative to base\_q\_idx. |
| \_\_u8 | `delta_q_v_ac` | Indicates the V AC quantizer relative to base\_q\_idx. |
| \_\_u8 | `qm_y` | Specifies the level in the quantizer matrix that should be used for luma plane decoding. |
| \_\_u8 | `qm_u` | Specifies the level in the quantizer matrix that should be used for chroma U plane decoding. |
| \_\_u8 | `qm_v` | Specifies the level in the quantizer matrix that should be used for chroma V plane decoding. |
| \_\_u8 | `delta_q_res` | Specifies the left shift which should be applied to decoded quantizer index delta values. |

`AV1 Quantization Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_AV1_QUANTIZATION_FLAG_DIFF_UV_DELTA` | 0x00000001 | If set, indicates that the U and V delta quantizer values are coded separately. If not set, indicates that the U and V delta quantizer values share a common value. |
| `V4L2_AV1_QUANTIZATION_FLAG_USING_QMATRIX` | 0x00000002 | If set, specifies that the quantizer matrix will be used to compute quantizers. |
| `V4L2_AV1_QUANTIZATION_FLAG_DELTA_Q_PRESENT` | 0x00000004 | Specifies whether quantizer index delta values are present. |

type v4l2\_av1\_tile\_info

AV1 Tile info as defined in section 6.8.14 “Tile info semantics” of ref:av1.

struct v4l2\_av1\_tile\_info

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `flags` | See [AV1 Tile Info flags](#av1-tile-info-flags) for more details. |
| \_\_u8 | `context_update_tile_id` | Specifies which tile to use for the CDF update. |
| \_\_u8 | `tile_cols` | Specifies the number of tiles across the frame. |
| \_\_u8 | `tile_rows` | Specifies the number of tiles down the frame. |
| \_\_u32 | `mi_col_starts[V4L2_AV1_MAX_TILE_COLS + 1]` | An array specifying the start column (in units of 4x4 luma samples) for each tile across the image. |
| \_\_u32 | `mi_row_starts[V4L2_AV1_MAX_TILE_ROWS + 1]` | An array specifying the start row (in units of 4x4 luma samples) for each tile across the image. |
| \_\_u32 | `width_in_sbs_minus_1[V4L2_AV1_MAX_TILE_COLS]` | Specifies the width of a tile minus 1 in units of superblocks. |
| \_\_u32 | `height_in_sbs_minus_1[V4L2_AV1_MAX_TILE_ROWS]` | Specifies the height of a tile minus 1 in units of superblocks. |
| \_\_u8 | `tile_size_bytes` | Specifies the number of bytes needed to code each tile size. |
| \_\_u8 | `reserved[3]` | Applications and drivers must set this to zero. |

`AV1 Tile Info Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_AV1_TILE_INFO_FLAG_UNIFORM_TILE_SPACING` | 0x00000001 | If set, means that the tiles are uniformly spaced across the frame. (In other words, all tiles are the same size except for the ones at the right and bottom edge which can be smaller). If not set means that the tile sizes are coded. |

type v4l2\_av1\_frame\_type

AV1 Frame Type

|  |  |  |
| --- | --- | --- |
| `V4L2_AV1_KEY_FRAME` | 0 | Key frame. |
| `V4L2_AV1_INTER_FRAME` | 1 | Inter frame. |
| `V4L2_AV1_INTRA_ONLY_FRAME` | 2 | Intra-only frame. |
| `V4L2_AV1_SWITCH_FRAME` | 3 | Switch frame. |

type v4l2\_av1\_interpolation\_filter

AV1 Interpolation Filter

|  |  |  |
| --- | --- | --- |
| `V4L2_AV1_INTERPOLATION_FILTER_EIGHTTAP` | 0 | Eight tap filter. |
| `V4L2_AV1_INTERPOLATION_FILTER_EIGHTTAP_SMOOTH` | 1 | Eight tap smooth filter. |
| `V4L2_AV1_INTERPOLATION_FILTER_EIGHTTAP_SHARP` | 2 | Eight tap sharp filter. |
| `V4L2_AV1_INTERPOLATION_FILTER_BILINEAR` | 3 | Bilinear filter. |
| `V4L2_AV1_INTERPOLATION_FILTER_SWITCHABLE` | 4 | Filter selection is signaled at the block level. |

type v4l2\_av1\_tx\_mode

AV1 Tx mode as described in section 6.8.21 “TX mode semantics” of [AV1](biblio.html#av1).

|  |  |  |
| --- | --- | --- |
| `V4L2_AV1_TX_MODE_ONLY_4X4` | 0 | The inverse transform will use only 4x4 transforms. |
| `V4L2_AV1_TX_MODE_LARGEST` | 1 | The inverse transform will use the largest transform size that fits inside the block. |
| `V4L2_AV1_TX_MODE_SELECT` | 2 | The choice of transform size is specified explicitly for each block. |

`V4L2_CID_STATELESS_AV1_FRAME (struct)`
:   Represents a Frame Header OBU. See 6.8 “Frame Header OBU semantics” of
    [AV1](biblio.html#av1) for more details.

type v4l2\_ctrl\_av1\_frame

struct v4l2\_ctrl\_av1\_frame

|  |  |  |
| --- | --- | --- |
| struct [`v4l2_av1_tile_info`](#c.V4L.v4l2_av1_tile_info "v4l2_av1_tile_info") | `tile_info` | Tile info |
| struct [`v4l2_av1_quantization`](#c.V4L.v4l2_av1_quantization "v4l2_av1_quantization") | `quantization` | Quantization parameters. |
| \_\_u8 | `superres_denom` | The denominator for the upscaling ratio. |
| struct [`v4l2_av1_segmentation`](#c.V4L.v4l2_av1_segmentation "v4l2_av1_segmentation") | `segmentation` | Segmentation parameters. |
| struct [`v4l2_av1_loop_filter`](#c.V4L.v4l2_av1_loop_filter "v4l2_av1_loop_filter") | `loop_filter` | Loop filter params |
| struct [`v4l2_av1_cdef`](#c.V4L.v4l2_av1_cdef "v4l2_av1_cdef") | `cdef` | CDEF params |
| \_\_u8 | `skip_mode_frame[2]` | Specifies the frames to use for compound prediction when skip\_mode is equal to 1. |
| \_\_u8 | `primary_ref_frame` | Specifies which reference frame contains the CDF values and other state that should be loaded at the start of the frame. |
| struct [`v4l2_av1_loop_restoration`](#c.V4L.v4l2_av1_loop_restoration "v4l2_av1_loop_restoration") | `loop_restoration` | Loop restoration parameters. |
| struct [`v4l2_av1_global_motion`](#c.V4L.v4l2_av1_global_motion "v4l2_av1_global_motion") | `global_motion` | Global motion parameters. |
| \_\_u32 | `flags` | See [AV1 Frame flags](#av1-frame-flags) for more details. |
| enum [`v4l2_av1_frame_type`](#c.V4L.v4l2_av1_frame_type "v4l2_av1_frame_type") | `frame_type` | Specifies the AV1 frame type |
| \_\_u32 | `order_hint` | Specifies OrderHintBits least significant bits of the expected output order for this frame. |
| \_\_u32 | `upscaled_width` | The upscaled width. |
| enum [`v4l2_av1_interpolation_filter`](#c.V4L.v4l2_av1_interpolation_filter "v4l2_av1_interpolation_filter") | `interpolation_filter` | Specifies the filter selection used for performing inter prediction. |
| enum [`v4l2_av1_tx_mode`](#c.V4L.v4l2_av1_tx_mode "v4l2_av1_tx_mode") | `tx_mode` | Specifies how the transform size is determined. |
| \_\_u32 | `frame_width_minus_1` | Add 1 to get the frame’s width. |
| \_\_u32 | `frame_height_minus_1` | Add 1 to get the frame’s height. |
| \_\_u16 | `render_width_minus_1` | Add 1 to get the render width of the frame in luma samples. |
| \_\_u16 | `render_height_minus_1` | Add 1 to get the render height of the frame in luma samples. |
| \_\_u32 | `current_frame_id` | Specifies the frame id number for the current frame. Frame id numbers are additional information that do not affect the decoding process, but provide decoders with a way of detecting missing reference frames so that appropriate action can be taken. |
| \_\_u8 | `buffer_removal_time[V4L2_AV1_MAX_OPERATING_POINTS]` | Specifies the frame removal time in units of DecCT clock ticks counted from the removal time of the last random access point for operating point opNum. |
| \_\_u8 | `reserved[4]` | Applications and drivers must set this to zero. |
| \_\_u32 | `order_hints[V4L2_AV1_TOTAL_REFS_PER_FRAME]` | Specifies the expected output order hint for each reference frame. This field corresponds to the OrderHints variable from the specification (section 5.9.2 “Uncompressed header syntax”). As such, this is only used for non-intra frames and ignored otherwise. order\_hints[0] is always ignored. |
| \_\_u64 | `reference_frame_ts[V4L2_AV1_TOTAL_REFS_PER_FRAME]` | The V4L2 timestamp for each of the reference frames enumerated in enum [`v4l2_av1_reference_frame`](#c.V4L.v4l2_av1_reference_frame "v4l2_av1_reference_frame") starting at `V4L2_AV1_REF_LAST_FRAME`. This represents the state of reference slot as described in the spec and updated by userland through the “Reference frame update process” in section 7.20 The timestamp refers to the `timestamp` field in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer"). Use the `v4l2_timeval_to_ns()` function to convert the struct `timeval` in struct [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "v4l2_buffer") to a \_\_u64. |
| \_\_s8 | `ref_frame_idx[V4L2_AV1_REFS_PER_FRAME]` | An index into `reference_frame_ts` representing the ordered list of references used by inter-frame. Matches the bitstream syntax element of the same name. |
| \_\_u8 | `refresh_frame_flags` | Contains a bitmask that specifies which reference frame slots will be updated with the current frame after it is decoded. |

`AV1 Frame Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_AV1_FRAME_FLAG_SHOW_FRAME` | 0x00000001 | If set, specifies that this frame should be immediately output once decoded. If not set, specifies that this frame should not be immediately output; it may be output later if a later uncompressed header uses show\_existing\_frame equal to 1. |
| `V4L2_AV1_FRAME_FLAG_SHOWABLE_FRAME` | 0x00000002 | If set, specifies that the frame may be output using the show\_existing\_frame mechanism. If not set, specifies that this frame will not be output using the show\_existing\_frame mechanism. |
| `V4L2_AV1_FRAME_FLAG_ERROR_RESILIENT_MODE` | 0x00000004 | Specifies whether error resilient mode is enabled. |
| `V4L2_AV1_FRAME_FLAG_DISABLE_CDF_UPDATE` | 0x00000008 | Specifies whether the CDF update in the symbol decoding process should be disabled. |
| `V4L2_AV1_FRAME_FLAG_ALLOW_SCREEN_CONTENT_TOOLS` | 0x00000010 | If set, indicates that intra blocks may use palette encoding. If not set, indicates that palette encoding is never used. |
| `V4L2_AV1_FRAME_FLAG_FORCE_INTEGER_MV` | 0x00000020 | If set, specifies that motion vectors will always be integers. If not set, specifies that motion vectors can contain fractional bits. |
| `V4L2_AV1_FRAME_FLAG_ALLOW_INTRABC` | 0x00000040 | If set, indicates that intra block copy may be used in this frame. If not set, indicates that intra block copy is not allowed in this frame. |
| `V4L2_AV1_FRAME_FLAG_USE_SUPERRES` | 0x00000080 | If set, indicates that upscaling is needed. |
| `V4L2_AV1_FRAME_FLAG_ALLOW_HIGH_PRECISION_MV` | 0x00000100 | If set, specifies that motion vectors are specified to eighth pel precision. If not set, specifies that motion vectors are specified to quarter pel precision; |
| `V4L2_AV1_FRAME_FLAG_IS_MOTION_MODE_SWITCHABLE` | 0x00000200 | If not set, specifies that only the SIMPLE motion mode will be used. |
| `V4L2_AV1_FRAME_FLAG_USE_REF_FRAME_MVS` | 0x00000400 | If set specifies that motion vector information from a previous frame can be used when decoding the current frame. If not set, specifies that this information will not be used. |
| `V4L2_AV1_FRAME_FLAG_DISABLE_FRAME_END_UPDATE_CDF` | 0x00000800 | If set indicates that the end of frame CDF update is disabled. If not set, indicates that the end of frame CDF update is enabled |
| `V4L2_AV1_FRAME_FLAG_ALLOW_WARPED_MOTION` | 0x00001000 | If set, indicates that the syntax element motion\_mode may be present, if not set, indicates that the syntax element motion\_mode will not be present. |
| `V4L2_AV1_FRAME_FLAG_REFERENCE_SELECT` | 0x00002000 | If set, specifies that the mode info for inter blocks contains the syntax element comp\_mode that indicates whether to use single or compound reference prediction. If not set, specifies that all inter blocks will use single prediction. |
| `V4L2_AV1_FRAME_FLAG_REDUCED_TX_SET` | 0x00004000 | If set, specifies that the frame is restricted to a reduced subset of the full set of transform types. |
| `V4L2_AV1_FRAME_FLAG_SKIP_MODE_ALLOWED` | 0x00008000 | This flag retains the same meaning as SkipModeAllowed in [AV1](biblio.html#av1). |
| `V4L2_AV1_FRAME_FLAG_SKIP_MODE_PRESENT` | 0x00010000 | If set, specifies that the syntax element skip\_mode will be present, if not set, specifies that skip\_mode will not be used for this frame. |
| `V4L2_AV1_FRAME_FLAG_FRAME_SIZE_OVERRIDE` | 0x00020000 | If set, specifies that the frame size will either be specified as the size of one of the reference frames, or computed from the frame\_width\_minus\_1 and frame\_height\_minus\_1 syntax elements. If not set, specifies that the frame size is equal to the size in the sequence header. |
| `V4L2_AV1_FRAME_FLAG_BUFFER_REMOVAL_TIME_PRESENT` | 0x00040000 | If set, specifies that buffer\_removal\_time is present. If not set, specifies that buffer\_removal\_time is not present. |
| `V4L2_AV1_FRAME_FLAG_FRAME_REFS_SHORT_SIGNALING` | 0x00080000 | If set, indicates that only two reference frames are explicitly signaled. If not set, indicates that all reference frames are explicitly signaled. |

`V4L2_CID_STATELESS_AV1_FILM_GRAIN (struct)`
:   Represents the optional film grain parameters. See section
    6.8.20 “Film grain params semantics” of [AV1](biblio.html#av1) for more details.

type v4l2\_ctrl\_av1\_film\_grain

struct v4l2\_ctrl\_av1\_film\_grain

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `flags` | See [AV1 Film Grain Flags](#av1-film-grain-flags). |
| \_\_u8 | `cr_mult` | Represents a multiplier for the cr component used in derivation of the input index to the cr component scaling function. |
| \_\_u16 | `grain_seed` | Specifies the starting value for the pseudo-random numbers used during film grain synthesis. |
| \_\_u8 | `film_grain_params_ref_idx` | Indicates which reference frame contains the film grain parameters to be used for this frame. |
| \_\_u8 | `num_y_points` | Specifies the number of points for the piece-wise linear scaling function of the luma component. |
| \_\_u8 | `point_y_value[V4L2_AV1_MAX_NUM_Y_POINTS]` | Represents the x (luma value) coordinate for the i-th point of the piecewise linear scaling function for luma component. The values are signaled on the scale of 0..255. In case of 10 bit video, these values correspond to luma values divided by 4. In case of 12 bit video, these values correspond to luma values divided by 16. |
| \_\_u8 | `point_y_scaling[V4L2_AV1_MAX_NUM_Y_POINTS]` | Represents the scaling (output) value for the i-th point of the piecewise linear scaling function for luma component. |
| \_\_u8 | `num_cb_points` | Specifies the number of points for the piece-wise linear scaling function of the cb component. |
| \_\_u8 | `point_cb_value[V4L2_AV1_MAX_NUM_CB_POINTS]` | Represents the x coordinate for the i-th point of the piece-wise linear scaling function for cb component. The values are signaled on the scale of 0..255. |
| \_\_u8 | `point_cb_scaling[V4L2_AV1_MAX_NUM_CB_POINTS]` | Represents the scaling (output) value for the i-th point of the piecewise linear scaling function for cb component. |
| \_\_u8 | `num_cr_points` | Represents the number of points for the piece-wise linear scaling function of the cr component. |
| \_\_u8 | `point_cr_value[V4L2_AV1_MAX_NUM_CR_POINTS]` | Represents the x coordinate for the i-th point of the piece-wise linear scaling function for cr component. The values are signaled on the scale of 0..255. |
| \_\_u8 | `point_cr_scaling[V4L2_AV1_MAX_NUM_CR_POINTS]` | Represents the scaling (output) value for the i-th point of the piecewise linear scaling function for cr component. |
| \_\_u8 | `grain_scaling_minus_8` | Represents the shift - 8 applied to the values of the chroma component. The grain\_scaling\_minus\_8 can take values of 0..3 and determines the range and quantization step of the standard deviation of film grain. |
| \_\_u8 | `ar_coeff_lag` | Specifies the number of auto-regressive coefficients for luma and chroma. |
| \_\_u8 | `ar_coeffs_y_plus_128[V4L2_AV1_AR_COEFFS_SIZE]` | Specifies auto-regressive coefficients used for the Y plane. |
| \_\_u8 | `ar_coeffs_cb_plus_128[V4L2_AV1_AR_COEFFS_SIZE]` | Specifies auto-regressive coefficients used for the U plane. |
| \_\_u8 | `ar_coeffs_cr_plus_128[V4L2_AV1_AR_COEFFS_SIZE]` | Specifies auto-regressive coefficients used for the V plane. |
| \_\_u8 | `ar_coeff_shift_minus_6` | Specifies the range of the auto-regressive coefficients. Values of 0, 1, 2, and 3 correspond to the ranges for auto-regressive coefficients of [-2, 2), [-1, 1), [-0.5, 0.5) and [-0.25, 0.25) respectively. |
| \_\_u8 | `grain_scale_shift` | Specifies how much the Gaussian random numbers should be scaled down during the grain synthesis process. |
| \_\_u8 | `cb_mult` | Represents a multiplier for the cb component used in derivation of the input index to the cb component scaling function. |
| \_\_u8 | `cb_luma_mult` | Represents a multiplier for the average luma component used in derivation of the input index to the cb component scaling function.. |
| \_\_u8 | `cr_luma_mult` | Represents a multiplier for the average luma component used in derivation of the input index to the cr component scaling function. |
| \_\_u16 | `cb_offset` | Represents an offset used in derivation of the input index to the cb component scaling function. |
| \_\_u16 | `cr_offset` | Represents an offset used in derivation of the input index to the cr component scaling function. |
| \_\_u8 | `reserved[4]` | Applications and drivers must set this to zero. |

`AV1 Film Grain Flags`

|  |  |  |
| --- | --- | --- |
| `V4L2_AV1_FILM_GRAIN_FLAG_APPLY_GRAIN` | 0x00000001 | If set, specifies that film grain should be added to this frame. If not set, specifies that film grain should not be added. |
| `V4L2_AV1_FILM_GRAIN_FLAG_UPDATE_GRAIN` | 0x00000002 | If set, means that a new set of parameters should be sent. If not set, specifies that the previous set of parameters should be used. |
| `V4L2_AV1_FILM_GRAIN_FLAG_CHROMA_SCALING_FROM_LUMA` | 0x00000004 | If set, specifies that the chroma scaling is inferred from the luma scaling. |
| `V4L2_AV1_FILM_GRAIN_FLAG_OVERLAP` | 0x00000008 | If set, indicates that the overlap between film grain blocks shall be applied. If not set, indicates that the overlap between film grain blocks shall not be applied. |
| `V4L2_AV1_FILM_GRAIN_FLAG_CLIP_TO_RESTRICTED_RANGE` | 0x00000010 | If set, indicates that clipping to the restricted (studio, i.e. limited) range shall be applied to the sample values after adding the film grain (see the semantics for color\_range for an explanation of studio swing). If not set, indicates that clipping to the full range shall be applied to the sample values after adding the film grain. |
