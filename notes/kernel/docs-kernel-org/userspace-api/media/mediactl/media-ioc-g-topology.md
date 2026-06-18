# 5.5.ioctl MEDIA_IOC_G_TOPOLOGY

> 출처(원문): https://docs.kernel.org/userspace-api/media/mediactl/media-ioc-g-topology.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5.5. ioctl MEDIA\_IOC\_G\_TOPOLOGY

## 5.5.1. Name

MEDIA\_IOC\_G\_TOPOLOGY - Enumerate the graph topology and graph element properties

## 5.5.2. Synopsis

MEDIA\_IOC\_G\_TOPOLOGY

`int ioctl(int fd, MEDIA_IOC_G_TOPOLOGY, struct media_v2_topology *argp)`

## 5.5.3. Arguments

`fd`
:   File descriptor returned by [`open()`](media-func-open.html#c.MC.open "open").

`argp`
:   Pointer to struct [`media_v2_topology`](#c.MC.media_v2_topology "media_v2_topology").

## 5.5.4. Description

The typical usage of this ioctl is to call it twice. On the first call,
the structure defined at struct
[`media_v2_topology`](#c.MC.media_v2_topology "media_v2_topology") should be zeroed. At
return, if no errors happen, this ioctl will return the
`topology_version` and the total number of entities, interfaces, pads
and links.

Before the second call, the userspace should allocate arrays to store
the graph elements that are desired, putting the pointers to them at the
ptr\_entities, ptr\_interfaces, ptr\_links and/or ptr\_pads, keeping the
other values untouched.

If the `topology_version` remains the same, the ioctl should fill the
desired arrays with the media graph elements.

type media\_v2\_topology

struct media\_v2\_topology

|  |  |  |
| --- | --- | --- |
| \_\_u64 | `topology_version` | Version of the media graph topology. When the graph is created, this field starts with zero. Every time a graph element is added or removed, this field is incremented. |
| \_\_u32 | `num_entities` | Number of entities in the graph |
| \_\_u32 | `reserved1` | Applications and drivers shall set this to 0. |
| \_\_u64 | `ptr_entities` | A pointer to a memory area where the entities array will be stored, converted to a 64-bits integer. It can be zero. if zero, the ioctl won’t store the entities. It will just update `num_entities` |
| \_\_u32 | `num_interfaces` | Number of interfaces in the graph |
| \_\_u32 | `reserved2` | Applications and drivers shall set this to 0. |
| \_\_u64 | `ptr_interfaces` | A pointer to a memory area where the interfaces array will be stored, converted to a 64-bits integer. It can be zero. if zero, the ioctl won’t store the interfaces. It will just update `num_interfaces` |
| \_\_u32 | `num_pads` | Total number of pads in the graph |
| \_\_u32 | `reserved3` | Applications and drivers shall set this to 0. |
| \_\_u64 | `ptr_pads` | A pointer to a memory area where the pads array will be stored, converted to a 64-bits integer. It can be zero. if zero, the ioctl won’t store the pads. It will just update `num_pads` |
| \_\_u32 | `num_links` | Total number of data and interface links in the graph |
| \_\_u32 | `reserved4` | Applications and drivers shall set this to 0. |
| \_\_u64 | `ptr_links` | A pointer to a memory area where the links array will be stored, converted to a 64-bits integer. It can be zero. if zero, the ioctl won’t store the links. It will just update `num_links` |

type media\_v2\_entity

struct media\_v2\_entity

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `id` | Unique ID for the entity. Do not expect that the ID will always be the same for each instance of the device. In other words, do not hardcode entity IDs in an application. |
| char | `name`[64] | Entity name as an UTF-8 NULL-terminated string. This name must be unique within the media topology. |
| \_\_u32 | `function` | Entity main function, see [Media entity functions](media-types.html#media-entity-functions) for details. |
| \_\_u32 | `flags` | Entity flags, see [Media entity flags](media-types.html#media-entity-flag) for details. Only valid if `MEDIA_V2_ENTITY_HAS_FLAGS(media_version)` returns true. The `media_version` is defined in struct [`media_device_info`](media-ioc-device-info.html#c.MC.media_device_info "media_device_info") and can be retrieved using [ioctl MEDIA\_IOC\_DEVICE\_INFO](media-ioc-device-info.html#media-ioc-device-info). |
| \_\_u32 | `reserved`[5] | Reserved for future extensions. Drivers and applications must set this array to zero. |

type media\_v2\_interface

struct media\_v2\_interface

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `id` | Unique ID for the interface. Do not expect that the ID will always be the same for each instance of the device. In other words, do not hardcode interface IDs in an application. |
| \_\_u32 | `intf_type` | Interface type, see [Media interface types](media-types.html#media-intf-type) for details. |
| \_\_u32 | `flags` | Interface flags. Currently unused. |
| \_\_u32 | `reserved`[9] | Reserved for future extensions. Drivers and applications must set this array to zero. |
| [`struct media_v2_intf_devnode`](#c.MC.media_v2_intf_devnode "MC.media_v2_intf_devnode") | `devnode` | Used only for device node interfaces. See [`media_v2_intf_devnode`](#c.MC.media_v2_intf_devnode "media_v2_intf_devnode") for details. |

type media\_v2\_intf\_devnode

struct media\_v2\_intf\_devnode

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `major` | Device node major number. |
| \_\_u32 | `minor` | Device node minor number. |

type media\_v2\_pad

struct media\_v2\_pad

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `id` | Unique ID for the pad. Do not expect that the ID will always be the same for each instance of the device. In other words, do not hardcode pad IDs in an application. |
| \_\_u32 | `entity_id` | Unique ID for the entity where this pad belongs. |
| \_\_u32 | `flags` | Pad flags, see [Media pad flags](media-types.html#media-pad-flag) for more details. |
| \_\_u32 | `index` | Pad index, starts at 0. Only valid if `MEDIA_V2_PAD_HAS_INDEX(media_version)` returns true. The `media_version` is defined in struct [`media_device_info`](media-ioc-device-info.html#c.MC.media_device_info "media_device_info") and can be retrieved using [ioctl MEDIA\_IOC\_DEVICE\_INFO](media-ioc-device-info.html#media-ioc-device-info). |
| \_\_u32 | `reserved`[4] | Reserved for future extensions. Drivers and applications must set this array to zero. |

type media\_v2\_link

struct media\_v2\_link

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `id` | Unique ID for the link. Do not expect that the ID will always be the same for each instance of the device. In other words, do not hardcode link IDs in an application. |
| \_\_u32 | `source_id` | On pad to pad links: unique ID for the source pad.  On interface to entity links: unique ID for the interface. |
| \_\_u32 | `sink_id` | On pad to pad links: unique ID for the sink pad.  On interface to entity links: unique ID for the entity. |
| \_\_u32 | `flags` | Link flags, see [Media link flags](media-types.html#media-link-flag) for more details. |
| \_\_u32 | `reserved`[6] | Reserved for future extensions. Drivers and applications must set this array to zero. |

## 5.5.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

ENOSPC
:   This is returned when either one or more of the num\_entities,
    num\_interfaces, num\_links or num\_pads are non-zero and are
    smaller than the actual number of elements inside the graph. This
    may happen if the `topology_version` changed when compared to the
    last time this ioctl was called. Userspace should usually free the
    area for the pointers, zero the `struct elements` and call this ioctl
    again.
