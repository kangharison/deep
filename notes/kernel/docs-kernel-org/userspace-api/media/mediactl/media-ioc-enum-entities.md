# 5.6.ioctl MEDIA_IOC_ENUM_ENTITIES

> 출처(원문): https://docs.kernel.org/userspace-api/media/mediactl/media-ioc-enum-entities.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5.6. ioctl MEDIA\_IOC\_ENUM\_ENTITIES

## 5.6.1. Name

MEDIA\_IOC\_ENUM\_ENTITIES - Enumerate entities and their properties

## 5.6.2. Synopsis

MEDIA\_IOC\_ENUM\_ENTITIES

`int ioctl(int fd, MEDIA_IOC_ENUM_ENTITIES, struct media_entity_desc *argp)`

## 5.6.3. Arguments

`fd`
:   File descriptor returned by [`open()`](media-func-open.html#c.MC.open "open").

`argp`
:   Pointer to struct [`media_entity_desc`](#c.MC.media_entity_desc "media_entity_desc").

## 5.6.4. Description

To query the attributes of an entity, applications set the id field of a
struct [`media_entity_desc`](#c.MC.media_entity_desc "media_entity_desc") structure and
call the MEDIA\_IOC\_ENUM\_ENTITIES ioctl with a pointer to this
structure. The driver fills the rest of the structure or returns an
EINVAL error code when the id is invalid.

Entities can be enumerated by or’ing the id with the
`MEDIA_ENT_ID_FLAG_NEXT` flag. The driver will return information
about the entity with the smallest id strictly larger than the requested
one (‘next entity’), or the `EINVAL` error code if there is none.

Entity IDs can be non-contiguous. Applications must *not* try to
enumerate entities by calling MEDIA\_IOC\_ENUM\_ENTITIES with increasing
id’s until they get an error.

type media\_entity\_desc

struct media\_entity\_desc

|  |  |  |  |
| --- | --- | --- | --- |
| \_\_u32 | `id` |  | Entity ID, set by the application. When the ID is or’ed with `MEDIA_ENT_ID_FLAG_NEXT`, the driver clears the flag and returns the first entity with a larger ID. Do not expect that the ID will always be the same for each instance of the device. In other words, do not hardcode entity IDs in an application. |
| char | `name`[32] |  | Entity name as an UTF-8 NULL-terminated string. This name must be unique within the media topology. |
| \_\_u32 | `type` |  | Entity type, see [Media entity functions](media-types.html#media-entity-functions) for details. |
| \_\_u32 | `revision` |  | Entity revision. Always zero (obsolete) |
| \_\_u32 | `flags` |  | Entity flags, see [Media entity flags](media-types.html#media-entity-flag) for details. |
| \_\_u32 | `group_id` |  | Entity group ID. Always zero (obsolete) |
| \_\_u16 | `pads` |  | Number of pads |
| \_\_u16 | `links` |  | Total number of outbound links. Inbound links are not counted in this field. |
| \_\_u32 | `reserved[4]` |  | Reserved for future extensions. Drivers and applications must set the array to zero. |
| union { | (anonymous) | | |
| struct | `dev` |  | Valid for (sub-)devices that create a single device node. |
|  | \_\_u32 | `major` | Device node major number. |
|  | \_\_u32 | `minor` | Device node minor number. |
| \_\_u8 | `raw`[184] |  |  |
| } |  | | |

## 5.6.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The struct [`media_entity_desc`](#c.MC.media_entity_desc "media_entity_desc") `id`
    references a non-existing entity.
