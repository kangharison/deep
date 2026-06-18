# DeviceTree Kernel API

> 출처(원문): https://docs.kernel.org/devicetree/kernel-api.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# DeviceTree Kernel API

## Core functions

struct device\_node \*of\_find\_all\_nodes(struct device\_node \*prev)
:   Get next node in global list

**Parameters**

`struct device_node *prev`
:   Previous node or NULL to start iteration
    [`of_node_put()`](#c.of_node_put "of_node_put") will be called on it

**Return**

A node pointer with refcount incremented, use
[`of_node_put()`](#c.of_node_put "of_node_put") on it when done.

bool of\_machine\_compatible\_match(const char \*const \*compats)
:   Test root of device tree against a compatible array

**Parameters**

`const char *const *compats`
:   NULL terminated array of compatible strings to look for in root node’s compatible property.

**Description**

Returns true if the root node has any of the given compatible values in its
compatible property.

int of\_machine\_read\_compatible(const char \*\*compatible, unsigned int index)
:   Get the compatible string of this machine

**Parameters**

`const char **compatible`
:   address at which the address of the compatible string will be
    stored

`unsigned int index`
:   index of the compatible entry in the list

**Return**

0 on success, negative error number on failure.

int of\_machine\_read\_model(const char \*\*model)
:   Get the model string of this machine

**Parameters**

`const char **model`
:   address at which the address of the model string will be stored

**Return**

0 on success, negative error number on failure.

const struct of\_device\_id \*of\_machine\_get\_match(const struct of\_device\_id \*matches)
:   Test root of device tree against an of\_device\_id array

**Parameters**

`const struct of_device_id *matches`
:   NULL terminated array of of\_device\_id match structures to search in

**Description**

Returns matched entry or NULL

const void \*of\_machine\_get\_match\_data(const struct of\_device\_id \*matches)
:   Tell if root of device tree has a matching of\_match structure

**Parameters**

`const struct of_device_id *matches`
:   NULL terminated array of of\_device\_id match structures to search in

**Description**

Returns data associated with matched entry or NULL

bool of\_device\_is\_available(const struct device\_node \*device)
:   check if a device is available for use

**Parameters**

`const struct device_node *device`
:   Node to check for availability

**Return**

True if the status property is absent or set to “okay” or “ok”,
false otherwise

bool of\_device\_is\_big\_endian(const struct device\_node \*device)
:   check if a device has BE registers

**Parameters**

`const struct device_node *device`
:   Node to check for endianness

**Return**

True if the device has a “big-endian” property, or if the kernel
was compiled for BE *and* the device has a “native-endian” property.
Returns false otherwise.

**Description**

> Callers would nominally use ioread32be/iowrite32be if
> [`of_device_is_big_endian()`](#c.of_device_is_big_endian "of_device_is_big_endian") == true, or readl/writel otherwise.

struct device\_node \*of\_get\_parent(const struct device\_node \*node)
:   Get a node’s parent if any

**Parameters**

`const struct device_node *node`
:   Node to get parent

**Return**

A node pointer with refcount incremented, use
[`of_node_put()`](#c.of_node_put "of_node_put") on it when done.

struct device\_node \*of\_get\_next\_parent(struct device\_node \*node)
:   Iterate to a node’s parent

**Parameters**

`struct device_node *node`
:   Node to get parent of

**Description**

This is like [`of_get_parent()`](#c.of_get_parent "of_get_parent") except that it drops the
refcount on the passed node, making it suitable for iterating
through a node’s parents.

**Return**

A node pointer with refcount incremented, use
[`of_node_put()`](#c.of_node_put "of_node_put") on it when done.

struct device\_node \*of\_get\_next\_child(const struct device\_node \*node, struct device\_node \*prev)
:   Iterate a node childs

**Parameters**

`const struct device_node *node`
:   parent node

`struct device_node *prev`
:   previous child of the parent node, or NULL to get first

**Return**

A node pointer with refcount incremented, use [`of_node_put()`](#c.of_node_put "of_node_put") on
it when done. Returns NULL when prev is the last child. Decrements the
refcount of prev.

struct device\_node \*of\_get\_next\_child\_with\_prefix(const struct device\_node \*node, struct device\_node \*prev, const char \*prefix)
:   Find the next child node with prefix

**Parameters**

`const struct device_node *node`
:   parent node

`struct device_node *prev`
:   previous child of the parent node, or NULL to get first

`const char *prefix`
:   prefix that the node name should have

**Description**

This function is like [`of_get_next_child()`](#c.of_get_next_child "of_get_next_child"), except that it automatically
skips any nodes whose name doesn’t have the given prefix.

**Return**

A node pointer with refcount incremented, use
[`of_node_put()`](#c.of_node_put "of_node_put") on it when done.

struct device\_node \*of\_get\_next\_available\_child(const struct device\_node \*node, struct device\_node \*prev)
:   Find the next available child node

**Parameters**

`const struct device_node *node`
:   parent node

`struct device_node *prev`
:   previous child of the parent node, or NULL to get first

**Description**

This function is like [`of_get_next_child()`](#c.of_get_next_child "of_get_next_child"), except that it
automatically skips any disabled nodes (i.e. status = “disabled”).

struct device\_node \*of\_get\_next\_reserved\_child(const struct device\_node \*node, struct device\_node \*prev)
:   Find the next reserved child node

**Parameters**

`const struct device_node *node`
:   parent node

`struct device_node *prev`
:   previous child of the parent node, or NULL to get first

**Description**

This function is like [`of_get_next_child()`](#c.of_get_next_child "of_get_next_child"), except that it
automatically skips any disabled nodes (i.e. status = “disabled”).

struct device\_node \*of\_get\_next\_cpu\_node(struct device\_node \*prev)
:   Iterate on cpu nodes

**Parameters**

`struct device_node *prev`
:   previous child of the /cpus node, or NULL to get first

**Description**

Unusable CPUs (those with the status property set to “fail” or “fail-...”)
will be skipped.

**Return**

A cpu node pointer with refcount incremented, use [`of_node_put()`](#c.of_node_put "of_node_put")
on it when done. Returns NULL when prev is the last child. Decrements
the refcount of prev.

struct device\_node \*of\_get\_compatible\_child(const struct device\_node \*parent, const char \*compatible)
:   Find compatible child node

**Parameters**

`const struct device_node *parent`
:   parent node

`const char *compatible`
:   compatible string

**Description**

Lookup child node whose compatible property contains the given compatible
string.

**Return**

a node pointer with refcount incremented, use [`of_node_put()`](#c.of_node_put "of_node_put") on it
when done; or NULL if not found.

struct device\_node \*of\_get\_child\_by\_name(const struct device\_node \*node, const char \*name)
:   Find the child node by name for a given parent

**Parameters**

`const struct device_node *node`
:   parent node

`const char *name`
:   child name to look for.

**Description**

This function looks for child node for given matching name

**Return**

A node pointer if found, with refcount incremented, use
[`of_node_put()`](#c.of_node_put "of_node_put") on it when done.
Returns NULL if node is not found.

struct device\_node \*of\_get\_available\_child\_by\_name(const struct device\_node \*node, const char \*name)
:   Find the available child node by name for a given parent

**Parameters**

`const struct device_node *node`
:   parent node

`const char *name`
:   child name to look for.

**Description**

This function looks for child node for given matching name and checks the
device’s availability for use.

**Return**

A node pointer if found, with refcount incremented, use
[`of_node_put()`](#c.of_node_put "of_node_put") on it when done.
Returns NULL if node is not found.

struct device\_node \*of\_find\_node\_opts\_by\_path(const char \*path, const char \*\*opts)
:   Find a node matching a full OF path

**Parameters**

`const char *path`
:   Either the full path to match, or if the path does not
    start with ‘/’, the name of a property of the /aliases
    node (an alias). In the case of an alias, the node
    matching the alias’ value will be returned.

`const char **opts`
:   Address of a pointer into which to store the start of
    an options string appended to the end of the path with
    a ‘:’ separator.

**Description**

Valid paths:
:   * /foo/bar Full path
    * foo Valid alias
    * foo/bar Valid alias + relative path

**Return**

A node pointer with refcount incremented, use
[`of_node_put()`](#c.of_node_put "of_node_put") on it when done.

struct device\_node \*of\_find\_node\_by\_name(struct device\_node \*from, const char \*name)
:   Find a node by its “name” property

**Parameters**

`struct device_node *from`
:   The node to start searching from or NULL; the node
    you pass will not be searched, only the next one
    will. Typically, you pass what the previous call
    returned. [`of_node_put()`](#c.of_node_put "of_node_put") will be called on **from**.

`const char *name`
:   The name string to match against

**Return**

A node pointer with refcount incremented, use
[`of_node_put()`](#c.of_node_put "of_node_put") on it when done.

struct device\_node \*of\_find\_node\_by\_type(struct device\_node \*from, const char \*type)
:   Find a node by its “device\_type” property

**Parameters**

`struct device_node *from`
:   The node to start searching from, or NULL to start searching
    the entire device tree. The node you pass will not be
    searched, only the next one will; typically, you pass
    what the previous call returned. [`of_node_put()`](#c.of_node_put "of_node_put") will be
    called on from for you.

`const char *type`
:   The type string to match against

**Return**

A node pointer with refcount incremented, use
[`of_node_put()`](#c.of_node_put "of_node_put") on it when done.

struct device\_node \*of\_find\_compatible\_node(struct device\_node \*from, const char \*type, const char \*compatible)
:   Find a node based on type and one of the tokens in its “compatible” property

**Parameters**

`struct device_node *from`
:   The node to start searching from or NULL, the node
    you pass will not be searched, only the next one
    will; typically, you pass what the previous call
    returned. [`of_node_put()`](#c.of_node_put "of_node_put") will be called on it

`const char *type`
:   The type string to match “device\_type” or NULL to ignore

`const char *compatible`
:   The string to match to one of the tokens in the device
    “compatible” list.

**Return**

A node pointer with refcount incremented, use
[`of_node_put()`](#c.of_node_put "of_node_put") on it when done.

struct device\_node \*of\_find\_node\_with\_property(struct device\_node \*from, const char \*prop\_name)
:   Find a node which has a property with the given name.

**Parameters**

`struct device_node *from`
:   The node to start searching from or NULL, the node
    you pass will not be searched, only the next one
    will; typically, you pass what the previous call
    returned. [`of_node_put()`](#c.of_node_put "of_node_put") will be called on it

`const char *prop_name`
:   The name of the property to look for.

**Return**

A node pointer with refcount incremented, use
[`of_node_put()`](#c.of_node_put "of_node_put") on it when done.

const struct of\_device\_id \*of\_match\_node(const struct of\_device\_id \*matches, const struct device\_node \*node)
:   Tell if a device\_node has a matching of\_match structure

**Parameters**

`const struct of_device_id *matches`
:   array of of device match structures to search in

`const struct device_node *node`
:   the of device structure to match against

**Description**

Low level utility function used by device matching.

struct device\_node \*of\_find\_matching\_node\_and\_match(struct device\_node \*from, const struct of\_device\_id \*matches, const struct of\_device\_id \*\*match)
:   Find a node based on an of\_device\_id match table.

**Parameters**

`struct device_node *from`
:   The node to start searching from or NULL, the node
    you pass will not be searched, only the next one
    will; typically, you pass what the previous call
    returned. [`of_node_put()`](#c.of_node_put "of_node_put") will be called on it

`const struct of_device_id *matches`
:   array of of device match structures to search in

`const struct of_device_id **match`
:   Updated to point at the matches entry which matched

**Return**

A node pointer with refcount incremented, use
[`of_node_put()`](#c.of_node_put "of_node_put") on it when done.

int of\_alias\_from\_compatible(const struct device\_node \*node, char \*alias, int len)
:   Lookup appropriate alias for a device node depending on compatible

**Parameters**

`const struct device_node *node`
:   pointer to a device tree node

`char *alias`
:   Pointer to buffer that alias value will be copied into

`int len`
:   Length of alias value

**Description**

Based on the value of the compatible property, this routine will attempt
to choose an appropriate alias value for a particular device tree node.
It does this by stripping the manufacturer prefix (as delimited by a ‘,’)
from the first entry in the compatible list property.

**Note**

The matching on just the “product” side of the compatible is a relic
from I2C and SPI. Please do not add any new user.

**Return**

This routine returns 0 on success, <0 on failure.

struct device\_node \*of\_find\_node\_by\_phandle(phandle handle)
:   Find a node given a phandle

**Parameters**

`phandle handle`
:   phandle of the node to find

**Return**

A node pointer with refcount incremented, use
[`of_node_put()`](#c.of_node_put "of_node_put") on it when done.

int of\_parse\_phandle\_with\_args\_map(const struct device\_node \*np, const char \*list\_name, const char \*stem\_name, int index, struct of\_phandle\_args \*out\_args)
:   Find a node pointed by phandle in a list and remap it

**Parameters**

`const struct device_node *np`
:   pointer to a device tree node containing a list

`const char *list_name`
:   property name that contains a list

`const char *stem_name`
:   stem of property names that specify phandles’ arguments count

`int index`
:   index of a phandle to parse out

`struct of_phandle_args *out_args`
:   optional pointer to output arguments structure (will be filled)

**Description**

This function is useful to parse lists of phandles and their arguments.
Returns 0 on success and fills out\_args, on error returns appropriate errno
value. The difference between this function and [`of_parse_phandle_with_args()`](#c.of_parse_phandle_with_args "of_parse_phandle_with_args")
is that this API remaps a phandle if the node the phandle points to has
a <**stem\_name**>-map property.

Caller is responsible to call [`of_node_put()`](#c.of_node_put "of_node_put") on the returned out\_args->np
pointer.

Example:

```
phandle1: node1 {
    #list-cells = <2>;
};

phandle2: node2 {
    #list-cells = <1>;
};

phandle3: node3 {
    #list-cells = <1>;
    list-map = <0 &phandle2 3>,
               <1 &phandle2 2>,
               <2 &phandle1 5 1>;
    list-map-mask = <0x3>;
};

node4 {
    list = <&phandle1 1 2 &phandle3 0>;
};
```

To get a device\_node of the `node2` node you may call this:
of\_parse\_phandle\_with\_args(node4, “list”, “list”, 1, `args`);

int of\_count\_phandle\_with\_args(const struct device\_node \*np, const char \*list\_name, const char \*cells\_name)
:   Find the number of phandles references in a property

**Parameters**

`const struct device_node *np`
:   pointer to a device tree node containing a list

`const char *list_name`
:   property name that contains a list

`const char *cells_name`
:   property name that specifies phandles’ arguments count

**Return**

The number of phandle + argument tuples within a property. It
is a typical pattern to encode a list of phandle and variable
arguments into a single property. The number of arguments is encoded
by a property in the phandle-target node. For example, a gpios
property would contain a list of GPIO specifies consisting of a
phandle and 1 or more arguments. The number of arguments are
determined by the #gpio-cells property in the node pointed to by the
phandle.

int of\_add\_property(struct device\_node \*np, struct property \*prop)
:   Add a property to a node

**Parameters**

`struct device_node *np`
:   Caller’s Device Node

`struct property *prop`
:   Property to add

int of\_remove\_property(struct device\_node \*np, struct property \*prop)
:   Remove a property from a node.

**Parameters**

`struct device_node *np`
:   Caller’s Device Node

`struct property *prop`
:   Property to remove

**Description**

Note that we don’t actually remove it, since we have given out
who-knows-how-many pointers to the data using get-property.
Instead we just move the property to the “dead properties”
list, so it won’t be found any more.

int of\_alias\_get\_id(const struct device\_node \*np, const char \*stem)
:   Get alias id for the given device\_node

**Parameters**

`const struct device_node *np`
:   Pointer to the given device\_node

`const char *stem`
:   Alias stem of the given device\_node

**Description**

The function travels the lookup table to get the alias id for the given
device\_node and alias stem.

**Return**

The alias id if found.

int of\_alias\_get\_highest\_id(const char \*stem)
:   Get highest alias id for the given stem

**Parameters**

`const char *stem`
:   Alias stem to be examined

**Description**

The function travels the lookup table to get the highest alias id for the
given alias stem. It returns the alias id if found.

bool of\_console\_check(const struct device\_node \*dn, char \*name, int index)
:   Test and setup console for DT setup

**Parameters**

`const struct device_node *dn`
:   Pointer to device node

`char *name`
:   Name to use for preferred console without index. ex. “ttyS”

`int index`
:   Index to use for preferred console.

**Description**

Check if the given device node matches the stdout-path property in the
/chosen node. If it does then register it as the preferred console.

**Return**

TRUE if console successfully setup. Otherwise return FALSE.

int of\_map\_id(const struct device\_node \*np, u32 id, const char \*map\_name, const char \*map\_mask\_name, struct device\_node \*\*target, u32 \*id\_out)
:   Translate an ID through a downstream mapping.

**Parameters**

`const struct device_node *np`
:   root complex device node.

`u32 id`
:   device ID to map.

`const char *map_name`
:   property name of the map to use.

`const char *map_mask_name`
:   optional property name of the mask to use.

`struct device_node **target`
:   optional pointer to a target device node.

`u32 *id_out`
:   optional pointer to receive the translated ID.

**Description**

Given a device ID, look up the appropriate implementation-defined
platform ID and/or the target device which receives transactions on that
ID, as per the “iommu-map” and “msi-map” bindings. Either of **target** or
**id\_out** may be NULL if only the other is required. If **target** points to
a non-NULL device node pointer, only entries targeting that node will be
matched; if it points to a NULL value, it will receive the device node of
the first matching target phandle, with a reference held.

**Return**

0 on success or a standard error code on failure.

void of\_node\_init(struct device\_node \*node)
:   initialize a devicetree node

**Parameters**

`struct device_node *node`
:   Pointer to device node that has been created by [`kzalloc()`](../core-api/mm-api.html#c.kzalloc "kzalloc")

**Description**

On return the device\_node refcount is set to one. Use [`of_node_put()`](#c.of_node_put "of_node_put")
on **node** when done to free the memory allocated for it. If the node
is NOT a dynamic node the memory will not be freed. The decision of
whether to free the memory will be done by node->`release()`, which is
`of_node_release()`.

bool of\_machine\_is\_compatible(const char \*compat)
:   Test root of device tree for a given compatible value

**Parameters**

`const char *compat`
:   compatible string to look for in root node’s compatible property.

**Return**

true if the root node has the given value in its compatible property.

struct device\_node \*of\_parse\_phandle(const struct device\_node \*np, const char \*phandle\_name, int index)
:   Resolve a phandle property to a device\_node pointer

**Parameters**

`const struct device_node *np`
:   Pointer to device node holding phandle property

`const char *phandle_name`
:   Name of property holding a phandle value

`int index`
:   For properties holding a table of phandles, this is the index into
    the table

**Return**

The device\_node pointer with refcount incremented. Use
[`of_node_put()`](#c.of_node_put "of_node_put") on it when done.

int of\_parse\_phandle\_with\_args(const struct device\_node \*np, const char \*list\_name, const char \*cells\_name, int index, struct of\_phandle\_args \*out\_args)
:   Find a node pointed by phandle in a list

**Parameters**

`const struct device_node *np`
:   pointer to a device tree node containing a list

`const char *list_name`
:   property name that contains a list

`const char *cells_name`
:   property name that specifies phandles’ arguments count

`int index`
:   index of a phandle to parse out

`struct of_phandle_args *out_args`
:   optional pointer to output arguments structure (will be filled)

**Description**

This function is useful to parse lists of phandles and their arguments.
Returns 0 on success and fills out\_args, on error returns appropriate
errno value.

Caller is responsible to call [`of_node_put()`](#c.of_node_put "of_node_put") on the returned out\_args->np
pointer.

Example:

```
phandle1: node1 {
    #list-cells = <2>;
};

phandle2: node2 {
    #list-cells = <1>;
};

node3 {
    list = <&phandle1 1 2 &phandle2 3>;
};
```

To get a device\_node of the `node2` node you may call this:
of\_parse\_phandle\_with\_args(node3, “list”, “#list-cells”, 1, `args`);

int of\_parse\_phandle\_with\_fixed\_args(const struct device\_node \*np, const char \*list\_name, int cell\_count, int index, struct of\_phandle\_args \*out\_args)
:   Find a node pointed by phandle in a list

**Parameters**

`const struct device_node *np`
:   pointer to a device tree node containing a list

`const char *list_name`
:   property name that contains a list

`int cell_count`
:   number of argument cells following the phandle

`int index`
:   index of a phandle to parse out

`struct of_phandle_args *out_args`
:   optional pointer to output arguments structure (will be filled)

**Description**

This function is useful to parse lists of phandles and their arguments.
Returns 0 on success and fills out\_args, on error returns appropriate
errno value.

Caller is responsible to call [`of_node_put()`](#c.of_node_put "of_node_put") on the returned out\_args->np
pointer.

Example:

```
phandle1: node1 {
};

phandle2: node2 {
};

node3 {
    list = <&phandle1 0 2 &phandle2 2 3>;
};
```

To get a device\_node of the `node2` node you may call this:
of\_parse\_phandle\_with\_fixed\_args(node3, “list”, 2, 1, `args`);

int of\_parse\_phandle\_with\_optional\_args(const struct device\_node \*np, const char \*list\_name, const char \*cells\_name, int index, struct of\_phandle\_args \*out\_args)
:   Find a node pointed by phandle in a list

**Parameters**

`const struct device_node *np`
:   pointer to a device tree node containing a list

`const char *list_name`
:   property name that contains a list

`const char *cells_name`
:   property name that specifies phandles’ arguments count

`int index`
:   index of a phandle to parse out

`struct of_phandle_args *out_args`
:   optional pointer to output arguments structure (will be filled)

**Description**

Same as [`of_parse_phandle_with_args()`](#c.of_parse_phandle_with_args "of_parse_phandle_with_args") except that if the cells\_name property
is not found, cell\_count of 0 is assumed.

This is used to useful, if you have a phandle which didn’t have arguments
before and thus doesn’t have a ‘#\*-cells’ property but is now migrated to
having arguments while retaining backwards compatibility.

bool of\_phandle\_args\_equal(const struct of\_phandle\_args \*a1, const struct of\_phandle\_args \*a2)
:   Compare two of\_phandle\_args

**Parameters**

`const struct of_phandle_args *a1`
:   First of\_phandle\_args to compare

`const struct of_phandle_args *a2`
:   Second of\_phandle\_args to compare

**Return**

True if a1 and a2 are the same (same node pointer, same phandle
args), false otherwise.

int of\_property\_count\_u8\_elems(const struct device\_node \*np, const char \*propname)
:   Count the number of u8 elements in a property

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

**Description**

Search for a property in a device node and count the number of u8 elements
in it.

**Return**

The number of elements on success, -EINVAL if the property does
not exist or its length does not match a multiple of u8 and -ENODATA if the
property does not have a value.

int of\_property\_count\_u16\_elems(const struct device\_node \*np, const char \*propname)
:   Count the number of u16 elements in a property

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

**Description**

Search for a property in a device node and count the number of u16 elements
in it.

**Return**

The number of elements on success, -EINVAL if the property does
not exist or its length does not match a multiple of u16 and -ENODATA if the
property does not have a value.

int of\_property\_count\_u32\_elems(const struct device\_node \*np, const char \*propname)
:   Count the number of u32 elements in a property

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

**Description**

Search for a property in a device node and count the number of u32 elements
in it.

**Return**

The number of elements on success, -EINVAL if the property does
not exist or its length does not match a multiple of u32 and -ENODATA if the
property does not have a value.

int of\_property\_count\_u64\_elems(const struct device\_node \*np, const char \*propname)
:   Count the number of u64 elements in a property

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

**Description**

Search for a property in a device node and count the number of u64 elements
in it.

**Return**

The number of elements on success, -EINVAL if the property does
not exist or its length does not match a multiple of u64 and -ENODATA if the
property does not have a value.

int of\_property\_read\_string\_array(const struct device\_node \*np, const char \*propname, const char \*\*out\_strs, size\_t sz)
:   Read an array of strings from a multiple strings property.

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

`const char **out_strs`
:   output array of string pointers.

`size_t sz`
:   number of array elements to read.

**Description**

Search for a property in a device tree node and retrieve a list of
terminated string values (pointer to data, not a copy) in that property.

**Return**

If **out\_strs** is NULL, the number of strings in the property is returned.

int of\_property\_count\_strings(const struct device\_node \*np, const char \*propname)
:   Find and return the number of strings from a multiple strings property.

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

**Description**

Search for a property in a device tree node and retrieve the number of null
terminated string contain in it.

**Return**

The number of strings on success, -EINVAL if the property does not
exist, -ENODATA if property does not have a value, and -EILSEQ if the string
is not null-terminated within the length of the property data.

int of\_property\_read\_string\_index(const struct device\_node \*np, const char \*propname, int index, const char \*\*output)
:   Find and read a string from a multiple strings property.

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

`int index`
:   index of the string in the list of strings

`const char **output`
:   pointer to null terminated return string, modified only if
    return value is 0.

**Description**

Search for a property in a device tree node and retrieve a null
terminated string value (pointer to data, not a copy) in the list of strings
contained in that property.

The out\_string pointer is modified only if a valid string can be decoded.

**Return**

0 on success, -EINVAL if the property does not exist, -ENODATA if
property does not have a value, and -EILSEQ if the string is not
null-terminated within the length of the property data.

bool of\_property\_present(const struct device\_node \*np, const char \*propname)
:   Test if a property is present in a node

**Parameters**

`const struct device_node *np`
:   device node to search for the property.

`const char *propname`
:   name of the property to be searched.

**Description**

Test for a property present in a device node.

**Return**

true if the property exists false otherwise.

int of\_property\_read\_u8\_array(const struct device\_node \*np, const char \*propname, u8 \*out\_values, size\_t sz)
:   Find and read an array of u8 from a property.

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

`u8 *out_values`
:   pointer to return value, modified only if return value is 0.

`size_t sz`
:   number of array elements to read

**Description**

Search for a property in a device node and read 8-bit value(s) from
it.

dts entry of array should be like:
:   `property = /bits/ 8 <0x50 0x60 0x70>;`

The out\_values is modified only if a valid u8 value can be decoded.

**Return**

0 on success, -EINVAL if the property does not exist,
-ENODATA if property does not have a value, and -EOVERFLOW if the
property data isn’t large enough.

int of\_property\_read\_u16\_array(const struct device\_node \*np, const char \*propname, u16 \*out\_values, size\_t sz)
:   Find and read an array of u16 from a property.

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

`u16 *out_values`
:   pointer to return value, modified only if return value is 0.

`size_t sz`
:   number of array elements to read

**Description**

Search for a property in a device node and read 16-bit value(s) from
it.

dts entry of array should be like:
:   `property = /bits/ 16 <0x5000 0x6000 0x7000>;`

The out\_values is modified only if a valid u16 value can be decoded.

**Return**

0 on success, -EINVAL if the property does not exist,
-ENODATA if property does not have a value, and -EOVERFLOW if the
property data isn’t large enough.

int of\_property\_read\_u32\_array(const struct device\_node \*np, const char \*propname, u32 \*out\_values, size\_t sz)
:   Find and read an array of 32 bit integers from a property.

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

`u32 *out_values`
:   pointer to return value, modified only if return value is 0.

`size_t sz`
:   number of array elements to read

**Description**

Search for a property in a device node and read 32-bit value(s) from
it.

The out\_values is modified only if a valid u32 value can be decoded.

**Return**

0 on success, -EINVAL if the property does not exist,
-ENODATA if property does not have a value, and -EOVERFLOW if the
property data isn’t large enough.

int of\_property\_read\_u64\_array(const struct device\_node \*np, const char \*propname, u64 \*out\_values, size\_t sz)
:   Find and read an array of 64 bit integers from a property.

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

`u64 *out_values`
:   pointer to return value, modified only if return value is 0.

`size_t sz`
:   number of array elements to read

**Description**

Search for a property in a device node and read 64-bit value(s) from
it.

The out\_values is modified only if a valid u64 value can be decoded.

**Return**

0 on success, -EINVAL if the property does not exist,
-ENODATA if property does not have a value, and -EOVERFLOW if the
property data isn’t large enough.

struct of\_changeset\_entry
:   Holds a changeset entry

**Definition**:

```
struct of_changeset_entry {
    struct list_head node;
    unsigned long action;
    struct device_node *np;
    struct property *prop;
    struct property *old_prop;
};
```

**Members**

`node`
:   list\_head for the log list

`action`
:   notifier action

`np`
:   pointer to the device node affected

`prop`
:   pointer to the property affected

`old_prop`
:   hold a pointer to the original property

**Description**

Every modification of the device tree during a changeset
is held in a list of of\_changeset\_entry structures.
That way we can recover from a partial application, or we can
revert the changeset

struct of\_changeset
:   changeset tracker structure

**Definition**:

```
struct of_changeset {
    struct list_head entries;
};
```

**Members**

`entries`
:   list\_head for the changeset entries

**Description**

changesets are a convenient way to apply bulk changes to the
live tree. In case of an error, changes are rolled-back.
changesets live on after initial application, and if not
destroyed after use, they can be reverted in one single call.

bool of\_device\_is\_system\_power\_controller(const struct device\_node \*np)
:   Tells if system-power-controller is found for device\_node

**Parameters**

`const struct device_node *np`
:   Pointer to the given device\_node

**Return**

true if present false otherwise

bool of\_have\_populated\_dt(void)
:   Has DT been populated by bootloader

**Parameters**

`void`
:   no arguments

**Return**

True if a DTB has been populated by the bootloader and it isn’t the
empty builtin one. False otherwise.

bool of\_property\_read\_bool(const struct device\_node \*np, const char \*propname)
:   Find a property

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

**Description**

Search for a boolean property in a device node. Usage on non-boolean
property types is deprecated.

**Return**

true if the property exists false otherwise.

bool of\_graph\_is\_present(const struct device\_node \*node)
:   check graph’s presence

**Parameters**

`const struct device_node *node`
:   pointer to device\_node containing graph port

**Return**

True if **node** has a port or ports (with a port) sub-node,
false otherwise.

int of\_property\_count\_elems\_of\_size(const struct device\_node \*np, const char \*propname, int elem\_size)
:   Count the number of elements in a property

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

`int elem_size`
:   size of the individual element

**Description**

Search for a property in a device node and count the number of elements of
size elem\_size in it.

**Return**

The number of elements on success, -EINVAL if the property does not
exist or its length does not match a multiple of elem\_size and -ENODATA if
the property does not have a value.

int of\_property\_read\_u8\_index(const struct device\_node \*np, const char \*propname, u32 index, u8 \*out\_value)
:   Find and read a u8 from a multi-value property.

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

`u32 index`
:   index of the u8 in the list of values

`u8 *out_value`
:   pointer to return value, modified only if no error.

**Description**

Search for a property in a device node and read nth 8-bit value from
it.

The out\_value is modified only if a valid u8 value can be decoded.

**Return**

0 on success, -EINVAL if the property does not exist,
-ENODATA if property does not have a value, and -EOVERFLOW if the
property data isn’t large enough.

int of\_property\_read\_u16\_index(const struct device\_node \*np, const char \*propname, u32 index, u16 \*out\_value)
:   Find and read a u16 from a multi-value property.

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

`u32 index`
:   index of the u16 in the list of values

`u16 *out_value`
:   pointer to return value, modified only if no error.

**Description**

Search for a property in a device node and read nth 16-bit value from
it.

The out\_value is modified only if a valid u16 value can be decoded.

**Return**

0 on success, -EINVAL if the property does not exist,
-ENODATA if property does not have a value, and -EOVERFLOW if the
property data isn’t large enough.

int of\_property\_read\_u32\_index(const struct device\_node \*np, const char \*propname, u32 index, u32 \*out\_value)
:   Find and read a u32 from a multi-value property.

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

`u32 index`
:   index of the u32 in the list of values

`u32 *out_value`
:   pointer to return value, modified only if no error.

**Description**

Search for a property in a device node and read nth 32-bit value from
it.

The out\_value is modified only if a valid u32 value can be decoded.

**Return**

0 on success, -EINVAL if the property does not exist,
-ENODATA if property does not have a value, and -EOVERFLOW if the
property data isn’t large enough.

int of\_property\_read\_u64\_index(const struct device\_node \*np, const char \*propname, u32 index, u64 \*out\_value)
:   Find and read a u64 from a multi-value property.

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

`u32 index`
:   index of the u64 in the list of values

`u64 *out_value`
:   pointer to return value, modified only if no error.

**Description**

Search for a property in a device node and read nth 64-bit value from
it.

The out\_value is modified only if a valid u64 value can be decoded.

**Return**

0 on success, -EINVAL if the property does not exist,
-ENODATA if property does not have a value, and -EOVERFLOW if the
property data isn’t large enough.

int of\_property\_read\_variable\_u8\_array(const struct device\_node \*np, const char \*propname, u8 \*out\_values, size\_t sz\_min, size\_t sz\_max)
:   Find and read an array of u8 from a property, with bounds on the minimum and maximum array size.

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

`u8 *out_values`
:   pointer to found values.

`size_t sz_min`
:   minimum number of array elements to read

`size_t sz_max`
:   maximum number of array elements to read, if zero there is no
    upper limit on the number of elements in the dts entry but only
    sz\_min will be read.

**Description**

Search for a property in a device node and read 8-bit value(s) from
it.

dts entry of array should be like:
:   `property = /bits/ 8 <0x50 0x60 0x70>;`

The out\_values is modified only if a valid u8 value can be decoded.

**Return**

The number of elements read on success, -EINVAL if the property
does not exist, -ENODATA if property does not have a value, and -EOVERFLOW
if the property data is smaller than sz\_min or longer than sz\_max.

int of\_property\_read\_variable\_u16\_array(const struct device\_node \*np, const char \*propname, u16 \*out\_values, size\_t sz\_min, size\_t sz\_max)
:   Find and read an array of u16 from a property, with bounds on the minimum and maximum array size.

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

`u16 *out_values`
:   pointer to found values.

`size_t sz_min`
:   minimum number of array elements to read

`size_t sz_max`
:   maximum number of array elements to read, if zero there is no
    upper limit on the number of elements in the dts entry but only
    sz\_min will be read.

**Description**

Search for a property in a device node and read 16-bit value(s) from
it.

dts entry of array should be like:
:   `property = /bits/ 16 <0x5000 0x6000 0x7000>;`

The out\_values is modified only if a valid u16 value can be decoded.

**Return**

The number of elements read on success, -EINVAL if the property
does not exist, -ENODATA if property does not have a value, and -EOVERFLOW
if the property data is smaller than sz\_min or longer than sz\_max.

int of\_property\_read\_variable\_u32\_array(const struct device\_node \*np, const char \*propname, u32 \*out\_values, size\_t sz\_min, size\_t sz\_max)
:   Find and read an array of 32 bit integers from a property, with bounds on the minimum and maximum array size.

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

`u32 *out_values`
:   pointer to return found values.

`size_t sz_min`
:   minimum number of array elements to read

`size_t sz_max`
:   maximum number of array elements to read, if zero there is no
    upper limit on the number of elements in the dts entry but only
    sz\_min will be read.

**Description**

Search for a property in a device node and read 32-bit value(s) from
it.

The out\_values is modified only if a valid u32 value can be decoded.

**Return**

The number of elements read on success, -EINVAL if the property
does not exist, -ENODATA if property does not have a value, and -EOVERFLOW
if the property data is smaller than sz\_min or longer than sz\_max.

int of\_property\_read\_u64(const struct device\_node \*np, const char \*propname, u64 \*out\_value)
:   Find and read a 64 bit integer from a property

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

`u64 *out_value`
:   pointer to return value, modified only if return value is 0.

**Description**

Search for a property in a device node and read a 64-bit value from
it.

The out\_value is modified only if a valid u64 value can be decoded.

**Return**

0 on success, -EINVAL if the property does not exist,
-ENODATA if property does not have a value, and -EOVERFLOW if the
property data isn’t large enough.

int of\_property\_read\_variable\_u64\_array(const struct device\_node \*np, const char \*propname, u64 \*out\_values, size\_t sz\_min, size\_t sz\_max)
:   Find and read an array of 64 bit integers from a property, with bounds on the minimum and maximum array size.

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

`u64 *out_values`
:   pointer to found values.

`size_t sz_min`
:   minimum number of array elements to read

`size_t sz_max`
:   maximum number of array elements to read, if zero there is no
    upper limit on the number of elements in the dts entry but only
    sz\_min will be read.

**Description**

Search for a property in a device node and read 64-bit value(s) from
it.

The out\_values is modified only if a valid u64 value can be decoded.

**Return**

The number of elements read on success, -EINVAL if the property
does not exist, -ENODATA if property does not have a value, and -EOVERFLOW
if the property data is smaller than sz\_min or longer than sz\_max.

int of\_property\_read\_string(const struct device\_node \*np, const char \*propname, const char \*\*out\_string)
:   Find and read a string from a property

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

`const char **out_string`
:   pointer to null terminated return string, modified only if
    return value is 0.

**Description**

Search for a property in a device tree node and retrieve a null
terminated string value (pointer to data, not a copy).

Note that the empty string “” has length of 1, thus -ENODATA cannot
be interpreted as an empty string.

The out\_string pointer is modified only if a valid string can be decoded.

**Return**

0 on success, -EINVAL if the property does not exist, -ENODATA if
property does not have a value, and -EILSEQ if the string is not
null-terminated within the length of the property data.

int of\_property\_match\_string(const struct device\_node \*np, const char \*propname, const char \*string)
:   Find string in a list and return index

**Parameters**

`const struct device_node *np`
:   pointer to the node containing the string list property

`const char *propname`
:   string list property name

`const char *string`
:   pointer to the string to search for in the string list

**Description**

Search for an exact match of string in a device node property which is a
string of lists.

**Return**

the index of the first occurrence of the string on success, -EINVAL
if the property does not exist, -ENODATA if the property does not have a
value, and -EILSEQ if the string is not null-terminated within the length of
the property data.

int of\_property\_read\_string\_helper(const struct device\_node \*np, const char \*propname, const char \*\*out\_strs, size\_t sz, int skip)
:   Utility helper for parsing string properties

**Parameters**

`const struct device_node *np`
:   device node from which the property value is to be read.

`const char *propname`
:   name of the property to be searched.

`const char **out_strs`
:   output array of string pointers.

`size_t sz`
:   number of array elements to read.

`int skip`
:   Number of strings to skip over at beginning of list.

**Description**

Don’t call this function directly. It is a utility helper for the
of\_property\_read\_string\*() family of functions.

int of\_graph\_parse\_endpoint(const struct device\_node \*node, struct [of\_endpoint](#c.of_endpoint "of_endpoint") \*endpoint)
:   parse common endpoint node properties

**Parameters**

`const struct device_node *node`
:   pointer to endpoint device\_node

`struct of_endpoint *endpoint`
:   pointer to the OF endpoint data structure

**Description**

The caller should hold a reference to **node**.

struct device\_node \*of\_graph\_get\_port\_by\_id(struct device\_node \*parent, u32 id)
:   get the port matching a given id

**Parameters**

`struct device_node *parent`
:   pointer to the parent device node

`u32 id`
:   id of the port

**Return**

A ‘port’ node pointer with refcount incremented. The caller
has to use [`of_node_put()`](#c.of_node_put "of_node_put") on it when done.

struct device\_node \*of\_graph\_get\_next\_port(const struct device\_node \*parent, struct device\_node \*prev)
:   get next port node.

**Parameters**

`const struct device_node *parent`
:   pointer to the parent device node, or parent ports node

`struct device_node *prev`
:   previous port node, or NULL to get first

**Description**

Parent device node can be used as **parent** whether device node has ports node
or not. It will work same as ports\*\*0\*\* node.

**Return**

A ‘port’ node pointer with refcount incremented. Refcount
of the passed **prev** node is decremented.

struct device\_node \*of\_graph\_get\_next\_port\_endpoint(const struct device\_node \*port, struct device\_node \*prev)
:   get next endpoint node in port. If it reached to end of the port, it will return NULL.

**Parameters**

`const struct device_node *port`
:   pointer to the target port node

`struct device_node *prev`
:   previous endpoint node, or NULL to get first

**Return**

An ‘endpoint’ node pointer with refcount incremented. Refcount
of the passed **prev** node is decremented.

struct device\_node \*of\_graph\_get\_next\_endpoint(const struct device\_node \*parent, struct device\_node \*prev)
:   get next endpoint node

**Parameters**

`const struct device_node *parent`
:   pointer to the parent device node

`struct device_node *prev`
:   previous endpoint node, or NULL to get first

**Return**

An ‘endpoint’ node pointer with refcount incremented. Refcount
of the passed **prev** node is decremented.

struct device\_node \*of\_graph\_get\_endpoint\_by\_regs(const struct device\_node \*parent, int port\_reg, int reg)
:   get endpoint node of specific identifiers

**Parameters**

`const struct device_node *parent`
:   pointer to the parent device node

`int port_reg`
:   identifier (value of reg property) of the parent port node

`int reg`
:   identifier (value of reg property) of the endpoint node

**Return**

An ‘endpoint’ node pointer which is identified by reg and at the same
is the child of a port node identified by port\_reg. reg and port\_reg are
ignored when they are -1. Use [`of_node_put()`](#c.of_node_put "of_node_put") on the pointer when done.

struct device\_node \*of\_graph\_get\_remote\_endpoint(const struct device\_node \*node)
:   get remote endpoint node

**Parameters**

`const struct device_node *node`
:   pointer to a local endpoint device\_node

**Return**

Remote endpoint node associated with remote endpoint node linked
to **node**. Use [`of_node_put()`](#c.of_node_put "of_node_put") on it when done.

struct device\_node \*of\_graph\_get\_port\_parent(struct device\_node \*node)
:   get port’s parent node

**Parameters**

`struct device_node *node`
:   pointer to a local endpoint device\_node

**Return**

device node associated with endpoint node linked
to **node**. Use [`of_node_put()`](#c.of_node_put "of_node_put") on it when done.

struct device\_node \*of\_graph\_get\_remote\_port\_parent(const struct device\_node \*node)
:   get remote port’s parent node

**Parameters**

`const struct device_node *node`
:   pointer to a local endpoint device\_node

**Return**

Remote device node associated with remote endpoint node linked
to **node**. Use [`of_node_put()`](#c.of_node_put "of_node_put") on it when done.

struct device\_node \*of\_graph\_get\_remote\_port(const struct device\_node \*node)
:   get remote port node

**Parameters**

`const struct device_node *node`
:   pointer to a local endpoint device\_node

**Return**

Remote port node associated with remote endpoint node linked
to **node**. Use [`of_node_put()`](#c.of_node_put "of_node_put") on it when done.

unsigned int of\_graph\_get\_endpoint\_count(const struct device\_node \*np)
:   get the number of endpoints in a device node

**Parameters**

`const struct device_node *np`
:   parent device node containing ports and endpoints

**Return**

count of endpoint of this device node

unsigned int of\_graph\_get\_port\_count(struct device\_node \*np)
:   get the number of port in a device or ports node

**Parameters**

`struct device_node *np`
:   pointer to the device or ports node

**Return**

count of port of this device or ports node

struct device\_node \*of\_graph\_get\_remote\_node(const struct device\_node \*node, u32 port, u32 endpoint)
:   get remote parent device\_node for given port/endpoint

**Parameters**

`const struct device_node *node`
:   pointer to parent device\_node containing graph port/endpoint

`u32 port`
:   identifier (value of reg property) of the parent port node

`u32 endpoint`
:   identifier (value of reg property) of the endpoint node

**Return**

Remote device node associated with remote endpoint node linked
to **node**. Use [`of_node_put()`](#c.of_node_put "of_node_put") on it when done.

struct of\_endpoint
:   the OF graph endpoint data structure

**Definition**:

```
struct of_endpoint {
    unsigned int port;
    unsigned int id;
    const struct device_node *local_node;
};
```

**Members**

`port`
:   identifier (value of reg property) of a port this endpoint belongs to

`id`
:   identifier (value of reg property) of this endpoint

`local_node`
:   pointer to device\_node of this endpoint

for\_each\_endpoint\_of\_node

`for_each_endpoint_of_node (parent, child)`

> iterate over every endpoint in a device node

**Parameters**

`parent`
:   parent device node containing ports and endpoints

`child`
:   loop variable pointing to the current endpoint node

**Description**

When breaking out of the loop, of\_node\_put(child) has to be called manually.

for\_each\_of\_graph\_port

`for_each_of_graph_port (parent, child)`

> iterate over every port in a device or ports node

**Parameters**

`parent`
:   parent device or ports node containing port

`child`
:   loop variable pointing to the current port node

**Description**

When breaking out of the loop, and continue to use the **child**, you need to
use return\_ptr(**child**) or no\_free\_ptr(**child**) not to call `__free()` for it.

for\_each\_of\_graph\_port\_endpoint

`for_each_of_graph_port_endpoint (parent, child)`

> iterate over every endpoint in a port node

**Parameters**

`parent`
:   parent port node

`child`
:   loop variable pointing to the current endpoint node

**Description**

When breaking out of the loop, and continue to use the **child**, you need to
use return\_ptr(**child**) or no\_free\_ptr(**child**) not to call `__free()` for it.

const \_\_be32 \*of\_translate\_dma\_region(struct device\_node \*dev, const \_\_be32 \*prop, phys\_addr\_t \*start, size\_t \*length)
:   Translate device tree address and size tuple

**Parameters**

`struct device_node *dev`
:   device tree node for which to translate

`const __be32 *prop`
:   pointer into array of cells

`phys_addr_t *start`
:   return value for the start of the DMA range

`size_t *length`
:   return value for the length of the DMA range

**Description**

Returns a pointer to the cell immediately following the translated DMA region.

int of\_property\_read\_reg(struct device\_node \*np, int idx, u64 \*addr, u64 \*size)
:   Retrieve the specified “reg” entry index without translating

**Parameters**

`struct device_node *np`
:   device tree node for which to retrieve “reg” from

`int idx`
:   “reg” entry index to read

`u64 *addr`
:   return value for the untranslated address

`u64 *size`
:   return value for the entry size

**Description**

Returns -EINVAL if “reg” is not found. Returns 0 on success with addr and
size values filled in.

bool of\_dma\_is\_coherent(struct device\_node \*np)
:   Check if device is coherent

**Parameters**

`struct device_node *np`
:   device node

**Description**

It returns true if “dma-coherent” property was found
for this device in the DT, or if DMA is coherent by
default for OF devices on the current platform and no
“dma-noncoherent” property was found for this device.

int of\_address\_to\_resource(struct device\_node \*dev, int index, struct resource \*r)
:   Translate device tree address and return as resource

**Parameters**

`struct device_node *dev`
:   Caller’s Device Node

`int index`
:   Index into the array

`struct resource *r`
:   Pointer to resource array

**Description**

Returns -EINVAL if the range cannot be converted to resource.

Note that if your address is a PIO address, the conversion will fail if
the physical address can’t be internally converted to an IO token with
`pci_address_to_pio()`, that is because it’s either called too early or it
can’t be matched to any host bridge IO space

void \_\_iomem \*of\_iomap(struct device\_node \*np, int index)
:   Maps the memory mapped IO for a given device\_node

**Parameters**

`struct device_node *np`
:   the device whose io range will be mapped

`int index`
:   index of the io range

**Description**

Returns a pointer to the mapped memory

unsigned int irq\_of\_parse\_and\_map(struct device\_node \*dev, int index)
:   Parse and map an interrupt into linux virq space

**Parameters**

`struct device_node *dev`
:   Device node of the device whose interrupt is to be mapped

`int index`
:   Index of the interrupt to map

**Description**

This function is a wrapper that chains [`of_irq_parse_one()`](#c.of_irq_parse_one "of_irq_parse_one") and
`irq_create_of_mapping()` to make things easier to callers

struct device\_node \*of\_irq\_find\_parent(struct device\_node \*child)
:   Given a device node, find its interrupt parent node

**Parameters**

`struct device_node *child`
:   pointer to device node

**Return**

A pointer to the interrupt parent node with refcount increased
or NULL if the interrupt parent could not be determined.

int of\_irq\_parse\_raw(const \_\_be32 \*addr, struct of\_phandle\_args \*out\_irq)
:   Low level interrupt tree parsing

**Parameters**

`const __be32 *addr`
:   address specifier (start of “reg” property of the device) in be32 format

`struct of_phandle_args *out_irq`
:   structure of\_phandle\_args updated by this function

**Description**

This function is a low-level interrupt tree walking function. It
can be used to do a partial walk with synthesized reg and interrupts
properties, for example when resolving PCI interrupts when no device
node exist for the parent. It takes an interrupt specifier structure as
input, walks the tree looking for any interrupt-map properties, translates
the specifier for each map, and then returns the translated map.

**Return**

0 on success and a negative number on error

**Note**

refcount of node **out\_irq->np** is increased by 1 on success.

int of\_irq\_parse\_one(struct device\_node \*device, int index, struct of\_phandle\_args \*out\_irq)
:   Resolve an interrupt for a device

**Parameters**

`struct device_node *device`
:   the device whose interrupt is to be resolved

`int index`
:   index of the interrupt to resolve

`struct of_phandle_args *out_irq`
:   structure of\_phandle\_args filled by this function

**Description**

This function resolves an interrupt for a node by walking the interrupt tree,
finding which interrupt controller node it is attached to, and returning the
interrupt specifier that can be used to retrieve a Linux IRQ number.

**Note**

refcount of node **out\_irq->np** is increased by 1 on success.

int of\_irq\_to\_resource(struct device\_node \*dev, int index, struct resource \*r)
:   Decode a node’s IRQ and return it as a resource

**Parameters**

`struct device_node *dev`
:   pointer to device tree node

`int index`
:   zero-based index of the irq

`struct resource *r`
:   pointer to resource structure to return result into.

int of\_irq\_get(struct device\_node \*dev, int index)
:   Decode a node’s IRQ and return it as a Linux IRQ number

**Parameters**

`struct device_node *dev`
:   pointer to device tree node

`int index`
:   zero-based index of the IRQ

**Return**

Linux IRQ number on success, or 0 on the IRQ mapping failure, or
-EPROBE\_DEFER if the IRQ domain is not yet created, or error code in case
of any other failure.

int of\_irq\_get\_byname(struct device\_node \*dev, const char \*name)
:   Decode a node’s IRQ and return it as a Linux IRQ number

**Parameters**

`struct device_node *dev`
:   pointer to device tree node

`const char *name`
:   IRQ name

**Return**

Linux IRQ number on success, or 0 on the IRQ mapping failure, or
-EPROBE\_DEFER if the IRQ domain is not yet created, or error code in case
of any other failure.

int of\_irq\_count(struct device\_node \*dev)
:   Count the number of IRQs a node uses

**Parameters**

`struct device_node *dev`
:   pointer to device tree node

int of\_irq\_to\_resource\_table(struct device\_node \*dev, struct resource \*res, int nr\_irqs)
:   Fill in resource table with node’s IRQ info

**Parameters**

`struct device_node *dev`
:   pointer to device tree node

`struct resource *res`
:   array of resources to fill in

`int nr_irqs`
:   the number of IRQs (and upper bound for num of **res** elements)

**Return**

The size of the filled in table (up to **nr\_irqs**).

u32 of\_msi\_xlate(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct device\_node \*\*msi\_np, u32 id\_in)
:   map a MSI ID and find relevant MSI controller node

**Parameters**

`struct device *dev`
:   device for which the mapping is to be done.

`struct device_node **msi_np`
:   Pointer to target MSI controller node

`u32 id_in`
:   Device ID.

**Description**

Walk up the device hierarchy looking for devices with a “msi-map”
or “msi-parent” property. If found, apply the mapping to **id\_in**.
If **msi\_np** points to a non-NULL device node pointer, only entries targeting
that node will be matched; if it points to a NULL value, it will receive the
device node of the first matching target phandle, with a reference held.

**Return**

The mapped MSI id.

struct [irq\_domain](../core-api/irq/irq-domain.html#c.irq_domain "irq_domain") \*of\_msi\_get\_domain(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, const struct device\_node \*np, enum irq\_domain\_bus\_token token)
:   Use msi-parent to find the relevant MSI domain

**Parameters**

`struct device *dev`
:   device for which the domain is requested

`const struct device_node *np`
:   device node for **dev**

`enum irq_domain_bus_token token`
:   bus type for this domain

**Description**

Parse the msi-parent property and returns the corresponding MSI domain.

**Return**

the MSI domain for this device (or NULL on failure).

void of\_msi\_configure(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, const struct device\_node \*np)
:   Set the msi\_domain field of a device

**Parameters**

`struct device *dev`
:   device structure to associate with an MSI irq domain

`const struct device_node *np`
:   device node for that device

void \*of\_fdt\_unflatten\_tree(const unsigned long \*blob, struct device\_node \*dad, struct device\_node \*\*mynodes)
:   create tree of device\_nodes from flat blob

**Parameters**

`const unsigned long *blob`
:   Flat device tree blob

`struct device_node *dad`
:   Parent device node

`struct device_node **mynodes`
:   The device tree created by the call

**Description**

unflattens the device-tree passed by the firmware, creating the
tree of `struct device_node`. It also fills the “name” and “type”
pointers of the nodes so the normal device-tree walking functions
can be used.

**Return**

NULL on failure or the memory chunk containing the unflattened
device tree on success.

## Driver model functions

int of\_driver\_match\_device(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, const struct [device\_driver](../driver-api/infrastructure.html#c.device_driver "device_driver") \*drv)
:   Tell if a driver’s of\_match\_table matches a device.

**Parameters**

`struct device *dev`
:   the device structure to match against

`const struct device_driver *drv`
:   the device\_driver structure to test

const struct of\_device\_id \*of\_match\_device(const struct of\_device\_id \*matches, const struct [device](../driver-api/infrastructure.html#c.device "device") \*dev)
:   Tell if a [`struct device`](../driver-api/infrastructure.html#c.device "device") matches an of\_device\_id list

**Parameters**

`const struct of_device_id *matches`
:   array of of\_device\_id match structures to search in

`const struct device *dev`
:   the OF device structure to match against

**Description**

Used by a driver to check whether an platform\_device present in the
system is in its list of supported devices.

int of\_dma\_configure\_id(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct device\_node \*np, bool force\_dma, const u32 \*id)
:   Setup DMA configuration

**Parameters**

`struct device *dev`
:   Device to apply DMA configuration

`struct device_node *np`
:   Pointer to OF node having DMA configuration

`bool force_dma`
:   Whether device is to be set up by `of_dma_configure()` even if
    DMA capability is not explicitly described by firmware.

`const u32 *id`
:   Optional const pointer value input id

**Description**

Try to get devices’s DMA configuration from DT and update it
accordingly.

If platform code needs to use its own special DMA configuration, it
can use a platform bus notifier and handle BUS\_NOTIFY\_ADD\_DEVICE events
to fix up DMA configuration.

ssize\_t of\_device\_modalias(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, char \*str, ssize\_t len)
:   Fill buffer with newline terminated modalias string

**Parameters**

`struct device *dev`
:   Calling device

`char *str`
:   Modalias string

`ssize_t len`
:   Size of **str**

void of\_device\_uevent(const struct [device](../driver-api/infrastructure.html#c.device "device") \*dev, struct kobj\_uevent\_env \*env)
:   Display OF related uevent information

**Parameters**

`const struct device *dev`
:   Device to display the uevent information for

`struct kobj_uevent_env *env`
:   Kernel object’s userspace event reference to fill up

void of\_device\_make\_bus\_id(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev)
:   Use the device node data to assign a unique name

**Parameters**

`struct device *dev`
:   pointer to device structure that is linked to a device tree node

**Description**

This routine will first try using the translated bus address to
derive a unique name. If it cannot, then it will prepend names from
parent nodes until a unique name can be derived.

struct of\_dev\_auxdata
:   lookup table entry for device names & platform\_data

**Definition**:

```
struct of_dev_auxdata {
    char *compatible;
    resource_size_t phys_addr;
    char *name;
    void *platform_data;
};
```

**Members**

`compatible`
:   compatible value of node to match against node

`phys_addr`
:   Start address of registers to match against node

`name`
:   Name to assign for matching nodes

`platform_data`
:   platform\_data to assign for matching nodes

**Description**

This lookup table allows the caller of [`of_platform_populate()`](#c.of_platform_populate "of_platform_populate") to override
the names of devices when creating devices from the device tree. The table
should be terminated with an empty entry. It also allows the platform\_data
pointer to be set.

The reason for this functionality is that some Linux infrastructure uses
the device name to look up a specific device, but the Linux-specific names
are not encoded into the device tree, so the kernel needs to provide specific
values.

**Note**

Using an auxdata lookup table should be considered a last resort when
converting a platform to use the DT. Normally the automatically generated
device name will not matter, and drivers should obtain data from the device
node instead of from an anonymous platform\_data pointer.

struct platform\_device \*of\_find\_device\_by\_node(struct device\_node \*np)
:   Find the platform\_device associated with a node

**Parameters**

`struct device_node *np`
:   Pointer to device tree node

**Description**

Takes a reference to the embedded [`struct device`](../driver-api/infrastructure.html#c.device "device") which needs to be dropped
after use.

**Return**

platform\_device pointer, or NULL if not found

struct platform\_device \*of\_device\_alloc(struct device\_node \*np, const char \*bus\_id, struct [device](../driver-api/infrastructure.html#c.device "device") \*parent)
:   Allocate and initialize an of\_device

**Parameters**

`struct device_node *np`
:   device node to assign to device

`const char *bus_id`
:   Name to assign to the device. May be null to use default name.

`struct device *parent`
:   Parent device.

struct platform\_device \*of\_platform\_device\_create(struct device\_node \*np, const char \*bus\_id, struct [device](../driver-api/infrastructure.html#c.device "device") \*parent)
:   Alloc, initialize and register an of\_device

**Parameters**

`struct device_node *np`
:   pointer to node to create device for

`const char *bus_id`
:   name to assign device

`struct device *parent`
:   Linux device model parent device.

**Return**

Pointer to created platform device, or NULL if a device was not
registered. Unavailable devices will not get registered.

int of\_platform\_bus\_probe(struct device\_node \*root, const struct of\_device\_id \*matches, struct [device](../driver-api/infrastructure.html#c.device "device") \*parent)
:   Probe the device-tree for platform buses

**Parameters**

`struct device_node *root`
:   parent of the first level to probe or NULL for the root of the tree

`const struct of_device_id *matches`
:   match table for bus nodes

`struct device *parent`
:   parent to hook devices from, NULL for toplevel

**Description**

Note that children of the provided root are not instantiated as devices
unless the specified root itself matches the bus list and is not NULL.

int of\_platform\_populate(struct device\_node \*root, const struct of\_device\_id \*matches, const struct [of\_dev\_auxdata](#c.of_dev_auxdata "of_dev_auxdata") \*lookup, struct [device](../driver-api/infrastructure.html#c.device "device") \*parent)
:   Populate platform\_devices from device tree data

**Parameters**

`struct device_node *root`
:   parent of the first level to probe or NULL for the root of the tree

`const struct of_device_id *matches`
:   match table, NULL to use the default

`const struct of_dev_auxdata *lookup`
:   auxdata table for matching id and platform\_data with device nodes

`struct device *parent`
:   parent to hook devices from, NULL for toplevel

**Description**

Similar to [`of_platform_bus_probe()`](#c.of_platform_bus_probe "of_platform_bus_probe"), this function walks the device tree
and creates devices from nodes. It differs in that it follows the modern
convention of requiring all device nodes to have a ‘compatible’ property,
and it is suitable for creating devices which are children of the root
node (of\_platform\_bus\_probe will only create children of the root which
are selected by the **matches** argument).

New board support should be using this function instead of
[`of_platform_bus_probe()`](#c.of_platform_bus_probe "of_platform_bus_probe").

**Return**

0 on success, < 0 on failure.

void of\_platform\_depopulate(struct [device](../driver-api/infrastructure.html#c.device "device") \*parent)
:   Remove devices populated from device tree

**Parameters**

`struct device *parent`
:   device which children will be removed

**Description**

Complementary to [`of_platform_populate()`](#c.of_platform_populate "of_platform_populate"), this function removes children
of the given device (and, recursively, their children) that have been
created from their respective device tree nodes (and only those,
leaving others - eg. manually created - unharmed).

int devm\_of\_platform\_populate(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev)
:   Populate platform\_devices from device tree data

**Parameters**

`struct device *dev`
:   device that requested to populate from device tree data

**Description**

Similar to [`of_platform_populate()`](#c.of_platform_populate "of_platform_populate"), but will automatically call
[`of_platform_depopulate()`](#c.of_platform_depopulate "of_platform_depopulate") when the device is unbound from the bus.

**Return**

0 on success, < 0 on failure.

void devm\_of\_platform\_depopulate(struct [device](../driver-api/infrastructure.html#c.device "device") \*dev)
:   Remove devices populated from device tree

**Parameters**

`struct device *dev`
:   device that requested to depopulate from device tree data

**Description**

Complementary to [`devm_of_platform_populate()`](#c.devm_of_platform_populate "devm_of_platform_populate"), this function removes children
of the given device (and, recursively, their children) that have been
created from their respective device tree nodes (and only those,
leaving others - eg. manually created - unharmed).

## Overlay and Dynamic DT functions

int of\_resolve\_phandles(struct device\_node \*overlay)
:   Relocate and resolve overlay against live tree

**Parameters**

`struct device_node *overlay`
:   Pointer to devicetree overlay to relocate and resolve

**Description**

Modify (relocate) values of local phandles in **overlay** to a range that
does not conflict with the live expanded devicetree. Update references
to the local phandles in **overlay**. Update (resolve) phandle references
in **overlay** that refer to the live expanded devicetree.

Phandle values in the live tree are in the range of
1 .. `live_tree_max_phandle()`. The range of phandle values in the overlay
also begin with at 1. Adjust the phandle values in the overlay to begin
at `live_tree_max_phandle()` + 1. Update references to the phandles to
the adjusted phandle values.

The name of each property in the “\_\_fixups\_\_” node in the overlay matches
the name of a symbol (a label) in the live tree. The values of each
property in the “\_\_fixups\_\_” node is a list of the property values in the
overlay that need to be updated to contain the phandle reference
corresponding to that symbol in the live tree. Update the references in
the overlay with the phandle values in the live tree.

**overlay** must be detached.

Resolving and applying **overlay** to the live expanded devicetree must be
protected by a mechanism to ensure that multiple overlays are processed
in a single threaded manner so that multiple overlays will not relocate
phandles to overlapping ranges. The mechanism to enforce this is not
yet implemented.

**Return**

`0` on success or a negative error value on error.

struct device\_node \*of\_node\_get(struct device\_node \*node)
:   Increment refcount of a node

**Parameters**

`struct device_node *node`
:   Node to inc refcount, NULL is supported to simplify writing of
    callers

**Return**

The node with refcount incremented.

void of\_node\_put(struct device\_node \*node)
:   Decrement refcount of a node

**Parameters**

`struct device_node *node`
:   Node to dec refcount, NULL is supported to simplify writing of
    callers

int of\_detach\_node(struct device\_node \*np)
:   “Unplug” a node from the device tree.

**Parameters**

`struct device_node *np`
:   Pointer to the caller’s Device Node

struct device\_node \*of\_changeset\_create\_node(struct [of\_changeset](#c.of_changeset "of_changeset") \*ocs, struct device\_node \*parent, const char \*full\_name)
:   Dynamically create a device node and attach to a given changeset.

**Parameters**

`struct of_changeset *ocs`
:   Pointer to changeset

`struct device_node *parent`
:   Pointer to parent device node

`const char *full_name`
:   Node full name

**Return**

Pointer to the created device node or NULL in case of an error.

void of\_changeset\_init(struct [of\_changeset](#c.of_changeset "of_changeset") \*ocs)
:   Initialize a changeset for use

**Parameters**

`struct of_changeset *ocs`
:   changeset pointer

**Description**

Initialize a changeset structure

void of\_changeset\_destroy(struct [of\_changeset](#c.of_changeset "of_changeset") \*ocs)
:   Destroy a changeset

**Parameters**

`struct of_changeset *ocs`
:   changeset pointer

**Description**

Destroys a changeset. Note that if a changeset is applied,
its changes to the tree cannot be reverted.

int of\_changeset\_apply(struct [of\_changeset](#c.of_changeset "of_changeset") \*ocs)
:   Applies a changeset

**Parameters**

`struct of_changeset *ocs`
:   changeset pointer

**Description**

Applies a changeset to the live tree.
Any side-effects of live tree state changes are applied here on
success, like creation/destruction of devices and side-effects
like creation of sysfs properties and directories.

**Return**

0 on success, a negative error value in case of an error.
On error the partially applied effects are reverted.

int of\_changeset\_revert(struct [of\_changeset](#c.of_changeset "of_changeset") \*ocs)
:   Reverts an applied changeset

**Parameters**

`struct of_changeset *ocs`
:   changeset pointer

**Description**

Reverts a changeset returning the state of the tree to what it
was before the application.
Any side-effects like creation/destruction of devices and
removal of sysfs properties and directories are applied.

**Return**

0 on success, a negative error value in case of an error.

int of\_changeset\_action(struct [of\_changeset](#c.of_changeset "of_changeset") \*ocs, unsigned long action, struct device\_node \*np, struct property \*prop)
:   Add an action to the tail of the changeset list

**Parameters**

`struct of_changeset *ocs`
:   changeset pointer

`unsigned long action`
:   action to perform

`struct device_node *np`
:   Pointer to device node

`struct property *prop`
:   Pointer to property

**Description**

On action being one of:
+ OF\_RECONFIG\_ATTACH\_NODE
+ OF\_RECONFIG\_DETACH\_NODE,
+ OF\_RECONFIG\_ADD\_PROPERTY
+ OF\_RECONFIG\_REMOVE\_PROPERTY,
+ OF\_RECONFIG\_UPDATE\_PROPERTY

**Return**

0 on success, a negative error value in case of an error.

int of\_changeset\_add\_prop\_string(struct [of\_changeset](#c.of_changeset "of_changeset") \*ocs, struct device\_node \*np, const char \*prop\_name, const char \*str)
:   Add a string property to a changeset

**Parameters**

`struct of_changeset *ocs`
:   changeset pointer

`struct device_node *np`
:   device node pointer

`const char *prop_name`
:   name of the property to be added

`const char *str`
:   pointer to null terminated string

**Description**

Create a string property and add it to a changeset.

**Return**

0 on success, a negative error value in case of an error.

int of\_changeset\_add\_prop\_string\_array(struct [of\_changeset](#c.of_changeset "of_changeset") \*ocs, struct device\_node \*np, const char \*prop\_name, const char \*const \*str\_array, size\_t sz)
:   Add a string list property to a changeset

**Parameters**

`struct of_changeset *ocs`
:   changeset pointer

`struct device_node *np`
:   device node pointer

`const char *prop_name`
:   name of the property to be added

`const char * const *str_array`
:   pointer to an array of null terminated strings

`size_t sz`
:   number of string array elements

**Description**

Create a string list property and add it to a changeset.

**Return**

0 on success, a negative error value in case of an error.

int of\_changeset\_add\_prop\_u32\_array(struct [of\_changeset](#c.of_changeset "of_changeset") \*ocs, struct device\_node \*np, const char \*prop\_name, const u32 \*array, size\_t sz)
:   Add a property of 32 bit integers property to a changeset

**Parameters**

`struct of_changeset *ocs`
:   changeset pointer

`struct device_node *np`
:   device node pointer

`const char *prop_name`
:   name of the property to be added

`const u32 *array`
:   pointer to an array of 32 bit integers

`size_t sz`
:   number of array elements

**Description**

Create a property of 32 bit integers and add it to a changeset.

**Return**

0 on success, a negative error value in case of an error.

int of\_changeset\_add\_prop\_bool(struct [of\_changeset](#c.of_changeset "of_changeset") \*ocs, struct device\_node \*np, const char \*prop\_name)
:   Add a boolean property (i.e. a property without any values) to a changeset.

**Parameters**

`struct of_changeset *ocs`
:   changeset pointer

`struct device_node *np`
:   device node pointer

`const char *prop_name`
:   name of the property to be added

**Description**

Create a boolean property and add it to a changeset.

**Return**

0 on success, a negative error value in case of an error.

int of\_changeset\_update\_prop\_string(struct [of\_changeset](#c.of_changeset "of_changeset") \*ocs, struct device\_node \*np, const char \*prop\_name, const char \*str)
:   Add a string property update to a changeset

**Parameters**

`struct of_changeset *ocs`
:   changeset pointer

`struct device_node *np`
:   device node pointer

`const char *prop_name`
:   name of the property to be updated

`const char *str`
:   pointer to null terminated string

**Description**

Create a string property to be updated and add it to a changeset.

**Return**

0 on success, a negative error value in case of an error.

int of\_overlay\_notifier\_register(struct notifier\_block \*nb)
:   Register notifier for overlay operations

**Parameters**

`struct notifier_block *nb`
:   Notifier block to register

**Description**

Register for notification on overlay operations on device tree nodes. The
reported actions defined by **of\_reconfig\_change**. The notifier callback
furthermore receives a pointer to the affected device tree node.

Note that a notifier callback is not supposed to store pointers to a device
tree node or its content beyond **OF\_OVERLAY\_POST\_REMOVE** corresponding to the
respective node it received.

int of\_overlay\_notifier\_unregister(struct notifier\_block \*nb)
:   Unregister notifier for overlay operations

**Parameters**

`struct notifier_block *nb`
:   Notifier block to unregister

int of\_overlay\_fdt\_apply(const void \*overlay\_fdt, u32 overlay\_fdt\_size, int \*ret\_ovcs\_id, const struct device\_node \*base)
:   Create and apply an overlay changeset

**Parameters**

`const void *overlay_fdt`
:   pointer to overlay FDT

`u32 overlay_fdt_size`
:   number of bytes in **overlay\_fdt**

`int *ret_ovcs_id`
:   pointer for returning created changeset id

`const struct device_node *base`
:   pointer for the target node to apply overlay

**Description**

Creates and applies an overlay changeset.

See `of_overlay_apply()` for important behavior information.

On error return, the changeset may be partially applied. This is especially
likely if an OF\_OVERLAY\_POST\_APPLY notifier returns an error. In this case
the caller should call [`of_overlay_remove()`](#c.of_overlay_remove "of_overlay_remove") with the value in **\*ret\_ovcs\_id**.

**Return**

0 on success, or a negative error number. **\*ret\_ovcs\_id** is set to
the value of overlay changeset id, which can be passed to [`of_overlay_remove()`](#c.of_overlay_remove "of_overlay_remove")
to remove the overlay.

int of\_overlay\_remove(int \*ovcs\_id)
:   Revert and free an overlay changeset

**Parameters**

`int *ovcs_id`
:   Pointer to overlay changeset id

**Description**

Removes an overlay if it is permissible. **ovcs\_id** was previously returned
by [`of_overlay_fdt_apply()`](#c.of_overlay_fdt_apply "of_overlay_fdt_apply").

If an error occurred while attempting to revert the overlay changeset,
then an attempt is made to re-apply any changeset entry that was
reverted. If an error occurs on re-apply then the state of the device
tree can not be determined, and any following attempt to apply or remove
an overlay changeset will be refused.

A non-zero return value will not revert the changeset if error is from:
:   * parameter checks
    * overlay changeset pre-remove notifier
    * overlay changeset entry revert

If an error is returned by an overlay changeset pre-remove notifier
then no further overlay changeset pre-remove notifier will be called.

If more than one notifier returns an error, then the last notifier
error to occur is returned.

A non-zero return value will revert the changeset if error is from:
:   * overlay changeset entry notifier
    * overlay changeset post-remove notifier

If an error is returned by an overlay changeset post-remove notifier
then no further overlay changeset post-remove notifier will be called.

**Return**

0 on success, or a negative error number. **\*ovcs\_id** is set to
zero after reverting the changeset, even if a subsequent error occurs.

int of\_overlay\_remove\_all(void)
:   Reverts and frees all overlay changesets

**Parameters**

`void`
:   no arguments

**Description**

Removes all overlays from the system in the correct order.

**Return**

0 on success, or a negative error number
