# 9.Video For Linux Two Header uAPI Symbols

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/videodev.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 9. Video For Linux Two Header uAPI Symbols

## 9.1. Enumeration values

* [`V4L2_BUF_TYPE_META_CAPTURE`](buffer.html#c.V4L.v4l2_buf_type "V4L.v4l2_buf_type"): include/uapi/linux/videodev2.h#154
* [`V4L2_BUF_TYPE_META_OUTPUT`](buffer.html#c.V4L.v4l2_buf_type "V4L.v4l2_buf_type"): include/uapi/linux/videodev2.h#155
* [`V4L2_BUF_TYPE_SDR_CAPTURE`](buffer.html#c.V4L.v4l2_buf_type "V4L.v4l2_buf_type"): include/uapi/linux/videodev2.h#152
* [`V4L2_BUF_TYPE_SDR_OUTPUT`](buffer.html#c.V4L.v4l2_buf_type "V4L.v4l2_buf_type"): include/uapi/linux/videodev2.h#153
* [`V4L2_BUF_TYPE_SLICED_VBI_CAPTURE`](buffer.html#c.V4L.v4l2_buf_type "V4L.v4l2_buf_type"): include/uapi/linux/videodev2.h#147
* [`V4L2_BUF_TYPE_SLICED_VBI_OUTPUT`](buffer.html#c.V4L.v4l2_buf_type "V4L.v4l2_buf_type"): include/uapi/linux/videodev2.h#148
* [`V4L2_BUF_TYPE_VBI_CAPTURE`](buffer.html#c.V4L.v4l2_buf_type "V4L.v4l2_buf_type"): include/uapi/linux/videodev2.h#145
* [`V4L2_BUF_TYPE_VBI_OUTPUT`](buffer.html#c.V4L.v4l2_buf_type "V4L.v4l2_buf_type"): include/uapi/linux/videodev2.h#146
* [`V4L2_BUF_TYPE_VIDEO_CAPTURE`](buffer.html#c.V4L.v4l2_buf_type "V4L.v4l2_buf_type"): include/uapi/linux/videodev2.h#142
* [`V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE`](buffer.html#c.V4L.v4l2_buf_type "V4L.v4l2_buf_type"): include/uapi/linux/videodev2.h#150
* [`V4L2_BUF_TYPE_VIDEO_OUTPUT`](buffer.html#c.V4L.v4l2_buf_type "V4L.v4l2_buf_type"): include/uapi/linux/videodev2.h#143
* [`V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE`](buffer.html#c.V4L.v4l2_buf_type "V4L.v4l2_buf_type"): include/uapi/linux/videodev2.h#151
* [`V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY`](buffer.html#c.V4L.v4l2_buf_type "V4L.v4l2_buf_type"): include/uapi/linux/videodev2.h#149
* [`V4L2_BUF_TYPE_VIDEO_OVERLAY`](buffer.html#c.V4L.v4l2_buf_type "V4L.v4l2_buf_type"): include/uapi/linux/videodev2.h#144
* [`V4L2_COLORSPACE_470_SYSTEM_BG`](colorspaces-defs.html#c.V4L.v4l2_colorspace "V4L.v4l2_colorspace"): include/uapi/linux/videodev2.h#234
* [`V4L2_COLORSPACE_470_SYSTEM_M`](colorspaces-defs.html#c.V4L.v4l2_colorspace "V4L.v4l2_colorspace"): include/uapi/linux/videodev2.h#229
* [`V4L2_COLORSPACE_BT2020`](colorspaces-defs.html#c.V4L.v4l2_colorspace "V4L.v4l2_colorspace"): include/uapi/linux/videodev2.h#249
* [`V4L2_COLORSPACE_DCI_P3`](colorspaces-defs.html#c.V4L.v4l2_colorspace "V4L.v4l2_colorspace"): include/uapi/linux/videodev2.h#255
* [`V4L2_COLORSPACE_DEFAULT`](colorspaces-defs.html#c.V4L.v4l2_colorspace "V4L.v4l2_colorspace"): include/uapi/linux/videodev2.h#208
* [`V4L2_COLORSPACE_JPEG`](colorspaces-defs.html#c.V4L.v4l2_colorspace "V4L.v4l2_colorspace"): include/uapi/linux/videodev2.h#240
* [`V4L2_COLORSPACE_LAST`](colorspaces-defs.html#c.V4L.v4l2_colorspace "V4L.v4l2_colorspace"): include/uapi/linux/videodev2.h#262
* [`V4L2_COLORSPACE_OPRGB`](colorspaces-defs.html#c.V4L.v4l2_colorspace "V4L.v4l2_colorspace"): include/uapi/linux/videodev2.h#246
* [`V4L2_COLORSPACE_RAW`](colorspaces-defs.html#c.V4L.v4l2_colorspace "V4L.v4l2_colorspace"): include/uapi/linux/videodev2.h#252
* [`V4L2_COLORSPACE_REC709`](colorspaces-defs.html#c.V4L.v4l2_colorspace "V4L.v4l2_colorspace"): include/uapi/linux/videodev2.h#217
* [`V4L2_COLORSPACE_SMPTE170M`](colorspaces-defs.html#c.V4L.v4l2_colorspace "V4L.v4l2_colorspace"): include/uapi/linux/videodev2.h#211
* [`V4L2_COLORSPACE_SMPTE240M`](colorspaces-defs.html#c.V4L.v4l2_colorspace "V4L.v4l2_colorspace"): include/uapi/linux/videodev2.h#214
* [`V4L2_COLORSPACE_SRGB`](colorspaces-defs.html#c.V4L.v4l2_colorspace "V4L.v4l2_colorspace"): include/uapi/linux/videodev2.h#243
* [V4L2\_CTRL\_COMPOUND\_TYPES](vidioc-queryctrl.html#vidioc-queryctrl): include/uapi/linux/videodev2.h#1956
* [`V4L2_CTRL_TYPE_AREA`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1960
* [`V4L2_CTRL_TYPE_AV1_FILM_GRAIN`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1995
* [`V4L2_CTRL_TYPE_AV1_FRAME`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1994
* [`V4L2_CTRL_TYPE_AV1_SEQUENCE`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1992
* [`V4L2_CTRL_TYPE_AV1_TILE_GROUP_ENTRY`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1993
* [`V4L2_CTRL_TYPE_BITMASK`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1952
* [`V4L2_CTRL_TYPE_BOOLEAN`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1946
* [`V4L2_CTRL_TYPE_BUTTON`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1948
* [`V4L2_CTRL_TYPE_CTRL_CLASS`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1950
* [`V4L2_CTRL_TYPE_FWHT_PARAMS`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1973
* [`V4L2_CTRL_TYPE_H264_DECODE_PARAMS`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1970
* [`V4L2_CTRL_TYPE_H264_PPS`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1967
* [`V4L2_CTRL_TYPE_H264_PRED_WEIGHTS`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1971
* [`V4L2_CTRL_TYPE_H264_SCALING_MATRIX`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1968
* [`V4L2_CTRL_TYPE_H264_SLICE_PARAMS`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1969
* [`V4L2_CTRL_TYPE_H264_SPS`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1966
* [`V4L2_CTRL_TYPE_HDR10_CLL_INFO`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1963
* [`V4L2_CTRL_TYPE_HDR10_MASTERING_DISPLAY`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1964
* [`V4L2_CTRL_TYPE_HEVC_DECODE_PARAMS`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1988
* [`V4L2_CTRL_TYPE_HEVC_EXT_SPS_LT_RPS`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1990
* [`V4L2_CTRL_TYPE_HEVC_EXT_SPS_ST_RPS`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1989
* [`V4L2_CTRL_TYPE_HEVC_PPS`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1985
* [`V4L2_CTRL_TYPE_HEVC_SCALING_MATRIX`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1987
* [`V4L2_CTRL_TYPE_HEVC_SLICE_PARAMS`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1986
* [`V4L2_CTRL_TYPE_HEVC_SPS`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1984
* [`V4L2_CTRL_TYPE_INTEGER`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1945
* [`V4L2_CTRL_TYPE_INTEGER64`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1949
* [`V4L2_CTRL_TYPE_INTEGER_MENU`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1953
* [`V4L2_CTRL_TYPE_MENU`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1947
* [`V4L2_CTRL_TYPE_MPEG2_PICTURE`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1979
* [`V4L2_CTRL_TYPE_MPEG2_QUANTISATION`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1977
* [`V4L2_CTRL_TYPE_MPEG2_SEQUENCE`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1978
* [`V4L2_CTRL_TYPE_RECT`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1961
* [`V4L2_CTRL_TYPE_STRING`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1951
* [`V4L2_CTRL_TYPE_U16`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1958
* [`V4L2_CTRL_TYPE_U32`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1959
* [`V4L2_CTRL_TYPE_U8`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1957
* [`V4L2_CTRL_TYPE_VP8_FRAME`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1975
* [`V4L2_CTRL_TYPE_VP9_COMPRESSED_HDR`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1981
* [`V4L2_CTRL_TYPE_VP9_FRAME`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1982
* [`V4L2_FIELD_ALTERNATE`](field-order.html#c.V4L.v4l2_field "V4L.v4l2_field"): include/uapi/linux/videodev2.h#101
* [`V4L2_FIELD_ANY`](field-order.html#c.V4L.v4l2_field "V4L.v4l2_field"): include/uapi/linux/videodev2.h#92
* [`V4L2_FIELD_BOTTOM`](field-order.html#c.V4L.v4l2_field "V4L.v4l2_field"): include/uapi/linux/videodev2.h#95
* [`V4L2_FIELD_INTERLACED`](field-order.html#c.V4L.v4l2_field "V4L.v4l2_field"): include/uapi/linux/videodev2.h#96
* [`V4L2_FIELD_INTERLACED_BT`](field-order.html#c.V4L.v4l2_field "V4L.v4l2_field"): include/uapi/linux/videodev2.h#107
* [`V4L2_FIELD_INTERLACED_TB`](field-order.html#c.V4L.v4l2_field "V4L.v4l2_field"): include/uapi/linux/videodev2.h#104
* [`V4L2_FIELD_NONE`](field-order.html#c.V4L.v4l2_field "V4L.v4l2_field"): include/uapi/linux/videodev2.h#93
* [`V4L2_FIELD_SEQ_BT`](field-order.html#c.V4L.v4l2_field "V4L.v4l2_field"): include/uapi/linux/videodev2.h#99
* [`V4L2_FIELD_SEQ_TB`](field-order.html#c.V4L.v4l2_field "V4L.v4l2_field"): include/uapi/linux/videodev2.h#98
* [`V4L2_FIELD_TOP`](field-order.html#c.V4L.v4l2_field "V4L.v4l2_field"): include/uapi/linux/videodev2.h#94
* [`V4L2_FRMIVAL_TYPE_CONTINUOUS`](vidioc-enum-frameintervals.html#c.V4L.v4l2_frmivaltypes "V4L.v4l2_frmivaltypes"): include/uapi/linux/videodev2.h#983
* [`V4L2_FRMIVAL_TYPE_DISCRETE`](vidioc-enum-frameintervals.html#c.V4L.v4l2_frmivaltypes "V4L.v4l2_frmivaltypes"): include/uapi/linux/videodev2.h#982
* [`V4L2_FRMIVAL_TYPE_STEPWISE`](vidioc-enum-frameintervals.html#c.V4L.v4l2_frmivaltypes "V4L.v4l2_frmivaltypes"): include/uapi/linux/videodev2.h#984
* [`V4L2_FRMSIZE_TYPE_CONTINUOUS`](vidioc-enum-framesizes.html#c.V4L.v4l2_frmsizetypes "V4L.v4l2_frmsizetypes"): include/uapi/linux/videodev2.h#947
* [`V4L2_FRMSIZE_TYPE_DISCRETE`](vidioc-enum-framesizes.html#c.V4L.v4l2_frmsizetypes "V4L.v4l2_frmsizetypes"): include/uapi/linux/videodev2.h#946
* [`V4L2_FRMSIZE_TYPE_STEPWISE`](vidioc-enum-framesizes.html#c.V4L.v4l2_frmsizetypes "V4L.v4l2_frmsizetypes"): include/uapi/linux/videodev2.h#948
* [`V4L2_HSV_ENC_180`](colorspaces-defs.html#c.V4L.v4l2_hsv_encoding "V4L.v4l2_hsv_encoding"): include/uapi/linux/videodev2.h#385
* [`V4L2_HSV_ENC_256`](colorspaces-defs.html#c.V4L.v4l2_hsv_encoding "V4L.v4l2_hsv_encoding"): include/uapi/linux/videodev2.h#388
* [`V4L2_MEMORY_DMABUF`](buffer.html#c.V4L.v4l2_memory "V4L.v4l2_memory"): include/uapi/linux/videodev2.h#199
* [`V4L2_MEMORY_MMAP`](buffer.html#c.V4L.v4l2_memory "V4L.v4l2_memory"): include/uapi/linux/videodev2.h#196
* [`V4L2_MEMORY_OVERLAY`](buffer.html#c.V4L.v4l2_memory "V4L.v4l2_memory"): include/uapi/linux/videodev2.h#198
* [`V4L2_MEMORY_USERPTR`](buffer.html#c.V4L.v4l2_memory "V4L.v4l2_memory"): include/uapi/linux/videodev2.h#197
* [`V4L2_PRIORITY_BACKGROUND`](vidioc-g-priority.html#c.V4L.v4l2_priority "V4L.v4l2_priority"): include/uapi/linux/videodev2.h#436
* [`V4L2_PRIORITY_DEFAULT`](vidioc-g-priority.html#c.V4L.v4l2_priority "V4L.v4l2_priority"): include/uapi/linux/videodev2.h#439
* [`V4L2_PRIORITY_INTERACTIVE`](vidioc-g-priority.html#c.V4L.v4l2_priority "V4L.v4l2_priority"): include/uapi/linux/videodev2.h#437
* [`V4L2_PRIORITY_RECORD`](vidioc-g-priority.html#c.V4L.v4l2_priority "V4L.v4l2_priority"): include/uapi/linux/videodev2.h#438
* [`V4L2_PRIORITY_UNSET`](vidioc-g-priority.html#c.V4L.v4l2_priority "V4L.v4l2_priority"): include/uapi/linux/videodev2.h#435
* [`V4L2_QUANTIZATION_DEFAULT`](colorspaces-defs.html#c.V4L.v4l2_quantization "V4L.v4l2_quantization"): include/uapi/linux/videodev2.h#408
* [`V4L2_QUANTIZATION_FULL_RANGE`](colorspaces-defs.html#c.V4L.v4l2_quantization "V4L.v4l2_quantization"): include/uapi/linux/videodev2.h#409
* [`V4L2_QUANTIZATION_LIM_RANGE`](colorspaces-defs.html#c.V4L.v4l2_quantization "V4L.v4l2_quantization"): include/uapi/linux/videodev2.h#410
* [`V4L2_TUNER_ANALOG_TV`](vidioc-g-tuner.html#c.V4L.v4l2_tuner_type "V4L.v4l2_tuner_type"): include/uapi/linux/videodev2.h#186
* [`V4L2_TUNER_RADIO`](vidioc-g-tuner.html#c.V4L.v4l2_tuner_type "V4L.v4l2_tuner_type"): include/uapi/linux/videodev2.h#185
* [`V4L2_TUNER_RF`](vidioc-g-tuner.html#c.V4L.v4l2_tuner_type "V4L.v4l2_tuner_type"): include/uapi/linux/videodev2.h#189
* [`V4L2_TUNER_SDR`](vidioc-g-tuner.html#c.V4L.v4l2_tuner_type "V4L.v4l2_tuner_type"): include/uapi/linux/videodev2.h#188
* [`V4L2_XFER_FUNC_709`](colorspaces-defs.html#c.V4L.v4l2_xfer_func "V4L.v4l2_xfer_func"): include/uapi/linux/videodev2.h#295
* [`V4L2_XFER_FUNC_DCI_P3`](colorspaces-defs.html#c.V4L.v4l2_xfer_func "V4L.v4l2_xfer_func"): include/uapi/linux/videodev2.h#300
* [`V4L2_XFER_FUNC_DEFAULT`](colorspaces-defs.html#c.V4L.v4l2_xfer_func "V4L.v4l2_xfer_func"): include/uapi/linux/videodev2.h#294
* [`V4L2_XFER_FUNC_LAST`](colorspaces-defs.html#c.V4L.v4l2_xfer_func "V4L.v4l2_xfer_func"): include/uapi/linux/videodev2.h#307
* [`V4L2_XFER_FUNC_NONE`](colorspaces-defs.html#c.V4L.v4l2_xfer_func "V4L.v4l2_xfer_func"): include/uapi/linux/videodev2.h#299
* [`V4L2_XFER_FUNC_OPRGB`](colorspaces-defs.html#c.V4L.v4l2_xfer_func "V4L.v4l2_xfer_func"): include/uapi/linux/videodev2.h#297
* [`V4L2_XFER_FUNC_SMPTE2084`](colorspaces-defs.html#c.V4L.v4l2_xfer_func "V4L.v4l2_xfer_func"): include/uapi/linux/videodev2.h#301
* [`V4L2_XFER_FUNC_SMPTE240M`](colorspaces-defs.html#c.V4L.v4l2_xfer_func "V4L.v4l2_xfer_func"): include/uapi/linux/videodev2.h#298
* [`V4L2_XFER_FUNC_SRGB`](colorspaces-defs.html#c.V4L.v4l2_xfer_func "V4L.v4l2_xfer_func"): include/uapi/linux/videodev2.h#296
* [`V4L2_YCBCR_ENC_601`](colorspaces-defs.html#c.V4L.v4l2_ycbcr_encoding "V4L.v4l2_ycbcr_encoding"): include/uapi/linux/videodev2.h#341
* [`V4L2_YCBCR_ENC_709`](colorspaces-defs.html#c.V4L.v4l2_ycbcr_encoding "V4L.v4l2_ycbcr_encoding"): include/uapi/linux/videodev2.h#344
* [`V4L2_YCBCR_ENC_BT2020`](colorspaces-defs.html#c.V4L.v4l2_ycbcr_encoding "V4L.v4l2_ycbcr_encoding"): include/uapi/linux/videodev2.h#362
* [`V4L2_YCBCR_ENC_BT2020_CONST_LUM`](colorspaces-defs.html#c.V4L.v4l2_ycbcr_encoding "V4L.v4l2_ycbcr_encoding"): include/uapi/linux/videodev2.h#365
* [`V4L2_YCBCR_ENC_DEFAULT`](colorspaces-defs.html#c.V4L.v4l2_ycbcr_encoding "V4L.v4l2_ycbcr_encoding"): include/uapi/linux/videodev2.h#338
* [`V4L2_YCBCR_ENC_LAST`](colorspaces-defs.html#c.V4L.v4l2_ycbcr_encoding "V4L.v4l2_ycbcr_encoding"): include/uapi/linux/videodev2.h#374
* [`V4L2_YCBCR_ENC_SMPTE240M`](colorspaces-defs.html#c.V4L.v4l2_ycbcr_encoding "V4L.v4l2_ycbcr_encoding"): include/uapi/linux/videodev2.h#368
* [`V4L2_YCBCR_ENC_SYCC`](colorspaces-defs.html#c.V4L.v4l2_ycbcr_encoding "V4L.v4l2_ycbcr_encoding"): include/uapi/linux/videodev2.h#358
* [`V4L2_YCBCR_ENC_XV601`](colorspaces-defs.html#c.V4L.v4l2_ycbcr_encoding "V4L.v4l2_ycbcr_encoding"): include/uapi/linux/videodev2.h#347
* [`V4L2_YCBCR_ENC_XV709`](colorspaces-defs.html#c.V4L.v4l2_ycbcr_encoding "V4L.v4l2_ycbcr_encoding"): include/uapi/linux/videodev2.h#350

## 9.2. Enumerations

* [`v4l2_buf_type`](buffer.html#c.V4L.v4l2_buf_type "V4L.v4l2_buf_type"): include/uapi/linux/videodev2.h#141
* [`v4l2_colorspace`](colorspaces-defs.html#c.V4L.v4l2_colorspace "V4L.v4l2_colorspace"): include/uapi/linux/videodev2.h#203
* [`v4l2_ctrl_type`](vidioc-queryctrl.html#c.V4L.v4l2_ctrl_type "V4L.v4l2_ctrl_type"): include/uapi/linux/videodev2.h#1944
* [`v4l2_field`](field-order.html#c.V4L.v4l2_field "V4L.v4l2_field"): include/uapi/linux/videodev2.h#88
* [`v4l2_frmivaltypes`](vidioc-enum-frameintervals.html#c.V4L.v4l2_frmivaltypes "V4L.v4l2_frmivaltypes"): include/uapi/linux/videodev2.h#981
* [`v4l2_frmsizetypes`](vidioc-enum-framesizes.html#c.V4L.v4l2_frmsizetypes "V4L.v4l2_frmsizetypes"): include/uapi/linux/videodev2.h#945
* [`v4l2_hsv_encoding`](colorspaces-defs.html#c.V4L.v4l2_hsv_encoding "V4L.v4l2_hsv_encoding"): include/uapi/linux/videodev2.h#382
* [`v4l2_memory`](buffer.html#c.V4L.v4l2_memory "V4L.v4l2_memory"): include/uapi/linux/videodev2.h#195
* [`v4l2_priority`](vidioc-g-priority.html#c.V4L.v4l2_priority "V4L.v4l2_priority"): include/uapi/linux/videodev2.h#434
* [`v4l2_quantization`](colorspaces-defs.html#c.V4L.v4l2_quantization "V4L.v4l2_quantization"): include/uapi/linux/videodev2.h#402
* [`v4l2_tuner_type`](vidioc-g-tuner.html#c.V4L.v4l2_tuner_type "V4L.v4l2_tuner_type"): include/uapi/linux/videodev2.h#184
* [`v4l2_xfer_func`](colorspaces-defs.html#c.V4L.v4l2_xfer_func "V4L.v4l2_xfer_func"): include/uapi/linux/videodev2.h#275
* [`v4l2_ycbcr_encoding`](colorspaces-defs.html#c.V4L.v4l2_ycbcr_encoding "V4L.v4l2_ycbcr_encoding"): include/uapi/linux/videodev2.h#323

## 9.3. IOCTL Commands

* [VIDIOC\_CREATE\_BUFS](vidioc-create-bufs.html#vidioc-create-bufs): include/uapi/linux/videodev2.h#2807
* [VIDIOC\_CROPCAP](vidioc-cropcap.html#vidioc-cropcap): include/uapi/linux/videodev2.h#2771
* [VIDIOC\_DBG\_G\_CHIP\_INFO](vidioc-dbg-g-chip-info.html#vidioc-dbg-g-chip-info): include/uapi/linux/videodev2.h#2822
* [VIDIOC\_DBG\_G\_REGISTER](vidioc-dbg-g-register.html#vidioc-dbg-g-register): include/uapi/linux/videodev2.h#2799
* [VIDIOC\_DBG\_S\_REGISTER](vidioc-dbg-g-register.html#vidioc-dbg-g-register): include/uapi/linux/videodev2.h#2798
* [VIDIOC\_DECODER\_CMD](vidioc-decoder-cmd.html#vidioc-decoder-cmd): include/uapi/linux/videodev2.h#2811
* [VIDIOC\_DQBUF](vidioc-qbuf.html#vidioc-qbuf): include/uapi/linux/videodev2.h#2741
* [VIDIOC\_DQEVENT](vidioc-dqevent.html#vidioc-dqevent): include/uapi/linux/videodev2.h#2804
* [VIDIOC\_DV\_TIMINGS\_CAP](vidioc-dv-timings-cap.html#vidioc-dv-timings-cap): include/uapi/linux/videodev2.h#2815
* [VIDIOC\_ENCODER\_CMD](vidioc-encoder-cmd.html#vidioc-encoder-cmd): include/uapi/linux/videodev2.h#2790
* [VIDIOC\_ENUMAUDIO](vidioc-enumaudio.html#vidioc-enumaudio): include/uapi/linux/videodev2.h#2778
* [VIDIOC\_ENUMAUDOUT](vidioc-enumaudioout.html#vidioc-enumaudout): include/uapi/linux/videodev2.h#2779
* [VIDIOC\_ENUMINPUT](vidioc-enuminput.html#vidioc-enuminput): include/uapi/linux/videodev2.h#2749
* [VIDIOC\_ENUMOUTPUT](vidioc-enumoutput.html#vidioc-enumoutput): include/uapi/linux/videodev2.h#2764
* [VIDIOC\_ENUMSTD](vidioc-enumstd.html#vidioc-enumstd): include/uapi/linux/videodev2.h#2748
* [VIDIOC\_ENUM\_DV\_TIMINGS](vidioc-enum-dv-timings.html#vidioc-enum-dv-timings): include/uapi/linux/videodev2.h#2813
* [VIDIOC\_ENUM\_FMT](vidioc-enum-fmt.html#vidioc-enum-fmt): include/uapi/linux/videodev2.h#2731
* [VIDIOC\_ENUM\_FRAMEINTERVALS](vidioc-enum-frameintervals.html#vidioc-enum-frameintervals): include/uapi/linux/videodev2.h#2788
* [VIDIOC\_ENUM\_FRAMESIZES](vidioc-enum-framesizes.html#vidioc-enum-framesizes): include/uapi/linux/videodev2.h#2787
* [VIDIOC\_ENUM\_FREQ\_BANDS](vidioc-enum-freq-bands.html#vidioc-enum-freq-bands): include/uapi/linux/videodev2.h#2816
* [VIDIOC\_EXPBUF](vidioc-expbuf.html#vidioc-expbuf): include/uapi/linux/videodev2.h#2740
* [VIDIOC\_G\_AUDIO](vidioc-g-audio.html#vidioc-g-audio): include/uapi/linux/videodev2.h#2754
* [VIDIOC\_G\_AUDOUT](vidioc-g-audioout.html#vidioc-g-audout): include/uapi/linux/videodev2.h#2765
* [VIDIOC\_G\_CROP](vidioc-g-crop.html#vidioc-g-crop): include/uapi/linux/videodev2.h#2772
* [VIDIOC\_G\_CTRL](vidioc-g-ctrl.html#vidioc-g-ctrl): include/uapi/linux/videodev2.h#2750
* [VIDIOC\_G\_DV\_TIMINGS](vidioc-g-dv-timings.html#vidioc-g-dv-timings): include/uapi/linux/videodev2.h#2803
* [VIDIOC\_G\_EDID](vidioc-g-edid.html#vidioc-g-edid): include/uapi/linux/videodev2.h#2760
* [VIDIOC\_G\_ENC\_INDEX](vidioc-g-enc-index.html#vidioc-g-enc-index): include/uapi/linux/videodev2.h#2789
* [VIDIOC\_G\_EXT\_CTRLS](vidioc-g-ext-ctrls.html#vidioc-g-ext-ctrls): include/uapi/linux/videodev2.h#2784
* [VIDIOC\_G\_FBUF](vidioc-g-fbuf.html#vidioc-g-fbuf): include/uapi/linux/videodev2.h#2736
* [VIDIOC\_G\_FMT](vidioc-g-fmt.html#vidioc-g-fmt): include/uapi/linux/videodev2.h#2732
* [VIDIOC\_G\_FREQUENCY](vidioc-g-frequency.html#vidioc-g-frequency): include/uapi/linux/videodev2.h#2769
* [VIDIOC\_G\_INPUT](vidioc-g-input.html#vidioc-g-input): include/uapi/linux/videodev2.h#2758
* [VIDIOC\_G\_JPEGCOMP](vidioc-g-jpegcomp.html#vidioc-g-jpegcomp): include/uapi/linux/videodev2.h#2774
* [VIDIOC\_G\_MODULATOR](vidioc-g-modulator.html#vidioc-g-modulator): include/uapi/linux/videodev2.h#2767
* [VIDIOC\_G\_OUTPUT](vidioc-g-output.html#vidioc-g-output): include/uapi/linux/videodev2.h#2762
* [VIDIOC\_G\_PARM](vidioc-g-parm.html#vidioc-g-parm): include/uapi/linux/videodev2.h#2744
* [VIDIOC\_G\_PRIORITY](vidioc-g-priority.html#vidioc-g-priority): include/uapi/linux/videodev2.h#2780
* [VIDIOC\_G\_SELECTION](vidioc-g-selection.html#vidioc-g-selection): include/uapi/linux/videodev2.h#2809
* [VIDIOC\_G\_SLICED\_VBI\_CAP](vidioc-g-sliced-vbi-cap.html#vidioc-g-sliced-vbi-cap): include/uapi/linux/videodev2.h#2782
* [VIDIOC\_G\_STD](vidioc-g-std.html#vidioc-g-std): include/uapi/linux/videodev2.h#2746
* [VIDIOC\_G\_TUNER](vidioc-g-tuner.html#vidioc-g-tuner): include/uapi/linux/videodev2.h#2752
* [VIDIOC\_LOG\_STATUS](vidioc-log-status.html#vidioc-log-status): include/uapi/linux/videodev2.h#2783
* [VIDIOC\_OVERLAY](vidioc-overlay.html#vidioc-overlay): include/uapi/linux/videodev2.h#2738
* [VIDIOC\_PREPARE\_BUF](vidioc-prepare-buf.html#vidioc-prepare-buf): include/uapi/linux/videodev2.h#2808
* [VIDIOC\_QBUF](vidioc-qbuf.html#vidioc-qbuf): include/uapi/linux/videodev2.h#2739
* [VIDIOC\_QUERYBUF](vidioc-querybuf.html#vidioc-querybuf): include/uapi/linux/videodev2.h#2735
* [VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap): include/uapi/linux/videodev2.h#2730
* [VIDIOC\_QUERYCTRL](vidioc-queryctrl.html#vidioc-queryctrl): include/uapi/linux/videodev2.h#2756
* [VIDIOC\_QUERYMENU](vidioc-queryctrl.html#vidioc-queryctrl): include/uapi/linux/videodev2.h#2757
* [VIDIOC\_QUERYSTD](vidioc-querystd.html#vidioc-querystd): include/uapi/linux/videodev2.h#2776
* [VIDIOC\_QUERY\_DV\_TIMINGS](vidioc-query-dv-timings.html#vidioc-query-dv-timings): include/uapi/linux/videodev2.h#2814
* [VIDIOC\_QUERY\_EXT\_CTRL](vidioc-queryctrl.html#vidioc-queryctrl): include/uapi/linux/videodev2.h#2824
* [VIDIOC\_REMOVE\_BUFS](vidioc-remove-bufs.html#vidioc-remove-bufs): include/uapi/linux/videodev2.h#2825
* [VIDIOC\_REQBUFS](vidioc-reqbufs.html#vidioc-reqbufs): include/uapi/linux/videodev2.h#2734
* [VIDIOC\_STREAMOFF](vidioc-streamon.html#vidioc-streamon): include/uapi/linux/videodev2.h#2743
* [VIDIOC\_STREAMON](vidioc-streamon.html#vidioc-streamon): include/uapi/linux/videodev2.h#2742
* [VIDIOC\_SUBSCRIBE\_EVENT](vidioc-subscribe-event.html#vidioc-subscribe-event): include/uapi/linux/videodev2.h#2805
* [VIDIOC\_S\_AUDIO](vidioc-g-audio.html#vidioc-g-audio): include/uapi/linux/videodev2.h#2755
* [VIDIOC\_S\_AUDOUT](vidioc-g-audioout.html#vidioc-g-audout): include/uapi/linux/videodev2.h#2766
* [VIDIOC\_S\_CROP](vidioc-g-crop.html#vidioc-g-crop): include/uapi/linux/videodev2.h#2773
* [VIDIOC\_S\_CTRL](vidioc-g-ctrl.html#vidioc-g-ctrl): include/uapi/linux/videodev2.h#2751
* [VIDIOC\_S\_DV\_TIMINGS](vidioc-g-dv-timings.html#vidioc-g-dv-timings): include/uapi/linux/videodev2.h#2802
* [VIDIOC\_S\_EDID](vidioc-g-edid.html#vidioc-g-edid): include/uapi/linux/videodev2.h#2761
* [VIDIOC\_S\_EXT\_CTRLS](vidioc-g-ext-ctrls.html#vidioc-g-ext-ctrls): include/uapi/linux/videodev2.h#2785
* [VIDIOC\_S\_FBUF](vidioc-g-fbuf.html#vidioc-g-fbuf): include/uapi/linux/videodev2.h#2737
* [VIDIOC\_S\_FMT](vidioc-g-fmt.html#vidioc-g-fmt): include/uapi/linux/videodev2.h#2733
* [VIDIOC\_S\_FREQUENCY](vidioc-g-frequency.html#vidioc-g-frequency): include/uapi/linux/videodev2.h#2770
* [VIDIOC\_S\_HW\_FREQ\_SEEK](vidioc-s-hw-freq-seek.html#vidioc-s-hw-freq-seek): include/uapi/linux/videodev2.h#2801
* [VIDIOC\_S\_INPUT](vidioc-g-input.html#vidioc-g-input): include/uapi/linux/videodev2.h#2759
* [VIDIOC\_S\_JPEGCOMP](vidioc-g-jpegcomp.html#vidioc-g-jpegcomp): include/uapi/linux/videodev2.h#2775
* [VIDIOC\_S\_MODULATOR](vidioc-g-modulator.html#vidioc-g-modulator): include/uapi/linux/videodev2.h#2768
* [VIDIOC\_S\_OUTPUT](vidioc-g-output.html#vidioc-g-output): include/uapi/linux/videodev2.h#2763
* [VIDIOC\_S\_PARM](vidioc-g-parm.html#vidioc-g-parm): include/uapi/linux/videodev2.h#2745
* [VIDIOC\_S\_PRIORITY](vidioc-g-priority.html#vidioc-g-priority): include/uapi/linux/videodev2.h#2781
* [VIDIOC\_S\_SELECTION](vidioc-g-selection.html#vidioc-g-selection): include/uapi/linux/videodev2.h#2810
* [VIDIOC\_S\_STD](vidioc-g-std.html#vidioc-g-std): include/uapi/linux/videodev2.h#2747
* [VIDIOC\_S\_TUNER](vidioc-g-tuner.html#vidioc-g-tuner): include/uapi/linux/videodev2.h#2753
* [VIDIOC\_TRY\_DECODER\_CMD](vidioc-decoder-cmd.html#vidioc-decoder-cmd): include/uapi/linux/videodev2.h#2812
* [VIDIOC\_TRY\_ENCODER\_CMD](vidioc-encoder-cmd.html#vidioc-encoder-cmd): include/uapi/linux/videodev2.h#2791
* [VIDIOC\_TRY\_EXT\_CTRLS](vidioc-g-ext-ctrls.html#vidioc-g-ext-ctrls): include/uapi/linux/videodev2.h#2786
* [VIDIOC\_TRY\_FMT](vidioc-g-fmt.html#vidioc-g-fmt): include/uapi/linux/videodev2.h#2777
* [VIDIOC\_UNSUBSCRIBE\_EVENT](vidioc-subscribe-event.html#vidioc-unsubscribe-event): include/uapi/linux/videodev2.h#2806

## 9.4. Macros and Definitions

* [V4L2\_AUDCAP\_AVL](vidioc-g-audio.html#audio-capability): include/uapi/linux/videodev2.h#2191
* [V4L2\_AUDCAP\_STEREO](vidioc-g-audio.html#audio-capability): include/uapi/linux/videodev2.h#2190
* [V4L2\_AUDMODE\_AVL](vidioc-g-audio.html#audio-mode): include/uapi/linux/videodev2.h#2194
* [V4L2\_BAND\_MODULATION\_AM](vidioc-enum-freq-bands.html#band-modulation): include/uapi/linux/videodev2.h#2133
* [V4L2\_BAND\_MODULATION\_FM](vidioc-enum-freq-bands.html#band-modulation): include/uapi/linux/videodev2.h#2132
* [V4L2\_BAND\_MODULATION\_VSB](vidioc-enum-freq-bands.html#band-modulation): include/uapi/linux/videodev2.h#2131
* [V4L2\_BUF\_CAP\_SUPPORTS\_DMABUF](vidioc-reqbufs.html#v4l2-buf-cap-supports-dmabuf): include/uapi/linux/videodev2.h#1101
* [V4L2\_BUF\_CAP\_SUPPORTS\_M2M\_HOLD\_CAPTURE\_BUF](vidioc-reqbufs.html#v4l2-buf-cap-supports-m2m-hold-capture-buf): include/uapi/linux/videodev2.h#1104
* [V4L2\_BUF\_CAP\_SUPPORTS\_MAX\_NUM\_BUFFERS](vidioc-reqbufs.html#v4l2-buf-cap-supports-max-num-buffers): include/uapi/linux/videodev2.h#1106
* [V4L2\_BUF\_CAP\_SUPPORTS\_MMAP](vidioc-reqbufs.html#v4l2-buf-cap-supports-mmap): include/uapi/linux/videodev2.h#1099
* [V4L2\_BUF\_CAP\_SUPPORTS\_MMAP\_CACHE\_HINTS](vidioc-reqbufs.html#v4l2-buf-cap-supports-mmap-cache-hints): include/uapi/linux/videodev2.h#1105
* [V4L2\_BUF\_CAP\_SUPPORTS\_ORPHANED\_BUFS](vidioc-reqbufs.html#v4l2-buf-cap-supports-orphaned-bufs): include/uapi/linux/videodev2.h#1103
* [V4L2\_BUF\_CAP\_SUPPORTS\_REMOVE\_BUFS](vidioc-reqbufs.html#v4l2-buf-cap-supports-remove-bufs): include/uapi/linux/videodev2.h#1107
* [V4L2\_BUF\_CAP\_SUPPORTS\_REQUESTS](vidioc-reqbufs.html#v4l2-buf-cap-supports-requests): include/uapi/linux/videodev2.h#1102
* [V4L2\_BUF\_CAP\_SUPPORTS\_USERPTR](vidioc-reqbufs.html#v4l2-buf-cap-supports-userptr): include/uapi/linux/videodev2.h#1100
* [V4L2\_BUF\_FLAG\_BFRAME](buffer.html#v4l2-buf-flag-bframe): include/uapi/linux/videodev2.h#1234
* [V4L2\_BUF\_FLAG\_DONE](buffer.html#v4l2-buf-flag-done): include/uapi/linux/videodev2.h#1228
* [V4L2\_BUF\_FLAG\_ERROR](buffer.html#v4l2-buf-flag-error): include/uapi/linux/videodev2.h#1236
* [V4L2\_BUF\_FLAG\_IN\_REQUEST](buffer.html#v4l2-buf-flag-in-request): include/uapi/linux/videodev2.h#1238
* [V4L2\_BUF\_FLAG\_KEYFRAME](buffer.html#v4l2-buf-flag-keyframe): include/uapi/linux/videodev2.h#1230
* [V4L2\_BUF\_FLAG\_LAST](buffer.html#v4l2-buf-flag-last): include/uapi/linux/videodev2.h#1258
* [V4L2\_BUF\_FLAG\_M2M\_HOLD\_CAPTURE\_BUF](buffer.html#v4l2-buf-flag-m2m-hold-capture-buf): include/uapi/linux/videodev2.h#1242
* [V4L2\_BUF\_FLAG\_MAPPED](buffer.html#v4l2-buf-flag-mapped): include/uapi/linux/videodev2.h#1224
* [V4L2\_BUF\_FLAG\_NO\_CACHE\_CLEAN](buffer.html#v4l2-buf-flag-no-cache-clean): include/uapi/linux/videodev2.h#1247
* [V4L2\_BUF\_FLAG\_NO\_CACHE\_INVALIDATE](buffer.html#v4l2-buf-flag-no-cache-invalidate): include/uapi/linux/videodev2.h#1246
* [V4L2\_BUF\_FLAG\_PFRAME](buffer.html#v4l2-buf-flag-pframe): include/uapi/linux/videodev2.h#1232
* [V4L2\_BUF\_FLAG\_PREPARED](buffer.html#v4l2-buf-flag-prepared): include/uapi/linux/videodev2.h#1244
* [V4L2\_BUF\_FLAG\_QUEUED](buffer.html#v4l2-buf-flag-queued): include/uapi/linux/videodev2.h#1226
* [V4L2\_BUF\_FLAG\_REQUEST\_FD](buffer.html#v4l2-buf-flag-request-fd): include/uapi/linux/videodev2.h#1260
* [V4L2\_BUF\_FLAG\_TIMECODE](buffer.html#v4l2-buf-flag-timecode): include/uapi/linux/videodev2.h#1240
* [V4L2\_BUF\_FLAG\_TIMESTAMP\_COPY](buffer.html#v4l2-buf-flag-timestamp-copy): include/uapi/linux/videodev2.h#1252
* [V4L2\_BUF\_FLAG\_TIMESTAMP\_MASK](buffer.html#v4l2-buf-flag-timestamp-mask): include/uapi/linux/videodev2.h#1249
* [V4L2\_BUF\_FLAG\_TIMESTAMP\_MONOTONIC](buffer.html#v4l2-buf-flag-timestamp-monotonic): include/uapi/linux/videodev2.h#1251
* [V4L2\_BUF\_FLAG\_TIMESTAMP\_UNKNOWN](buffer.html#v4l2-buf-flag-timestamp-unknown): include/uapi/linux/videodev2.h#1250
* [V4L2\_BUF\_FLAG\_TSTAMP\_SRC\_EOF](buffer.html#v4l2-buf-flag-tstamp-src-eof): include/uapi/linux/videodev2.h#1255
* [V4L2\_BUF\_FLAG\_TSTAMP\_SRC\_MASK](buffer.html#v4l2-buf-flag-tstamp-src-mask): include/uapi/linux/videodev2.h#1254
* [V4L2\_BUF\_FLAG\_TSTAMP\_SRC\_SOE](buffer.html#v4l2-buf-flag-tstamp-src-soe): include/uapi/linux/videodev2.h#1256
* [V4L2\_CAP\_ASYNCIO](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#2841
* [V4L2\_CAP\_AUDIO](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#503
* [V4L2\_CAP\_DEVICE\_CAPS](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#521
* [V4L2\_CAP\_EDID](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#513
* [V4L2\_CAP\_EXT\_PIX\_FORMAT](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#508
* [V4L2\_CAP\_HW\_FREQ\_SEEK](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#490
* [V4L2\_CAP\_IO\_MC](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#519
* [V4L2\_CAP\_META\_CAPTURE](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#510
* [V4L2\_CAP\_META\_OUTPUT](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#515
* [V4L2\_CAP\_MODULATOR](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#505
* [V4L2\_CAP\_RADIO](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#504
* [V4L2\_CAP\_RDS\_CAPTURE](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#488
* [V4L2\_CAP\_RDS\_OUTPUT](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#491
* [V4L2\_CAP\_READWRITE](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#512
* [V4L2\_CAP\_SDR\_CAPTURE](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#507
* [V4L2\_CAP\_SDR\_OUTPUT](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#509
* [V4L2\_CAP\_SLICED\_VBI\_CAPTURE](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#486
* [V4L2\_CAP\_SLICED\_VBI\_OUTPUT](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#487
* [V4L2\_CAP\_STREAMING](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#514
* [`V4L2_CAP_TIMEPERFRAME`](vidioc-g-parm.html#c.V4L.v4l2_captureparm "V4L.v4l2_captureparm"): include/uapi/linux/videodev2.h#1359
* [V4L2\_CAP\_TOUCH](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#517
* [V4L2\_CAP\_TUNER](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#502
* [V4L2\_CAP\_VBI\_CAPTURE](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#484
* [V4L2\_CAP\_VBI\_OUTPUT](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#485
* [V4L2\_CAP\_VIDEO\_CAPTURE](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#481
* [V4L2\_CAP\_VIDEO\_CAPTURE\_MPLANE](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#494
* [V4L2\_CAP\_VIDEO\_M2M](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#500
* [V4L2\_CAP\_VIDEO\_M2M\_MPLANE](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#498
* [V4L2\_CAP\_VIDEO\_OUTPUT](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#482
* [V4L2\_CAP\_VIDEO\_OUTPUT\_MPLANE](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#496
* [V4L2\_CAP\_VIDEO\_OUTPUT\_OVERLAY](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#489
* [V4L2\_CAP\_VIDEO\_OVERLAY](vidioc-querycap.html#device-capabilities): include/uapi/linux/videodev2.h#483
* [V4L2\_CHIP\_FL\_READABLE](vidioc-dbg-g-register.html#vidioc-dbg-g-register): include/uapi/linux/videodev2.h#2674
* [V4L2\_CHIP\_FL\_WRITABLE](vidioc-dbg-g-register.html#vidioc-dbg-g-register): include/uapi/linux/videodev2.h#2675
* [V4L2\_CHIP\_MATCH\_AC97](vidioc-dbg-g-register.html#vidioc-dbg-g-register): include/uapi/linux/videodev2.h#2657
* [V4L2\_CHIP\_MATCH\_BRIDGE](vidioc-dbg-g-register.html#vidioc-dbg-g-register): include/uapi/linux/videodev2.h#2650
* [V4L2\_CHIP\_MATCH\_HOST](vidioc-dbg-g-register.html#vidioc-dbg-g-register): include/uapi/linux/videodev2.h#2654
* [V4L2\_CHIP\_MATCH\_I2C\_ADDR](vidioc-dbg-g-register.html#vidioc-dbg-g-register): include/uapi/linux/videodev2.h#2656
* [V4L2\_CHIP\_MATCH\_I2C\_DRIVER](vidioc-dbg-g-register.html#vidioc-dbg-g-register): include/uapi/linux/videodev2.h#2655
* [V4L2\_CHIP\_MATCH\_SUBDEV](vidioc-dbg-g-register.html#vidioc-dbg-g-register): include/uapi/linux/videodev2.h#2651
* [V4L2\_CID\_PRIVATE\_BASE](control.html#control): include/uapi/linux/videodev2.h#2061
* [`V4L2_COLORSPACE_ADOBERGB`](colorspaces-defs.html#c.V4L.v4l2_colorspace "V4L.v4l2_colorspace"): include/uapi/linux/videodev2.h#430
* [V4L2\_CTRL\_FLAG\_DISABLED](vidioc-queryctrl.html#control-flags): include/uapi/linux/videodev2.h#2040
* [V4L2\_CTRL\_FLAG\_DYNAMIC\_ARRAY](vidioc-queryctrl.html#control-flags): include/uapi/linux/videodev2.h#2051
* [V4L2\_CTRL\_FLAG\_EXECUTE\_ON\_WRITE](vidioc-queryctrl.html#control-flags): include/uapi/linux/videodev2.h#2049
* [V4L2\_CTRL\_FLAG\_GRABBED](vidioc-queryctrl.html#control-flags): include/uapi/linux/videodev2.h#2041
* [V4L2\_CTRL\_FLAG\_HAS\_PAYLOAD](vidioc-queryctrl.html#control-flags): include/uapi/linux/videodev2.h#2048
* [V4L2\_CTRL\_FLAG\_HAS\_WHICH\_MIN\_MAX](vidioc-queryctrl.html#control-flags): include/uapi/linux/videodev2.h#2052
* [V4L2\_CTRL\_FLAG\_INACTIVE](vidioc-queryctrl.html#control-flags): include/uapi/linux/videodev2.h#2044
* [V4L2\_CTRL\_FLAG\_MODIFY\_LAYOUT](vidioc-queryctrl.html#control-flags): include/uapi/linux/videodev2.h#2050
* [V4L2\_CTRL\_FLAG\_NEXT\_COMPOUND](control.html#control): include/uapi/linux/videodev2.h#2056
* [V4L2\_CTRL\_FLAG\_NEXT\_CTRL](control.html#control): include/uapi/linux/videodev2.h#2055
* [V4L2\_CTRL\_FLAG\_READ\_ONLY](vidioc-queryctrl.html#control-flags): include/uapi/linux/videodev2.h#2042
* [V4L2\_CTRL\_FLAG\_SLIDER](vidioc-queryctrl.html#control-flags): include/uapi/linux/videodev2.h#2045
* [V4L2\_CTRL\_FLAG\_UPDATE](vidioc-queryctrl.html#control-flags): include/uapi/linux/videodev2.h#2043
* [V4L2\_CTRL\_FLAG\_VOLATILE](vidioc-queryctrl.html#control-flags): include/uapi/linux/videodev2.h#2047
* [V4L2\_CTRL\_FLAG\_WRITE\_ONLY](vidioc-queryctrl.html#control-flags): include/uapi/linux/videodev2.h#2046
* [V4L2\_DEC\_CMD\_FLUSH](vidioc-decoder-cmd.html#decoder-cmds): include/uapi/linux/videodev2.h#2253
* [V4L2\_DEC\_CMD\_PAUSE](vidioc-decoder-cmd.html#decoder-cmds): include/uapi/linux/videodev2.h#2251
* [V4L2\_DEC\_CMD\_PAUSE\_TO\_BLACK](vidioc-decoder-cmd.html#decoder-cmds): include/uapi/linux/videodev2.h#2259
* [V4L2\_DEC\_CMD\_RESUME](vidioc-decoder-cmd.html#decoder-cmds): include/uapi/linux/videodev2.h#2252
* [V4L2\_DEC\_CMD\_START](vidioc-decoder-cmd.html#decoder-cmds): include/uapi/linux/videodev2.h#2249
* [V4L2\_DEC\_CMD\_START\_MUTE\_AUDIO](vidioc-decoder-cmd.html#decoder-cmds): include/uapi/linux/videodev2.h#2256
* [V4L2\_DEC\_CMD\_STOP](vidioc-decoder-cmd.html#decoder-cmds): include/uapi/linux/videodev2.h#2250
* [V4L2\_DEC\_CMD\_STOP\_IMMEDIATELY](vidioc-decoder-cmd.html#decoder-cmds): include/uapi/linux/videodev2.h#2263
* [V4L2\_DEC\_CMD\_STOP\_TO\_BLACK](vidioc-decoder-cmd.html#decoder-cmds): include/uapi/linux/videodev2.h#2262
* [V4L2\_DEC\_START\_FMT\_GOP](vidioc-decoder-cmd.html#decoder-cmds): include/uapi/linux/videodev2.h#2270
* [V4L2\_DEC\_START\_FMT\_NONE](vidioc-decoder-cmd.html#decoder-cmds): include/uapi/linux/videodev2.h#2268
* [V4L2\_DV\_BT\_656\_1120](vidioc-g-dv-timings.html#dv-timing-types): include/uapi/linux/videodev2.h#1724
* [V4L2\_DV\_BT\_CAP\_CUSTOM](vidioc-g-fbuf.html#framebuffer-cap): include/uapi/linux/videodev2.h#1771
* [V4L2\_DV\_BT\_CAP\_INTERLACED](vidioc-g-fbuf.html#framebuffer-cap): include/uapi/linux/videodev2.h#1765
* [V4L2\_DV\_BT\_CAP\_PROGRESSIVE](vidioc-g-fbuf.html#framebuffer-cap): include/uapi/linux/videodev2.h#1767
* [V4L2\_DV\_BT\_CAP\_REDUCED\_BLANKING](vidioc-g-fbuf.html#framebuffer-cap): include/uapi/linux/videodev2.h#1769
* [V4L2\_DV\_BT\_STD\_CEA861](vidioc-g-dv-timings.html#dv-bt-standards): include/uapi/linux/videodev2.h#1623
* [V4L2\_DV\_BT\_STD\_CVT](vidioc-g-dv-timings.html#dv-bt-standards): include/uapi/linux/videodev2.h#1625
* [V4L2\_DV\_BT\_STD\_DMT](vidioc-g-dv-timings.html#dv-bt-standards): include/uapi/linux/videodev2.h#1624
* [V4L2\_DV\_BT\_STD\_GTF](vidioc-g-dv-timings.html#dv-bt-standards): include/uapi/linux/videodev2.h#1626
* [V4L2\_DV\_BT\_STD\_SDI](vidioc-g-dv-timings.html#dv-bt-standards): include/uapi/linux/videodev2.h#1627
* [V4L2\_DV\_FL\_CAN\_DETECT\_REDUCED\_FPS](vidioc-g-dv-timings.html#dv-bt-standards): include/uapi/linux/videodev2.h#1697
* [V4L2\_DV\_FL\_CAN\_REDUCE\_FPS](vidioc-g-dv-timings.html#dv-bt-standards): include/uapi/linux/videodev2.h#1643
* [V4L2\_DV\_FL\_FIRST\_FIELD\_EXTRA\_LINE](vidioc-g-dv-timings.html#dv-bt-standards): include/uapi/linux/videodev2.h#1673
* [V4L2\_DV\_FL\_HALF\_LINE](vidioc-g-dv-timings.html#dv-bt-standards): include/uapi/linux/videodev2.h#1660
* [V4L2\_DV\_FL\_HAS\_CEA861\_VIC](vidioc-g-dv-timings.html#dv-bt-standards): include/uapi/linux/videodev2.h#1684
* [V4L2\_DV\_FL\_HAS\_HDMI\_VIC](vidioc-g-dv-timings.html#dv-bt-standards): include/uapi/linux/videodev2.h#1690
* [V4L2\_DV\_FL\_HAS\_PICTURE\_ASPECT](vidioc-g-dv-timings.html#dv-bt-standards): include/uapi/linux/videodev2.h#1679
* [V4L2\_DV\_FL\_IS\_CE\_VIDEO](vidioc-g-dv-timings.html#dv-bt-standards): include/uapi/linux/videodev2.h#1668
* [V4L2\_DV\_FL\_REDUCED\_BLANKING](vidioc-g-dv-timings.html#dv-bt-standards): include/uapi/linux/videodev2.h#1637
* [V4L2\_DV\_FL\_REDUCED\_FPS](vidioc-g-dv-timings.html#dv-bt-standards): include/uapi/linux/videodev2.h#1653
* [`V4L2_DV_HSYNC_POS_POL`](vidioc-g-dv-timings.html#c.V4L.v4l2_bt_timings "V4L.v4l2_bt_timings"): include/uapi/linux/videodev2.h#1620
* [`V4L2_DV_INTERLACED`](vidioc-g-dv-timings.html#c.V4L.v4l2_bt_timings "V4L.v4l2_bt_timings"): include/uapi/linux/videodev2.h#1616
* [`V4L2_DV_PROGRESSIVE`](vidioc-g-dv-timings.html#c.V4L.v4l2_bt_timings "V4L.v4l2_bt_timings"): include/uapi/linux/videodev2.h#1615
* [`V4L2_DV_VSYNC_POS_POL`](vidioc-g-dv-timings.html#c.V4L.v4l2_bt_timings "V4L.v4l2_bt_timings"): include/uapi/linux/videodev2.h#1619
* [V4L2\_ENC\_CMD\_PAUSE](vidioc-encoder-cmd.html#encoder-cmds): include/uapi/linux/videodev2.h#2232
* [V4L2\_ENC\_CMD\_RESUME](vidioc-encoder-cmd.html#encoder-cmds): include/uapi/linux/videodev2.h#2233
* [V4L2\_ENC\_CMD\_START](vidioc-encoder-cmd.html#encoder-cmds): include/uapi/linux/videodev2.h#2230
* [V4L2\_ENC\_CMD\_STOP](vidioc-encoder-cmd.html#encoder-cmds): include/uapi/linux/videodev2.h#2231
* [V4L2\_ENC\_CMD\_STOP\_AT\_GOP\_END](vidioc-encoder-cmd.html#encoder-flags): include/uapi/linux/videodev2.h#2236
* [`V4L2_ENC_IDX_ENTRIES`](vidioc-g-enc-index.html#c.V4L.v4l2_enc_idx "V4L.v4l2_enc_idx"): include/uapi/linux/videodev2.h#2221
* [`V4L2_ENC_IDX_FRAME_B`](vidioc-g-enc-index.html#c.V4L.v4l2_enc_idx "V4L.v4l2_enc_idx"): include/uapi/linux/videodev2.h#2210
* [`V4L2_ENC_IDX_FRAME_I`](vidioc-g-enc-index.html#c.V4L.v4l2_enc_idx "V4L.v4l2_enc_idx"): include/uapi/linux/videodev2.h#2208
* [`V4L2_ENC_IDX_FRAME_MASK`](vidioc-g-enc-index.html#c.V4L.v4l2_enc_idx "V4L.v4l2_enc_idx"): include/uapi/linux/videodev2.h#2211
* [`V4L2_ENC_IDX_FRAME_P`](vidioc-g-enc-index.html#c.V4L.v4l2_enc_idx "V4L.v4l2_enc_idx"): include/uapi/linux/videodev2.h#2209
* [V4L2\_EVENT\_ALL](vidioc-dqevent.html#event-type): include/uapi/linux/videodev2.h#2550
* [V4L2\_EVENT\_CTRL](vidioc-dqevent.html#event-type): include/uapi/linux/videodev2.h#2553
* [V4L2\_EVENT\_CTRL\_CH\_DIMENSIONS](vidioc-dqevent.html#ctrl-changes-flags): include/uapi/linux/videodev2.h#2569
* [V4L2\_EVENT\_CTRL\_CH\_FLAGS](vidioc-dqevent.html#ctrl-changes-flags): include/uapi/linux/videodev2.h#2567
* [V4L2\_EVENT\_CTRL\_CH\_RANGE](vidioc-dqevent.html#ctrl-changes-flags): include/uapi/linux/videodev2.h#2568
* [V4L2\_EVENT\_CTRL\_CH\_VALUE](vidioc-dqevent.html#ctrl-changes-flags): include/uapi/linux/videodev2.h#2566
* [V4L2\_EVENT\_EOS](vidioc-dqevent.html#event-type): include/uapi/linux/videodev2.h#2552
* [V4L2\_EVENT\_FRAME\_SYNC](vidioc-dqevent.html#event-type): include/uapi/linux/videodev2.h#2554
* [`V4L2_EVENT_MD_FL_HAVE_FRAME_SEQ`](vidioc-dqevent.html#c.V4L.v4l2_event_motion_det "V4L.v4l2_event_motion_det"): include/uapi/linux/videodev2.h#2595
* [V4L2\_EVENT\_MOTION\_DET](vidioc-dqevent.html#event-type): include/uapi/linux/videodev2.h#2556
* [V4L2\_EVENT\_PRIVATE\_START](vidioc-dqevent.html#event-type): include/uapi/linux/videodev2.h#2557
* [V4L2\_EVENT\_SOURCE\_CHANGE](vidioc-dqevent.html#event-type): include/uapi/linux/videodev2.h#2555
* [V4L2\_EVENT\_SRC\_CH\_RESOLUTION](vidioc-dqevent.html#src-changes-flags): include/uapi/linux/videodev2.h#2589
* [V4L2\_EVENT\_SUB\_FL\_ALLOW\_FEEDBACK](vidioc-subscribe-event.html#event-flags): include/uapi/linux/videodev2.h#2632
* [V4L2\_EVENT\_SUB\_FL\_SEND\_INITIAL](vidioc-subscribe-event.html#event-flags): include/uapi/linux/videodev2.h#2631
* [V4L2\_EVENT\_VSYNC](vidioc-dqevent.html#event-type): include/uapi/linux/videodev2.h#2551
* [V4L2\_FBUF\_CAP\_BITMAP\_CLIPPING](vidioc-g-fbuf.html#framebuffer-cap): include/uapi/linux/videodev2.h#1315
* [V4L2\_FBUF\_CAP\_CHROMAKEY](vidioc-g-fbuf.html#framebuffer-cap): include/uapi/linux/videodev2.h#1312
* [V4L2\_FBUF\_CAP\_EXTERNOVERLAY](vidioc-g-fbuf.html#framebuffer-cap): include/uapi/linux/videodev2.h#1311
* [V4L2\_FBUF\_CAP\_GLOBAL\_ALPHA](vidioc-g-fbuf.html#framebuffer-cap): include/uapi/linux/videodev2.h#1318
* [V4L2\_FBUF\_CAP\_LIST\_CLIPPING](vidioc-g-fbuf.html#framebuffer-cap): include/uapi/linux/videodev2.h#1314
* [V4L2\_FBUF\_CAP\_LOCAL\_ALPHA](vidioc-g-fbuf.html#framebuffer-cap): include/uapi/linux/videodev2.h#1317
* [V4L2\_FBUF\_CAP\_LOCAL\_INV\_ALPHA](vidioc-g-fbuf.html#framebuffer-cap): include/uapi/linux/videodev2.h#1319
* [V4L2\_FBUF\_CAP\_SRC\_CHROMAKEY](vidioc-g-fbuf.html#framebuffer-cap): include/uapi/linux/videodev2.h#1320
* [V4L2\_FBUF\_FLAG\_CHROMAKEY](vidioc-g-fbuf.html#framebuffer-flags): include/uapi/linux/videodev2.h#1324
* [V4L2\_FBUF\_FLAG\_GLOBAL\_ALPHA](vidioc-g-fbuf.html#framebuffer-flags): include/uapi/linux/videodev2.h#1326
* [V4L2\_FBUF\_FLAG\_LOCAL\_ALPHA](vidioc-g-fbuf.html#framebuffer-flags): include/uapi/linux/videodev2.h#1325
* [V4L2\_FBUF\_FLAG\_LOCAL\_INV\_ALPHA](vidioc-g-fbuf.html#framebuffer-flags): include/uapi/linux/videodev2.h#1327
* [V4L2\_FBUF\_FLAG\_OVERLAY](vidioc-g-fbuf.html#framebuffer-flags): include/uapi/linux/videodev2.h#1323
* [V4L2\_FBUF\_FLAG\_PRIMARY](vidioc-g-fbuf.html#framebuffer-flags): include/uapi/linux/videodev2.h#1322
* [V4L2\_FBUF\_FLAG\_SRC\_CHROMAKEY](vidioc-g-fbuf.html#framebuffer-flags): include/uapi/linux/videodev2.h#1328
* [V4L2\_FMTDESC\_FLAG\_ENUM\_ALL](vidioc-enum-fmt.html#fmtdesc-flags): include/uapi/linux/videodev2.h#939
* [V4L2\_FMT\_FLAG\_COMPRESSED](vidioc-enum-fmt.html#fmtdesc-flags): include/uapi/linux/videodev2.h#926
* [V4L2\_FMT\_FLAG\_CONTINUOUS\_BYTESTREAM](vidioc-enum-fmt.html#fmtdesc-flags): include/uapi/linux/videodev2.h#928
* [V4L2\_FMT\_FLAG\_CSC\_COLORSPACE](vidioc-enum-fmt.html#fmtdesc-flags): include/uapi/linux/videodev2.h#931
* [V4L2\_FMT\_FLAG\_CSC\_HSV\_ENC](vidioc-enum-fmt.html#fmtdesc-flags): include/uapi/linux/videodev2.h#934
* [V4L2\_FMT\_FLAG\_CSC\_QUANTIZATION](vidioc-enum-fmt.html#fmtdesc-flags): include/uapi/linux/videodev2.h#935
* [V4L2\_FMT\_FLAG\_CSC\_XFER\_FUNC](vidioc-enum-fmt.html#fmtdesc-flags): include/uapi/linux/videodev2.h#932
* [V4L2\_FMT\_FLAG\_CSC\_YCBCR\_ENC](vidioc-enum-fmt.html#fmtdesc-flags): include/uapi/linux/videodev2.h#933
* [V4L2\_FMT\_FLAG\_DYN\_RESOLUTION](vidioc-enum-fmt.html#fmtdesc-flags): include/uapi/linux/videodev2.h#929
* [V4L2\_FMT\_FLAG\_EMULATED](vidioc-enum-fmt.html#fmtdesc-flags): include/uapi/linux/videodev2.h#927
* [V4L2\_FMT\_FLAG\_ENC\_CAP\_FRAME\_INTERVAL](vidioc-enum-fmt.html#fmtdesc-flags): include/uapi/linux/videodev2.h#930
* [V4L2\_FMT\_FLAG\_META\_LINE\_BASED](vidioc-enum-fmt.html#fmtdesc-flags): include/uapi/linux/videodev2.h#936
* [V4L2\_INPUT\_TYPE\_CAMERA](vidioc-enuminput.html#input-type): include/uapi/linux/videodev2.h#1807
* [V4L2\_INPUT\_TYPE\_TOUCH](vidioc-enuminput.html#input-type): include/uapi/linux/videodev2.h#1808
* [V4L2\_INPUT\_TYPE\_TUNER](vidioc-enuminput.html#input-type): include/uapi/linux/videodev2.h#1806
* [V4L2\_IN\_CAP\_DV\_TIMINGS](vidioc-enuminput.html#input-capabilities): include/uapi/linux/videodev2.h#1837
* [V4L2\_IN\_CAP\_NATIVE\_SIZE](vidioc-enuminput.html#input-capabilities): include/uapi/linux/videodev2.h#1840
* [V4L2\_IN\_CAP\_STD](vidioc-enuminput.html#input-capabilities): include/uapi/linux/videodev2.h#1839
* [V4L2\_IN\_ST\_COLOR\_KILL](vidioc-enuminput.html#input-status): include/uapi/linux/videodev2.h#1822
* [V4L2\_IN\_ST\_HFLIP](vidioc-enuminput.html#input-status): include/uapi/linux/videodev2.h#1817
* [V4L2\_IN\_ST\_MACROVISION](vidioc-enuminput.html#input-status): include/uapi/linux/videodev2.h#1832
* [V4L2\_IN\_ST\_NO\_ACCESS](vidioc-enuminput.html#input-status): include/uapi/linux/videodev2.h#1833
* [V4L2\_IN\_ST\_NO\_CARRIER](vidioc-enuminput.html#input-status): include/uapi/linux/videodev2.h#1829
* [V4L2\_IN\_ST\_NO\_COLOR](vidioc-enuminput.html#input-status): include/uapi/linux/videodev2.h#1813
* [V4L2\_IN\_ST\_NO\_EQU](vidioc-enuminput.html#input-status): include/uapi/linux/videodev2.h#1828
* [V4L2\_IN\_ST\_NO\_H\_LOCK](vidioc-enuminput.html#input-status): include/uapi/linux/videodev2.h#1821
* [V4L2\_IN\_ST\_NO\_POWER](vidioc-enuminput.html#input-status): include/uapi/linux/videodev2.h#1811
* [V4L2\_IN\_ST\_NO\_SIGNAL](vidioc-enuminput.html#input-status): include/uapi/linux/videodev2.h#1812
* [V4L2\_IN\_ST\_NO\_STD\_LOCK](vidioc-enuminput.html#input-status): include/uapi/linux/videodev2.h#1824
* [V4L2\_IN\_ST\_NO\_SYNC](vidioc-enuminput.html#input-status): include/uapi/linux/videodev2.h#1827
* [V4L2\_IN\_ST\_NO\_V\_LOCK](vidioc-enuminput.html#input-status): include/uapi/linux/videodev2.h#1823
* [V4L2\_IN\_ST\_VFLIP](vidioc-enuminput.html#input-status): include/uapi/linux/videodev2.h#1818
* [V4L2\_IN\_ST\_VTR](vidioc-enuminput.html#input-status): include/uapi/linux/videodev2.h#1834
* [V4L2\_JPEG\_MARKER\_APP](vidioc-g-jpegcomp.html#jpeg-markers): include/uapi/linux/videodev2.h#1062
* [V4L2\_JPEG\_MARKER\_COM](vidioc-g-jpegcomp.html#jpeg-markers): include/uapi/linux/videodev2.h#1060
* [V4L2\_JPEG\_MARKER\_DHT](vidioc-g-jpegcomp.html#jpeg-markers): include/uapi/linux/videodev2.h#1057
* [V4L2\_JPEG\_MARKER\_DQT](vidioc-g-jpegcomp.html#jpeg-markers): include/uapi/linux/videodev2.h#1058
* [V4L2\_JPEG\_MARKER\_DRI](vidioc-g-jpegcomp.html#jpeg-markers): include/uapi/linux/videodev2.h#1059
* [V4L2\_MEMORY\_FLAG\_NON\_COHERENT](vidioc-reqbufs.html#v4l2-memory-flag-non-coherent): include/uapi/linux/videodev2.h#1096
* [V4L2\_META\_FMT\_C3ISP\_PARAMS](metafmt-c3-isp.html#v4l2-meta-fmt-c3isp-params): include/uapi/linux/videodev2.h#880
* [V4L2\_META\_FMT\_C3ISP\_STATS](metafmt-c3-isp.html#v4l2-meta-fmt-c3isp-stats): include/uapi/linux/videodev2.h#881
* [V4L2\_META\_FMT\_D4XX](metafmt-d4xx.html#v4l2-meta-fmt-d4xx): include/uapi/linux/videodev2.h#870
* [V4L2\_META\_FMT\_GENERIC\_8](metafmt-generic.html#v4l2-meta-fmt-generic-8): include/uapi/linux/videodev2.h#897
* [V4L2\_META\_FMT\_GENERIC\_CSI2\_10](metafmt-generic.html#v4l2-meta-fmt-generic-csi2-10): include/uapi/linux/videodev2.h#898
* [V4L2\_META\_FMT\_GENERIC\_CSI2\_12](metafmt-generic.html#v4l2-meta-fmt-generic-csi2-12): include/uapi/linux/videodev2.h#899
* [V4L2\_META\_FMT\_GENERIC\_CSI2\_14](metafmt-generic.html#v4l2-meta-fmt-generic-csi2-14): include/uapi/linux/videodev2.h#900
* [V4L2\_META\_FMT\_GENERIC\_CSI2\_16](metafmt-generic.html#v4l2-meta-fmt-generic-csi2-16): include/uapi/linux/videodev2.h#901
* [V4L2\_META\_FMT\_GENERIC\_CSI2\_20](metafmt-generic.html#v4l2-meta-fmt-generic-csi2-20): include/uapi/linux/videodev2.h#902
* [V4L2\_META\_FMT\_GENERIC\_CSI2\_24](metafmt-generic.html#v4l2-meta-fmt-generic-csi2-24): include/uapi/linux/videodev2.h#903
* [V4L2\_META\_FMT\_MALI\_C55\_PARAMS](metafmt-arm-mali-c55.html#v4l2-meta-fmt-mali-c55-params): include/uapi/linux/videodev2.h#889
* [V4L2\_META\_FMT\_MALI\_C55\_STATS](metafmt-arm-mali-c55.html#v4l2-meta-fmt-mali-c55-stats): include/uapi/linux/videodev2.h#890
* [V4L2\_META\_FMT\_RK\_ISP1\_EXT\_PARAMS](metafmt-rkisp1.html#v4l2-meta-fmt-rk-isp1-ext-params): include/uapi/linux/videodev2.h#877
* [V4L2\_META\_FMT\_RK\_ISP1\_PARAMS](metafmt-rkisp1.html#v4l2-meta-fmt-rk-isp1-params): include/uapi/linux/videodev2.h#875
* [V4L2\_META\_FMT\_RK\_ISP1\_STAT\_3A](metafmt-rkisp1.html#v4l2-meta-fmt-rk-isp1-stat-3a): include/uapi/linux/videodev2.h#876
* [V4L2\_META\_FMT\_RPI\_BE\_CFG](metafmt-pisp-be.html#v4l2-meta-fmt-rpi-be-cfg): include/uapi/linux/videodev2.h#884
* [V4L2\_META\_FMT\_RPI\_FE\_CFG](metafmt-pisp-fe.html#v4l2-meta-fmt-rpi-fe-cfg): include/uapi/linux/videodev2.h#885
* [V4L2\_META\_FMT\_RPI\_FE\_STATS](metafmt-pisp-fe.html#v4l2-meta-fmt-rpi-fe-stats): include/uapi/linux/videodev2.h#886
* [V4L2\_META\_FMT\_UVC](metafmt-uvc.html#v4l2-meta-fmt-uvc): include/uapi/linux/videodev2.h#869
* [V4L2\_META\_FMT\_UVC\_MSXU\_1\_5](metafmt-uvc-msxu-1-5.html#v4l2-meta-fmt-uvc-msxu-1-5): include/uapi/linux/videodev2.h#871
* [V4L2\_META\_FMT\_VIVID](metafmt-vivid.html#v4l2-meta-fmt-vivid): include/uapi/linux/videodev2.h#872
* [V4L2\_META\_FMT\_VSP1\_HGO](metafmt-vsp1-hgo.html#v4l2-meta-fmt-vsp1-hgo): include/uapi/linux/videodev2.h#867
* [V4L2\_META\_FMT\_VSP1\_HGT](metafmt-vsp1-hgt.html#v4l2-meta-fmt-vsp1-hgt): include/uapi/linux/videodev2.h#868
* [V4L2\_MODE\_HIGHQUALITY](vidioc-g-parm.html#parm-flags): include/uapi/linux/videodev2.h#1358
* [V4L2\_MPEG\_VBI\_IVTV\_CAPTION\_525](dev-sliced-vbi.html#itv0-line-identifier-constants): include/uapi/linux/videodev2.h#2396
* [V4L2\_MPEG\_VBI\_IVTV\_MAGIC0](dev-sliced-vbi.html#v4l2-mpeg-vbi-fmt-ivtv-magic): include/uapi/linux/videodev2.h#2414
* [V4L2\_MPEG\_VBI\_IVTV\_MAGIC1](dev-sliced-vbi.html#v4l2-mpeg-vbi-fmt-ivtv-magic): include/uapi/linux/videodev2.h#2415
* [V4L2\_MPEG\_VBI\_IVTV\_TELETEXT\_B](dev-sliced-vbi.html#itv0-line-identifier-constants): include/uapi/linux/videodev2.h#2395
* [V4L2\_MPEG\_VBI\_IVTV\_VPS](dev-sliced-vbi.html#itv0-line-identifier-constants): include/uapi/linux/videodev2.h#2398
* [V4L2\_MPEG\_VBI\_IVTV\_WSS\_625](dev-sliced-vbi.html#itv0-line-identifier-constants): include/uapi/linux/videodev2.h#2397
* [V4L2\_OUTPUT\_TYPE\_ANALOG](vidioc-enumoutput.html#output-type): include/uapi/linux/videodev2.h#1857
* [V4L2\_OUTPUT\_TYPE\_ANALOGVGAOVERLAY](vidioc-enumoutput.html#output-type): include/uapi/linux/videodev2.h#1858
* [V4L2\_OUTPUT\_TYPE\_MODULATOR](vidioc-enumoutput.html#output-type): include/uapi/linux/videodev2.h#1856
* [V4L2\_OUT\_CAP\_DV\_TIMINGS](vidioc-enumoutput.html#output-capabilities): include/uapi/linux/videodev2.h#1861
* [V4L2\_OUT\_CAP\_NATIVE\_SIZE](vidioc-enumoutput.html#output-capabilities): include/uapi/linux/videodev2.h#1864
* [V4L2\_OUT\_CAP\_STD](vidioc-enumoutput.html#output-capabilities): include/uapi/linux/videodev2.h#1863
* [V4L2\_PIX\_FMT\_ABGR32](pixfmt-rgb.html#v4l2-pix-fmt-abgr32): include/uapi/linux/videodev2.h#579
* [V4L2\_PIX\_FMT\_ABGR444](pixfmt-rgb.html#v4l2-pix-fmt-abgr444): include/uapi/linux/videodev2.h#555
* [V4L2\_PIX\_FMT\_ABGR555](pixfmt-rgb.html#v4l2-pix-fmt-abgr555): include/uapi/linux/videodev2.h#564
* [V4L2\_PIX\_FMT\_ABGR64\_12](pixfmt-rgb.html#v4l2-pix-fmt-abgr64-12): include/uapi/linux/videodev2.h#596
* [V4L2\_PIX\_FMT\_AJPG](pixfmt-reserved.html#v4l2-pix-fmt-ajpg): include/uapi/linux/videodev2.h#823
* [V4L2\_PIX\_FMT\_ARGB2101010](pixfmt-rgb.html#v4l2-pix-fmt-argb2101010): include/uapi/linux/videodev2.h#590
* [V4L2\_PIX\_FMT\_ARGB32](pixfmt-rgb.html#v4l2-pix-fmt-argb32): include/uapi/linux/videodev2.h#586
* [V4L2\_PIX\_FMT\_ARGB444](pixfmt-rgb.html#v4l2-pix-fmt-argb444): include/uapi/linux/videodev2.h#551
* [V4L2\_PIX\_FMT\_ARGB555](pixfmt-rgb.html#v4l2-pix-fmt-argb555): include/uapi/linux/videodev2.h#560
* [V4L2\_PIX\_FMT\_ARGB555X](pixfmt-rgb.html#v4l2-pix-fmt-argb555x): include/uapi/linux/videodev2.h#570
* [V4L2\_PIX\_FMT\_AV1](pixfmt-compressed.html#v4l2-pix-fmt-av1): include/uapi/linux/videodev2.h#778
* [V4L2\_PIX\_FMT\_AV1\_FRAME](pixfmt-compressed.html#v4l2-pix-fmt-av1-frame): include/uapi/linux/videodev2.h#777
* [V4L2\_PIX\_FMT\_AYUV32](pixfmt-packed-yuv.html#v4l2-pix-fmt-ayuv32): include/uapi/linux/videodev2.h#634
* [V4L2\_PIX\_FMT\_BGR24](pixfmt-rgb.html#v4l2-pix-fmt-bgr24): include/uapi/linux/videodev2.h#576
* [V4L2\_PIX\_FMT\_BGR32](pixfmt-rgb.html#v4l2-pix-fmt-bgr32): include/uapi/linux/videodev2.h#578
* [V4L2\_PIX\_FMT\_BGR48](pixfmt-rgb.html#v4l2-pix-fmt-bgr48): include/uapi/linux/videodev2.h#594
* [V4L2\_PIX\_FMT\_BGR48\_12](pixfmt-rgb.html#v4l2-pix-fmt-bgr48-12): include/uapi/linux/videodev2.h#593
* [V4L2\_PIX\_FMT\_BGR666](pixfmt-rgb.html#v4l2-pix-fmt-bgr666): include/uapi/linux/videodev2.h#575
* [V4L2\_PIX\_FMT\_BGRA32](pixfmt-rgb.html#v4l2-pix-fmt-bgra32): include/uapi/linux/videodev2.h#581
* [V4L2\_PIX\_FMT\_BGRA444](pixfmt-rgb.html#v4l2-pix-fmt-bgra444): include/uapi/linux/videodev2.h#557
* [V4L2\_PIX\_FMT\_BGRA555](pixfmt-rgb.html#v4l2-pix-fmt-bgra555): include/uapi/linux/videodev2.h#566
* [V4L2\_PIX\_FMT\_BGRX32](pixfmt-rgb.html#v4l2-pix-fmt-bgrx32): include/uapi/linux/videodev2.h#582
* [V4L2\_PIX\_FMT\_BGRX444](pixfmt-rgb.html#v4l2-pix-fmt-bgrx444): include/uapi/linux/videodev2.h#558
* [V4L2\_PIX\_FMT\_BGRX555](pixfmt-rgb.html#v4l2-pix-fmt-bgrx555): include/uapi/linux/videodev2.h#567
* [V4L2\_PIX\_FMT\_CIT\_YYVYUY](pixfmt-reserved.html#v4l2-pix-fmt-cit-yyvyuy): include/uapi/linux/videodev2.h#805
* [V4L2\_PIX\_FMT\_CNF4](pixfmt-cnf4.html#v4l2-pix-fmt-cnf4): include/uapi/linux/videodev2.h#819
* [V4L2\_PIX\_FMT\_CPIA1](pixfmt-reserved.html#v4l2-pix-fmt-cpia1): include/uapi/linux/videodev2.h#784
* [V4L2\_PIX\_FMT\_DV](pixfmt-reserved.html#v4l2-pix-fmt-dv): include/uapi/linux/videodev2.h#755
* [V4L2\_PIX\_FMT\_ET61X251](pixfmt-reserved.html#v4l2-pix-fmt-et61x251): include/uapi/linux/videodev2.h#790
* [V4L2\_PIX\_FMT\_FLAG\_PREMUL\_ALPHA](pixfmt-v4l2.html#format-flags): include/uapi/linux/videodev2.h#910
* [V4L2\_PIX\_FMT\_FLAG\_SET\_CSC](pixfmt-v4l2.html#v4l2-pix-fmt-flag-set-csc): include/uapi/linux/videodev2.h#911
* [V4L2\_PIX\_FMT\_FWHT](pixfmt-compressed.html#v4l2-pix-fmt-fwht): include/uapi/linux/videodev2.h#773
* [V4L2\_PIX\_FMT\_FWHT\_STATELESS](pixfmt-compressed.html#v4l2-pix-fmt-fwht-stateless): include/uapi/linux/videodev2.h#774
* [V4L2\_PIX\_FMT\_GREY](pixfmt-yuv-luma.html#v4l2-pix-fmt-grey): include/uapi/linux/videodev2.h#599
* [V4L2\_PIX\_FMT\_H263](pixfmt-compressed.html#v4l2-pix-fmt-h263): include/uapi/linux/videodev2.h#760
* [V4L2\_PIX\_FMT\_H264](pixfmt-compressed.html#v4l2-pix-fmt-h264): include/uapi/linux/videodev2.h#757
* [V4L2\_PIX\_FMT\_H264\_MVC](pixfmt-compressed.html#v4l2-pix-fmt-h264-mvc): include/uapi/linux/videodev2.h#759
* [V4L2\_PIX\_FMT\_H264\_NO\_SC](pixfmt-compressed.html#v4l2-pix-fmt-h264-no-sc): include/uapi/linux/videodev2.h#758
* [V4L2\_PIX\_FMT\_H264\_SLICE](pixfmt-compressed.html#v4l2-pix-fmt-h264-slice): include/uapi/linux/videodev2.h#775
* [V4L2\_PIX\_FMT\_HEVC](pixfmt-compressed.html#v4l2-pix-fmt-hevc): include/uapi/linux/videodev2.h#772
* [V4L2\_PIX\_FMT\_HEVC\_SLICE](pixfmt-compressed.html#v4l2-pix-fmt-hevc-slice): include/uapi/linux/videodev2.h#776
* [V4L2\_PIX\_FMT\_HEXTILE](pixfmt-reserved.html#v4l2-pix-fmt-hextile): include/uapi/linux/videodev2.h#824
* [V4L2\_PIX\_FMT\_HI240](pixfmt-reserved.html#v4l2-pix-fmt-hi240): include/uapi/linux/videodev2.h#820
* [`V4L2_PIX_FMT_HM12`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "V4L.v4l2_pix_format"): include/uapi/linux/videodev2.h#2835
* [V4L2\_PIX\_FMT\_HSV24](pixfmt-packed-hsv.html#v4l2-pix-fmt-hsv24): include/uapi/linux/videodev2.h#749
* [V4L2\_PIX\_FMT\_HSV32](pixfmt-packed-hsv.html#v4l2-pix-fmt-hsv32): include/uapi/linux/videodev2.h#750
* [V4L2\_PIX\_FMT\_INZI](pixfmt-inzi.html#v4l2-pix-fmt-inzi): include/uapi/linux/videodev2.h#818
* [V4L2\_PIX\_FMT\_IPU3\_SBGGR10](pixfmt-srggb10-ipu3.html#v4l2-pix-fmt-ipu3-sbggr10): include/uapi/linux/videodev2.h#827
* [V4L2\_PIX\_FMT\_IPU3\_SGBRG10](pixfmt-srggb10-ipu3.html#v4l2-pix-fmt-ipu3-sgbrg10): include/uapi/linux/videodev2.h#828
* [V4L2\_PIX\_FMT\_IPU3\_SGRBG10](pixfmt-srggb10-ipu3.html#v4l2-pix-fmt-ipu3-sgrbg10): include/uapi/linux/videodev2.h#829
* [V4L2\_PIX\_FMT\_IPU3\_SRGGB10](pixfmt-srggb10-ipu3.html#v4l2-pix-fmt-ipu3-srggb10): include/uapi/linux/videodev2.h#830
* [V4L2\_PIX\_FMT\_IPU3\_Y10](pixfmt-yuv-luma.html#v4l2-pix-fmt-ipu3-y10): include/uapi/linux/videodev2.h#612
* [V4L2\_PIX\_FMT\_JL2005BCD](pixfmt-reserved.html#v4l2-pix-fmt-jl2005bcd): include/uapi/linux/videodev2.h#797
* [V4L2\_PIX\_FMT\_JPEG](pixfmt-compressed.html#v4l2-pix-fmt-jpeg): include/uapi/linux/videodev2.h#754
* [V4L2\_PIX\_FMT\_JPGL](pixfmt-reserved.html#v4l2-pix-fmt-jpgl): include/uapi/linux/videodev2.h#807
* [V4L2\_PIX\_FMT\_KONICA420](pixfmt-reserved.html#v4l2-pix-fmt-konica420): include/uapi/linux/videodev2.h#806
* [V4L2\_PIX\_FMT\_M420](pixfmt-m420.html#v4l2-pix-fmt-m420): include/uapi/linux/videodev2.h#640
* [V4L2\_PIX\_FMT\_MJPEG](pixfmt-reserved.html#v4l2-pix-fmt-mjpeg): include/uapi/linux/videodev2.h#753
* [V4L2\_PIX\_FMT\_MM21](pixfmt-yuv-planar.html#v4l2-pix-fmt-mm21): include/uapi/linux/videodev2.h#815
* [V4L2\_PIX\_FMT\_MPEG](pixfmt-compressed.html#v4l2-pix-fmt-mpeg): include/uapi/linux/videodev2.h#756
* [V4L2\_PIX\_FMT\_MPEG1](pixfmt-compressed.html#v4l2-pix-fmt-mpeg1): include/uapi/linux/videodev2.h#761
* [V4L2\_PIX\_FMT\_MPEG2](pixfmt-compressed.html#v4l2-pix-fmt-mpeg2): include/uapi/linux/videodev2.h#762
* [V4L2\_PIX\_FMT\_MPEG2\_SLICE](pixfmt-compressed.html#v4l2-pix-fmt-mpeg2-slice): include/uapi/linux/videodev2.h#763
* [V4L2\_PIX\_FMT\_MPEG4](pixfmt-compressed.html#v4l2-pix-fmt-mpeg4): include/uapi/linux/videodev2.h#764
* [V4L2\_PIX\_FMT\_MR97310A](pixfmt-reserved.html#v4l2-pix-fmt-mr97310a): include/uapi/linux/videodev2.h#796
* [V4L2\_PIX\_FMT\_MT2110R](pixfmt-yuv-planar.html#v4l2-pix-fmt-mt2110r): include/uapi/linux/videodev2.h#817
* [V4L2\_PIX\_FMT\_MT2110T](pixfmt-yuv-planar.html#v4l2-pix-fmt-mt2110t): include/uapi/linux/videodev2.h#816
* [V4L2\_PIX\_FMT\_MT21C](pixfmt-reserved.html#v4l2-pix-fmt-mt21c): include/uapi/linux/videodev2.h#814
* [V4L2\_PIX\_FMT\_NV12](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv12): include/uapi/linux/videodev2.h#652
* [V4L2\_PIX\_FMT\_NV12M](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv12m): include/uapi/linux/videodev2.h#664
* [V4L2\_PIX\_FMT\_NV12MT](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv12mt): include/uapi/linux/videodev2.h#696
* [V4L2\_PIX\_FMT\_NV12MT\_16X16](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv12mt-16x16): include/uapi/linux/videodev2.h#697
* [V4L2\_PIX\_FMT\_NV12M\_10BE\_8L128](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv12m-10be-8l128): include/uapi/linux/videodev2.h#699
* [V4L2\_PIX\_FMT\_NV12M\_8L128](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv12m-8l128): include/uapi/linux/videodev2.h#698
* [V4L2\_PIX\_FMT\_NV12\_10BE\_8L128](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv12-10be-8l128): include/uapi/linux/videodev2.h#693
* [V4L2\_PIX\_FMT\_NV12\_16L16](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv12-16l16): include/uapi/linux/videodev2.h#688
* [V4L2\_PIX\_FMT\_NV12\_32L32](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv12-32l32): include/uapi/linux/videodev2.h#689
* [V4L2\_PIX\_FMT\_NV12\_4L4](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv12-4l4): include/uapi/linux/videodev2.h#687
* [V4L2\_PIX\_FMT\_NV12\_8L128](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv12-8l128): include/uapi/linux/videodev2.h#692
* [V4L2\_PIX\_FMT\_NV15](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv15): include/uapi/linux/videodev2.h#654
* [V4L2\_PIX\_FMT\_NV15\_4L4](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv15-4l4): include/uapi/linux/videodev2.h#690
* [V4L2\_PIX\_FMT\_NV16](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv16): include/uapi/linux/videodev2.h#655
* [V4L2\_PIX\_FMT\_NV16M](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv16m): include/uapi/linux/videodev2.h#666
* [V4L2\_PIX\_FMT\_NV20](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv20): include/uapi/linux/videodev2.h#657
* [V4L2\_PIX\_FMT\_NV21](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv21): include/uapi/linux/videodev2.h#653
* [V4L2\_PIX\_FMT\_NV21M](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv21m): include/uapi/linux/videodev2.h#665
* [V4L2\_PIX\_FMT\_NV24](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv24): include/uapi/linux/videodev2.h#658
* [V4L2\_PIX\_FMT\_NV42](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv42): include/uapi/linux/videodev2.h#659
* [V4L2\_PIX\_FMT\_NV61](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv61): include/uapi/linux/videodev2.h#656
* [V4L2\_PIX\_FMT\_NV61M](pixfmt-yuv-planar.html#v4l2-pix-fmt-nv61m): include/uapi/linux/videodev2.h#667
* [V4L2\_PIX\_FMT\_OV511](pixfmt-reserved.html#v4l2-pix-fmt-ov511): include/uapi/linux/videodev2.h#801
* [V4L2\_PIX\_FMT\_OV518](pixfmt-reserved.html#v4l2-pix-fmt-ov518): include/uapi/linux/videodev2.h#802
* [V4L2\_PIX\_FMT\_P010](pixfmt-yuv-planar.html#v4l2-pix-fmt-p010): include/uapi/linux/videodev2.h#660
* [V4L2\_PIX\_FMT\_P010\_4L4](pixfmt-yuv-planar.html#v4l2-pix-fmt-p010-4l4): include/uapi/linux/videodev2.h#691
* [V4L2\_PIX\_FMT\_P012](pixfmt-yuv-planar.html#v4l2-pix-fmt-p012): include/uapi/linux/videodev2.h#661
* [V4L2\_PIX\_FMT\_P012M](pixfmt-yuv-planar.html#v4l2-pix-fmt-p012m): include/uapi/linux/videodev2.h#668
* [V4L2\_PIX\_FMT\_PAC207](pixfmt-reserved.html#v4l2-pix-fmt-pac207): include/uapi/linux/videodev2.h#795
* [V4L2\_PIX\_FMT\_PAL8](pixfmt-indexed.html#v4l2-pix-fmt-pal8): include/uapi/linux/videodev2.h#617
* [V4L2\_PIX\_FMT\_PISP\_COMP1\_BGGR](pixfmt-srggb8-pisp-comp.html#v4l2-pix-fmt-pisp-comp1-bggr): include/uapi/linux/videodev2.h#836
* [V4L2\_PIX\_FMT\_PISP\_COMP1\_GBRG](pixfmt-srggb8-pisp-comp.html#v4l2-pix-fmt-pisp-comp1-gbrg): include/uapi/linux/videodev2.h#835
* [V4L2\_PIX\_FMT\_PISP\_COMP1\_GRBG](pixfmt-srggb8-pisp-comp.html#v4l2-pix-fmt-pisp-comp1-grbg): include/uapi/linux/videodev2.h#834
* [V4L2\_PIX\_FMT\_PISP\_COMP1\_MONO](pixfmt-srggb8-pisp-comp.html#v4l2-pix-fmt-pisp-comp1-mono): include/uapi/linux/videodev2.h#837
* [V4L2\_PIX\_FMT\_PISP\_COMP1\_RGGB](pixfmt-srggb8-pisp-comp.html#v4l2-pix-fmt-pisp-comp1-rggb): include/uapi/linux/videodev2.h#833
* [V4L2\_PIX\_FMT\_PISP\_COMP2\_BGGR](pixfmt-srggb8-pisp-comp.html#v4l2-pix-fmt-pisp-comp2-bggr): include/uapi/linux/videodev2.h#841
* [V4L2\_PIX\_FMT\_PISP\_COMP2\_GBRG](pixfmt-srggb8-pisp-comp.html#v4l2-pix-fmt-pisp-comp2-gbrg): include/uapi/linux/videodev2.h#840
* [V4L2\_PIX\_FMT\_PISP\_COMP2\_GRBG](pixfmt-srggb8-pisp-comp.html#v4l2-pix-fmt-pisp-comp2-grbg): include/uapi/linux/videodev2.h#839
* [V4L2\_PIX\_FMT\_PISP\_COMP2\_MONO](pixfmt-srggb8-pisp-comp.html#v4l2-pix-fmt-pisp-comp2-mono): include/uapi/linux/videodev2.h#842
* [V4L2\_PIX\_FMT\_PISP\_COMP2\_RGGB](pixfmt-srggb8-pisp-comp.html#v4l2-pix-fmt-pisp-comp2-rggb): include/uapi/linux/videodev2.h#838
* [V4L2\_PIX\_FMT\_PJPG](pixfmt-reserved.html#v4l2-pix-fmt-pjpg): include/uapi/linux/videodev2.h#800
* [`V4L2_PIX_FMT_PRIV_MAGIC`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "V4L.v4l2_pix_format"): include/uapi/linux/videodev2.h#907
* [V4L2\_PIX\_FMT\_PWC1](pixfmt-reserved.html#v4l2-pix-fmt-pwc1): include/uapi/linux/videodev2.h#788
* [V4L2\_PIX\_FMT\_PWC2](pixfmt-reserved.html#v4l2-pix-fmt-pwc2): include/uapi/linux/videodev2.h#789
* [V4L2\_PIX\_FMT\_QC08C](pixfmt-reserved.html#v4l2-pix-fmt-qc08c): include/uapi/linux/videodev2.h#821
* [V4L2\_PIX\_FMT\_QC10C](pixfmt-reserved.html#v4l2-pix-fmt-qc10c): include/uapi/linux/videodev2.h#822
* [V4L2\_PIX\_FMT\_RAW\_CRU10](pixfmt-rawnn-cru.html#v4l2-pix-fmt-raw-cru10): include/uapi/linux/videodev2.h#845
* [V4L2\_PIX\_FMT\_RAW\_CRU12](pixfmt-rawnn-cru.html#v4l2-pix-fmt-raw-cru12): include/uapi/linux/videodev2.h#846
* [V4L2\_PIX\_FMT\_RAW\_CRU14](pixfmt-rawnn-cru.html#v4l2-pix-fmt-raw-cru14): include/uapi/linux/videodev2.h#847
* [V4L2\_PIX\_FMT\_RAW\_CRU20](pixfmt-rawnn-cru.html#v4l2-pix-fmt-raw-cru20): include/uapi/linux/videodev2.h#848
* [V4L2\_PIX\_FMT\_RGB24](pixfmt-rgb.html#v4l2-pix-fmt-rgb24): include/uapi/linux/videodev2.h#577
* [V4L2\_PIX\_FMT\_RGB32](pixfmt-rgb.html#v4l2-pix-fmt-rgb32): include/uapi/linux/videodev2.h#583
* [V4L2\_PIX\_FMT\_RGB332](pixfmt-rgb.html#v4l2-pix-fmt-rgb332): include/uapi/linux/videodev2.h#549
* [V4L2\_PIX\_FMT\_RGB444](pixfmt-rgb.html#v4l2-pix-fmt-rgb444): include/uapi/linux/videodev2.h#550
* [V4L2\_PIX\_FMT\_RGB48](pixfmt-rgb.html#v4l2-pix-fmt-rgb48): include/uapi/linux/videodev2.h#595
* [V4L2\_PIX\_FMT\_RGB555](pixfmt-rgb.html#v4l2-pix-fmt-rgb555): include/uapi/linux/videodev2.h#559
* [V4L2\_PIX\_FMT\_RGB555X](pixfmt-rgb.html#v4l2-pix-fmt-rgb555x): include/uapi/linux/videodev2.h#569
* [V4L2\_PIX\_FMT\_RGB565](pixfmt-rgb.html#v4l2-pix-fmt-rgb565): include/uapi/linux/videodev2.h#568
* [V4L2\_PIX\_FMT\_RGB565X](pixfmt-rgb.html#v4l2-pix-fmt-rgb565x): include/uapi/linux/videodev2.h#572
* [V4L2\_PIX\_FMT\_RGBA1010102](pixfmt-rgb.html#v4l2-pix-fmt-rgba1010102): include/uapi/linux/videodev2.h#589
* [V4L2\_PIX\_FMT\_RGBA32](pixfmt-rgb.html#v4l2-pix-fmt-rgba32): include/uapi/linux/videodev2.h#584
* [V4L2\_PIX\_FMT\_RGBA444](pixfmt-rgb.html#v4l2-pix-fmt-rgba444): include/uapi/linux/videodev2.h#553
* [V4L2\_PIX\_FMT\_RGBA555](pixfmt-rgb.html#v4l2-pix-fmt-rgba555): include/uapi/linux/videodev2.h#562
* [V4L2\_PIX\_FMT\_RGBX1010102](pixfmt-rgb.html#v4l2-pix-fmt-rgbx1010102): include/uapi/linux/videodev2.h#588
* [V4L2\_PIX\_FMT\_RGBX32](pixfmt-rgb.html#v4l2-pix-fmt-rgbx32): include/uapi/linux/videodev2.h#585
* [V4L2\_PIX\_FMT\_RGBX444](pixfmt-rgb.html#v4l2-pix-fmt-rgbx444): include/uapi/linux/videodev2.h#554
* [V4L2\_PIX\_FMT\_RGBX555](pixfmt-rgb.html#v4l2-pix-fmt-rgbx555): include/uapi/linux/videodev2.h#563
* [V4L2\_PIX\_FMT\_RV30](pixfmt-compressed.html#v4l2-pix-fmt-rv30): include/uapi/linux/videodev2.h#780
* [V4L2\_PIX\_FMT\_RV40](pixfmt-compressed.html#v4l2-pix-fmt-rv40): include/uapi/linux/videodev2.h#781
* [V4L2\_PIX\_FMT\_S5C\_UYVY\_JPG](pixfmt-reserved.html#v4l2-pix-fmt-s5c-uyvy-jpg): include/uapi/linux/videodev2.h#809
* [V4L2\_PIX\_FMT\_SBGGR10](pixfmt-srggb10.html#v4l2-pix-fmt-sbggr10): include/uapi/linux/videodev2.h#706
* [V4L2\_PIX\_FMT\_SBGGR10ALAW8](pixfmt-srggb10alaw8.html#v4l2-pix-fmt-sbggr10alaw8): include/uapi/linux/videodev2.h#716
* [V4L2\_PIX\_FMT\_SBGGR10DPCM8](pixfmt-srggb10dpcm8.html#v4l2-pix-fmt-sbggr10dpcm8): include/uapi/linux/videodev2.h#721
* [V4L2\_PIX\_FMT\_SBGGR10P](pixfmt-srggb10p.html#v4l2-pix-fmt-sbggr10p): include/uapi/linux/videodev2.h#711
* [V4L2\_PIX\_FMT\_SBGGR12](pixfmt-srggb12.html#v4l2-pix-fmt-sbggr12): include/uapi/linux/videodev2.h#725
* [V4L2\_PIX\_FMT\_SBGGR12P](pixfmt-srggb12p.html#v4l2-pix-fmt-sbggr12p): include/uapi/linux/videodev2.h#730
* [V4L2\_PIX\_FMT\_SBGGR14](pixfmt-srggb14.html#v4l2-pix-fmt-sbggr14): include/uapi/linux/videodev2.h#734
* [V4L2\_PIX\_FMT\_SBGGR14P](pixfmt-srggb14p.html#v4l2-pix-fmt-sbggr14p): include/uapi/linux/videodev2.h#739
* [V4L2\_PIX\_FMT\_SBGGR16](pixfmt-srggb16.html#v4l2-pix-fmt-sbggr16): include/uapi/linux/videodev2.h#743
* [V4L2\_PIX\_FMT\_SBGGR8](pixfmt-srggb8.html#v4l2-pix-fmt-sbggr8): include/uapi/linux/videodev2.h#702
* [V4L2\_PIX\_FMT\_SE401](pixfmt-reserved.html#v4l2-pix-fmt-se401): include/uapi/linux/videodev2.h#808
* [V4L2\_PIX\_FMT\_SGBRG10](pixfmt-srggb10.html#v4l2-pix-fmt-sgbrg10): include/uapi/linux/videodev2.h#707
* [V4L2\_PIX\_FMT\_SGBRG10ALAW8](pixfmt-srggb10alaw8.html#v4l2-pix-fmt-sgbrg10alaw8): include/uapi/linux/videodev2.h#717
* [V4L2\_PIX\_FMT\_SGBRG10DPCM8](pixfmt-srggb10dpcm8.html#v4l2-pix-fmt-sgbrg10dpcm8): include/uapi/linux/videodev2.h#722
* [V4L2\_PIX\_FMT\_SGBRG10P](pixfmt-srggb10p.html#v4l2-pix-fmt-sgbrg10p): include/uapi/linux/videodev2.h#712
* [V4L2\_PIX\_FMT\_SGBRG12](pixfmt-srggb12.html#v4l2-pix-fmt-sgbrg12): include/uapi/linux/videodev2.h#726
* [V4L2\_PIX\_FMT\_SGBRG12P](pixfmt-srggb12p.html#v4l2-pix-fmt-sgbrg12p): include/uapi/linux/videodev2.h#731
* [V4L2\_PIX\_FMT\_SGBRG14](pixfmt-srggb14.html#v4l2-pix-fmt-sgbrg14): include/uapi/linux/videodev2.h#735
* [V4L2\_PIX\_FMT\_SGBRG14P](pixfmt-srggb14p.html#v4l2-pix-fmt-sgbrg14p): include/uapi/linux/videodev2.h#740
* [V4L2\_PIX\_FMT\_SGBRG16](pixfmt-srggb16.html#v4l2-pix-fmt-sgbrg16): include/uapi/linux/videodev2.h#744
* [V4L2\_PIX\_FMT\_SGBRG8](pixfmt-srggb8.html#v4l2-pix-fmt-sgbrg8): include/uapi/linux/videodev2.h#703
* [V4L2\_PIX\_FMT\_SGRBG10](pixfmt-srggb10.html#v4l2-pix-fmt-sgrbg10): include/uapi/linux/videodev2.h#708
* [V4L2\_PIX\_FMT\_SGRBG10ALAW8](pixfmt-srggb10alaw8.html#v4l2-pix-fmt-sgrbg10alaw8): include/uapi/linux/videodev2.h#718
* [V4L2\_PIX\_FMT\_SGRBG10DPCM8](pixfmt-srggb10dpcm8.html#v4l2-pix-fmt-sgrbg10dpcm8): include/uapi/linux/videodev2.h#723
* [V4L2\_PIX\_FMT\_SGRBG10P](pixfmt-srggb10p.html#v4l2-pix-fmt-sgrbg10p): include/uapi/linux/videodev2.h#713
* [V4L2\_PIX\_FMT\_SGRBG12](pixfmt-srggb12.html#v4l2-pix-fmt-sgrbg12): include/uapi/linux/videodev2.h#727
* [V4L2\_PIX\_FMT\_SGRBG12P](pixfmt-srggb12p.html#v4l2-pix-fmt-sgrbg12p): include/uapi/linux/videodev2.h#732
* [V4L2\_PIX\_FMT\_SGRBG14](pixfmt-srggb14.html#v4l2-pix-fmt-sgrbg14): include/uapi/linux/videodev2.h#736
* [V4L2\_PIX\_FMT\_SGRBG14P](pixfmt-srggb14p.html#v4l2-pix-fmt-sgrbg14p): include/uapi/linux/videodev2.h#741
* [V4L2\_PIX\_FMT\_SGRBG16](pixfmt-srggb16.html#v4l2-pix-fmt-sgrbg16): include/uapi/linux/videodev2.h#745
* [V4L2\_PIX\_FMT\_SGRBG8](pixfmt-srggb8.html#v4l2-pix-fmt-sgrbg8): include/uapi/linux/videodev2.h#704
* [V4L2\_PIX\_FMT\_SN9C10X](pixfmt-reserved.html#v4l2-pix-fmt-sn9c10x): include/uapi/linux/videodev2.h#786
* [V4L2\_PIX\_FMT\_SN9C2028](pixfmt-reserved.html#v4l2-pix-fmt-sn9c2028): include/uapi/linux/videodev2.h#798
* [V4L2\_PIX\_FMT\_SN9C20X\_I420](pixfmt-reserved.html#v4l2-pix-fmt-sn9c20x-i420): include/uapi/linux/videodev2.h#787
* [V4L2\_PIX\_FMT\_SPCA501](pixfmt-reserved.html#v4l2-pix-fmt-spca501): include/uapi/linux/videodev2.h#791
* [V4L2\_PIX\_FMT\_SPCA505](pixfmt-reserved.html#v4l2-pix-fmt-spca505): include/uapi/linux/videodev2.h#792
* [V4L2\_PIX\_FMT\_SPCA508](pixfmt-reserved.html#v4l2-pix-fmt-spca508): include/uapi/linux/videodev2.h#793
* [V4L2\_PIX\_FMT\_SPCA561](pixfmt-reserved.html#v4l2-pix-fmt-spca561): include/uapi/linux/videodev2.h#794
* [V4L2\_PIX\_FMT\_SPK](pixfmt-compressed.html#v4l2-pix-fmt-spk): include/uapi/linux/videodev2.h#779
* [V4L2\_PIX\_FMT\_SQ905C](pixfmt-reserved.html#v4l2-pix-fmt-sq905c): include/uapi/linux/videodev2.h#799
* [V4L2\_PIX\_FMT\_SRGGB10](pixfmt-srggb10.html#v4l2-pix-fmt-srggb10): include/uapi/linux/videodev2.h#709
* [V4L2\_PIX\_FMT\_SRGGB10ALAW8](pixfmt-srggb10alaw8.html#v4l2-pix-fmt-srggb10alaw8): include/uapi/linux/videodev2.h#719
* [V4L2\_PIX\_FMT\_SRGGB10DPCM8](pixfmt-srggb10dpcm8.html#v4l2-pix-fmt-srggb10dpcm8): include/uapi/linux/videodev2.h#724
* [V4L2\_PIX\_FMT\_SRGGB10P](pixfmt-srggb10p.html#v4l2-pix-fmt-srggb10p): include/uapi/linux/videodev2.h#714
* [V4L2\_PIX\_FMT\_SRGGB12](pixfmt-srggb12.html#v4l2-pix-fmt-srggb12): include/uapi/linux/videodev2.h#728
* [V4L2\_PIX\_FMT\_SRGGB12P](pixfmt-srggb12p.html#v4l2-pix-fmt-srggb12p): include/uapi/linux/videodev2.h#733
* [V4L2\_PIX\_FMT\_SRGGB14](pixfmt-srggb14.html#v4l2-pix-fmt-srggb14): include/uapi/linux/videodev2.h#737
* [V4L2\_PIX\_FMT\_SRGGB14P](pixfmt-srggb14p.html#v4l2-pix-fmt-srggb14p): include/uapi/linux/videodev2.h#742
* [V4L2\_PIX\_FMT\_SRGGB16](pixfmt-srggb16.html#v4l2-pix-fmt-srggb16): include/uapi/linux/videodev2.h#746
* [V4L2\_PIX\_FMT\_SRGGB8](pixfmt-srggb8.html#v4l2-pix-fmt-srggb8): include/uapi/linux/videodev2.h#705
* [V4L2\_PIX\_FMT\_STV0680](pixfmt-reserved.html#v4l2-pix-fmt-stv0680): include/uapi/linux/videodev2.h#803
* [`V4L2_PIX_FMT_SUNXI_TILED_NV12`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "V4L.v4l2_pix_format"): include/uapi/linux/videodev2.h#2836
* [V4L2\_PIX\_FMT\_TM6000](pixfmt-reserved.html#v4l2-pix-fmt-tm6000): include/uapi/linux/videodev2.h#804
* [V4L2\_PIX\_FMT\_UV8](pixfmt-uv8.html#v4l2-pix-fmt-uv8): include/uapi/linux/videodev2.h#620
* [V4L2\_PIX\_FMT\_UYVY](pixfmt-packed-yuv.html#v4l2-pix-fmt-uyvy): include/uapi/linux/videodev2.h#626
* [V4L2\_PIX\_FMT\_VC1\_ANNEX\_G](pixfmt-compressed.html#v4l2-pix-fmt-vc1-annex-g): include/uapi/linux/videodev2.h#766
* [V4L2\_PIX\_FMT\_VC1\_ANNEX\_L](pixfmt-compressed.html#v4l2-pix-fmt-vc1-annex-l): include/uapi/linux/videodev2.h#767
* [V4L2\_PIX\_FMT\_VP8](pixfmt-compressed.html#v4l2-pix-fmt-vp8): include/uapi/linux/videodev2.h#768
* [V4L2\_PIX\_FMT\_VP8\_FRAME](pixfmt-compressed.html#v4l2-pix-fmt-vp8-frame): include/uapi/linux/videodev2.h#769
* [V4L2\_PIX\_FMT\_VP9](pixfmt-compressed.html#v4l2-pix-fmt-vp9): include/uapi/linux/videodev2.h#770
* [V4L2\_PIX\_FMT\_VP9\_FRAME](pixfmt-compressed.html#v4l2-pix-fmt-vp9-frame): include/uapi/linux/videodev2.h#771
* [V4L2\_PIX\_FMT\_VUYA32](pixfmt-packed-yuv.html#v4l2-pix-fmt-vuya32): include/uapi/linux/videodev2.h#636
* [V4L2\_PIX\_FMT\_VUYX32](pixfmt-packed-yuv.html#v4l2-pix-fmt-vuyx32): include/uapi/linux/videodev2.h#637
* [V4L2\_PIX\_FMT\_VYUY](pixfmt-packed-yuv.html#v4l2-pix-fmt-vyuy): include/uapi/linux/videodev2.h#627
* [V4L2\_PIX\_FMT\_WNVA](pixfmt-reserved.html#v4l2-pix-fmt-wnva): include/uapi/linux/videodev2.h#785
* [V4L2\_PIX\_FMT\_XBGR32](pixfmt-rgb.html#v4l2-pix-fmt-xbgr32): include/uapi/linux/videodev2.h#580
* [V4L2\_PIX\_FMT\_XBGR444](pixfmt-rgb.html#v4l2-pix-fmt-xbgr444): include/uapi/linux/videodev2.h#556
* [V4L2\_PIX\_FMT\_XBGR555](pixfmt-rgb.html#v4l2-pix-fmt-xbgr555): include/uapi/linux/videodev2.h#565
* [V4L2\_PIX\_FMT\_XRGB32](pixfmt-rgb.html#v4l2-pix-fmt-xrgb32): include/uapi/linux/videodev2.h#587
* [V4L2\_PIX\_FMT\_XRGB444](pixfmt-rgb.html#v4l2-pix-fmt-xrgb444): include/uapi/linux/videodev2.h#552
* [V4L2\_PIX\_FMT\_XRGB555](pixfmt-rgb.html#v4l2-pix-fmt-xrgb555): include/uapi/linux/videodev2.h#561
* [V4L2\_PIX\_FMT\_XRGB555X](pixfmt-rgb.html#v4l2-pix-fmt-xrgb555x): include/uapi/linux/videodev2.h#571
* [V4L2\_PIX\_FMT\_XVID](pixfmt-compressed.html#v4l2-pix-fmt-xvid): include/uapi/linux/videodev2.h#765
* [V4L2\_PIX\_FMT\_XYUV32](pixfmt-packed-yuv.html#v4l2-pix-fmt-xyuv32): include/uapi/linux/videodev2.h#635
* [V4L2\_PIX\_FMT\_Y012](pixfmt-yuv-luma.html#v4l2-pix-fmt-y012): include/uapi/linux/videodev2.h#604
* [V4L2\_PIX\_FMT\_Y10](pixfmt-yuv-luma.html#v4l2-pix-fmt-y10): include/uapi/linux/videodev2.h#602
* [V4L2\_PIX\_FMT\_Y10BPACK](pixfmt-yuv-luma.html#v4l2-pix-fmt-y10bpack): include/uapi/linux/videodev2.h#610
* [V4L2\_PIX\_FMT\_Y10P](pixfmt-yuv-luma.html#v4l2-pix-fmt-y10p): include/uapi/linux/videodev2.h#611
* [V4L2\_PIX\_FMT\_Y12](pixfmt-yuv-luma.html#v4l2-pix-fmt-y12): include/uapi/linux/videodev2.h#603
* [V4L2\_PIX\_FMT\_Y12I](pixfmt-y12i.html#v4l2-pix-fmt-y12i): include/uapi/linux/videodev2.h#811
* [V4L2\_PIX\_FMT\_Y12P](pixfmt-yuv-luma.html#v4l2-pix-fmt-y12p): include/uapi/linux/videodev2.h#613
* [V4L2\_PIX\_FMT\_Y14](pixfmt-yuv-luma.html#v4l2-pix-fmt-y14): include/uapi/linux/videodev2.h#605
* [V4L2\_PIX\_FMT\_Y14P](pixfmt-yuv-luma.html#v4l2-pix-fmt-y14p): include/uapi/linux/videodev2.h#614
* [V4L2\_PIX\_FMT\_Y16](pixfmt-yuv-luma.html#v4l2-pix-fmt-y16): include/uapi/linux/videodev2.h#606
* [V4L2\_PIX\_FMT\_Y16I](pixfmt-y16i.html#v4l2-pix-fmt-y16i): include/uapi/linux/videodev2.h#812
* [V4L2\_PIX\_FMT\_Y16\_BE](pixfmt-yuv-luma.html#v4l2-pix-fmt-y16-be): include/uapi/linux/videodev2.h#607
* [V4L2\_PIX\_FMT\_Y210](pixfmt-packed-yuv.html#v4l2-pix-fmt-y210): include/uapi/linux/videodev2.h#647
* [V4L2\_PIX\_FMT\_Y212](pixfmt-packed-yuv.html#v4l2-pix-fmt-y212): include/uapi/linux/videodev2.h#648
* [V4L2\_PIX\_FMT\_Y216](pixfmt-packed-yuv.html#v4l2-pix-fmt-y216): include/uapi/linux/videodev2.h#649
* [V4L2\_PIX\_FMT\_Y4](pixfmt-reserved.html#v4l2-pix-fmt-y4): include/uapi/linux/videodev2.h#600
* [V4L2\_PIX\_FMT\_Y41P](pixfmt-packed-yuv.html#v4l2-pix-fmt-y41p): include/uapi/linux/videodev2.h#628
* [V4L2\_PIX\_FMT\_Y6](pixfmt-reserved.html#v4l2-pix-fmt-y6): include/uapi/linux/videodev2.h#601
* [V4L2\_PIX\_FMT\_Y8I](pixfmt-y8i.html#v4l2-pix-fmt-y8i): include/uapi/linux/videodev2.h#810
* [V4L2\_PIX\_FMT\_YUV24](pixfmt-packed-yuv.html#v4l2-pix-fmt-yuv24): include/uapi/linux/videodev2.h#632
* [V4L2\_PIX\_FMT\_YUV32](pixfmt-packed-yuv.html#v4l2-pix-fmt-yuv32): include/uapi/linux/videodev2.h#633
* [V4L2\_PIX\_FMT\_YUV410](pixfmt-yuv-planar.html#v4l2-pix-fmt-yuv410): include/uapi/linux/videodev2.h#671
* [V4L2\_PIX\_FMT\_YUV411P](pixfmt-yuv-planar.html#v4l2-pix-fmt-yuv411p): include/uapi/linux/videodev2.h#673
* [V4L2\_PIX\_FMT\_YUV420](pixfmt-yuv-planar.html#v4l2-pix-fmt-yuv420): include/uapi/linux/videodev2.h#674
* [V4L2\_PIX\_FMT\_YUV420M](pixfmt-yuv-planar.html#v4l2-pix-fmt-yuv420m): include/uapi/linux/videodev2.h#679
* [V4L2\_PIX\_FMT\_YUV422M](pixfmt-yuv-planar.html#v4l2-pix-fmt-yuv422m): include/uapi/linux/videodev2.h#681
* [V4L2\_PIX\_FMT\_YUV422P](pixfmt-yuv-planar.html#v4l2-pix-fmt-yuv422p): include/uapi/linux/videodev2.h#676
* [V4L2\_PIX\_FMT\_YUV444](pixfmt-packed-yuv.html#v4l2-pix-fmt-yuv444): include/uapi/linux/videodev2.h#629
* [V4L2\_PIX\_FMT\_YUV444M](pixfmt-yuv-planar.html#v4l2-pix-fmt-yuv444m): include/uapi/linux/videodev2.h#683
* [V4L2\_PIX\_FMT\_YUV48\_12](pixfmt-packed-yuv.html#v4l2-pix-fmt-yuv48-12): include/uapi/linux/videodev2.h#641
* [V4L2\_PIX\_FMT\_YUV555](pixfmt-packed-yuv.html#v4l2-pix-fmt-yuv555): include/uapi/linux/videodev2.h#630
* [V4L2\_PIX\_FMT\_YUV565](pixfmt-packed-yuv.html#v4l2-pix-fmt-yuv565): include/uapi/linux/videodev2.h#631
* [V4L2\_PIX\_FMT\_YUVA32](pixfmt-packed-yuv.html#v4l2-pix-fmt-yuva32): include/uapi/linux/videodev2.h#638
* [V4L2\_PIX\_FMT\_YUVX32](pixfmt-packed-yuv.html#v4l2-pix-fmt-yuvx32): include/uapi/linux/videodev2.h#639
* [V4L2\_PIX\_FMT\_YUYV](pixfmt-packed-yuv.html#v4l2-pix-fmt-yuyv): include/uapi/linux/videodev2.h#623
* [V4L2\_PIX\_FMT\_YVU410](pixfmt-yuv-planar.html#v4l2-pix-fmt-yvu410): include/uapi/linux/videodev2.h#672
* [V4L2\_PIX\_FMT\_YVU420](pixfmt-yuv-planar.html#v4l2-pix-fmt-yvu420): include/uapi/linux/videodev2.h#675
* [V4L2\_PIX\_FMT\_YVU420M](pixfmt-yuv-planar.html#v4l2-pix-fmt-yvu420m): include/uapi/linux/videodev2.h#680
* [V4L2\_PIX\_FMT\_YVU422M](pixfmt-yuv-planar.html#v4l2-pix-fmt-yvu422m): include/uapi/linux/videodev2.h#682
* [V4L2\_PIX\_FMT\_YVU444M](pixfmt-yuv-planar.html#v4l2-pix-fmt-yvu444m): include/uapi/linux/videodev2.h#684
* [V4L2\_PIX\_FMT\_YVYU](pixfmt-packed-yuv.html#v4l2-pix-fmt-yvyu): include/uapi/linux/videodev2.h#625
* [V4L2\_PIX\_FMT\_YYUV](pixfmt-reserved.html#v4l2-pix-fmt-yyuv): include/uapi/linux/videodev2.h#624
* [V4L2\_PIX\_FMT\_Z16](pixfmt-z16.html#v4l2-pix-fmt-z16): include/uapi/linux/videodev2.h#813
* [V4L2\_RDS\_BLOCK\_A](dev-rds.html#v4l2-rds-block): include/uapi/linux/videodev2.h#2168
* [V4L2\_RDS\_BLOCK\_B](dev-rds.html#v4l2-rds-block): include/uapi/linux/videodev2.h#2169
* [V4L2\_RDS\_BLOCK\_C](dev-rds.html#v4l2-rds-block): include/uapi/linux/videodev2.h#2170
* [V4L2\_RDS\_BLOCK\_CORRECTED](dev-rds.html#v4l2-rds-block): include/uapi/linux/videodev2.h#2175
* [V4L2\_RDS\_BLOCK\_C\_ALT](dev-rds.html#v4l2-rds-block): include/uapi/linux/videodev2.h#2172
* [V4L2\_RDS\_BLOCK\_D](dev-rds.html#v4l2-rds-block): include/uapi/linux/videodev2.h#2171
* [V4L2\_RDS\_BLOCK\_ERROR](dev-rds.html#v4l2-rds-block): include/uapi/linux/videodev2.h#2176
* [V4L2\_RDS\_BLOCK\_INVALID](dev-rds.html#v4l2-rds-block): include/uapi/linux/videodev2.h#2173
* [V4L2\_RDS\_BLOCK\_MSK](dev-rds.html#v4l2-rds-block): include/uapi/linux/videodev2.h#2167
* [V4L2\_SDR\_FMT\_CS14LE](pixfmt-sdr-cs14le.html#v4l2-sdr-fmt-cs14le): include/uapi/linux/videodev2.h#854
* [V4L2\_SDR\_FMT\_CS8](pixfmt-sdr-cs08.html#v4l2-sdr-fmt-cs8): include/uapi/linux/videodev2.h#853
* [V4L2\_SDR\_FMT\_CU16LE](pixfmt-sdr-cu16le.html#v4l2-sdr-fmt-cu16le): include/uapi/linux/videodev2.h#852
* [V4L2\_SDR\_FMT\_CU8](pixfmt-sdr-cu08.html#v4l2-sdr-fmt-cu8): include/uapi/linux/videodev2.h#851
* [V4L2\_SDR\_FMT\_PCU16BE](pixfmt-sdr-pcu16be.html#v4l2-sdr-fmt-pcu16be): include/uapi/linux/videodev2.h#856
* [V4L2\_SDR\_FMT\_PCU18BE](pixfmt-sdr-pcu18be.html#v4l2-sdr-fmt-pcu18be): include/uapi/linux/videodev2.h#857
* [V4L2\_SDR\_FMT\_PCU20BE](pixfmt-sdr-pcu20be.html#v4l2-sdr-fmt-pcu20be): include/uapi/linux/videodev2.h#858
* [V4L2\_SDR\_FMT\_RU12LE](pixfmt-sdr-ru12le.html#v4l2-sdr-fmt-ru12le): include/uapi/linux/videodev2.h#855
* [V4L2\_SLICED\_CAPTION\_525](vidioc-g-sliced-vbi-cap.html#vbi-services): include/uapi/linux/videodev2.h#2352
* [V4L2\_SLICED\_TELETEXT\_B](vidioc-g-sliced-vbi-cap.html#vbi-services): include/uapi/linux/videodev2.h#2348
* [V4L2\_SLICED\_VBI\_525](vidioc-g-sliced-vbi-cap.html#vbi-services): include/uapi/linux/videodev2.h#2356
* [V4L2\_SLICED\_VBI\_625](vidioc-g-sliced-vbi-cap.html#vbi-services): include/uapi/linux/videodev2.h#2357
* [V4L2\_SLICED\_VPS](vidioc-g-sliced-vbi-cap.html#vbi-services): include/uapi/linux/videodev2.h#2350
* [V4L2\_SLICED\_WSS\_625](vidioc-g-sliced-vbi-cap.html#vbi-services): include/uapi/linux/videodev2.h#2354
* [V4L2\_STD\_525\_60](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1526
* [V4L2\_STD\_625\_50](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1531
* [V4L2\_STD\_ALL](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1540
* [V4L2\_STD\_ATSC](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1536
* [V4L2\_STD\_ATSC\_16\_VSB](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1448
* [V4L2\_STD\_ATSC\_8\_VSB](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1447
* [V4L2\_STD\_B](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1499
* [V4L2\_STD\_BG](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1512
* [V4L2\_STD\_DK](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1510
* [V4L2\_STD\_G](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1502
* [V4L2\_STD\_GH](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1508
* [V4L2\_STD\_H](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1504
* [V4L2\_STD\_L](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1506
* [V4L2\_STD\_MN](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1514
* [V4L2\_STD\_MTS](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1520
* [V4L2\_STD\_NTSC](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1468
* [V4L2\_STD\_NTSC\_443](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1434
* [V4L2\_STD\_NTSC\_M](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1432
* [V4L2\_STD\_NTSC\_M\_JP](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1433
* [V4L2\_STD\_NTSC\_M\_KR](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1435
* [V4L2\_STD\_PAL](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1494
* [V4L2\_STD\_PAL\_60](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1430
* [V4L2\_STD\_PAL\_B](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1418
* [V4L2\_STD\_PAL\_B1](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1419
* [V4L2\_STD\_PAL\_BG](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1483
* [V4L2\_STD\_PAL\_D](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1423
* [V4L2\_STD\_PAL\_D1](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1424
* [V4L2\_STD\_PAL\_DK](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1486
* [V4L2\_STD\_PAL\_G](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1420
* [V4L2\_STD\_PAL\_H](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1421
* [V4L2\_STD\_PAL\_I](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1422
* [V4L2\_STD\_PAL\_K](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1425
* [V4L2\_STD\_PAL\_M](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1427
* [V4L2\_STD\_PAL\_N](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1428
* [V4L2\_STD\_PAL\_Nc](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1429
* [V4L2\_STD\_SECAM](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1476
* [V4L2\_STD\_SECAM\_B](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1437
* [V4L2\_STD\_SECAM\_D](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1438
* [V4L2\_STD\_SECAM\_DK](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1472
* [V4L2\_STD\_SECAM\_G](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1439
* [V4L2\_STD\_SECAM\_H](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1440
* [V4L2\_STD\_SECAM\_K](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1441
* [V4L2\_STD\_SECAM\_K1](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1442
* [V4L2\_STD\_SECAM\_L](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1443
* [V4L2\_STD\_SECAM\_LC](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1444
* [V4L2\_STD\_UNKNOWN](vidioc-enumstd.html#v4l2-std-id): include/uapi/linux/videodev2.h#1539
* [V4L2\_TCH\_FMT\_DELTA\_TD08](pixfmt-tch-td08.html#v4l2-tch-fmt-delta-td08): include/uapi/linux/videodev2.h#862
* [V4L2\_TCH\_FMT\_DELTA\_TD16](pixfmt-tch-td16.html#v4l2-tch-fmt-delta-td16): include/uapi/linux/videodev2.h#861
* [V4L2\_TCH\_FMT\_TU08](pixfmt-tch-tu08.html#v4l2-tch-fmt-tu08): include/uapi/linux/videodev2.h#864
* [V4L2\_TCH\_FMT\_TU16](pixfmt-tch-tu16.html#v4l2-tch-fmt-tu16): include/uapi/linux/videodev2.h#863
* [V4L2\_TC\_FLAG\_COLORFRAME](buffer.html#timecode-flags): include/uapi/linux/videodev2.h#1030
* [V4L2\_TC\_FLAG\_DROPFRAME](buffer.html#timecode-flags): include/uapi/linux/videodev2.h#1029
* [V4L2\_TC\_TYPE\_24FPS](buffer.html#timecode-type): include/uapi/linux/videodev2.h#1022
* [V4L2\_TC\_TYPE\_25FPS](buffer.html#timecode-type): include/uapi/linux/videodev2.h#1023
* [V4L2\_TC\_TYPE\_30FPS](buffer.html#timecode-type): include/uapi/linux/videodev2.h#1024
* [V4L2\_TC\_TYPE\_50FPS](buffer.html#timecode-type): include/uapi/linux/videodev2.h#1025
* [V4L2\_TC\_TYPE\_60FPS](buffer.html#timecode-type): include/uapi/linux/videodev2.h#1026
* [V4L2\_TC\_USERBITS\_8BITCHARS](buffer.html#timecode-flags): include/uapi/linux/videodev2.h#1033
* [V4L2\_TC\_USERBITS\_USERDEFINED](buffer.html#timecode-flags): include/uapi/linux/videodev2.h#1032
* [V4L2\_TC\_USERBITS\_field](buffer.html#timecode-flags): include/uapi/linux/videodev2.h#1031
* [V4L2\_TUNER\_CAP\_1HZ](vidioc-g-tuner.html#tuner-capability): include/uapi/linux/videodev2.h#2106
* [V4L2\_TUNER\_CAP\_FREQ\_BANDS](vidioc-g-tuner.html#tuner-capability): include/uapi/linux/videodev2.h#2104
* [V4L2\_TUNER\_CAP\_HWSEEK\_BOUNDED](vidioc-g-tuner.html#tuner-capability): include/uapi/linux/videodev2.h#2095
* [V4L2\_TUNER\_CAP\_HWSEEK\_PROG\_LIM](vidioc-g-tuner.html#tuner-capability): include/uapi/linux/videodev2.h#2105
* [V4L2\_TUNER\_CAP\_HWSEEK\_WRAP](vidioc-g-tuner.html#tuner-capability): include/uapi/linux/videodev2.h#2096
* [V4L2\_TUNER\_CAP\_LANG1](vidioc-g-tuner.html#tuner-capability): include/uapi/linux/videodev2.h#2100
* [V4L2\_TUNER\_CAP\_LANG2](vidioc-g-tuner.html#tuner-capability): include/uapi/linux/videodev2.h#2098
* [V4L2\_TUNER\_CAP\_LOW](vidioc-g-tuner.html#tuner-capability): include/uapi/linux/videodev2.h#2093
* [V4L2\_TUNER\_CAP\_NORM](vidioc-g-tuner.html#tuner-capability): include/uapi/linux/videodev2.h#2094
* [V4L2\_TUNER\_CAP\_RDS](vidioc-g-tuner.html#tuner-capability): include/uapi/linux/videodev2.h#2101
* [V4L2\_TUNER\_CAP\_RDS\_BLOCK\_IO](vidioc-g-tuner.html#tuner-capability): include/uapi/linux/videodev2.h#2102
* [V4L2\_TUNER\_CAP\_RDS\_CONTROLS](vidioc-g-tuner.html#tuner-capability): include/uapi/linux/videodev2.h#2103
* [V4L2\_TUNER\_CAP\_SAP](vidioc-g-tuner.html#tuner-capability): include/uapi/linux/videodev2.h#2099
* [V4L2\_TUNER\_CAP\_STEREO](vidioc-g-tuner.html#tuner-capability): include/uapi/linux/videodev2.h#2097
* [V4L2\_TUNER\_MODE\_LANG1](vidioc-g-tuner.html#tuner-audmode): include/uapi/linux/videodev2.h#2121
* [V4L2\_TUNER\_MODE\_LANG1\_LANG2](vidioc-g-tuner.html#tuner-audmode): include/uapi/linux/videodev2.h#2122
* [V4L2\_TUNER\_MODE\_LANG2](vidioc-g-tuner.html#tuner-audmode): include/uapi/linux/videodev2.h#2119
* [V4L2\_TUNER\_MODE\_MONO](vidioc-g-tuner.html#tuner-audmode): include/uapi/linux/videodev2.h#2117
* [V4L2\_TUNER\_MODE\_SAP](vidioc-g-tuner.html#tuner-audmode): include/uapi/linux/videodev2.h#2120
* [V4L2\_TUNER\_MODE\_STEREO](vidioc-g-tuner.html#tuner-audmode): include/uapi/linux/videodev2.h#2118
* [V4L2\_TUNER\_SUB\_LANG1](vidioc-g-tuner.html#tuner-rxsubchans): include/uapi/linux/videodev2.h#2113
* [V4L2\_TUNER\_SUB\_LANG2](vidioc-g-tuner.html#tuner-rxsubchans): include/uapi/linux/videodev2.h#2111
* [V4L2\_TUNER\_SUB\_MONO](vidioc-g-tuner.html#tuner-rxsubchans): include/uapi/linux/videodev2.h#2109
* [V4L2\_TUNER\_SUB\_RDS](vidioc-g-tuner.html#tuner-rxsubchans): include/uapi/linux/videodev2.h#2114
* [V4L2\_TUNER\_SUB\_SAP](vidioc-g-tuner.html#tuner-rxsubchans): include/uapi/linux/videodev2.h#2112
* [V4L2\_TUNER\_SUB\_STEREO](vidioc-g-tuner.html#tuner-rxsubchans): include/uapi/linux/videodev2.h#2110
* [V4L2\_VBI\_INTERLACED](dev-raw-vbi.html#vbifmt-flags): include/uapi/linux/videodev2.h#2320
* [`V4L2_VBI_ITU_525_F1_START`](dev-raw-vbi.html#c.V4L.v4l2_vbi_format "V4L.v4l2_vbi_format"): include/uapi/linux/videodev2.h#2323
* [`V4L2_VBI_ITU_525_F2_START`](dev-raw-vbi.html#c.V4L.v4l2_vbi_format "V4L.v4l2_vbi_format"): include/uapi/linux/videodev2.h#2324
* [`V4L2_VBI_ITU_625_F1_START`](dev-raw-vbi.html#c.V4L.v4l2_vbi_format "V4L.v4l2_vbi_format"): include/uapi/linux/videodev2.h#2325
* [`V4L2_VBI_ITU_625_F2_START`](dev-raw-vbi.html#c.V4L.v4l2_vbi_format "V4L.v4l2_vbi_format"): include/uapi/linux/videodev2.h#2326
* [V4L2\_VBI\_UNSYNC](dev-raw-vbi.html#vbifmt-flags): include/uapi/linux/videodev2.h#2319
* [`V4L2_XFER_FUNC_ADOBERGB`](colorspaces-defs.html#c.V4L.v4l2_xfer_func "V4L.v4l2_xfer_func"): include/uapi/linux/videodev2.h#431

## 9.5. Structures

* [`v4l2_area`](ext-ctrls-image-source.html#c.V4L.v4l2_area "V4L.v4l2_area"): include/uapi/linux/videodev2.h#454
* [`v4l2_audio`](vidioc-g-audio.html#c.V4L.v4l2_audio "V4L.v4l2_audio"): include/uapi/linux/videodev2.h#2181
* [`v4l2_audioout`](vidioc-g-audioout.html#c.V4L.v4l2_audioout "V4L.v4l2_audioout"): include/uapi/linux/videodev2.h#2196
* [`v4l2_bt_timings`](vidioc-g-dv-timings.html#c.V4L.v4l2_bt_timings "V4L.v4l2_bt_timings"): include/uapi/linux/videodev2.h#1591
* [`v4l2_bt_timings_cap`](vidioc-dv-timings-cap.html#c.V4L.v4l2_bt_timings_cap "V4L.v4l2_bt_timings_cap"): include/uapi/linux/videodev2.h#1752
* [`v4l2_buffer`](buffer.html#c.V4L.v4l2_buffer "V4L.v4l2_buffer"): include/uapi/linux/videodev2.h#1178
* [`v4l2_capability`](vidioc-querycap.html#c.V4L.v4l2_capability "V4L.v4l2_capability"): include/uapi/linux/videodev2.h#470
* [`v4l2_captureparm`](vidioc-g-parm.html#c.V4L.v4l2_captureparm "V4L.v4l2_captureparm"): include/uapi/linux/videodev2.h#1348
* [`v4l2_clip`](dev-overlay.html#c.V4L.v4l2_clip "V4L.v4l2_clip"): include/uapi/linux/videodev2.h#1330
* [`v4l2_control`](vidioc-g-ctrl.html#c.V4L.v4l2_control "V4L.v4l2_control"): include/uapi/linux/videodev2.h#1869
* [`v4l2_create_buffers`](vidioc-create-bufs.html#c.V4L.v4l2_create_buffers "V4L.v4l2_create_buffers"): include/uapi/linux/videodev2.h#2701
* [`v4l2_crop`](vidioc-g-crop.html#c.V4L.v4l2_crop "V4L.v4l2_crop"): include/uapi/linux/videodev2.h#1380
* [`v4l2_cropcap`](vidioc-cropcap.html#c.V4L.v4l2_cropcap "V4L.v4l2_cropcap"): include/uapi/linux/videodev2.h#1373
* [`v4l2_dbg_chip_info`](vidioc-dbg-g-chip-info.html#c.V4L.v4l2_dbg_chip_info "V4L.v4l2_dbg_chip_info"): include/uapi/linux/videodev2.h#2678
* [`v4l2_dbg_match`](vidioc-dbg-g-register.html#c.V4L.v4l2_dbg_match "V4L.v4l2_dbg_match"): include/uapi/linux/videodev2.h#2659
* [`v4l2_dbg_register`](vidioc-dbg-g-register.html#c.V4L.v4l2_dbg_register "V4L.v4l2_dbg_register"): include/uapi/linux/videodev2.h#2667
* [`v4l2_decoder_cmd`](vidioc-decoder-cmd.html#c.V4L.v4l2_decoder_cmd "V4L.v4l2_decoder_cmd"): include/uapi/linux/videodev2.h#2274
* [`v4l2_dv_timings`](vidioc-g-dv-timings.html#c.V4L.v4l2_dv_timings "V4L.v4l2_dv_timings"): include/uapi/linux/videodev2.h#1715
* [`v4l2_dv_timings_cap`](vidioc-dv-timings-cap.html#c.V4L.v4l2_dv_timings_cap "V4L.v4l2_dv_timings_cap"): include/uapi/linux/videodev2.h#1779
* [`v4l2_enc_idx`](vidioc-g-enc-index.html#c.V4L.v4l2_enc_idx "V4L.v4l2_enc_idx"): include/uapi/linux/videodev2.h#2222
* [`v4l2_enc_idx_entry`](vidioc-g-enc-index.html#c.V4L.v4l2_enc_idx_entry "V4L.v4l2_enc_idx_entry"): include/uapi/linux/videodev2.h#2213
* [`v4l2_encoder_cmd`](vidioc-encoder-cmd.html#c.V4L.v4l2_encoder_cmd "V4L.v4l2_encoder_cmd"): include/uapi/linux/videodev2.h#2238
* [`v4l2_enum_dv_timings`](vidioc-enum-dv-timings.html#c.V4L.v4l2_enum_dv_timings "V4L.v4l2_enum_dv_timings"): include/uapi/linux/videodev2.h#1734
* [`v4l2_event`](vidioc-dqevent.html#c.V4L.v4l2_event "V4L.v4l2_event"): include/uapi/linux/videodev2.h#2610
* [`v4l2_event_ctrl`](vidioc-dqevent.html#c.V4L.v4l2_event_ctrl "V4L.v4l2_event_ctrl"): include/uapi/linux/videodev2.h#2571
* [`v4l2_event_frame_sync`](vidioc-dqevent.html#c.V4L.v4l2_event_frame_sync "V4L.v4l2_event_frame_sync"): include/uapi/linux/videodev2.h#2585
* [`v4l2_event_motion_det`](vidioc-dqevent.html#c.V4L.v4l2_event_motion_det "V4L.v4l2_event_motion_det"): include/uapi/linux/videodev2.h#2604
* [`v4l2_event_src_change`](vidioc-dqevent.html#c.V4L.v4l2_event_src_change "V4L.v4l2_event_src_change"): include/uapi/linux/videodev2.h#2591
* [`v4l2_event_subscription`](vidioc-subscribe-event.html#c.V4L.v4l2_event_subscription "V4L.v4l2_event_subscription"): include/uapi/linux/videodev2.h#2634
* [`v4l2_event_vsync`](vidioc-dqevent.html#c.V4L.v4l2_event_vsync "V4L.v4l2_event_vsync"): include/uapi/linux/videodev2.h#2560
* [`v4l2_exportbuffer`](vidioc-expbuf.html#c.V4L.v4l2_exportbuffer "V4L.v4l2_exportbuffer"): include/uapi/linux/videodev2.h#1281
* [`v4l2_ext_control`](vidioc-g-ext-ctrls.html#c.V4L.v4l2_ext_control "V4L.v4l2_ext_control"): include/uapi/linux/videodev2.h#1874
* [`v4l2_ext_controls`](vidioc-g-ext-ctrls.html#c.V4L.v4l2_ext_controls "V4L.v4l2_ext_controls"): include/uapi/linux/videodev2.h#1917
* [`v4l2_fmtdesc`](vidioc-enum-fmt.html#c.V4L.v4l2_fmtdesc "V4L.v4l2_fmtdesc"): include/uapi/linux/videodev2.h#916
* [`v4l2_format`](vidioc-g-fmt.html#c.V4L.v4l2_format "V4L.v4l2_format"): include/uapi/linux/videodev2.h#2521
* [`v4l2_fract`](vidioc-enumstd.html#c.V4L.v4l2_fract "V4L.v4l2_fract"): include/uapi/linux/videodev2.h#449
* [`v4l2_framebuffer`](vidioc-g-fbuf.html#c.V4L.v4l2_framebuffer "V4L.v4l2_framebuffer"): include/uapi/linux/videodev2.h#1293
* [`v4l2_frequency`](vidioc-g-frequency.html#c.V4L.v4l2_frequency "V4L.v4l2_frequency"): include/uapi/linux/videodev2.h#2124
* [`v4l2_frequency_band`](vidioc-enum-freq-bands.html#c.V4L.v4l2_frequency_band "V4L.v4l2_frequency_band"): include/uapi/linux/videodev2.h#2135
* [`v4l2_frmival_stepwise`](vidioc-enum-frameintervals.html#c.V4L.v4l2_frmival_stepwise "V4L.v4l2_frmival_stepwise"): include/uapi/linux/videodev2.h#987
* [`v4l2_frmivalenum`](vidioc-enum-frameintervals.html#c.V4L.v4l2_frmivalenum "V4L.v4l2_frmivalenum"): include/uapi/linux/videodev2.h#993
* [`v4l2_frmsize_discrete`](vidioc-enum-framesizes.html#c.V4L.v4l2_frmsize_discrete "V4L.v4l2_frmsize_discrete"): include/uapi/linux/videodev2.h#951
* [`v4l2_frmsize_stepwise`](vidioc-enum-framesizes.html#c.V4L.v4l2_frmsize_stepwise "V4L.v4l2_frmsize_stepwise"): include/uapi/linux/videodev2.h#956
* [`v4l2_frmsizeenum`](vidioc-enum-framesizes.html#c.V4L.v4l2_frmsizeenum "V4L.v4l2_frmsizeenum"): include/uapi/linux/videodev2.h#965
* [`v4l2_hw_freq_seek`](vidioc-s-hw-freq-seek.html#c.V4L.v4l2_hw_freq_seek "V4L.v4l2_hw_freq_seek"): include/uapi/linux/videodev2.h#2146
* [`v4l2_input`](vidioc-enuminput.html#c.V4L.v4l2_input "V4L.v4l2_input"): include/uapi/linux/videodev2.h#1793
* [`v4l2_jpegcompression`](vidioc-g-jpegcomp.html#c.V4L.v4l2_jpegcompression "V4L.v4l2_jpegcompression"): include/uapi/linux/videodev2.h#1036
* [`v4l2_meta_format`](dev-meta.html#c.V4L.v4l2_meta_format "V4L.v4l2_meta_format"): include/uapi/linux/videodev2.h#2501
* [`v4l2_modulator`](vidioc-g-modulator.html#c.V4L.v4l2_modulator "V4L.v4l2_modulator"): include/uapi/linux/videodev2.h#2081
* [`v4l2_mpeg_vbi_ITV0`](dev-sliced-vbi.html#c.V4L.v4l2_mpeg_vbi_itv0 "V4L.v4l2_mpeg_vbi_itv0"): include/uapi/linux/videodev2.h#2410
* [`v4l2_mpeg_vbi_fmt_ivtv`](dev-sliced-vbi.html#c.V4L.v4l2_mpeg_vbi_fmt_ivtv "V4L.v4l2_mpeg_vbi_fmt_ivtv"): include/uapi/linux/videodev2.h#2417
* [`v4l2_mpeg_vbi_itv0`](dev-sliced-vbi.html#c.V4L.v4l2_mpeg_vbi_itv0 "V4L.v4l2_mpeg_vbi_itv0"): include/uapi/linux/videodev2.h#2405
* [`v4l2_mpeg_vbi_itv0_line`](dev-sliced-vbi.html#c.V4L.v4l2_mpeg_vbi_itv0_line "V4L.v4l2_mpeg_vbi_itv0_line"): include/uapi/linux/videodev2.h#2400
* [`v4l2_output`](vidioc-enumoutput.html#c.V4L.v4l2_output "V4L.v4l2_output"): include/uapi/linux/videodev2.h#1845
* [`v4l2_outputparm`](vidioc-g-parm.html#c.V4L.v4l2_outputparm "V4L.v4l2_outputparm"): include/uapi/linux/videodev2.h#1361
* [`v4l2_pix_format`](pixfmt-v4l2.html#c.V4L.v4l2_pix_format "V4L.v4l2_pix_format"): include/uapi/linux/videodev2.h#526
* [`v4l2_pix_format_mplane`](pixfmt-v4l2-mplane.html#c.V4L.v4l2_pix_format_mplane "V4L.v4l2_pix_format_mplane"): include/uapi/linux/videodev2.h#2459
* [`v4l2_plane`](buffer.html#c.V4L.v4l2_plane "V4L.v4l2_plane"): include/uapi/linux/videodev2.h#1131
* [`v4l2_plane_pix_format`](pixfmt-v4l2-mplane.html#c.V4L.v4l2_plane_pix_format "V4L.v4l2_plane_pix_format"): include/uapi/linux/videodev2.h#2437
* [`v4l2_query_ext_ctrl`](vidioc-queryctrl.html#c.V4L.v4l2_query_ext_ctrl "V4L.v4l2_query_ext_ctrl"): include/uapi/linux/videodev2.h#2012
* [`v4l2_queryctrl`](vidioc-queryctrl.html#c.V4L.v4l2_queryctrl "V4L.v4l2_queryctrl"): include/uapi/linux/videodev2.h#1999
* [`v4l2_querymenu`](vidioc-queryctrl.html#c.V4L.v4l2_querymenu "V4L.v4l2_querymenu"): include/uapi/linux/videodev2.h#2029
* [`v4l2_rds_data`](dev-rds.html#c.V4L.v4l2_rds_data "V4L.v4l2_rds_data"): include/uapi/linux/videodev2.h#2161
* [`v4l2_rect`](dev-overlay.html#c.V4L.v4l2_rect "V4L.v4l2_rect"): include/uapi/linux/videodev2.h#442
* [`v4l2_remove_buffers`](vidioc-remove-bufs.html#c.V4L.v4l2_remove_buffers "V4L.v4l2_remove_buffers"): include/uapi/linux/videodev2.h#2719
* [`v4l2_requestbuffers`](vidioc-reqbufs.html#c.V4L.v4l2_requestbuffers "V4L.v4l2_requestbuffers"): include/uapi/linux/videodev2.h#1087
* [`v4l2_sdr_format`](dev-sdr.html#c.V4L.v4l2_sdr_format "V4L.v4l2_sdr_format"): include/uapi/linux/videodev2.h#2484
* [`v4l2_selection`](vidioc-g-selection.html#c.V4L.v4l2_selection "V4L.v4l2_selection"): include/uapi/linux/videodev2.h#1398
* [`v4l2_sliced_vbi_cap`](vidioc-g-sliced-vbi-cap.html#c.V4L.v4l2_sliced_vbi_cap "V4L.v4l2_sliced_vbi_cap"): include/uapi/linux/videodev2.h#2359
* [`v4l2_sliced_vbi_data`](dev-sliced-vbi.html#c.V4L.v4l2_sliced_vbi_data "V4L.v4l2_sliced_vbi_data"): include/uapi/linux/videodev2.h#2370
* [`v4l2_sliced_vbi_format`](dev-sliced-vbi.html#c.V4L.v4l2_sliced_vbi_format "V4L.v4l2_sliced_vbi_format"): include/uapi/linux/videodev2.h#2335
* [`v4l2_standard`](vidioc-enumstd.html#c.V4L.v4l2_standard "V4L.v4l2_standard"): include/uapi/linux/videodev2.h#1543
* [`v4l2_streamparm`](vidioc-g-parm.html#c.V4L.v4l2_streamparm "V4L.v4l2_streamparm"): include/uapi/linux/videodev2.h#2537
* [`v4l2_timecode`](buffer.html#c.V4L.v4l2_timecode "V4L.v4l2_timecode"): include/uapi/linux/videodev2.h#1011
* [`v4l2_tuner`](vidioc-g-tuner.html#c.V4L.v4l2_tuner "V4L.v4l2_tuner"): include/uapi/linux/videodev2.h#2067
* [`v4l2_vbi_format`](dev-raw-vbi.html#c.V4L.v4l2_vbi_format "V4L.v4l2_vbi_format"): include/uapi/linux/videodev2.h#2307
* [`v4l2_window`](dev-overlay.html#c.V4L.v4l2_window "V4L.v4l2_window"): include/uapi/linux/videodev2.h#1335
