# Generic System Interconnect Subsystem

> 출처(원문): https://docs.kernel.org/driver-api/interconnect.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Generic System Interconnect Subsystem

## Introduction

This framework is designed to provide a standard kernel interface to control
the settings of the interconnects on an SoC. These settings can be throughput,
latency and priority between multiple interconnected devices or functional
blocks. This can be controlled dynamically in order to save power or provide
maximum performance.

The interconnect bus is hardware with configurable parameters, which can be
set on a data path according to the requests received from various drivers.
An example of interconnect buses are the interconnects between various
components or functional blocks in chipsets. There can be multiple interconnects
on an SoC that can be multi-tiered.

Below is a simplified diagram of a real-world SoC interconnect bus topology.

```
+----------------+    +----------------+
| HW Accelerator |--->|      M NoC     |<---------------+
+----------------+    +----------------+                |
                        |      |                    +------------+
 +-----+  +-------------+      V       +------+     |            |
 | DDR |  |                +--------+  | PCIe |     |            |
 +-----+  |                | Slaves |  +------+     |            |
   ^ ^    |                +--------+     |         |   C NoC    |
   | |    V                               V         |            |
+------------------+   +------------------------+   |            |   +-----+
|                  |-->|                        |-->|            |-->| CPU |
|                  |-->|                        |<--|            |   +-----+
|     Mem NoC      |   |         S NoC          |   +------------+
|                  |<--|                        |---------+    |
|                  |<--|                        |<------+ |    |   +--------+
+------------------+   +------------------------+       | |    +-->| Slaves |
  ^  ^    ^    ^          ^                             | |        +--------+
  |  |    |    |          |                             | V
+------+  |  +-----+   +-----+  +---------+   +----------------+   +--------+
| CPUs |  |  | GPU |   | DSP |  | Masters |-->|       P NoC    |-->| Slaves |
+------+  |  +-----+   +-----+  +---------+   +----------------+   +--------+
          |
      +-------+
      | Modem |
      +-------+
```

## Terminology

Interconnect provider is the software definition of the interconnect hardware.
The interconnect providers on the above diagram are M NoC, S NoC, C NoC, P NoC
and Mem NoC.

Interconnect node is the software definition of the interconnect hardware
port. Each interconnect provider consists of multiple interconnect nodes,
which are connected to other SoC components including other interconnect
providers. The point on the diagram where the CPUs connect to the memory is
called an interconnect node, which belongs to the Mem NoC interconnect provider.

Interconnect endpoints are the first or the last element of the path. Every
endpoint is a node, but not every node is an endpoint.

Interconnect path is everything between two endpoints including all the nodes
that have to be traversed to reach from a source to destination node. It may
include multiple master-slave pairs across several interconnect providers.

Interconnect consumers are the entities which make use of the data paths exposed
by the providers. The consumers send requests to providers requesting various
throughput, latency and priority. Usually the consumers are device drivers, that
send request based on their needs. An example for a consumer is a video decoder
that supports various formats and image sizes.

## Interconnect providers

Interconnect provider is an entity that implements methods to initialize and
configure interconnect bus hardware. The interconnect provider drivers should
be registered with the interconnect provider core.

struct icc\_node\_data
:   icc node data

**Definition**:

```
struct icc_node_data {
    struct icc_node *node;
    u32 tag;
};
```

**Members**

`node`
:   icc node

`tag`
:   tag

struct icc\_onecell\_data
:   driver data for onecell interconnect providers

**Definition**:

```
struct icc_onecell_data {
    unsigned int num_nodes;
    struct icc_node *nodes[];
};
```

**Members**

`num_nodes`
:   number of nodes in this device

`nodes`
:   array of pointers to the nodes in this device

struct icc\_provider
:   interconnect provider (controller) entity that might provide multiple interconnect controls

**Definition**:

```
struct icc_provider {
    struct list_head        provider_list;
    struct list_head        nodes;
    int (*set)(struct icc_node *src, struct icc_node *dst);
    int (*aggregate)(struct icc_node *node, u32 tag, u32 avg_bw, u32 peak_bw, u32 *agg_avg, u32 *agg_peak);
    void (*pre_aggregate)(struct icc_node *node);
    int (*get_bw)(struct icc_node *node, u32 *avg, u32 *peak);
    struct icc_node* (*xlate)(const struct of_phandle_args *spec, void *data);
    struct icc_node_data* (*xlate_extended)(const struct of_phandle_args *spec, void *data);
    struct device           *dev;
    int users;
    bool inter_set;
    void *data;
};
```

**Members**

`provider_list`
:   list of the registered interconnect providers

`nodes`
:   internal list of the interconnect provider nodes

`set`
:   pointer to device specific set operation function

`aggregate`
:   pointer to device specific aggregate operation function

`pre_aggregate`
:   pointer to device specific function that is called
    before the aggregation begins (optional)

`get_bw`
:   pointer to device specific function to get current bandwidth

`xlate`
:   provider-specific callback for mapping nodes from phandle arguments

`xlate_extended`
:   vendor-specific callback for mapping node data from phandle arguments

`dev`
:   the device this interconnect provider belongs to

`users`
:   count of active users

`inter_set`
:   whether inter-provider pairs will be configured with **set**

`data`
:   pointer to private data

struct icc\_node
:   entity that is part of the interconnect topology

**Definition**:

```
struct icc_node {
    int id;
    const char              *name;
    struct icc_node         **links;
    size_t num_links;
    struct icc_provider     *provider;
    struct list_head        node_list;
    struct list_head        search_list;
    struct icc_node         *reverse;
    u8 is_traversed:1;
    struct hlist_head       req_list;
    u32 avg_bw;
    u32 peak_bw;
    u32 init_avg;
    u32 init_peak;
    void *data;
};
```

**Members**

`id`
:   platform specific node id

`name`
:   node name used in debugfs

`links`
:   a list of targets pointing to where we can go next when traversing

`num_links`
:   number of links to other interconnect nodes

`provider`
:   points to the interconnect provider of this node

`node_list`
:   the list entry in the parent provider’s “nodes” list

`search_list`
:   list used when walking the nodes graph

`reverse`
:   pointer to previous node when walking the nodes graph

`is_traversed`
:   flag that is used when walking the nodes graph

`req_list`
:   a list of QoS constraint requests associated with this node

`avg_bw`
:   aggregated value of average bandwidth requests from all consumers

`peak_bw`
:   aggregated value of peak bandwidth requests from all consumers

`init_avg`
:   average bandwidth value that is read from the hardware during init

`init_peak`
:   peak bandwidth value that is read from the hardware during init

`data`
:   pointer to private data

struct [icc\_node](#c.icc_node "icc_node") \*icc\_node\_create\_dyn(void)
:   create a node with dynamic id

**Parameters**

`void`
:   no arguments

**Return**

icc\_node pointer on success, or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on error

struct [icc\_node](#c.icc_node "icc_node") \*icc\_node\_create(int id)
:   create a node

**Parameters**

`int id`
:   node id

**Return**

icc\_node pointer on success, or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on error

void icc\_node\_destroy(int id)
:   destroy a node

**Parameters**

`int id`
:   node id

int icc\_node\_set\_name(struct [icc\_node](#c.icc_node "icc_node") \*node, const struct [icc\_provider](#c.icc_provider "icc_provider") \*provider, const char \*name)
:   set node name

**Parameters**

`struct icc_node *node`
:   node

`const struct icc_provider *provider`
:   node provider

`const char *name`
:   node name

**Return**

0 on success, or -ENOMEM on allocation failure

int icc\_link\_nodes(struct [icc\_node](#c.icc_node "icc_node") \*src\_node, struct [icc\_node](#c.icc_node "icc_node") \*\*dst\_node)
:   create link between two nodes

**Parameters**

`struct icc_node *src_node`
:   source node

`struct icc_node **dst_node`
:   destination node

**Description**

Create a link between two nodes. The nodes might belong to different
interconnect providers and the **dst\_node** might not exist (if the
provider driver has not probed yet). So just create the **dst\_node**
and when the actual provider driver is probed, the rest of the node
data is filled.

**Return**

0 on success, or an error code otherwise

int icc\_link\_create(struct [icc\_node](#c.icc_node "icc_node") \*node, const int dst\_id)
:   create a link between two nodes

**Parameters**

`struct icc_node *node`
:   source node id

`const int dst_id`
:   destination node id

**Description**

Create a link between two nodes. The nodes might belong to different
interconnect providers and the **dst\_id** node might not exist (if the
provider driver has not probed yet). So just create the **dst\_id** node
and when the actual provider driver is probed, the rest of the node
data is filled.

**Return**

0 on success, or an error code otherwise

void icc\_node\_add(struct [icc\_node](#c.icc_node "icc_node") \*node, struct [icc\_provider](#c.icc_provider "icc_provider") \*provider)
:   add interconnect node to interconnect provider

**Parameters**

`struct icc_node *node`
:   pointer to the interconnect node

`struct icc_provider *provider`
:   pointer to the interconnect provider

void icc\_node\_del(struct [icc\_node](#c.icc_node "icc_node") \*node)
:   delete interconnect node from interconnect provider

**Parameters**

`struct icc_node *node`
:   pointer to the interconnect node

int icc\_nodes\_remove(struct [icc\_provider](#c.icc_provider "icc_provider") \*provider)
:   remove all previously added nodes from provider

**Parameters**

`struct icc_provider *provider`
:   the interconnect provider we are removing nodes from

**Return**

0 on success, or an error code otherwise

void icc\_provider\_init(struct [icc\_provider](#c.icc_provider "icc_provider") \*provider)
:   initialize a new interconnect provider

**Parameters**

`struct icc_provider *provider`
:   the interconnect provider to initialize

**Description**

Must be called before adding nodes to the provider.

int icc\_provider\_register(struct [icc\_provider](#c.icc_provider "icc_provider") \*provider)
:   register a new interconnect provider

**Parameters**

`struct icc_provider *provider`
:   the interconnect provider to register

**Return**

0 on success, or an error code otherwise

void icc\_provider\_deregister(struct [icc\_provider](#c.icc_provider "icc_provider") \*provider)
:   deregister an interconnect provider

**Parameters**

`struct icc_provider *provider`
:   the interconnect provider to deregister

## Interconnect consumers

Interconnect consumers are the clients which use the interconnect APIs to
get paths between endpoints and set their bandwidth/latency/QoS requirements
for these interconnect paths.

struct icc\_path \*of\_icc\_get\_by\_index(struct [device](infrastructure.html#c.device "device") \*dev, int idx)
:   get a path handle from a DT node based on index

**Parameters**

`struct device *dev`
:   device pointer for the consumer device

`int idx`
:   interconnect path index

**Description**

This function will search for a path between two endpoints and return an
icc\_path handle on success. Use [`icc_put()`](#c.icc_put "icc_put") to release constraints when they
are not needed anymore.
If the interconnect API is disabled, NULL is returned and the consumer
drivers will still build. Drivers are free to handle this specifically,
but they don’t have to.

**Return**

icc\_path pointer on success or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on error. NULL is returned
when the API is disabled or the “interconnects” DT property is missing.

struct icc\_path \*of\_icc\_get(struct [device](infrastructure.html#c.device "device") \*dev, const char \*name)
:   get a path handle from a DT node based on name

**Parameters**

`struct device *dev`
:   device pointer for the consumer device

`const char *name`
:   interconnect path name

**Description**

This function will search for a path between two endpoints and return an
icc\_path handle on success. Use [`icc_put()`](#c.icc_put "icc_put") to release constraints when they
are not needed anymore.
If the interconnect API is disabled, NULL is returned and the consumer
drivers will still build. Drivers are free to handle this specifically,
but they don’t have to.

**Return**

icc\_path pointer on success or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on error. NULL is returned
when the API is disabled or the “interconnects” DT property is missing.

struct icc\_path \*icc\_get(struct [device](infrastructure.html#c.device "device") \*dev, const char \*src, const char \*dst)
:   get a path handle between two endpoints

**Parameters**

`struct device *dev`
:   device pointer for the consumer device

`const char *src`
:   source node name

`const char *dst`
:   destination node name

**Description**

This function will search for a path between two endpoints and return an
icc\_path handle on success. Use [`icc_put()`](#c.icc_put "icc_put") to release constraints when they
are not needed anymore.

**Return**

icc\_path pointer on success or [`ERR_PTR()`](../core-api/kernel-api.html#c.ERR_PTR "ERR_PTR") on error. NULL is returned
when the API is disabled.

void icc\_set\_tag(struct icc\_path \*path, u32 tag)
:   set an optional tag on a path

**Parameters**

`struct icc_path *path`
:   the path we want to tag

`u32 tag`
:   the tag value

**Description**

This function allows consumers to append a tag to the requests associated
with a path, so that a different aggregation could be done based on this tag.

const char \*icc\_get\_name(struct icc\_path \*path)
:   Get name of the icc path

**Parameters**

`struct icc_path *path`
:   interconnect path

**Description**

This function is used by an interconnect consumer to get the name of the icc
path.

Returns a valid pointer on success, or NULL otherwise.

int icc\_set\_bw(struct icc\_path \*path, u32 avg\_bw, u32 peak\_bw)
:   set bandwidth constraints on an interconnect path

**Parameters**

`struct icc_path *path`
:   interconnect path

`u32 avg_bw`
:   average bandwidth in kilobytes per second

`u32 peak_bw`
:   peak bandwidth in kilobytes per second

**Description**

This function is used by an interconnect consumer to express its own needs
in terms of bandwidth for a previously requested path between two endpoints.
The requests are aggregated and each node is updated accordingly. The entire
path is locked by a mutex to ensure that the `set()` is completed.
The **path** can be NULL when the “interconnects” DT properties is missing,
which will mean that no constraints will be set.

Returns 0 on success, or an appropriate error code otherwise.

void icc\_put(struct icc\_path \*path)
:   release the reference to the icc\_path

**Parameters**

`struct icc_path *path`
:   interconnect path

**Description**

Use this function to release the constraints on a path when the path is
no longer needed. The constraints will be re-aggregated.

int of\_icc\_bulk\_get(struct [device](infrastructure.html#c.device "device") \*dev, int num\_paths, struct icc\_bulk\_data \*paths)
:   get interconnect paths

**Parameters**

`struct device *dev`
:   the device requesting the path

`int num_paths`
:   the number of icc\_bulk\_data

`struct icc_bulk_data *paths`
:   the table with the paths we want to get

**Description**

Returns 0 on success or negative errno otherwise.

void icc\_bulk\_put(int num\_paths, struct icc\_bulk\_data \*paths)
:   put a list of interconnect paths

**Parameters**

`int num_paths`
:   the number of icc\_bulk\_data

`struct icc_bulk_data *paths`
:   the icc\_bulk\_data table with the paths being put

int icc\_bulk\_set\_bw(int num\_paths, const struct icc\_bulk\_data \*paths)
:   set bandwidth to a set of paths

**Parameters**

`int num_paths`
:   the number of icc\_bulk\_data

`const struct icc_bulk_data *paths`
:   the icc\_bulk\_data table containing the paths and bandwidth

**Description**

Returns 0 on success or negative errno otherwise.

int icc\_bulk\_enable(int num\_paths, const struct icc\_bulk\_data \*paths)
:   enable a previously disabled set of paths

**Parameters**

`int num_paths`
:   the number of icc\_bulk\_data

`const struct icc_bulk_data *paths`
:   the icc\_bulk\_data table containing the paths and bandwidth

**Description**

Returns 0 on success or negative errno otherwise.

void icc\_bulk\_disable(int num\_paths, const struct icc\_bulk\_data \*paths)
:   disable a set of interconnect paths

**Parameters**

`int num_paths`
:   the number of icc\_bulk\_data

`const struct icc_bulk_data *paths`
:   the icc\_bulk\_data table containing the paths and bandwidth

int devm\_of\_icc\_bulk\_get(struct [device](infrastructure.html#c.device "device") \*dev, int num\_paths, struct icc\_bulk\_data \*paths)
:   resource managed of\_icc\_bulk\_get

**Parameters**

`struct device *dev`
:   the device requesting the path

`int num_paths`
:   the number of icc\_bulk\_data

`struct icc_bulk_data *paths`
:   the table with the paths we want to get

**Description**

Returns 0 on success or negative errno otherwise.

## Interconnect debugfs interfaces

Like several other subsystems interconnect will create some files for debugging
and introspection. Files in debugfs are not considered ABI so application
software shouldn’t rely on format details change between kernel versions.

`/sys/kernel/debug/interconnect/interconnect_summary`:

Show all interconnect nodes in the system with their aggregated bandwidth
request. Indented under each node show bandwidth requests from each device.

`/sys/kernel/debug/interconnect/interconnect_graph`:

Show the interconnect graph in the graphviz dot format. It shows all
interconnect nodes and links in the system and groups together nodes from the
same provider as subgraphs. The format is human-readable and can also be piped
through dot to generate diagrams in many graphical formats:

```
$ cat /sys/kernel/debug/interconnect/interconnect_graph | \
        dot -Tsvg > interconnect_graph.svg
```

The `test-client` directory provides interfaces for issuing BW requests to
any arbitrary path. Note that for safety reasons, this feature is disabled by
default without a Kconfig to enable it. Enabling it requires code changes to
`#define INTERCONNECT_ALLOW_WRITE_DEBUGFS`. Example usage:

```
cd /sys/kernel/debug/interconnect/test-client/

# Configure node endpoints for the path from CPU to DDR on
# qcom/sm8550.
echo chm_apps > src_node
echo ebi > dst_node

# Get path between src_node and dst_node. This is only
# necessary after updating the node endpoints.
echo 1 > get

# Set desired BW to 1GBps avg and 2GBps peak.
echo 1000000 > avg_bw
echo 2000000 > peak_bw

# Vote for avg_bw and peak_bw on the latest path from "get".
# Voting for multiple paths is possible by repeating this
# process for different nodes endpoints.
echo 1 > commit
```
