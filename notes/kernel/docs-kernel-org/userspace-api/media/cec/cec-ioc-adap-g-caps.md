# 2.5.ioctl CEC_ADAP_G_CAPS

> 출처(원문): https://docs.kernel.org/userspace-api/media/cec/cec-ioc-adap-g-caps.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.5. ioctl CEC\_ADAP\_G\_CAPS

## 2.5.1. Name

CEC\_ADAP\_G\_CAPS - Query device capabilities

## 2.5.2. Synopsis

CEC\_ADAP\_G\_CAPS

`int ioctl(int fd, CEC_ADAP_G_CAPS, struct cec_caps *argp)`

## 2.5.3. Arguments

`fd`
:   File descriptor returned by [`open()`](cec-func-open.html#c.CEC.open "open").

`argp`

## 2.5.4. Description

All cec devices must support [ioctl CEC\_ADAP\_G\_CAPS](#cec-adap-g-caps). To query
device information, applications call the ioctl with a pointer to a
struct [`cec_caps`](#c.CEC.cec_caps "cec_caps"). The driver fills the structure and
returns the information to the application. The ioctl never fails.

type cec\_caps

struct cec\_caps

|  |  |  |
| --- | --- | --- |
| char | `driver[32]` | The name of the cec adapter driver. |
| char | `name[32]` | The name of this CEC adapter. The combination `driver` and `name` must be unique. |
| \_\_u32 | `available_log_addrs` | The maximum number of logical addresses that can be configured. |
| \_\_u32 | `capabilities` | The capabilities of the CEC adapter, see [CEC Capabilities Flags](#cec-capabilities). |
| \_\_u32 | `version` | CEC Framework API version, formatted with the `KERNEL_VERSION()` macro. |

CEC Capabilities Flags

|  |  |  |
| --- | --- | --- |
| `CEC_CAP_PHYS_ADDR` | 0x00000001 | Userspace has to configure the physical address by calling [ioctl CEC\_ADAP\_S\_PHYS\_ADDR](cec-ioc-adap-g-phys-addr.html#cec-adap-s-phys-addr). If this capability isn’t set, then setting the physical address is handled by the kernel whenever the EDID is set (for an HDMI receiver) or read (for an HDMI transmitter). |
| `CEC_CAP_LOG_ADDRS` | 0x00000002 | Userspace has to configure the logical addresses by calling [ioctl CEC\_ADAP\_S\_LOG\_ADDRS](cec-ioc-adap-g-log-addrs.html#cec-adap-s-log-addrs). If this capability isn’t set, then the kernel will have configured this. |
| `CEC_CAP_TRANSMIT` | 0x00000004 | Userspace can transmit CEC messages by calling [ioctl CEC\_TRANSMIT](cec-ioc-receive.html#cec-transmit). This implies that userspace can be a follower as well, since being able to transmit messages is a prerequisite of becoming a follower. If this capability isn’t set, then the kernel will handle all CEC transmits and process all CEC messages it receives. |
| `CEC_CAP_PASSTHROUGH` | 0x00000008 | Userspace can use the passthrough mode by calling [ioctl CEC\_S\_MODE](cec-ioc-g-mode.html#cec-s-mode). |
| `CEC_CAP_RC` | 0x00000010 | This adapter supports the remote control protocol. |
| `CEC_CAP_MONITOR_ALL` | 0x00000020 | The CEC hardware can monitor all messages, not just directed and broadcast messages. |
| `CEC_CAP_NEEDS_HPD` | 0x00000040 | The CEC hardware is only active if the HDMI Hotplug Detect pin is high. This makes it impossible to use CEC to wake up displays that set the HPD pin low when in standby mode, but keep the CEC bus alive. |
| `CEC_CAP_MONITOR_PIN` | 0x00000080 | The CEC hardware can monitor CEC pin changes from low to high voltage and vice versa. When in pin monitoring mode the application will receive `CEC_EVENT_PIN_CEC_LOW` and `CEC_EVENT_PIN_CEC_HIGH` events. |
| `CEC_CAP_CONNECTOR_INFO` | 0x00000100 | If this capability is set, then [ioctl CEC\_ADAP\_G\_CONNECTOR\_INFO](cec-ioc-adap-g-conn-info.html#cec-adap-g-connector-info) can be used. |
| `CEC_CAP_REPLY_VENDOR_ID` | 0x00000200 | If this capability is set, then [CEC\_MSG\_FL\_REPLY\_VENDOR\_ID](cec-ioc-receive.html#cec-msg-flags) can be used. |

## 2.5.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.
