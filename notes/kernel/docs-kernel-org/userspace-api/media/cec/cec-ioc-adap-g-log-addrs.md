# 2.6.ioctls CEC_ADAP_G_LOG_ADDRS and CEC_ADAP_S_LOG_ADDRS

> 출처(원문): https://docs.kernel.org/userspace-api/media/cec/cec-ioc-adap-g-log-addrs.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.6. ioctls CEC\_ADAP\_G\_LOG\_ADDRS and CEC\_ADAP\_S\_LOG\_ADDRS

## 2.6.1. Name

CEC\_ADAP\_G\_LOG\_ADDRS, CEC\_ADAP\_S\_LOG\_ADDRS - Get or set the logical addresses

## 2.6.2. Synopsis

CEC\_ADAP\_G\_LOG\_ADDRS

`int ioctl(int fd, CEC_ADAP_G_LOG_ADDRS, struct cec_log_addrs *argp)`

CEC\_ADAP\_S\_LOG\_ADDRS

`int ioctl(int fd, CEC_ADAP_S_LOG_ADDRS, struct cec_log_addrs *argp)`

## 2.6.3. Arguments

`fd`
:   File descriptor returned by [`open()`](cec-func-open.html#c.CEC.open "open").

`argp`
:   Pointer to struct [`cec_log_addrs`](#c.CEC.cec_log_addrs "cec_log_addrs").

## 2.6.4. Description

To query the current CEC logical addresses, applications call
[ioctl CEC\_ADAP\_G\_LOG\_ADDRS](#cec-adap-g-log-addrs) with a pointer to a
struct [`cec_log_addrs`](#c.CEC.cec_log_addrs "cec_log_addrs") where the driver stores the logical addresses.

To set new logical addresses, applications fill in
struct [`cec_log_addrs`](#c.CEC.cec_log_addrs "cec_log_addrs") and call [ioctl CEC\_ADAP\_S\_LOG\_ADDRS](#cec-adap-s-log-addrs)
with a pointer to this struct. The [ioctl CEC\_ADAP\_S\_LOG\_ADDRS](#cec-adap-s-log-addrs)
is only available if `CEC_CAP_LOG_ADDRS` is set (the `ENOTTY` error code is
returned otherwise). The [ioctl CEC\_ADAP\_S\_LOG\_ADDRS](#cec-adap-s-log-addrs)
can only be called by a file descriptor in initiator mode (see [ioctls CEC\_G\_MODE and CEC\_S\_MODE](cec-ioc-g-mode.html#cec-s-mode)), if not
the `EBUSY` error code will be returned.

To clear existing logical addresses set `num_log_addrs` to 0. All other fields
will be ignored in that case. The adapter will go to the unconfigured state and the
`cec_version`, `vendor_id` and `osd_name` fields are all reset to their default
values (CEC version 2.0, no vendor ID and an empty OSD name).

If the physical address is valid (see [ioctl CEC\_ADAP\_S\_PHYS\_ADDR](cec-ioc-adap-g-phys-addr.html#cec-adap-s-phys-addr)),
then this ioctl will block until all requested logical
addresses have been claimed. If the file descriptor is in non-blocking mode then it will
not wait for the logical addresses to be claimed, instead it just returns 0.

A [CEC\_EVENT\_STATE\_CHANGE](cec-ioc-dqevent.html#cec-event-state-change) event is sent when the
logical addresses are claimed or cleared.

Attempting to call [ioctl CEC\_ADAP\_S\_LOG\_ADDRS](#cec-adap-s-log-addrs) when
logical address types are already defined will return with error `EBUSY`.

type cec\_log\_addrs

struct cec\_log\_addrs

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `log_addr[CEC_MAX_LOG_ADDRS]` | The actual logical addresses that were claimed. This is set by the driver. If no logical address could be claimed, then it is set to `CEC_LOG_ADDR_INVALID`. If this adapter is Unregistered, then `log_addr[0]` is set to 0xf and all others to `CEC_LOG_ADDR_INVALID`. |
| \_\_u16 | `log_addr_mask` | The bitmask of all logical addresses this adapter has claimed. If this adapter is Unregistered then `log_addr_mask` sets bit 15 and clears all other bits. If this adapter is not configured at all, then `log_addr_mask` is set to 0. Set by the driver. |
| \_\_u8 | `cec_version` | The CEC version that this adapter shall use. See [CEC Versions](#cec-versions). Used to implement the `CEC_MSG_CEC_VERSION` and `CEC_MSG_REPORT_FEATURES` messages. Note that [CEC\_OP\_CEC\_VERSION\_1\_3A](#cec-op-cec-version-1-3a) is not allowed by the CEC framework. |
| \_\_u8 | `num_log_addrs` | Number of logical addresses to set up. Must be ≤ `available_log_addrs` as returned by [ioctl CEC\_ADAP\_G\_CAPS](cec-ioc-adap-g-caps.html#cec-adap-g-caps). All arrays in this structure are only filled up to index `available_log_addrs`-1. The remaining array elements will be ignored. Note that the CEC 2.0 standard allows for a maximum of 2 logical addresses, although some hardware has support for more. `CEC_MAX_LOG_ADDRS` is 4. The driver will return the actual number of logical addresses it could claim, which may be less than what was requested. If this field is set to 0, then the CEC adapter shall clear all claimed logical addresses and all other fields will be ignored. |
| \_\_u32 | `vendor_id` | The vendor ID is a 24-bit number that identifies the specific vendor or entity. Based on this ID vendor specific commands may be defined. If you do not want a vendor ID then set it to `CEC_VENDOR_ID_NONE`. |
| \_\_u32 | `flags` | Flags. See [Flags for struct cec\_log\_addrs](#cec-log-addrs-flags) for a list of available flags. |
| char | `osd_name[15]` | The On-Screen Display name as is returned by the `CEC_MSG_SET_OSD_NAME` message. |
| \_\_u8 | `primary_device_type[CEC_MAX_LOG_ADDRS]` | Primary device type for each logical address. See [CEC Primary Device Types](#cec-prim-dev-types) for possible types. |
| \_\_u8 | `log_addr_type[CEC_MAX_LOG_ADDRS]` | Logical address types. See [CEC Logical Address Types](#cec-log-addr-types) for possible types. The driver will update this with the actual logical address type that it claimed (e.g. it may have to fallback to [CEC\_LOG\_ADDR\_TYPE\_UNREGISTERED](#cec-log-addr-type-unregistered)). |
| \_\_u8 | `all_device_types[CEC_MAX_LOG_ADDRS]` | CEC 2.0 specific: the bit mask of all device types. See [CEC All Device Types Flags](#cec-all-dev-types-flags). It is used in the CEC 2.0 `CEC_MSG_REPORT_FEATURES` message. For CEC 1.4 you can either leave this field to 0, or fill it in according to the CEC 2.0 guidelines to give the CEC framework more information about the device type, even though the framework won’t use it directly in the CEC message. |
| \_\_u8 | `features[CEC_MAX_LOG_ADDRS][12]` | Features for each logical address. It is used in the CEC 2.0 `CEC_MSG_REPORT_FEATURES` message. The 12 bytes include both the RC Profile and the Device Features. For CEC 1.4 you can either leave this field to all 0, or fill it in according to the CEC 2.0 guidelines to give the CEC framework more information about the device type, even though the framework won’t use it directly in the CEC message. |

Flags for struct cec\_log\_addrs

|  |  |  |
| --- | --- | --- |
| `CEC_LOG_ADDRS_FL_ALLOW_UNREG_FALLBACK` | 1 | By default if no logical address of the requested type can be claimed, then it will go back to the unconfigured state. If this flag is set, then it will fallback to the Unregistered logical address. Note that if the Unregistered logical address was explicitly requested, then this flag has no effect. |
| `CEC_LOG_ADDRS_FL_ALLOW_RC_PASSTHRU` | 2 | By default the `CEC_MSG_USER_CONTROL_PRESSED` and `CEC_MSG_USER_CONTROL_RELEASED` messages are only passed on to the follower(s), if any. If this flag is set, then these messages are also passed on to the remote control input subsystem and will appear as keystrokes. This features needs to be enabled explicitly. If CEC is used to enter e.g. passwords, then you may not want to enable this to avoid trivial snooping of the keystrokes. |
| `CEC_LOG_ADDRS_FL_CDC_ONLY` | 4 | If this flag is set, then the device is CDC-Only. CDC-Only CEC devices are CEC devices that can only handle CDC messages.  All other messages are ignored. |

CEC Versions

|  |  |  |
| --- | --- | --- |
| `CEC_OP_CEC_VERSION_1_3A` | 4 | CEC version according to the HDMI 1.3a standard. |
| `CEC_OP_CEC_VERSION_1_4B` | 5 | CEC version according to the HDMI 1.4b standard. |
| `CEC_OP_CEC_VERSION_2_0` | 6 | CEC version according to the HDMI 2.0 standard. |

CEC Primary Device Types

|  |  |  |
| --- | --- | --- |
| `CEC_OP_PRIM_DEVTYPE_TV` | 0 | Use for a TV. |
| `CEC_OP_PRIM_DEVTYPE_RECORD` | 1 | Use for a recording device. |
| `CEC_OP_PRIM_DEVTYPE_TUNER` | 3 | Use for a device with a tuner. |
| `CEC_OP_PRIM_DEVTYPE_PLAYBACK` | 4 | Use for a playback device. |
| `CEC_OP_PRIM_DEVTYPE_AUDIOSYSTEM` | 5 | Use for an audio system (e.g. an audio/video receiver). |
| `CEC_OP_PRIM_DEVTYPE_SWITCH` | 6 | Use for a CEC switch. |
| `CEC_OP_PRIM_DEVTYPE_VIDEOPROC` | 7 | Use for a video processor device. |

CEC Logical Address Types

|  |  |  |
| --- | --- | --- |
| `CEC_LOG_ADDR_TYPE_TV` | 0 | Use for a TV. |
| `CEC_LOG_ADDR_TYPE_RECORD` | 1 | Use for a recording device. |
| `CEC_LOG_ADDR_TYPE_TUNER` | 2 | Use for a tuner device. |
| `CEC_LOG_ADDR_TYPE_PLAYBACK` | 3 | Use for a playback device. |
| `CEC_LOG_ADDR_TYPE_AUDIOSYSTEM` | 4 | Use for an audio system device. |
| `CEC_LOG_ADDR_TYPE_SPECIFIC` | 5 | Use for a second TV or for a video processor device. |
| `CEC_LOG_ADDR_TYPE_UNREGISTERED` | 6 | Use this if you just want to remain unregistered. Used for pure CEC switches or CDC-only devices (CDC: Capability Discovery and Control). |

CEC All Device Types Flags

|  |  |  |
| --- | --- | --- |
| `CEC_OP_ALL_DEVTYPE_TV` | 0x80 | This supports the TV type. |
| `CEC_OP_ALL_DEVTYPE_RECORD` | 0x40 | This supports the Recording type. |
| `CEC_OP_ALL_DEVTYPE_TUNER` | 0x20 | This supports the Tuner type. |
| `CEC_OP_ALL_DEVTYPE_PLAYBACK` | 0x10 | This supports the Playback type. |
| `CEC_OP_ALL_DEVTYPE_AUDIOSYSTEM` | 0x08 | This supports the Audio System type. |
| `CEC_OP_ALL_DEVTYPE_SWITCH` | 0x04 | This supports the CEC Switch or Video Processing type. |

## 2.6.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

The [ioctl CEC\_ADAP\_S\_LOG\_ADDRS](#cec-adap-s-log-addrs) can return the following
error codes:

ENOTTY
:   The `CEC_CAP_LOG_ADDRS` capability wasn’t set, so this ioctl is not supported.

EBUSY
:   The CEC adapter is currently configuring itself, or it is already configured and
    `num_log_addrs` is non-zero, or another filehandle is in exclusive follower or
    initiator mode, or the filehandle is in mode `CEC_MODE_NO_INITIATOR`.

EINVAL
:   The contents of struct [`cec_log_addrs`](#c.CEC.cec_log_addrs "cec_log_addrs") is invalid.
