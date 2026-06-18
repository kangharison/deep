# 12.The Linux USB Video Class (UVC) driver

> 출처(원문): https://docs.kernel.org/userspace-api/media/drivers/uvcvideo.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 12. The Linux USB Video Class (UVC) driver

This file documents some driver-specific aspects of the UVC driver, such as
driver-specific ioctls and implementation notes.

Questions and remarks can be sent to the Linux UVC development mailing list at
[linux-media@vger.kernel.org](mailto:linux-media%40vger.kernel.org).

## 12.1. Extension Unit (XU) support

### 12.1.1. Introduction

The UVC specification allows for vendor-specific extensions through extension
units (XUs). The Linux UVC driver supports extension unit controls (XU controls)
through two separate mechanisms:

> * through mappings of XU controls to V4L2 controls
> * through a driver-specific ioctl interface

The first one allows generic V4L2 applications to use XU controls by mapping
certain XU controls onto V4L2 controls, which then show up during ordinary
control enumeration.

The second mechanism requires uvcvideo-specific knowledge for the application to
access XU controls but exposes the entire UVC XU concept to user space for
maximum flexibility.

Both mechanisms complement each other and are described in more detail below.

### 12.1.2. Control mappings

The UVC driver provides an API for user space applications to define so-called
control mappings at runtime. These allow for individual XU controls or byte
ranges thereof to be mapped to new V4L2 controls. Such controls appear and
function exactly like normal V4L2 controls (i.e. the stock controls, such as
brightness, contrast, etc.). However, reading or writing of such a V4L2 controls
triggers a read or write of the associated XU control.

The ioctl used to create these control mappings is called UVCIOC\_CTRL\_MAP.
Previous driver versions (before 0.2.0) required another ioctl to be used
beforehand (UVCIOC\_CTRL\_ADD) to pass XU control information to the UVC driver.
This is no longer necessary as newer uvcvideo versions query the information
directly from the device.

For details on the UVCIOC\_CTRL\_MAP ioctl please refer to the section titled
“IOCTL reference” below.

3. Driver specific XU control interface

For applications that need to access XU controls directly, e.g. for testing
purposes, firmware upload, or accessing binary controls, a second mechanism to
access XU controls is provided in the form of a driver-specific ioctl, namely
UVCIOC\_CTRL\_QUERY.

A call to this ioctl allows applications to send queries to the UVC driver that
directly map to the low-level UVC control requests.

In order to make such a request the UVC unit ID of the control’s extension unit
and the control selector need to be known. This information either needs to be
hardcoded in the application or queried using other ways such as by parsing the
UVC descriptor or, if available, using the media controller API to enumerate a
device’s entities.

Unless the control size is already known it is necessary to first make a
UVC\_GET\_LEN requests in order to be able to allocate a sufficiently large buffer
and set the buffer size to the correct value. Similarly, to find out whether
UVC\_GET\_CUR or UVC\_SET\_CUR are valid requests for a given control, a
UVC\_GET\_INFO request should be made. The bits 0 (GET supported) and 1 (SET
supported) of the resulting byte indicate which requests are valid.

With the addition of the UVCIOC\_CTRL\_QUERY ioctl the UVCIOC\_CTRL\_GET and
UVCIOC\_CTRL\_SET ioctls have become obsolete since their functionality is a
subset of the former ioctl. For the time being they are still supported but
application developers are encouraged to use UVCIOC\_CTRL\_QUERY instead.

For details on the UVCIOC\_CTRL\_QUERY ioctl please refer to the section titled
“IOCTL reference” below.

### 12.1.3. Security

The API doesn’t currently provide a fine-grained access control facility. The
UVCIOC\_CTRL\_ADD and UVCIOC\_CTRL\_MAP ioctls require super user permissions.

Suggestions on how to improve this are welcome.

### 12.1.4. Debugging

In order to debug problems related to XU controls or controls in general it is
recommended to enable the UVC\_TRACE\_CONTROL bit in the module parameter ‘trace’.
This causes extra output to be written into the system log.

### 12.1.5. IOCTL reference

#### 12.1.5.1. UVCIOC\_CTRL\_MAP - Map a UVC control to a V4L2 control

Argument: `struct uvc_xu_control_mapping`

**Description**:

> This ioctl creates a mapping between a UVC control or part of a UVC
> control and a V4L2 control. Once mappings are defined, userspace
> applications can access vendor-defined UVC control through the V4L2
> control API.
>
> To create a mapping, applications fill the uvc\_xu\_control\_mapping
> structure with information about an existing UVC control defined with
> UVCIOC\_CTRL\_ADD and a new V4L2 control.
>
> A UVC control can be mapped to several V4L2 controls. For instance,
> a UVC pan/tilt control could be mapped to separate pan and tilt V4L2
> controls. The UVC control is divided into non overlapping fields using
> the ‘size’ and ‘offset’ fields and are then independently mapped to
> V4L2 control.
>
> For signed integer V4L2 controls the data\_type field should be set to
> UVC\_CTRL\_DATA\_TYPE\_SIGNED. Other values are currently ignored.

**Return value**:

> On success 0 is returned. On error -1 is returned and errno is set
> appropriately.
>
> ENOMEM
> :   Not enough memory to perform the operation.
>
> EPERM
> :   Insufficient privileges (super user privileges are required).
>
> EINVAL
> :   No such UVC control.
>
> EOVERFLOW
> :   The requested offset and size would overflow the UVC control.
>
> EEXIST
> :   Mapping already exists.

**Data types**:

```
* struct uvc_xu_control_mapping

__u32   id              V4L2 control identifier
__u8    name[32]        V4L2 control name
__u8    entity[16]      UVC extension unit GUID
__u8    selector        UVC control selector
__u8    size            V4L2 control size (in bits)
__u8    offset          V4L2 control offset (in bits)
enum v4l2_ctrl_type
        v4l2_type       V4L2 control type
enum uvc_control_data_type
        data_type       UVC control data type
struct uvc_menu_info
        *menu_info      Array of menu entries (for menu controls only)
__u32   menu_count      Number of menu entries (for menu controls only)

* struct uvc_menu_info

__u32   value           Menu entry value used by the device
__u8    name[32]        Menu entry name


* enum uvc_control_data_type

UVC_CTRL_DATA_TYPE_RAW          Raw control (byte array)
UVC_CTRL_DATA_TYPE_SIGNED       Signed integer
UVC_CTRL_DATA_TYPE_UNSIGNED     Unsigned integer
UVC_CTRL_DATA_TYPE_BOOLEAN      Boolean
UVC_CTRL_DATA_TYPE_ENUM         Enumeration
UVC_CTRL_DATA_TYPE_BITMASK      Bitmask
UVC_CTRL_DATA_TYPE_RECT         Rectangular area
```

#### 12.1.5.2. UVCIOC\_CTRL\_QUERY - Query a UVC XU control

Argument: `struct uvc_xu_control_query`

**Description**:

> This ioctl queries a UVC XU control identified by its extension unit ID
> and control selector.
>
> There are a number of different queries available that closely
> correspond to the low-level control requests described in the UVC
> specification. These requests are:
>
> UVC\_GET\_CUR
> :   Obtain the current value of the control.
>
> UVC\_GET\_MIN
> :   Obtain the minimum value of the control.
>
> UVC\_GET\_MAX
> :   Obtain the maximum value of the control.
>
> UVC\_GET\_DEF
> :   Obtain the default value of the control.
>
> UVC\_GET\_RES
> :   Query the resolution of the control, i.e. the step size of the
>     allowed control values.
>
> UVC\_GET\_LEN
> :   Query the size of the control in bytes.
>
> UVC\_GET\_INFO
> :   Query the control information bitmap, which indicates whether
>     get/set requests are supported.
>
> UVC\_SET\_CUR
> :   Update the value of the control.
>
> Applications must set the ‘size’ field to the correct length for the
> control. Exceptions are the UVC\_GET\_LEN and UVC\_GET\_INFO queries, for
> which the size must be set to 2 and 1, respectively. The ‘data’ field
> must point to a valid writable buffer big enough to hold the indicated
> number of data bytes.
>
> Data is copied directly from the device without any driver-side
> processing. Applications are responsible for data buffer formatting,
> including little-endian/big-endian conversion. This is particularly
> important for the result of the UVC\_GET\_LEN requests, which is always
> returned as a little-endian 16-bit integer by the device.

**Return value**:

> On success 0 is returned. On error -1 is returned and errno is set
> appropriately.
>
> ENOENT
> :   The device does not support the given control or the specified
>     extension unit could not be found.
>
> ENOBUFS
> :   The specified buffer size is incorrect (too big or too small).
>
> EINVAL
> :   An invalid request code was passed.
>
> EBADRQC
> :   The given request is not supported by the given control.
>
> EFAULT
> :   The data pointer references an inaccessible memory area.

**Data types**:

```
* struct uvc_xu_control_query

__u8    unit            Extension unit ID
__u8    selector        Control selector
__u8    query           Request code to send to the device
__u16   size            Control data size (in bytes)
__u8    *data           Control value
```

## 12.2. Driver-specific V4L2 controls

The uvcvideo driver implements the following UVC-specific controls:

`V4L2_CID_UVC_REGION_OF_INTEREST_RECT (struct)`
:   This control determines the region of interest (ROI). ROI is a
    rectangular area represented by a struct `v4l2_rect`. The
    rectangle is in global sensor coordinates using pixel units. It is
    independent of the field of view, not impacted by any cropping or
    scaling.

    Use `V4L2_CTRL_WHICH_MIN_VAL` and `V4L2_CTRL_WHICH_MAX_VAL` to query
    the range of rectangle sizes.

    Setting a ROI allows the camera to optimize the capture for the region.
    The value of `V4L2_CID_REGION_OF_INTEREST_AUTO` control determines
    the detailed behavior.

    An example of use of this control, can be found in the:
    Chrome OS USB camera HAL.
    <https://chromium.googlesource.com/chromiumos/platform2/+/refs/heads/release-R121-15699.B/camera/hal/usb/>

`V4L2_CID_UVC_REGION_OF_INTEREST_AUTO (bitmask)`
:   This determines which, if any, on-board features should track to the
    Region of Interest specified by the current value of
    `V4L2_CID_UVD__REGION_OF_INTEREST_RECT`.

    Max value is a mask indicating all supported Auto Controls.

|  |  |
| --- | --- |
| `V4L2_UVC_REGION_OF_INTEREST_AUTO_EXPOSURE` | Setting this bit causes automatic exposure to track the region of interest instead of the whole image. |
| `V4L2_UVC_REGION_OF_INTEREST_AUTO_IRIS` | Setting this bit causes automatic iris to track the region of interest instead of the whole image. |
| `V4L2_UVC_REGION_OF_INTEREST_AUTO_WHITE_BALANCE` | Setting this bit causes automatic white balance to track the region of interest instead of the whole image. |
| `V4L2_UVC_REGION_OF_INTEREST_AUTO_FOCUS` | Setting this bit causes automatic focus adjustment to track the region of interest instead of the whole image. |
| `V4L2_UVC_REGION_OF_INTEREST_AUTO_FACE_DETECT` | Setting this bit causes automatic face detection to track the region of interest instead of the whole image. |
| `V4L2_UVC_REGION_OF_INTEREST_AUTO_DETECT_AND_TRACK` | Setting this bit enables automatic face detection and tracking. The current value of `V4L2_CID_REGION_OF_INTEREST_RECT` may be updated by the driver. |
| `V4L2_UVC_REGION_OF_INTEREST_AUTO_IMAGE_STABILIZATION` | Setting this bit enables automatic image stabilization. The current value of `V4L2_CID_REGION_OF_INTEREST_RECT` may be updated by the driver. |
| `V4L2_UVC_REGION_OF_INTEREST_AUTO_HIGHER_QUALITY` | Setting this bit enables automatically capture the specified region with higher quality if possible. |
