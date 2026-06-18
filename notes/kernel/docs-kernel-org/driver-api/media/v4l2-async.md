# 3.22.V4L2 async kAPI

> 출처(원문): https://docs.kernel.org/driver-api/media/v4l2-async.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.22. V4L2 async kAPI

enum v4l2\_async\_match\_type
:   type of asynchronous subdevice logic to be used in order to identify a match

**Constants**

`V4L2_ASYNC_MATCH_TYPE_I2C`
:   Match will check for I2C adapter ID and address

`V4L2_ASYNC_MATCH_TYPE_FWNODE`
:   Match will use firmware node

**Description**

This `enum is` used by the asynchronous connection logic to define the
algorithm that will be used to match an asynchronous device.

struct v4l2\_async\_match\_desc
:   async connection match information

**Definition**:

```
struct v4l2_async_match_desc {
    enum v4l2_async_match_type type;
    union {
        struct fwnode_handle *fwnode;
        struct {
            int adapter_id;
            unsigned short address;
        } i2c;
    };
};
```

**Members**

`type`
:   type of match that will be used

`{unnamed_union}`
:   anonymous

`fwnode`
:   pointer to `struct fwnode_handle` to be matched.
    Used if **match\_type** is `V4L2_ASYNC_MATCH_TYPE_FWNODE`.

`i2c`
:   embedded `struct with` I2C parameters to be matched.
    Both **match.i2c.adapter\_id** and **match.i2c.address**
    should be matched.
    Used if **match\_type** is `V4L2_ASYNC_MATCH_TYPE_I2C`.

`i2c.adapter_id`
:   I2C adapter ID to be matched.
    Used if **match\_type** is `V4L2_ASYNC_MATCH_TYPE_I2C`.

`i2c.address`
:   I2C address to be matched.
    Used if **match\_type** is `V4L2_ASYNC_MATCH_TYPE_I2C`.

struct v4l2\_async\_connection
:   sub-device connection descriptor, as known to a bridge

**Definition**:

```
struct v4l2_async_connection {
    struct v4l2_async_match_desc match;
    struct v4l2_async_notifier *notifier;
    struct list_head asc_entry;
    struct list_head asc_subdev_entry;
    struct v4l2_subdev *sd;
};
```

**Members**

`match`
:   `struct of` match type and per-bus type matching data sets

`notifier`
:   the async notifier the connection is related to

`asc_entry`
:   used to add [`struct v4l2_async_connection`](#c.v4l2_async_connection "v4l2_async_connection") objects to the
    notifier **waiting\_list** or **done\_list**

`asc_subdev_entry`
:   entry in `struct v4l2_async_subdev`.asc\_list list

`sd`
:   the related sub-device

**Description**

When this `struct is` used as a member in a driver specific struct, the driver
specific `struct shall` contain the [`struct v4l2_async_connection`](#c.v4l2_async_connection "v4l2_async_connection") as its first
member.

struct v4l2\_async\_notifier\_operations
:   Asynchronous V4L2 notifier operations

**Definition**:

```
struct v4l2_async_notifier_operations {
    int (*bound)(struct v4l2_async_notifier *notifier, struct v4l2_subdev *subdev, struct v4l2_async_connection *asc);
    int (*complete)(struct v4l2_async_notifier *notifier);
    void (*unbind)(struct v4l2_async_notifier *notifier, struct v4l2_subdev *subdev, struct v4l2_async_connection *asc);
    void (*destroy)(struct v4l2_async_connection *asc);
};
```

**Members**

`bound`
:   a sub-device has been bound by the given connection

`complete`
:   All connections have been bound successfully. The complete
    callback is only executed for the root notifier.

`unbind`
:   a subdevice is leaving

`destroy`
:   the asc is about to be freed

struct v4l2\_async\_notifier
:   v4l2\_device notifier data

**Definition**:

```
struct v4l2_async_notifier {
    const struct v4l2_async_notifier_operations *ops;
    struct v4l2_device *v4l2_dev;
    struct v4l2_subdev *sd;
    struct v4l2_async_notifier *parent;
    struct list_head waiting_list;
    struct list_head done_list;
    struct list_head notifier_entry;
};
```

**Members**

`ops`
:   notifier operations

`v4l2_dev`
:   v4l2\_device of the root notifier, NULL otherwise

`sd`
:   sub-device that registered the notifier, NULL otherwise

`parent`
:   parent notifier

`waiting_list`
:   list of [`struct v4l2_async_connection`](#c.v4l2_async_connection "v4l2_async_connection"), waiting for their
    drivers

`done_list`
:   list of [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev"), already probed

`notifier_entry`
:   member in a global list of notifiers

struct v4l2\_async\_subdev\_endpoint
:   Entry in sub-device’s fwnode list

**Definition**:

```
struct v4l2_async_subdev_endpoint {
    struct list_head async_subdev_endpoint_entry;
    struct fwnode_handle *endpoint;
};
```

**Members**

`async_subdev_endpoint_entry`
:   An entry in async\_subdev\_endpoint\_list of
    [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")

`endpoint`
:   Endpoint fwnode agains which to match the sub-device

void v4l2\_async\_debug\_init(struct dentry \*debugfs\_dir)
:   Initialize debugging tools.

**Parameters**

`struct dentry *debugfs_dir`
:   pointer to the parent debugfs `struct dentry`

void v4l2\_async\_nf\_init(struct [v4l2\_async\_notifier](#c.v4l2_async_notifier "v4l2_async_notifier") \*notifier, struct [v4l2\_device](v4l2-device.html#c.v4l2_device "v4l2_device") \*v4l2\_dev)
:   Initialize a notifier.

**Parameters**

`struct v4l2_async_notifier *notifier`
:   pointer to [`struct v4l2_async_notifier`](#c.v4l2_async_notifier "v4l2_async_notifier")

`struct v4l2_device *v4l2_dev`
:   pointer to [`struct v4l2_device`](v4l2-device.html#c.v4l2_device "v4l2_device")

**Description**

This function initializes the notifier **asc\_entry**. It must be called
before adding a subdevice to a notifier, using one of:
[`v4l2_async_nf_add_fwnode_remote()`](#c.v4l2_async_nf_add_fwnode_remote "v4l2_async_nf_add_fwnode_remote"),
[`v4l2_async_nf_add_fwnode()`](#c.v4l2_async_nf_add_fwnode "v4l2_async_nf_add_fwnode") or
[`v4l2_async_nf_add_i2c()`](#c.v4l2_async_nf_add_i2c "v4l2_async_nf_add_i2c").

void v4l2\_async\_subdev\_nf\_init(struct [v4l2\_async\_notifier](#c.v4l2_async_notifier "v4l2_async_notifier") \*notifier, struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd)
:   Initialize a sub-device notifier.

**Parameters**

`struct v4l2_async_notifier *notifier`
:   pointer to [`struct v4l2_async_notifier`](#c.v4l2_async_notifier "v4l2_async_notifier")

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")

**Description**

This function initializes the notifier **asc\_list**. It must be called
before adding a subdevice to a notifier, using one of:
[`v4l2_async_nf_add_fwnode_remote()`](#c.v4l2_async_nf_add_fwnode_remote "v4l2_async_nf_add_fwnode_remote"), [`v4l2_async_nf_add_fwnode()`](#c.v4l2_async_nf_add_fwnode "v4l2_async_nf_add_fwnode") or
[`v4l2_async_nf_add_i2c()`](#c.v4l2_async_nf_add_i2c "v4l2_async_nf_add_i2c").

v4l2\_async\_nf\_add\_fwnode

`v4l2_async_nf_add_fwnode (notifier, fwnode, type)`

> Allocate and add a fwnode async subdev to the notifier’s master asc\_list.

**Parameters**

`notifier`
:   pointer to [`struct v4l2_async_notifier`](#c.v4l2_async_notifier "v4l2_async_notifier")

`fwnode`
:   fwnode handle of the sub-device to be matched, pointer to
    `struct fwnode_handle`

`type`
:   Type of the driver’s async sub-device or connection struct. The
    [`struct v4l2_async_connection`](#c.v4l2_async_connection "v4l2_async_connection") shall be the first member of the
    driver’s async struct, i.e. both begin at the same memory address.

**Description**

Allocate a fwnode-matched asc of size asc\_struct\_size, and add it to the
notifiers **asc\_list**. The function also gets a reference of the fwnode which
is released later at notifier cleanup time.

v4l2\_async\_nf\_add\_fwnode\_remote

`v4l2_async_nf_add_fwnode_remote (notifier, ep, type)`

> Allocate and add a fwnode remote async subdev to the notifier’s master asc\_list.

**Parameters**

`notifier`
:   pointer to [`struct v4l2_async_notifier`](#c.v4l2_async_notifier "v4l2_async_notifier")

`ep`
:   local endpoint pointing to the remote connection to be matched,
    pointer to `struct fwnode_handle`

`type`
:   Type of the driver’s async connection struct. The [`struct
    v4l2_async_connection`](#c.v4l2_async_connection "v4l2_async_connection") shall be the first member of the driver’s async
    connection struct, i.e. both begin at the same memory address.

**Description**

Gets the remote endpoint of a given local endpoint, set it up for fwnode
matching and adds the async connection to the notifier’s **asc\_list**. The
function also gets a reference of the fwnode which is released later at
notifier cleanup time.

This is just like [`v4l2_async_nf_add_fwnode()`](#c.v4l2_async_nf_add_fwnode "v4l2_async_nf_add_fwnode"), but with the
exception that the fwnode refers to a local endpoint, not the remote one.

v4l2\_async\_nf\_add\_i2c

`v4l2_async_nf_add_i2c (notifier, adapter, address, type)`

> Allocate and add an i2c async subdev to the notifier’s master asc\_list.

**Parameters**

`notifier`
:   pointer to [`struct v4l2_async_notifier`](#c.v4l2_async_notifier "v4l2_async_notifier")

`adapter`
:   I2C adapter ID to be matched

`address`
:   I2C address of connection to be matched

`type`
:   Type of the driver’s async connection struct. The [`struct
    v4l2_async_connection`](#c.v4l2_async_connection "v4l2_async_connection") shall be the first member of the driver’s async
    connection struct, i.e. both begin at the same memory address.

**Description**

Same as [`v4l2_async_nf_add_fwnode()`](#c.v4l2_async_nf_add_fwnode "v4l2_async_nf_add_fwnode") but for I2C matched
connections.

int v4l2\_async\_subdev\_endpoint\_add(struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd, struct fwnode\_handle \*fwnode)
:   Add an endpoint fwnode to async sub-device matching list

**Parameters**

`struct v4l2_subdev *sd`
:   the sub-device

`struct fwnode_handle *fwnode`
:   the endpoint fwnode to match

**Description**

Add a fwnode to the async sub-device’s matching list. This allows registering
multiple async sub-devices from a single device.

Note that calling [`v4l2_subdev_cleanup()`](v4l2-subdev.html#c.v4l2_subdev_cleanup "v4l2_subdev_cleanup") as part of the sub-device’s cleanup
if endpoints have been added to the sub-device’s fwnode matching list.

Returns an error on failure, 0 on success.

struct [v4l2\_async\_connection](#c.v4l2_async_connection "v4l2_async_connection") \*v4l2\_async\_connection\_unique(struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd)
:   return a unique [`struct v4l2_async_connection`](#c.v4l2_async_connection "v4l2_async_connection") for a sub-device

**Parameters**

`struct v4l2_subdev *sd`
:   the sub-device

**Description**

Return an async connection for a sub-device, when there is a single
one only.

int v4l2\_async\_nf\_register(struct [v4l2\_async\_notifier](#c.v4l2_async_notifier "v4l2_async_notifier") \*notifier)
:   registers a subdevice asynchronous notifier

**Parameters**

`struct v4l2_async_notifier *notifier`
:   pointer to [`struct v4l2_async_notifier`](#c.v4l2_async_notifier "v4l2_async_notifier")

void v4l2\_async\_nf\_unregister(struct [v4l2\_async\_notifier](#c.v4l2_async_notifier "v4l2_async_notifier") \*notifier)
:   unregisters a subdevice asynchronous notifier

**Parameters**

`struct v4l2_async_notifier *notifier`
:   pointer to [`struct v4l2_async_notifier`](#c.v4l2_async_notifier "v4l2_async_notifier")

void v4l2\_async\_nf\_cleanup(struct [v4l2\_async\_notifier](#c.v4l2_async_notifier "v4l2_async_notifier") \*notifier)
:   clean up notifier resources

**Parameters**

`struct v4l2_async_notifier *notifier`
:   the notifier the resources of which are to be cleaned up

**Description**

Release memory resources related to a notifier, including the async
connections allocated for the purposes of the notifier but not the notifier
itself. The user is responsible for calling this function to clean up the
notifier after calling [`v4l2_async_nf_add_fwnode_remote()`](#c.v4l2_async_nf_add_fwnode_remote "v4l2_async_nf_add_fwnode_remote"),
[`v4l2_async_nf_add_fwnode()`](#c.v4l2_async_nf_add_fwnode "v4l2_async_nf_add_fwnode") or [`v4l2_async_nf_add_i2c()`](#c.v4l2_async_nf_add_i2c "v4l2_async_nf_add_i2c").

There is no harm from calling [`v4l2_async_nf_cleanup()`](#c.v4l2_async_nf_cleanup "v4l2_async_nf_cleanup") in other
cases as long as its memory has been zeroed after it has been
allocated.

v4l2\_async\_register\_subdev

`v4l2_async_register_subdev (sd)`

> registers a sub-device to the asynchronous subdevice framework

**Parameters**

`sd`
:   pointer to [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")

int v4l2\_async\_register\_subdev\_sensor(struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd)
:   registers a sensor sub-device to the asynchronous sub-device framework and parse set up common sensor related devices

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to struct [`v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")

**Description**

This function is just like [`v4l2_async_register_subdev()`](#c.v4l2_async_register_subdev "v4l2_async_register_subdev") with the exception
that calling it will also parse firmware interfaces for remote references
using `v4l2_async_nf_parse_fwnode_sensor()` and registers the
async sub-devices. The sub-device is similarly unregistered by calling
[`v4l2_async_unregister_subdev()`](#c.v4l2_async_unregister_subdev "v4l2_async_unregister_subdev").

While registered, the subdev module is marked as in-use.

An error is returned if the module is no longer loaded on any attempts
to register it.

void v4l2\_async\_unregister\_subdev(struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd)
:   unregisters a sub-device to the asynchronous subdevice framework

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")
