# DRM RAS over Generic Netlink

> 출처(원문): https://docs.kernel.org/gpu/drm-ras.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# DRM RAS over Generic Netlink

The DRM RAS (Reliability, Availability, Serviceability) interface provides a
standardized way for GPU/accelerator drivers to expose error counters and
other reliability nodes to user space via Generic Netlink. This allows
diagnostic tools, monitoring daemons, or test infrastructure to query hardware
health in a uniform way across different DRM drivers.

Key Goals:

* Provide a standardized RAS solution for GPU and accelerator drivers, enabling
  data center monitoring and reliability operations.
* Implement a single drm-ras Generic Netlink family to meet modern Netlink YAML
  specifications and centralize all RAS-related communication in one namespace.
* Support a basic error counter interface, addressing the immediate, essential
  monitoring needs.
* Offer a flexible, future-proof interface that can be extended to support
  additional types of RAS data in the future.
* Allow multiple nodes per driver, enabling drivers to register separate
  nodes for different IP blocks, sub-blocks, or other logical subdivisions
  as applicable.

## Nodes

Nodes are logical abstractions representing an error type or error source within
the device. Currently, only error counter nodes is supported.

Drivers are responsible for registering and unregistering nodes via the
`drm_ras_node_register()` and `drm_ras_node_unregister()` APIs.

### Node Management

This module provides the infrastructure to manage RAS (Reliability,
Availability, and Serviceability) nodes for DRM drivers. Each
DRM driver may register one or more RAS nodes, which represent
logical components capable of reporting error counters and other
reliability metrics.

The nodes are stored in a global xarray drm\_ras\_xa to allow
efficient lookup by ID. Nodes can be registered or unregistered
dynamically at runtime.

A Generic Netlink family drm\_ras exposes two main operations to
userspace:

1. LIST\_NODES: Dump all currently registered RAS nodes.
   The user receives an array of node IDs, names, and types.
2. GET\_ERROR\_COUNTER: Get error counters of a given node.
   Userspace must provide Node ID, Error ID (Optional for specific counter).
   Returns all counters of a node if only Node ID is provided or specific
   error counters.

Node registration:

* `drm_ras_node_register()`: Registers a new node and assigns
  it a unique ID in the xarray.
* `drm_ras_node_unregister()`: Removes a previously registered
  node from the xarray.

Node type:

* ERROR\_COUNTER:
  :   + Currently, only error counters are supported.
      + The driver must implement the `query_error_counter()` callback to provide
        the name and the value of the error counter.
      + The driver must provide a error\_counter\_range.last value informing the
        last valid error ID.
      + The driver can provide a error\_counter\_range.first value informing the
        first valid error ID.
      + The error counters in the driver doesn’t need to be contiguous, but the
        driver must return -ENOENT to the query\_error\_counter as an indication
        that the ID should be skipped and not listed in the netlink API.

Netlink handlers:

* [`drm_ras_nl_list_nodes_dumpit()`](#c.drm_ras_nl_list_nodes_dumpit "drm_ras_nl_list_nodes_dumpit"): Implements the LIST\_NODES
  operation, iterating over the xarray.
* [`drm_ras_nl_get_error_counter_dumpit()`](#c.drm_ras_nl_get_error_counter_dumpit "drm_ras_nl_get_error_counter_dumpit"): Implements the GET\_ERROR\_COUNTER dumpit
  operation, fetching all counters from a specific node.
* [`drm_ras_nl_get_error_counter_doit()`](#c.drm_ras_nl_get_error_counter_doit "drm_ras_nl_get_error_counter_doit"): Implements the GET\_ERROR\_COUNTER doit
  operation, fetching a counter value from a specific node.

int drm\_ras\_nl\_list\_nodes\_dumpit(struct [sk\_buff](../networking/kapi.html#c.sk_buff "sk_buff") \*skb, struct netlink\_callback \*cb)
:   Dump all registered RAS nodes

**Parameters**

`struct sk_buff *skb`
:   Netlink message buffer

`struct netlink_callback *cb`
:   Callback context for multi-part dumps

**Description**

Iterates over all registered RAS nodes in the global xarray and appends
their attributes (ID, name, type) to the given netlink message buffer.
Uses **cb->ctx** to track progress in case the message buffer fills up, allowing
multi-part dump support. On buffer overflow, updates the context to resume
from the last node on the next invocation.

**Return**

0 if all nodes fit in **skb**, number of bytes added to **skb** if
the buffer filled up (requires multi-part continuation), or
a negative error code on failure.

int drm\_ras\_nl\_get\_error\_counter\_dumpit(struct [sk\_buff](../networking/kapi.html#c.sk_buff "sk_buff") \*skb, struct netlink\_callback \*cb)
:   Dump all Error Counters

**Parameters**

`struct sk_buff *skb`
:   Netlink message buffer

`struct netlink_callback *cb`
:   Callback context for multi-part dumps

**Description**

Iterates over all error counters in a given Node and appends
their attributes (ID, name, value) to the given netlink message buffer.
Uses **cb->ctx** to track progress in case the message buffer fills up, allowing
multi-part dump support. On buffer overflow, updates the context to resume
from the last node on the next invocation.

**Return**

0 if all errors fit in **skb**, number of bytes added to **skb** if
the buffer filled up (requires multi-part continuation), or
a negative error code on failure.

int drm\_ras\_nl\_get\_error\_counter\_doit(struct [sk\_buff](../networking/kapi.html#c.sk_buff "sk_buff") \*skb, struct genl\_info \*info)
:   Query an error counter of an node

**Parameters**

`struct sk_buff *skb`
:   Netlink message buffer

`struct genl_info *info`
:   Generic Netlink info containing attributes of the request

**Description**

Extracts the node ID and error ID from the netlink attributes and
retrieves the current value of the corresponding error counter. Sends the
result back to the requesting user via the standard Genl reply.

**Return**

0 on success, or negative errno on failure.

## Generic Netlink Usage

The interface is implemented as a Generic Netlink family named `drm-ras`.
User space tools can:

* List registered nodes with the `list-nodes` command.
* List all error counters in an node with the `get-error-counter` command with `node-id`
  as a parameter.
* Query specific error counter values with the `get-error-counter` command, using both
  `node-id` and `error-id` as parameters.

### YAML-based Interface

The interface is described in a YAML specification `Documentation/netlink/specs/drm_ras.yaml`

This YAML is used to auto-generate user space bindings via
`tools/net/ynl/pyynl/ynl_gen_c.py`, and drives the structure of netlink
attributes and operations.

### Usage Notes

* User space must first enumerate nodes to obtain their IDs.
* Node IDs or Node names can be used for all further queries, such as error counters.
* Error counters can be queried by either the Error ID or Error name.
* Query Parameters should be defined as part of the uAPI to ensure user interface stability.
* The interface supports future extension by adding new node types and
  additional attributes.

Example: List nodes using ynl

```
sudo ynl --family drm_ras --dump list-nodes
[{'device-name': '0000:03:00.0',
'node-id': 0,
'node-name': 'correctable-errors',
'node-type': 'error-counter'},
{'device-name': '0000:03:00.0',
 'node-id': 1,
 'node-name': 'uncorrectable-errors',
 'node-type': 'error-counter'}]
```

Example: List all error counters using ynl

```
sudo ynl --family drm_ras --dump get-error-counter --json '{"node-id":0}'
[{'error-id': 1, 'error-name': 'error_name1', 'error-value': 0},
{'error-id': 2, 'error-name': 'error_name2', 'error-value': 0}]
```

Example: Query an error counter for a given node

```
sudo ynl --family drm_ras --do get-error-counter --json '{"node-id":0, "error-id":1}'
{'error-id': 1, 'error-name': 'error_name1', 'error-value': 0}
```
