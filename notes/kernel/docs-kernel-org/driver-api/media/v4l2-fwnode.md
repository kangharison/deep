# 3.23.V4L2 fwnode kAPI

> 출처(원문): https://docs.kernel.org/driver-api/media/v4l2-fwnode.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.23. V4L2 fwnode kAPI

struct v4l2\_fwnode\_endpoint
:   the endpoint data structure

> **Definition**:
>
> ```
> struct v4l2_fwnode_endpoint {
>       struct fwnode_endpoint base;
>       enum v4l2_mbus_type bus_type;
>       struct {
>             struct v4l2_mbus_config_parallel parallel;
>             struct v4l2_mbus_config_mipi_csi1 mipi_csi1;
>             struct v4l2_mbus_config_mipi_csi2 mipi_csi2;
>       } bus;
>       u64 *link_frequencies;
>       unsigned int nr_of_link_frequencies;
> };
> ```

**Members**

`base`
:   fwnode endpoint of the v4l2\_fwnode

`bus_type`
:   bus type

`bus`
:   bus configuration data structure

`bus.parallel`
:   embedded [`struct v4l2_mbus_config_parallel`](v4l2-mediabus.html#c.v4l2_mbus_config_parallel "v4l2_mbus_config_parallel").
    Used if the bus is parallel.

`bus.mipi_csi1`
:   embedded [`struct v4l2_mbus_config_mipi_csi1`](v4l2-mediabus.html#c.v4l2_mbus_config_mipi_csi1 "v4l2_mbus_config_mipi_csi1").
    Used if the bus is MIPI Alliance’s Camera Serial
    Interface version 1 (MIPI CSI1) or Standard
    Mobile Imaging Architecture’s Compact Camera Port 2
    (SMIA CCP2).

`bus.mipi_csi2`
:   embedded [`struct v4l2_mbus_config_mipi_csi2`](v4l2-mediabus.html#c.v4l2_mbus_config_mipi_csi2 "v4l2_mbus_config_mipi_csi2").
    Used if the bus is MIPI Alliance’s Camera Serial
    Interface version 2 (MIPI CSI2).

`link_frequencies`
:   array of supported link frequencies

`nr_of_link_frequencies`
:   number of elements in link\_frequenccies array

V4L2\_FWNODE\_PROPERTY\_UNSET

`V4L2_FWNODE_PROPERTY_UNSET`

> > identify a non initialized property
>
> **Description**
>
> All properties in [`struct v4l2_fwnode_device_properties`](#c.v4l2_fwnode_device_properties "v4l2_fwnode_device_properties") are initialized
> to this value.

enum v4l2\_fwnode\_orientation
:   possible device orientation

**Constants**

`V4L2_FWNODE_ORIENTATION_FRONT`
:   device installed on the front side

`V4L2_FWNODE_ORIENTATION_BACK`
:   device installed on the back side

`V4L2_FWNODE_ORIENTATION_EXTERNAL`
:   device externally located

struct v4l2\_fwnode\_device\_properties
:   fwnode device properties

> **Definition**:
>
> ```
> struct v4l2_fwnode_device_properties {
>       enum v4l2_fwnode_orientation orientation;
>       unsigned int rotation;
> };
> ```

**Members**

`orientation`
:   device orientation. See [`enum v4l2_fwnode_orientation`](#c.v4l2_fwnode_orientation "v4l2_fwnode_orientation")

`rotation`
:   device rotation

struct v4l2\_fwnode\_link
:   a link between two endpoints

> **Definition**:
>
> ```
> struct v4l2_fwnode_link {
>       struct fwnode_handle *local_node;
>       unsigned int local_port;
>       unsigned int local_id;
>       struct fwnode_handle *remote_node;
>       unsigned int remote_port;
>       unsigned int remote_id;
> };
> ```

**Members**

`local_node`
:   pointer to device\_node of this endpoint

`local_port`
:   identifier of the port this endpoint belongs to

`local_id`
:   identifier of the id this endpoint belongs to

`remote_node`
:   pointer to device\_node of the remote endpoint

`remote_port`
:   identifier of the port the remote endpoint belongs to

`remote_id`
:   identifier of the id the remote endpoint belongs to

enum v4l2\_connector\_type
:   connector type

**Constants**

`V4L2_CONN_UNKNOWN`
:   unknown connector type, no V4L2 connector configuration

`V4L2_CONN_COMPOSITE`
:   analog composite connector

`V4L2_CONN_SVIDEO`
:   analog svideo connector

struct v4l2\_connector\_link
:   connector link data structure

> **Definition**:
>
> ```
> struct v4l2_connector_link {
>       struct list_head head;
>       struct v4l2_fwnode_link fwnode_link;
> };
> ```

**Members**

`head`
:   structure to be used to add the link to the
    [`struct v4l2_fwnode_connector`](#c.v4l2_fwnode_connector "v4l2_fwnode_connector")

`fwnode_link`
:   [`struct v4l2_fwnode_link`](#c.v4l2_fwnode_link "v4l2_fwnode_link") link between the connector and the
    device the connector belongs to.

struct v4l2\_fwnode\_connector\_analog
:   analog connector data structure

> **Definition**:
>
> ```
> struct v4l2_fwnode_connector_analog {
>       v4l2_std_id sdtv_stds;
> };
> ```

**Members**

`sdtv_stds`
:   sdtv standards this connector supports, set to V4L2\_STD\_ALL
    if no restrictions are specified.

struct v4l2\_fwnode\_connector
:   the connector data structure

> **Definition**:
>
> ```
> struct v4l2_fwnode_connector {
>       const char *name;
>       const char *label;
>       enum v4l2_connector_type type;
>       struct list_head links;
>       unsigned int nr_of_links;
>       union {
>             struct v4l2_fwnode_connector_analog analog;
>       } connector;
> };
> ```

**Members**

`name`
:   the connector device name

`label`
:   optional connector label

`type`
:   connector type

`links`
:   list of all connector [`struct v4l2_connector_link`](#c.v4l2_connector_link "v4l2_connector_link") links

`nr_of_links`
:   total number of links

`connector`
:   connector configuration

`connector.analog`
:   analog connector configuration
    [`struct v4l2_fwnode_connector_analog`](#c.v4l2_fwnode_connector_analog "v4l2_fwnode_connector_analog")

enum v4l2\_fwnode\_bus\_type
:   Video bus types defined by firmware properties

**Constants**

`V4L2_FWNODE_BUS_TYPE_GUESS`
:   Default value if no bus-type fwnode property

`V4L2_FWNODE_BUS_TYPE_CSI2_CPHY`
:   MIPI CSI-2 bus, C-PHY physical layer

`V4L2_FWNODE_BUS_TYPE_CSI1`
:   MIPI CSI-1 bus

`V4L2_FWNODE_BUS_TYPE_CCP2`
:   SMIA Compact Camera Port 2 bus

`V4L2_FWNODE_BUS_TYPE_CSI2_DPHY`
:   MIPI CSI-2 bus, D-PHY physical layer

`V4L2_FWNODE_BUS_TYPE_PARALLEL`
:   Camera Parallel Interface bus

`V4L2_FWNODE_BUS_TYPE_BT656`
:   BT.656 video format bus-type

`V4L2_FWNODE_BUS_TYPE_DPI`
:   Video Parallel Interface bus

`NR_OF_V4L2_FWNODE_BUS_TYPE`
:   Number of bus-types

int v4l2\_fwnode\_endpoint\_parse(struct fwnode\_handle \*fwnode, struct [v4l2\_fwnode\_endpoint](#c.v4l2_fwnode_endpoint "v4l2_fwnode_endpoint") \*vep)
:   parse all fwnode node properties

**Parameters**

`struct fwnode_handle *fwnode`
:   pointer to the endpoint’s fwnode handle (may be NULL)

`struct v4l2_fwnode_endpoint *vep`
:   pointer to the V4L2 fwnode data structure

**Description**

This function parses the V4L2 fwnode endpoint specific parameters from the
firmware. There are two ways to use this function, either by letting it
obtain the type of the bus (by setting the **vep.bus\_type** field to
V4L2\_MBUS\_UNKNOWN) or specifying the bus type explicitly to one of the [`enum
v4l2_mbus_type`](v4l2-mediabus.html#c.v4l2_mbus_type "v4l2_mbus_type") types.

When **vep.bus\_type** is V4L2\_MBUS\_UNKNOWN, the function will use the “bus-type”
property to determine the type when it is available. The caller is
responsible for validating the contents of **vep.bus\_type** field after the call
returns.

As a deprecated functionality to support older DT bindings without “bus-type”
property for devices that support multiple types, if the “bus-type” property
does not exist, the function will attempt to guess the type based on the
endpoint properties available. NEVER RELY ON GUESSING THE BUS TYPE IN NEW
DRIVERS OR BINDINGS.

It is also possible to set **vep.bus\_type** corresponding to an actual bus. In
this case the function will only attempt to parse properties related to this
bus, and it will return an error if the value of the “bus-type” property
corresponds to a different bus.

The caller is required to initialise all fields of **vep**, either with
explicitly values, or by zeroing them.

The function does not change the V4L2 fwnode endpoint state if it fails.

**NOTE**

This function does not parse “link-frequencies” property as its size is
not known in advance. Please use [`v4l2_fwnode_endpoint_alloc_parse()`](#c.v4l2_fwnode_endpoint_alloc_parse "v4l2_fwnode_endpoint_alloc_parse") if you
need properties of variable size.

**Return**

`0` on success or a negative error code on failure:
`-ENOMEM` on memory allocation failure
`-EINVAL` on parsing failure
`-ENXIO` on mismatching bus types
`-EPROBE_DEFER` on NULL **fwnode**

void v4l2\_fwnode\_endpoint\_free(struct [v4l2\_fwnode\_endpoint](#c.v4l2_fwnode_endpoint "v4l2_fwnode_endpoint") \*vep)
:   free the V4L2 fwnode acquired by [`v4l2_fwnode_endpoint_alloc_parse()`](#c.v4l2_fwnode_endpoint_alloc_parse "v4l2_fwnode_endpoint_alloc_parse")

**Parameters**

`struct v4l2_fwnode_endpoint *vep`
:   the V4L2 fwnode the resources of which are to be released

**Description**

It is safe to call this function with NULL argument or on a V4L2 fwnode the
parsing of which failed.

int v4l2\_fwnode\_endpoint\_alloc\_parse(struct fwnode\_handle \*fwnode, struct [v4l2\_fwnode\_endpoint](#c.v4l2_fwnode_endpoint "v4l2_fwnode_endpoint") \*vep)
:   parse all fwnode node properties

**Parameters**

`struct fwnode_handle *fwnode`
:   pointer to the endpoint’s fwnode handle (may be NULL)

`struct v4l2_fwnode_endpoint *vep`
:   pointer to the V4L2 fwnode data structure

**Description**

This function parses the V4L2 fwnode endpoint specific parameters from the
firmware. There are two ways to use this function, either by letting it
obtain the type of the bus (by setting the **vep.bus\_type** field to
V4L2\_MBUS\_UNKNOWN) or specifying the bus type explicitly to one of the [`enum
v4l2_mbus_type`](v4l2-mediabus.html#c.v4l2_mbus_type "v4l2_mbus_type") types.

When **vep.bus\_type** is V4L2\_MBUS\_UNKNOWN, the function will use the “bus-type”
property to determine the type when it is available. The caller is
responsible for validating the contents of **vep.bus\_type** field after the call
returns.

As a deprecated functionality to support older DT bindings without “bus-type”
property for devices that support multiple types, if the “bus-type” property
does not exist, the function will attempt to guess the type based on the
endpoint properties available. NEVER RELY ON GUESSING THE BUS TYPE IN NEW
DRIVERS OR BINDINGS.

It is also possible to set **vep.bus\_type** corresponding to an actual bus. In
this case the function will only attempt to parse properties related to this
bus, and it will return an error if the value of the “bus-type” property
corresponds to a different bus.

The caller is required to initialise all fields of **vep**, either with
explicitly values, or by zeroing them.

The function does not change the V4L2 fwnode endpoint state if it fails.

[`v4l2_fwnode_endpoint_alloc_parse()`](#c.v4l2_fwnode_endpoint_alloc_parse "v4l2_fwnode_endpoint_alloc_parse") has two important differences to
[`v4l2_fwnode_endpoint_parse()`](#c.v4l2_fwnode_endpoint_parse "v4l2_fwnode_endpoint_parse"):

1. It also parses variable size data.
2. The memory it has allocated to store the variable size data must be freed
   using [`v4l2_fwnode_endpoint_free()`](#c.v4l2_fwnode_endpoint_free "v4l2_fwnode_endpoint_free") when no longer needed.

**Return**

`0` on success or a negative error code on failure:
`-ENOMEM` on memory allocation failure
`-EINVAL` on parsing failure
`-ENXIO` on mismatching bus types
`-EPROBE_DEFER` on NULL **fwnode**

int v4l2\_fwnode\_parse\_link(struct fwnode\_handle \*fwnode, struct [v4l2\_fwnode\_link](#c.v4l2_fwnode_link "v4l2_fwnode_link") \*link)
:   parse a link between two endpoints

**Parameters**

`struct fwnode_handle *fwnode`
:   pointer to the endpoint’s fwnode at the local end of the link

`struct v4l2_fwnode_link *link`
:   pointer to the V4L2 fwnode link data structure

**Description**

Fill the link structure with the local and remote nodes and port numbers.
The local\_node and remote\_node fields are set to point to the local and
remote port’s parent nodes respectively (the port parent node being the
parent node of the port node if that node isn’t a ‘ports’ node, or the
grand-parent node of the port node otherwise).

A reference is taken to both the local and remote nodes, the caller must use
[`v4l2_fwnode_put_link()`](#c.v4l2_fwnode_put_link "v4l2_fwnode_put_link") to drop the references when done with the
link.

**Return**

0 on success, or -ENOLINK if the remote endpoint fwnode can’t be
found.

void v4l2\_fwnode\_put\_link(struct [v4l2\_fwnode\_link](#c.v4l2_fwnode_link "v4l2_fwnode_link") \*link)
:   drop references to nodes in a link

**Parameters**

`struct v4l2_fwnode_link *link`
:   pointer to the V4L2 fwnode link data structure

**Description**

Drop references to the local and remote nodes in the link. This function
must be called on every link parsed with [`v4l2_fwnode_parse_link()`](#c.v4l2_fwnode_parse_link "v4l2_fwnode_parse_link").

void v4l2\_fwnode\_connector\_free(struct [v4l2\_fwnode\_connector](#c.v4l2_fwnode_connector "v4l2_fwnode_connector") \*connector)
:   free the V4L2 connector acquired memory

**Parameters**

`struct v4l2_fwnode_connector *connector`
:   the V4L2 connector resources of which are to be released

**Description**

Free all allocated memory and put all links acquired by
[`v4l2_fwnode_connector_parse()`](#c.v4l2_fwnode_connector_parse "v4l2_fwnode_connector_parse") and [`v4l2_fwnode_connector_add_link()`](#c.v4l2_fwnode_connector_add_link "v4l2_fwnode_connector_add_link").

It is safe to call this function with NULL argument or on a V4L2 connector
the parsing of which failed.

int v4l2\_fwnode\_connector\_parse(struct fwnode\_handle \*fwnode, struct [v4l2\_fwnode\_connector](#c.v4l2_fwnode_connector "v4l2_fwnode_connector") \*connector)
:   initialize the ‘[`struct v4l2_fwnode_connector`](#c.v4l2_fwnode_connector "v4l2_fwnode_connector")’

**Parameters**

`struct fwnode_handle *fwnode`
:   pointer to the subdev endpoint’s fwnode handle where the connector
    is connected to or to the connector endpoint fwnode handle.

`struct v4l2_fwnode_connector *connector`
:   pointer to the V4L2 fwnode connector data structure

**Description**

Fill the [`struct v4l2_fwnode_connector`](#c.v4l2_fwnode_connector "v4l2_fwnode_connector") with the connector type, label and
all [`enum v4l2_connector_type`](#c.v4l2_connector_type "v4l2_connector_type") specific connector data. The label is optional
so it is set to `NULL` if no one was found. The function initialize the links
to zero. Adding links to the connector is done by calling
[`v4l2_fwnode_connector_add_link()`](#c.v4l2_fwnode_connector_add_link "v4l2_fwnode_connector_add_link").

The memory allocated for the label must be freed when no longer needed.
Freeing the memory is done by [`v4l2_fwnode_connector_free()`](#c.v4l2_fwnode_connector_free "v4l2_fwnode_connector_free").

**Return**

* `0` on success or a negative error code on failure:
* `-EINVAL` if **fwnode** is invalid
* `-ENOTCONN` if connector type is unknown or connector device can’t be found

int v4l2\_fwnode\_connector\_add\_link(struct fwnode\_handle \*fwnode, struct [v4l2\_fwnode\_connector](#c.v4l2_fwnode_connector "v4l2_fwnode_connector") \*connector)
:   add a link between a connector node and a v4l2-subdev node.

**Parameters**

`struct fwnode_handle *fwnode`
:   pointer to the subdev endpoint’s fwnode handle where the connector
    is connected to

`struct v4l2_fwnode_connector *connector`
:   pointer to the V4L2 fwnode connector data structure

**Description**

Add a new [`struct v4l2_connector_link`](#c.v4l2_connector_link "v4l2_connector_link") link to the
[`struct v4l2_fwnode_connector`](#c.v4l2_fwnode_connector "v4l2_fwnode_connector") connector links list. The link local\_node
points to the connector node, the remote\_node to the host v4l2 (sub)dev.

The taken references to remote\_node and local\_node must be dropped and the
allocated memory must be freed when no longer needed. Both is done by calling
[`v4l2_fwnode_connector_free()`](#c.v4l2_fwnode_connector_free "v4l2_fwnode_connector_free").

**Return**

* `0` on success or a negative error code on failure:
* `-EINVAL` if **fwnode** or **connector** is invalid or **connector** type is unknown
* `-ENOMEM` on link memory allocation failure
* `-ENOTCONN` if remote connector device can’t be found
* `-ENOLINK` if link parsing between v4l2 (sub)dev and connector fails

int v4l2\_fwnode\_device\_parse(struct [device](../infrastructure.html#c.device "device") \*dev, struct [v4l2\_fwnode\_device\_properties](#c.v4l2_fwnode_device_properties "v4l2_fwnode_device_properties") \*props)
:   parse fwnode device properties

**Parameters**

`struct device *dev`
:   pointer to [`struct device`](../infrastructure.html#c.device "device")

`struct v4l2_fwnode_device_properties *props`
:   pointer to [`struct v4l2_fwnode_device_properties`](#c.v4l2_fwnode_device_properties "v4l2_fwnode_device_properties") where to store the
    parsed properties values

**Description**

This function parses and validates the V4L2 fwnode device properties from the
firmware interface, and fills the **struct** v4l2\_fwnode\_device\_properties
provided by the caller.

**Return**

% 0 on success
`-EINVAL` if a parsed property value is not valid
