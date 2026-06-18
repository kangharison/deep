# 3.17.V4L2 DV Timings functions

> 출처(원문): https://docs.kernel.org/driver-api/media/v4l2-dv-timings.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.17. V4L2 DV Timings functions

struct v4l2\_fract v4l2\_calc\_timeperframe(const struct v4l2\_dv\_timings \*t)
:   helper function to calculate timeperframe based v4l2\_dv\_timings fields.

**Parameters**

`const struct v4l2_dv_timings *t`
:   Timings for the video mode.

**Description**

Calculates the expected timeperframe using the pixel clock value and
horizontal/vertical measures. This means that v4l2\_dv\_timings structure
must be correctly and fully filled.

v4l2\_check\_dv\_timings\_fnc
:   **Typedef**: timings check callback

**Syntax**

> `bool v4l2_check_dv_timings_fnc (const struct v4l2_dv_timings *t, void *handle)`

**Parameters**

`const struct v4l2_dv_timings *t`
:   the v4l2\_dv\_timings struct.

`void *handle`
:   a handle from the driver.

**Description**

Returns true if the given timings are valid.

bool v4l2\_valid\_dv\_timings(const struct v4l2\_dv\_timings \*t, const struct v4l2\_dv\_timings\_cap \*cap, [v4l2\_check\_dv\_timings\_fnc](#c.v4l2_check_dv_timings_fnc "v4l2_check_dv_timings_fnc") fnc, void \*fnc\_handle)
:   are these timings valid?

**Parameters**

`const struct v4l2_dv_timings *t`
:   the v4l2\_dv\_timings struct.

`const struct v4l2_dv_timings_cap *cap`
:   the v4l2\_dv\_timings\_cap capabilities.

`v4l2_check_dv_timings_fnc fnc`
:   callback to check if this timing is OK. May be NULL.

`void *fnc_handle`
:   a handle that is passed on to **fnc**.

**Description**

Returns true if the given dv\_timings `struct is` supported by the
hardware capabilities and the callback function (if non-NULL), returns
false otherwise.

int v4l2\_enum\_dv\_timings\_cap(struct v4l2\_enum\_dv\_timings \*t, const struct v4l2\_dv\_timings\_cap \*cap, [v4l2\_check\_dv\_timings\_fnc](#c.v4l2_check_dv_timings_fnc "v4l2_check_dv_timings_fnc") fnc, void \*fnc\_handle)
:   Helper function to enumerate possible DV timings based on capabilities

**Parameters**

`struct v4l2_enum_dv_timings *t`
:   the v4l2\_enum\_dv\_timings struct.

`const struct v4l2_dv_timings_cap *cap`
:   the v4l2\_dv\_timings\_cap capabilities.

`v4l2_check_dv_timings_fnc fnc`
:   callback to check if this timing is OK. May be NULL.

`void *fnc_handle`
:   a handle that is passed on to **fnc**.

**Description**

This enumerates dv\_timings using the full list of possible CEA-861 and DMT
timings, filtering out any timings that are not supported based on the
hardware capabilities and the callback function (if non-NULL).

If a valid timing for the given index is found, it will fill in **t** and
return 0, otherwise it returns -EINVAL.

bool v4l2\_find\_dv\_timings\_cap(struct v4l2\_dv\_timings \*t, const struct v4l2\_dv\_timings\_cap \*cap, unsigned pclock\_delta, [v4l2\_check\_dv\_timings\_fnc](#c.v4l2_check_dv_timings_fnc "v4l2_check_dv_timings_fnc") fnc, void \*fnc\_handle)
:   Find the closest timings struct

**Parameters**

`struct v4l2_dv_timings *t`
:   the v4l2\_enum\_dv\_timings struct.

`const struct v4l2_dv_timings_cap *cap`
:   the v4l2\_dv\_timings\_cap capabilities.

`unsigned pclock_delta`
:   maximum delta between t->pixelclock and the timing `struct
    under` consideration.

`v4l2_check_dv_timings_fnc fnc`
:   callback to check if a given timings `struct is` OK. May be NULL.

`void *fnc_handle`
:   a handle that is passed on to **fnc**.

**Description**

This function tries to map the given timings to an entry in the
full list of possible CEA-861 and DMT timings, filtering out any timings
that are not supported based on the hardware capabilities and the callback
function (if non-NULL).

On success it will fill in **t** with the found timings and it returns true.
On failure it will return false.

bool v4l2\_find\_dv\_timings\_cea861\_vic(struct v4l2\_dv\_timings \*t, u8 vic)
:   find timings based on CEA-861 VIC

**Parameters**

`struct v4l2_dv_timings *t`
:   the timings data.

`u8 vic`
:   CEA-861 VIC code

**Description**

On success it will fill in **t** with the found timings and it returns true.
On failure it will return false.

bool v4l2\_match\_dv\_timings(const struct v4l2\_dv\_timings \*measured, const struct v4l2\_dv\_timings \*standard, unsigned pclock\_delta, bool match\_reduced\_fps)
:   do two timings match?

**Parameters**

`const struct v4l2_dv_timings *measured`
:   the measured timings data.

`const struct v4l2_dv_timings *standard`
:   the timings according to the standard.

`unsigned pclock_delta`
:   maximum delta in Hz between standard->pixelclock and
    the measured timings.

`bool match_reduced_fps`
:   if true, then fail if V4L2\_DV\_FL\_REDUCED\_FPS does not
    match.

**Description**

Returns true if the two timings match, returns false otherwise.

void v4l2\_print\_dv\_timings(const char \*dev\_prefix, const char \*prefix, const struct v4l2\_dv\_timings \*t, bool detailed)
:   log the contents of a dv\_timings struct

**Parameters**

`const char *dev_prefix`
:   device prefix for each log line.

`const char *prefix`
:   additional prefix for each log line, may be NULL.

`const struct v4l2_dv_timings *t`
:   the timings data.

`bool detailed`
:   if true, give a detailed log.

bool v4l2\_detect\_cvt(unsigned int frame\_height, unsigned int hfreq, unsigned int vsync, unsigned int active\_width, u32 polarities, bool interlaced, const struct v4l2\_dv\_timings\_cap \*cap, struct v4l2\_dv\_timings \*fmt)
:   detect if the given timings follow the CVT standard

**Parameters**

`unsigned int frame_height`
:   the total height of the frame (including blanking) in lines.

`unsigned int hfreq`
:   the horizontal frequency in Hz.

`unsigned int vsync`
:   the height of the vertical sync in lines.

`unsigned int active_width`
:   active width of image (does not include blanking). This
    information is needed only in case of version 2 of reduced blanking.
    In other cases, this parameter does not have any effect on timings.

`u32 polarities`
:   the horizontal and vertical polarities (same as `struct
    v4l2_bt_timings` polarities).

`bool interlaced`
:   if this flag is true, it indicates interlaced format

`const struct v4l2_dv_timings_cap *cap`
:   the v4l2\_dv\_timings\_cap capabilities.

`struct v4l2_dv_timings *fmt`
:   the resulting timings.

**Description**

This function will attempt to detect if the given values correspond to a
valid CVT format. If so, then it will return true, and fmt will be filled
in with the found CVT timings.

bool v4l2\_detect\_gtf(unsigned int frame\_height, unsigned int hfreq, unsigned int vsync, u32 polarities, bool interlaced, struct v4l2\_fract aspect, const struct v4l2\_dv\_timings\_cap \*cap, struct v4l2\_dv\_timings \*fmt)
:   detect if the given timings follow the GTF standard

**Parameters**

`unsigned int frame_height`
:   the total height of the frame (including blanking) in lines.

`unsigned int hfreq`
:   the horizontal frequency in Hz.

`unsigned int vsync`
:   the height of the vertical sync in lines.

`u32 polarities`
:   the horizontal and vertical polarities (same as `struct
    v4l2_bt_timings` polarities).

`bool interlaced`
:   if this flag is true, it indicates interlaced format

`struct v4l2_fract aspect`
:   preferred aspect ratio. GTF has no method of determining the
    aspect ratio in order to derive the image width from the
    image height, so it has to be passed explicitly. Usually
    the native screen aspect ratio is used for this. If it
    is not filled in correctly, then 16:9 will be assumed.

`const struct v4l2_dv_timings_cap *cap`
:   the v4l2\_dv\_timings\_cap capabilities.

`struct v4l2_dv_timings *fmt`
:   the resulting timings.

**Description**

This function will attempt to detect if the given values correspond to a
valid GTF format. If so, then it will return true, and fmt will be filled
in with the found GTF timings.

struct v4l2\_fract v4l2\_calc\_aspect\_ratio(u8 hor\_landscape, u8 vert\_portrait)
:   calculate the aspect ratio based on bytes 0x15 and 0x16 from the EDID.

**Parameters**

`u8 hor_landscape`
:   byte 0x15 from the EDID.

`u8 vert_portrait`
:   byte 0x16 from the EDID.

**Description**

Determines the aspect ratio from the EDID.
See VESA Enhanced EDID standard, release A, rev 2, section 3.6.2:
“Horizontal and Vertical Screen Size or Aspect Ratio”

struct v4l2\_fract v4l2\_dv\_timings\_aspect\_ratio(const struct v4l2\_dv\_timings \*t)
:   calculate the aspect ratio based on the v4l2\_dv\_timings information.

**Parameters**

`const struct v4l2_dv_timings *t`
:   the timings data.

bool can\_reduce\_fps(struct v4l2\_bt\_timings \*bt)
:   check if conditions for reduced fps are true.

**Parameters**

`struct v4l2_bt_timings *bt`
:   v4l2 timing structure

**Description**

For different timings reduced fps is allowed if the following conditions
are met:

> * For CVT timings: if reduced blanking v2 (vsync == 8) is true.
> * For CEA861 timings: if `V4L2_DV_FL_CAN_REDUCE_FPS` flag is true.

struct v4l2\_hdmi\_colorimetry
:   describes the HDMI colorimetry information

> **Definition**:
>
> ```
> struct v4l2_hdmi_colorimetry {
>       enum v4l2_colorspace colorspace;
>       enum v4l2_ycbcr_encoding ycbcr_enc;
>       enum v4l2_quantization quantization;
>       enum v4l2_xfer_func xfer_func;
> };
> ```

**Members**

`colorspace`
:   `enum v4l2_colorspace`, the colorspace

`ycbcr_enc`
:   `enum v4l2_ycbcr_encoding`, Y’CbCr encoding

`quantization`
:   `enum v4l2_quantization`, colorspace quantization

`xfer_func`
:   `enum v4l2_xfer_func`, colorspace transfer function
