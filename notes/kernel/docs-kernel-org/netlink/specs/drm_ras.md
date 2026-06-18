# Familydrm-rasnetlink specification

> 출처(원문): https://docs.kernel.org/netlink/specs/drm_ras.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# [Family `drm-ras` netlink specification](#id2)

## [Summary](#id3)

DRM RAS (Reliability, Availability, Serviceability) over Generic Netlink. Provides a standardized mechanism for DRM drivers to register “nodes” representing hardware/software components capable of reporting error counters. Userspace tools can query the list of nodes or individual error counters via the Generic Netlink interface.

## [Operations](#id4)

### [list-nodes](#id5)

Retrieve the full list of currently registered DRM RAS nodes. Each node includes its dynamically assigned ID, name, and type. **Important:** User space must call this operation first to obtain the node IDs. These IDs are required for all subsequent operations on nodes, such as querying error counters.

attribute-set:
:   [node-attrs](#drm-ras-attribute-set-node-attrs)

flags:
:   [`admin-perm`]

dump:
:   **reply**
    :   attributes:
        :   [`node-id`, `device-name`, `node-name`, `node-type`]

### [get-error-counter](#id6)

Retrieve error counter for a given node. The response includes the id, the name, and even the current value of each counter.

attribute-set:
:   [error-counter-attrs](#drm-ras-attribute-set-error-counter-attrs)

flags:
:   [`admin-perm`]

do:
:   **request**
    :   attributes:
        :   [`node-id`, `error-id`]

    **reply**
    :   attributes:
        :   [`error-id`, `error-name`, `error-value`]

dump:
:   **request**
    :   attributes:
        :   [`node-id`]

    **reply**
    :   attributes:
        :   [`error-id`, `error-name`, `error-value`]

## [Definitions](#id7)

### [node-type](#id8)

type:
:   enum

value-start:
:   1

doc:
:   Type of the node. Currently, only error-counter nodes are supported, which expose reliability counters for a hardware/software component.

entries:
:   * `error-counter`

## [Attribute sets](#id9)

### [node-attrs](#id10)

#### node-id (`u32`)

doc:
:   Unique identifier for the node. Assigned dynamically by the DRM RAS core upon registration.

#### device-name (`string`)

doc:
:   Device name chosen by the driver at registration. Can be a PCI BDF, UUID, or module name if unique.

#### node-name (`string`)

doc:
:   Node name chosen by the driver at registration. Can be an IP block name, or any name that identifies the RAS node inside the device.

#### node-type (`u32`)

doc:
:   Type of this node, identifying its function.

enum:
:   [node-type](#drm-ras-definition-node-type)

### [error-counter-attrs](#id11)

#### node-id (`u32`)

doc:
:   Node ID targeted by this error counter operation.

#### error-id (`u32`)

doc:
:   Unique identifier for a specific error counter within an node.

#### error-name (`string`)

doc:
:   Name of the error.

#### error-value (`u32`)

doc:
:   Current value of the requested error counter.
