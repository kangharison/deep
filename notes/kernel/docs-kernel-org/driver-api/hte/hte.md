# The Linux Hardware Timestamping Engine (HTE)

> 출처(원문): https://docs.kernel.org/driver-api/hte/hte.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# The Linux Hardware Timestamping Engine (HTE)

Author:
:   Dipen Patel

## Introduction

Certain devices have built in hardware timestamping engines which can
monitor sets of system signals, lines, buses etc... in realtime for state
change; upon detecting the change they can automatically store the timestamp at
the moment of occurrence. Such functionality may help achieve better accuracy
in obtaining timestamps than using software counterparts i.e. ktime and
friends.

This document describes the API that can be used by hardware timestamping
engine provider and consumer drivers that want to use the hardware timestamping
engine (HTE) framework. Both consumers and providers must include
`#include <linux/hte.h>`.

## The HTE framework APIs for the providers

int hte\_push\_ts\_ns(const struct [hte\_chip](#c.hte_chip "hte_chip") \*chip, u32 xlated\_id, struct [hte\_ts\_data](#c.hte_ts_data "hte_ts_data") \*data)
:   Push timestamp data in nanoseconds.

**Parameters**

`const struct hte_chip *chip`
:   The HTE chip, used during the registration.

`u32 xlated_id`
:   entity id understood by both subsystem and provider, this is
    obtained from xlate callback during request API.

`struct hte_ts_data *data`
:   timestamp data.

**Description**

It is used by the provider to push timestamp data.

**Return**

0 on success or a negative error code on failure.

int devm\_hte\_register\_chip(struct [hte\_chip](#c.hte_chip "hte_chip") \*chip)
:   Resource managed API to register HTE chip.

**Parameters**

`struct hte_chip *chip`
:   the HTE chip to add to subsystem.

**Description**

It is used by the provider to register itself with the HTE subsystem.
The unregistration is done automatically when the provider exits.

**Return**

0 on success or a negative error code on failure.

## The HTE framework APIs for the consumers

int hte\_ts\_put(struct [hte\_ts\_desc](#c.hte_ts_desc "hte_ts_desc") \*desc)
:   Release and disable timestamp for the given desc.

**Parameters**

`struct hte_ts_desc *desc`
:   timestamp descriptor.

**Context**

`debugfs_remove_recursive()` function call may use sleeping locks,
not suitable from atomic context.

**Return**

0 on success or a negative error code on failure.

int hte\_disable\_ts(struct [hte\_ts\_desc](#c.hte_ts_desc "hte_ts_desc") \*desc)
:   Disable timestamp on given descriptor.

**Parameters**

`struct hte_ts_desc *desc`
:   ts descriptor, this is the same as returned by the request API.

**Description**

The API does not release any resources associated with desc.

**Context**

Holds mutex lock, not suitable from atomic context.

**Return**

0 on success or a negative error code on failure.

int hte\_enable\_ts(struct [hte\_ts\_desc](#c.hte_ts_desc "hte_ts_desc") \*desc)
:   Enable timestamp on given descriptor.

**Parameters**

`struct hte_ts_desc *desc`
:   ts descriptor, this is the same as returned by the request API.

**Context**

Holds mutex lock, not suitable from atomic context.

**Return**

0 on success or a negative error code on failure.

int of\_hte\_req\_count(struct [device](../infrastructure.html#c.device "device") \*dev)
:   Return the number of entities to timestamp.

**Parameters**

`struct device *dev`
:   The HTE consumer.

**Description**

The function returns the total count of the requested entities to timestamp
by parsing device tree.

**Return**

Positive number on success, -ENOENT if no entries,
-EINVAL for other errors.

int hte\_ts\_get(struct [device](../infrastructure.html#c.device "device") \*dev, struct [hte\_ts\_desc](#c.hte_ts_desc "hte_ts_desc") \*desc, int index)
:   The function to initialize and obtain HTE desc.

**Parameters**

`struct device *dev`
:   HTE consumer/client device, used in case of parsing device tree node.

`struct hte_ts_desc *desc`
:   Pre-allocated timestamp descriptor.

`int index`
:   The index will be used as an index to parse line\_id from the
    device tree node if node is present.

**Description**

The function initializes the consumer provided HTE descriptor. If consumer
has device tree node, index is used to parse the line id and other details.
The function needs to be called before using any request APIs.

**Context**

Holds mutex lock.

**Return**

Returns 0 on success or negative error code on failure.

int hte\_request\_ts\_ns(struct [hte\_ts\_desc](#c.hte_ts_desc "hte_ts_desc") \*desc, [hte\_ts\_cb\_t](#c.hte_ts_cb_t "hte_ts_cb_t") cb, [hte\_ts\_sec\_cb\_t](#c.hte_ts_sec_cb_t "hte_ts_sec_cb_t") tcb, void \*data)
:   The API to request and enable hardware timestamp in nanoseconds.

**Parameters**

`struct hte_ts_desc *desc`
:   Pre-allocated and initialized timestamp descriptor.

`hte_ts_cb_t cb`
:   Callback to push the timestamp data to consumer.

`hte_ts_sec_cb_t tcb`
:   Optional callback. If its provided, subsystem initializes
    workqueue. It is called when cb returns HTE\_RUN\_SECOND\_CB.

`void *data`
:   Client data, used during cb and tcb callbacks.

**Description**

The entity is provider specific for example, GPIO lines, signals, buses
etc...The API allocates necessary resources and enables the timestamp.

**Context**

Holds mutex lock.

**Return**

Returns 0 on success or negative error code on failure.

int devm\_hte\_request\_ts\_ns(struct [device](../infrastructure.html#c.device "device") \*dev, struct [hte\_ts\_desc](#c.hte_ts_desc "hte_ts_desc") \*desc, [hte\_ts\_cb\_t](#c.hte_ts_cb_t "hte_ts_cb_t") cb, [hte\_ts\_sec\_cb\_t](#c.hte_ts_sec_cb_t "hte_ts_sec_cb_t") tcb, void \*data)
:   Resource managed API to request and enable hardware timestamp in nanoseconds.

**Parameters**

`struct device *dev`
:   HTE consumer/client device.

`struct hte_ts_desc *desc`
:   Pre-allocated and initialized timestamp descriptor.

`hte_ts_cb_t cb`
:   Callback to push the timestamp data to consumer.

`hte_ts_sec_cb_t tcb`
:   Optional callback. If its provided, subsystem initializes
    workqueue. It is called when cb returns HTE\_RUN\_SECOND\_CB.

`void *data`
:   Client data, used during cb and tcb callbacks.

**Description**

The entity is provider specific for example, GPIO lines, signals, buses
etc...The API allocates necessary resources and enables the timestamp. It
deallocates and disables automatically when the consumer exits.

**Context**

Holds mutex lock.

**Return**

Returns 0 on success or negative error code on failure.

int hte\_init\_line\_attr(struct [hte\_ts\_desc](#c.hte_ts_desc "hte_ts_desc") \*desc, u32 line\_id, unsigned long edge\_flags, const char \*name, void \*data)
:   Initialize line attributes.

**Parameters**

`struct hte_ts_desc *desc`
:   Pre-allocated timestamp descriptor.

`u32 line_id`
:   line id.

`unsigned long edge_flags`
:   edge flags related to line\_id.

`const char *name`
:   name of the line.

`void *data`
:   line data related to line\_id.

**Description**

Zeroes out line attributes and initializes with provided arguments.
The function needs to be called before calling any consumer facing
functions.

**Context**

Any.

**Return**

0 on success or negative error code for the failure.

int hte\_get\_clk\_src\_info(const struct [hte\_ts\_desc](#c.hte_ts_desc "hte_ts_desc") \*desc, struct [hte\_clk\_info](#c.hte_clk_info "hte_clk_info") \*ci)
:   Get the clock source information for a ts descriptor.

**Parameters**

`const struct hte_ts_desc *desc`
:   ts descriptor, same as returned from request API.

`struct hte_clk_info *ci`
:   The API fills this structure with the clock information data.

**Context**

Any context.

**Return**

0 on success else negative error code on failure.

## The HTE framework public structures

enum hte\_edge
:   HTE line edge flags.

**Constants**

`HTE_EDGE_NO_SETUP`
:   No edge setup. In this case consumer will setup edges,
    for example during request irq call.

`HTE_RISING_EDGE_TS`
:   Rising edge.

`HTE_FALLING_EDGE_TS`
:   Falling edge.

enum hte\_return
:   HTE subsystem return values used during callback.

**Constants**

`HTE_CB_HANDLED`
:   The consumer handled the data.

`HTE_RUN_SECOND_CB`
:   The consumer needs further processing, in that case
    HTE subsystem calls secondary callback provided by the consumer where it
    is allowed to sleep.

struct hte\_ts\_data
:   HTE timestamp data.

**Definition**:

```
struct hte_ts_data {
    u64 tsc;
    u64 seq;
    int raw_level;
};
```

**Members**

`tsc`
:   Timestamp value.

`seq`
:   Sequence counter of the timestamps.

`raw_level`
:   Level of the line at the timestamp if provider supports it,
    -1 otherwise.

struct hte\_clk\_info
:   Clock source info that HTE provider uses to timestamp.

**Definition**:

```
struct hte_clk_info {
    u64 hz;
    clockid_t type;
};
```

**Members**

`hz`
:   Supported clock rate in HZ, for example 1KHz clock = 1000.

`type`
:   Supported clock type.

hte\_ts\_cb\_t
:   **Typedef**: HTE timestamp data processing primary callback.

**Syntax**

> `enum hte_return hte_ts_cb_t (struct hte_ts_data *ts, void *data)`

**Parameters**

`struct hte_ts_data *ts`
:   HW timestamp data.

`void *data`
:   Client supplied data.

**Description**

The callback is used to push timestamp data to the client and it is
not allowed to sleep.

hte\_ts\_sec\_cb\_t
:   **Typedef**: HTE timestamp data processing secondary callback.

**Syntax**

> `enum hte_return hte_ts_sec_cb_t (void *data)`

**Parameters**

`void *data`
:   Client supplied data.

**Description**

This is used when the client needs further processing where it is
allowed to sleep.

struct hte\_line\_attr
:   Line attributes.

**Definition**:

```
struct hte_line_attr {
    u32 line_id;
    void *line_data;
    unsigned long edge_flags;
    const char *name;
};
```

**Members**

`line_id`
:   The logical ID understood by the consumers and providers.

`line_data`
:   Line data related to line\_id.

`edge_flags`
:   Edge setup flags.

`name`
:   Descriptive name of the entity that is being monitored for the
    hardware timestamping. If null, HTE core will construct the name.

struct hte\_ts\_desc
:   HTE timestamp descriptor.

**Definition**:

```
struct hte_ts_desc {
    struct hte_line_attr attr;
    void *hte_data;
};
```

**Members**

`attr`
:   The line attributes.

`hte_data`
:   Subsystem’s private data, set by HTE subsystem.

**Description**

This structure is a communication token between consumers to subsystem
and subsystem to providers.

struct hte\_ops
:   HTE operations set by providers.

**Definition**:

```
struct hte_ops {
    int (*request)(struct hte_chip *chip, struct hte_ts_desc *desc, u32 xlated_id);
    int (*release)(struct hte_chip *chip, struct hte_ts_desc *desc, u32 xlated_id);
    int (*enable)(struct hte_chip *chip, u32 xlated_id);
    int (*disable)(struct hte_chip *chip, u32 xlated_id);
    int (*get_clk_src_info)(struct hte_chip *chip, struct hte_clk_info *ci);
};
```

**Members**

`request`
:   Hook for requesting a HTE timestamp. Returns 0 on success,
    non-zero for failures.

`release`
:   Hook for releasing a HTE timestamp. Returns 0 on success,
    non-zero for failures.

`enable`
:   Hook to enable the specified timestamp. Returns 0 on success,
    non-zero for failures.

`disable`
:   Hook to disable specified timestamp. Returns 0 on success,
    non-zero for failures.

`get_clk_src_info`
:   Hook to get the clock information the provider uses
    to timestamp. Returns 0 for success and negative error code for failure. On
    success HTE subsystem fills up provided [`struct hte_clk_info`](#c.hte_clk_info "hte_clk_info").

**Description**

xlated\_id parameter is used to communicate between HTE subsystem and the
providers and is translated by the provider.

struct hte\_chip
:   Abstract HTE chip.

**Definition**:

```
struct hte_chip {
    const char *name;
    struct device *dev;
    const struct hte_ops *ops;
    u32 nlines;
    int (*xlate_of)(struct hte_chip *gc, const struct of_phandle_args *args, struct hte_ts_desc *desc, u32 *xlated_id);
    int (*xlate_plat)(struct hte_chip *gc, struct hte_ts_desc *desc, u32 *xlated_id);
    bool (*match_from_linedata)(const struct hte_chip *chip, const struct hte_ts_desc *hdesc);
    u8 of_hte_n_cells;
    struct hte_device *gdev;
    void *data;
};
```

**Members**

`name`
:   functional name of the HTE IP block.

`dev`
:   device providing the HTE.

`ops`
:   callbacks for this HTE.

`nlines`
:   number of lines/signals supported by this chip.

`xlate_of`
:   Callback which translates consumer supplied logical ids to
    physical ids, return 0 for the success and negative for the failures.
    It stores (between 0 to **nlines**) in xlated\_id parameter for the success.

`xlate_plat`
:   Same as above but for the consumers with no DT node.

`match_from_linedata`
:   Match HTE device using the line\_data.

`of_hte_n_cells`
:   Number of cells used to form the HTE specifier.

`gdev`
:   HTE subsystem abstract device, internal to the HTE subsystem.

`data`
:   chip specific private data.

## More on the HTE timestamp data

The `struct hte_ts_data` is used to pass timestamp details between the
consumers and the providers. It expresses timestamp data in nanoseconds in
u64. An example of the typical timestamp data life cycle, for the GPIO line is
as follows:

```
- Monitors GPIO line change.
- Detects the state change on GPIO line.
- Converts timestamps in nanoseconds.
- Stores GPIO raw level in raw_level variable if the provider has that
hardware capability.
- Pushes this hte_ts_data object to HTE subsystem.
- HTE subsystem increments seq counter and invokes consumer provided callback.
Based on callback return value, the HTE core invokes secondary callback in
the thread context.
```

## HTE subsystem debugfs attributes

HTE subsystem creates debugfs attributes at `/sys/kernel/debug/hte/`.
It also creates line/signal-related debugfs attributes at
`/sys/kernel/debug/hte/<provider>/<label or line id>/`. Note that these
attributes are read-only.

ts\_requested
:   The total number of entities requested from the given provider,
    where entity is specified by the provider and could represent
    lines, GPIO, chip signals, buses etc...
    The attribute will be available at
    `/sys/kernel/debug/hte/<provider>/`.

total\_ts
:   The total number of entities supported by the provider.
    The attribute will be available at
    `/sys/kernel/debug/hte/<provider>/`.

dropped\_timestamps
:   The dropped timestamps for a given line.
    The attribute will be available at
    `/sys/kernel/debug/hte/<provider>/<label or line id>/`.
