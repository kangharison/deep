# 5.7.ioctl MEDIA_IOC_ENUM_LINKS

> 출처(원문): https://docs.kernel.org/userspace-api/media/mediactl/media-ioc-enum-links.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5.7. ioctl MEDIA\_IOC\_ENUM\_LINKS

## 5.7.1. Name

MEDIA\_IOC\_ENUM\_LINKS - Enumerate all pads and links for a given entity

## 5.7.2. Synopsis

MEDIA\_IOC\_ENUM\_LINKS

`int ioctl(int fd, MEDIA_IOC_ENUM_LINKS, struct media_links_enum *argp)`

## 5.7.3. Arguments

`fd`
:   File descriptor returned by [`open()`](media-func-open.html#c.MC.open "open").

`argp`
:   Pointer to struct [`media_links_enum`](#c.MC.media_links_enum "media_links_enum").

## 5.7.4. Description

To enumerate pads and/or links for a given entity, applications set the
entity field of a struct [`media_links_enum`](#c.MC.media_links_enum "media_links_enum")
structure and initialize the struct
[`media_pad_desc`](#c.MC.media_pad_desc "media_pad_desc") and struct
[`media_link_desc`](#c.MC.media_link_desc "media_link_desc") structure arrays pointed by
the `pads` and `links` fields. They then call the
MEDIA\_IOC\_ENUM\_LINKS ioctl with a pointer to this structure.

If the `pads` field is not NULL, the driver fills the `pads` array
with information about the entity’s pads. The array must have enough
room to store all the entity’s pads. The number of pads can be retrieved
with [ioctl MEDIA\_IOC\_ENUM\_ENTITIES](media-ioc-enum-entities.html#media-ioc-enum-entities).

If the `links` field is not NULL, the driver fills the `links` array
with information about the entity’s outbound links. The array must have
enough room to store all the entity’s outbound links. The number of
outbound links can be retrieved with [ioctl MEDIA\_IOC\_ENUM\_ENTITIES](media-ioc-enum-entities.html#media-ioc-enum-entities).

Only forward links that originate at one of the entity’s source pads are
returned during the enumeration process.

type media\_links\_enum

struct media\_links\_enum

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `entity` | Entity id, set by the application. |
| struct [`media_pad_desc`](#c.MC.media_pad_desc "media_pad_desc") | \*`pads` | Pointer to a pads array allocated by the application. Ignored if NULL. |
| struct [`media_link_desc`](#c.MC.media_link_desc "media_link_desc") | \*`links` | Pointer to a links array allocated by the application. Ignored if NULL. |
| \_\_u32 | `reserved[4]` | Reserved for future extensions. Drivers and applications must set the array to zero. |

type media\_pad\_desc

struct media\_pad\_desc

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `entity` | ID of the entity this pad belongs to. |
| \_\_u16 | `index` | Pad index, starts at 0. |
| \_\_u32 | `flags` | Pad flags, see [Media pad flags](media-types.html#media-pad-flag) for more details. |
| \_\_u32 | `reserved[2]` | Reserved for future extensions. Drivers and applications must set the array to zero. |

type media\_link\_desc

struct media\_link\_desc

|  |  |  |
| --- | --- | --- |
| struct [`media_pad_desc`](#c.MC.media_pad_desc "media_pad_desc") | `source` | Pad at the origin of this link. |
| struct [`media_pad_desc`](#c.MC.media_pad_desc "media_pad_desc") | `sink` | Pad at the target of this link. |
| \_\_u32 | `flags` | Link flags, see [Media link flags](media-types.html#media-link-flag) for more details. |
| \_\_u32 | `reserved[2]` | Reserved for future extensions. Drivers and applications must set the array to zero. |

## 5.7.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The struct [`media_links_enum`](#c.MC.media_links_enum "media_links_enum") `id`
    references a non-existing entity.
