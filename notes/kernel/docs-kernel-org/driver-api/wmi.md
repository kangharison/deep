# WMI Driver API

> 출처(원문): https://docs.kernel.org/driver-api/wmi.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# WMI Driver API

The WMI driver core supports a more modern bus-based interface for interacting
with WMI devices, and an older GUID-based interface. The latter interface is
considered to be deprecated, so new WMI drivers should generally avoid it since
it has some issues with multiple WMI devices sharing the same GUID.
The modern bus-based interface instead maps each WMI device to a
[`struct wmi_device`](#c.wmi_device "wmi_device"), so it supports WMI devices sharing the
same GUID. Drivers can then register a [`struct wmi_driver`](#c.wmi_driver "wmi_driver")
which will be bound to compatible WMI devices by the driver core.

struct wmi\_device
:   WMI device structure

**Definition**:

```
struct wmi_device {
    struct device dev;
    bool setable;
};
```

**Members**

`dev`
:   Device associated with this WMI device

`setable`
:   True for devices implementing the Set Control Method

**Description**

This represents WMI devices discovered by the WMI driver core.

to\_wmi\_device

`to_wmi_device (device)`

> Helper macro to cast a device to a wmi\_device

**Parameters**

`device`
:   device struct

**Description**

Cast a [`struct device`](infrastructure.html#c.device "device") to a [`struct wmi_device`](#c.wmi_device "wmi_device").

struct wmi\_buffer
:   WMI data buffer

**Definition**:

```
struct wmi_buffer {
    size_t length;
    void *data;
};
```

**Members**

`length`
:   Buffer length in bytes

`data`
:   Pointer to the buffer content

**Description**

This structure is used to exchange data with the WMI driver core.

struct wmi\_string
:   WMI string representation

**Definition**:

```
struct wmi_string {
    __le16 length;
    __le16 chars[];
};
```

**Members**

`length`
:   Size of **chars** in bytes

`chars`
:   UTF16-LE characters with optional nul termination and padding

**Description**

This structure is used when exchanging string data over the WMI interface.

struct wmi\_driver
:   WMI driver structure

**Definition**:

```
struct wmi_driver {
    struct device_driver driver;
    const struct wmi_device_id *id_table;
    size_t min_event_size;
    bool no_singleton;
    int (*probe)(struct wmi_device *wdev, const void *context);
    void (*remove)(struct wmi_device *wdev);
    void (*shutdown)(struct wmi_device *wdev);
    void (*notify)(struct wmi_device *device, union acpi_object *data);
    void (*notify_new)(struct wmi_device *device, const struct wmi_buffer *data);
};
```

**Members**

`driver`
:   Driver model structure

`id_table`
:   List of WMI GUIDs supported by this driver

`min_event_size`
:   Minimum event payload size supported by this driver

`no_singleton`
:   Driver can be instantiated multiple times

`probe`
:   Callback for device binding

`remove`
:   Callback for device unbinding

`shutdown`
:   Callback for device shutdown

`notify`
:   Callback for receiving WMI events (deprecated)

`notify_new`
:   Callback for receiving WMI events

**Description**

This represents WMI drivers which handle WMI devices. The data inside the buffer
passed to the **notify\_new** callback is guaranteed to be aligned on a 8-byte boundary.
The minimum supported size for said buffer can be specified using **min\_event\_size**.
WMI drivers that still use the deprecated **notify** callback can still set **min\_event\_size**
to 0 in order to signal that they support WMI events which provide no event data.

to\_wmi\_driver

`to_wmi_driver (drv)`

> Helper macro to cast a driver to a wmi\_driver

**Parameters**

`drv`
:   driver struct

**Description**

Cast a [`struct device_driver`](infrastructure.html#c.device_driver "device_driver") to a [`struct wmi_driver`](#c.wmi_driver "wmi_driver").

wmi\_driver\_register

`wmi_driver_register (driver)`

> Helper macro to register a WMI driver

**Parameters**

`driver`
:   wmi\_driver struct

**Description**

Helper macro for registering a WMI driver. It automatically passes
THIS\_MODULE to the underlying function.

module\_wmi\_driver

`module_wmi_driver (__wmi_driver)`

> Helper macro to register/unregister a WMI driver

**Parameters**

`__wmi_driver`
:   wmi\_driver struct

**Description**

Helper macro for WMI drivers which do not do anything special in module
init/exit. This eliminates a lot of boilerplate. Each module may only
use this macro once, and calling it replaces [`module_init()`](basics.html#c.module_init "module_init") and [`module_exit()`](basics.html#c.module_exit "module_exit").

ssize\_t wmi\_string\_to\_utf8s(const struct [wmi\_string](#c.wmi_string "wmi_string") \*str, u8 \*dst, size\_t length)
:   Convert a WMI string into a UTF8 string.

**Parameters**

`const struct wmi_string *str`
:   WMI string representation

`u8 *dst`
:   Buffer to fill with UTF8 characters

`size_t length`
:   Length of the destination buffer

**Description**

Convert as WMI string into a standard UTF8 string. The conversion will stop
once a NUL character is detected or when the buffer is full. Any invalid UTF16
characters will be ignored. The resulting UTF8 string will always be NUL-terminated
when this function returns successfully.

**Return**

Length of the resulting UTF8 string or negative errno code on failure.

ssize\_t wmi\_string\_from\_utf8s(struct [wmi\_string](#c.wmi_string "wmi_string") \*str, size\_t max\_chars, const u8 \*src, size\_t src\_length)
:   Convert a UTF8 string into a WMI string.

**Parameters**

`struct wmi_string *str`
:   WMI string representation

`size_t max_chars`
:   Maximum number of UTF16 code points to store inside the WMI string

`const u8 *src`
:   UTF8 string to convert

`size_t src_length`
:   Length of the source string without any trailing NUL-characters

**Description**

Convert a UTF8 string into a WMI string. The conversion will stop when the WMI string is
full. The resulting WMI string will always be NUL-terminated and have its length field set
to and appropriate value when this function returns successfully.

**Return**

Number of UTF16 code points inside the WMI string or negative errno code on failure.

int wmi\_instance\_count(const char \*guid\_string)
:   Get number of WMI object instances

**Parameters**

`const char *guid_string`
:   36 char string of the form fa50ff2b-f2e8-45de-83fa-65417f2f49ba

**Description**

Get the number of WMI object instances.

**Return**

Number of WMI object instances or negative error code.

u8 wmidev\_instance\_count(struct [wmi\_device](#c.wmi_device "wmi_device") \*wdev)
:   Get number of WMI object instances

**Parameters**

`struct wmi_device *wdev`
:   A wmi bus device from a driver

**Description**

Get the number of WMI object instances.

**Return**

Number of WMI object instances.

acpi\_status wmi\_evaluate\_method(const char \*guid\_string, u8 instance, u32 method\_id, const struct acpi\_buffer \*in, struct acpi\_buffer \*out)
:   Evaluate a WMI method (deprecated)

**Parameters**

`const char *guid_string`
:   36 char string of the form fa50ff2b-f2e8-45de-83fa-65417f2f49ba

`u8 instance`
:   Instance index

`u32 method_id`
:   Method ID to call

`const struct acpi_buffer *in`
:   Mandatory buffer containing input for the method call

`struct acpi_buffer *out`
:   Empty buffer to return the method results

**Description**

Call an ACPI-WMI method, the caller must free **out**.

**Return**

acpi\_status signaling success or error.

acpi\_status wmidev\_evaluate\_method(struct [wmi\_device](#c.wmi_device "wmi_device") \*wdev, u8 instance, u32 method\_id, const struct acpi\_buffer \*in, struct acpi\_buffer \*out)
:   Evaluate a WMI method (deprecated)

**Parameters**

`struct wmi_device *wdev`
:   A wmi bus device from a driver

`u8 instance`
:   Instance index

`u32 method_id`
:   Method ID to call

`const struct acpi_buffer *in`
:   Mandatory buffer containing input for the method call

`struct acpi_buffer *out`
:   Empty buffer to return the method results

**Description**

Call an ACPI-WMI method, the caller must free **out**.

**Return**

acpi\_status signaling success or error.

int wmidev\_invoke\_method(struct [wmi\_device](#c.wmi_device "wmi_device") \*wdev, u8 instance, u32 method\_id, const struct [wmi\_buffer](#c.wmi_buffer "wmi_buffer") \*in, struct [wmi\_buffer](#c.wmi_buffer "wmi_buffer") \*out, size\_t min\_size)
:   Invoke a WMI method that returns values

**Parameters**

`struct wmi_device *wdev`
:   A wmi bus device from a driver

`u8 instance`
:   Instance index

`u32 method_id`
:   Method ID to call

`const struct wmi_buffer *in`
:   Mandatory WMI buffer containing input for the method call

`struct wmi_buffer *out`
:   Mandatory WMI buffer to return the method results

`size_t min_size`
:   Minimum size of the method result data in bytes

**Description**

Invoke a WMI method that returns values, the caller must free the resulting
data inside **out** using [`kfree()`](../core-api/mm-api.html#c.kfree "kfree"). Said data is guaranteed to be aligned on a
8-byte boundary. Use [`wmidev_invoke_procedure()`](#c.wmidev_invoke_procedure "wmidev_invoke_procedure") for WMI methods that
return no values.

**Return**

0 on success or negative error code on failure.

int wmidev\_invoke\_procedure(struct [wmi\_device](#c.wmi_device "wmi_device") \*wdev, u8 instance, u32 method\_id, const struct [wmi\_buffer](#c.wmi_buffer "wmi_buffer") \*in)
:   Invoke a WMI method that does not return values

**Parameters**

`struct wmi_device *wdev`
:   A wmi bus device from a driver

`u8 instance`
:   Instance index

`u32 method_id`
:   Method ID to call

`const struct wmi_buffer *in`
:   Mandatory WMI buffer containing input for the method call

**Description**

Invoke a WMI method that does not return any values. Use [`wmidev_invoke_method()`](#c.wmidev_invoke_method "wmidev_invoke_method")
for WMI methods that do return values.

**Return**

0 on success or negative error code on failure.

acpi\_status wmi\_query\_block(const char \*guid\_string, u8 instance, struct acpi\_buffer \*out)
:   Return contents of a WMI block (deprecated)

**Parameters**

`const char *guid_string`
:   36 char string of the form fa50ff2b-f2e8-45de-83fa-65417f2f49ba

`u8 instance`
:   Instance index

`struct acpi_buffer *out`
:   Empty buffer to return the contents of the data block to

**Description**

Query a ACPI-WMI block, the caller must free **out**.

**Return**

ACPI object containing the content of the WMI block.

union acpi\_object \*wmidev\_block\_query(struct [wmi\_device](#c.wmi_device "wmi_device") \*wdev, u8 instance)
:   Return contents of a WMI block (deprectated)

**Parameters**

`struct wmi_device *wdev`
:   A wmi bus device from a driver

`u8 instance`
:   Instance index

**Description**

Query an ACPI-WMI block, the caller must free the result.

**Return**

ACPI object containing the content of the WMI block.

int wmidev\_query\_block(struct [wmi\_device](#c.wmi_device "wmi_device") \*wdev, u8 instance, struct [wmi\_buffer](#c.wmi_buffer "wmi_buffer") \*out, size\_t min\_size)
:   Return contents of a WMI data block

**Parameters**

`struct wmi_device *wdev`
:   A wmi bus device from a driver

`u8 instance`
:   Instance index

`struct wmi_buffer *out`
:   WMI buffer to fill

`size_t min_size`
:   Minimum size of the result data in bytes

**Description**

Query a WMI data block, the caller must free the resulting data inside **out**
using [`kfree()`](../core-api/mm-api.html#c.kfree "kfree"). Said data is guaranteed to be aligned on a 8-byte boundary.

**Return**

0 on success or a negative error code on failure.

acpi\_status wmi\_set\_block(const char \*guid\_string, u8 instance, const struct acpi\_buffer \*in)
:   Write to a WMI block (deprecated)

**Parameters**

`const char *guid_string`
:   36 char string of the form fa50ff2b-f2e8-45de-83fa-65417f2f49ba

`u8 instance`
:   Instance index

`const struct acpi_buffer *in`
:   Buffer containing new values for the data block

**Description**

Write the contents of the input buffer to an ACPI-WMI data block.

**Return**

acpi\_status signaling success or error.

acpi\_status wmidev\_block\_set(struct [wmi\_device](#c.wmi_device "wmi_device") \*wdev, u8 instance, const struct acpi\_buffer \*in)
:   Write to a WMI block (deprecated)

**Parameters**

`struct wmi_device *wdev`
:   A wmi bus device from a driver

`u8 instance`
:   Instance index

`const struct acpi_buffer *in`
:   Buffer containing new values for the data block

**Description**

Write contents of the input buffer to an ACPI-WMI data block.

**Return**

acpi\_status signaling success or error.

int wmidev\_set\_block(struct [wmi\_device](#c.wmi_device "wmi_device") \*wdev, u8 instance, const struct [wmi\_buffer](#c.wmi_buffer "wmi_buffer") \*in)
:   Write to a WMI data block

**Parameters**

`struct wmi_device *wdev`
:   A wmi bus device from a driver

`u8 instance`
:   Instance index

`const struct wmi_buffer *in`
:   WMI buffer containing new values for the data block

**Description**

Write the content of **in** into a WMI data block.

**Return**

0 on success or negative error code on failure.

acpi\_status wmi\_install\_notify\_handler(const char \*guid, wmi\_notify\_handler handler, void \*data)
:   Register handler for WMI events (deprecated)

**Parameters**

`const char *guid`
:   36 char string of the form fa50ff2b-f2e8-45de-83fa-65417f2f49ba

`wmi_notify_handler handler`
:   Function to handle notifications

`void *data`
:   Data to be returned to handler when event is fired

**Description**

Register a handler for events sent to the ACPI-WMI mapper device.

**Return**

acpi\_status signaling success or error.

acpi\_status wmi\_remove\_notify\_handler(const char \*guid)
:   Unregister handler for WMI events (deprecated)

**Parameters**

`const char *guid`
:   36 char string of the form fa50ff2b-f2e8-45de-83fa-65417f2f49ba

**Description**

Unregister handler for events sent to the ACPI-WMI mapper device.

**Return**

acpi\_status signaling success or error.

bool wmi\_has\_guid(const char \*guid\_string)
:   Check if a GUID is available

**Parameters**

`const char *guid_string`
:   36 char string of the form fa50ff2b-f2e8-45de-83fa-65417f2f49ba

**Description**

Check if a given GUID is defined by \_WDG.

**Return**

True if GUID is available, false otherwise.

char \*wmi\_get\_acpi\_device\_uid(const char \*guid\_string)
:   Get \_UID name of ACPI device that defines GUID (deprecated)

**Parameters**

`const char *guid_string`
:   36 char string of the form fa50ff2b-f2e8-45de-83fa-65417f2f49ba

**Description**

Find the \_UID of ACPI device associated with this WMI GUID.

**Return**

The ACPI \_UID field value or NULL if the WMI GUID was not found.

void wmi\_driver\_unregister(struct [wmi\_driver](#c.wmi_driver "wmi_driver") \*driver)
:   Unregister a WMI driver

**Parameters**

`struct wmi_driver *driver`
:   WMI driver to unregister

**Description**

Unregisters a WMI driver from the WMI bus.
