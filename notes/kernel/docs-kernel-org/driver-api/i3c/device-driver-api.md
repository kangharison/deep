# I3C device driver API

> 출처(원문): https://docs.kernel.org/driver-api/i3c/device-driver-api.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# I3C device driver API

enum i3c\_error\_code
:   I3C error codes

**Constants**

`I3C_ERROR_UNKNOWN`
:   unknown error, usually means the error is not I3C
    related

`I3C_ERROR_M0`
:   M0 error

`I3C_ERROR_M1`
:   M1 error

`I3C_ERROR_M2`
:   M2 error

**Description**

These are the standard error codes as defined by the I3C specification.
When -EIO is returned by the `i3c_device_do_i3c_xfers()` or
`i3c_device_send_hdr_cmds()` one can check the error code in
`struct_i3c_xfer.err` or `struct i3c_hdr_cmd`.err to get a better idea of
what went wrong.

enum i3c\_xfer\_mode
:   I3C xfer mode ids

**Constants**

`I3C_HDR_DDR`
:   DDR mode

`I3C_HDR_TSP`
:   TSP mode

`I3C_HDR_TSL`
:   TSL mode

`I3C_SDR`
:   SDR mode (NOT HDR mode)

struct i3c\_xfer
:   I3C data transfer

**Definition**:

```
struct i3c_xfer {
    union {
        u8 rnw;
        u8 cmd;
    };
    u16 len;
    u16 actual_len;
    union {
        void *in;
        const void *out;
    } data;
    enum i3c_error_code err;
};
```

**Members**

`{unnamed_union}`
:   anonymous

`rnw`
:   encodes the transfer direction. true for a read, false for a write

`cmd`
:   Read/Write command in HDR mode, read: 0x80 - 0xff, write: 0x00 - 0x7f

`len`
:   transfer length in bytes of the transfer

`actual_len`
:   actual length in bytes are transferred by the controller

`data`
:   input/output buffer

`data.in`
:   input buffer. Must point to a DMA-able buffer

`data.out`
:   output buffer. Must point to a DMA-able buffer

`err`
:   I3C error code

enum i3c\_dcr
:   I3C DCR values

**Constants**

`I3C_DCR_GENERIC_DEVICE`
:   generic I3C device

struct i3c\_device\_info
:   I3C device information

**Definition**:

```
struct i3c_device_info {
    u64 pid;
    u8 bcr;
    u8 dcr;
    u8 static_addr;
    u8 dyn_addr;
    u8 hdr_cap;
    u8 max_read_ds;
    u8 max_write_ds;
    u8 max_ibi_len;
    u32 max_read_turnaround;
    u16 max_read_len;
    u16 max_write_len;
};
```

**Members**

`pid`
:   Provisioned ID

`bcr`
:   Bus Characteristic Register

`dcr`
:   Device Characteristic Register

`static_addr`
:   static/I2C address

`dyn_addr`
:   dynamic address

`hdr_cap`
:   supported HDR modes

`max_read_ds`
:   max read speed information

`max_write_ds`
:   max write speed information

`max_ibi_len`
:   max IBI payload length

`max_read_turnaround`
:   max read turn-around time in micro-seconds

`max_read_len`
:   max private SDR read length in bytes

`max_write_len`
:   max private SDR write length in bytes

**Description**

These are all basic information that should be advertised by an I3C device.
Some of them are optional depending on the device type and device
capabilities.
For each I3C slave attached to a master with
[`i3c_master_add_i3c_dev_locked()`](master-driver-api.html#c.i3c_master_add_i3c_dev_locked "i3c_master_add_i3c_dev_locked"), the core will send the relevant CCC command
to retrieve these data.

struct i3c\_driver
:   I3C device driver

**Definition**:

```
struct i3c_driver {
    struct device_driver driver;
    int (*probe)(struct i3c_device *dev);
    void (*remove)(struct i3c_device *dev);
    const struct i3c_device_id *id_table;
};
```

**Members**

`driver`
:   inherit from device\_driver

`probe`
:   I3C device probe method

`remove`
:   I3C device remove method

`id_table`
:   I3C device match table. Will be used by the framework to decide
    which device to bind to this driver

dev\_to\_i3cdev

`dev_to_i3cdev (__dev)`

> Returns the I3C device containing **dev**

**Parameters**

`__dev`
:   device object

**Return**

a pointer to an I3C device object.

module\_i3c\_driver

`module_i3c_driver (__drv)`

> Register a module providing an I3C driver

**Parameters**

`__drv`
:   the I3C driver to register

**Description**

Provide generic init/exit functions that simply register/unregister an I3C
driver.
Should be used by any driver that does not require extra init/cleanup steps.

int i3c\_i2c\_driver\_register(struct [i3c\_driver](#c.i3c_driver "i3c_driver") \*i3cdrv, struct [i2c\_driver](../i2c.html#c.i2c_driver "i2c_driver") \*i2cdrv)
:   Register an i2c and an i3c driver

**Parameters**

`struct i3c_driver *i3cdrv`
:   the I3C driver to register

`struct i2c_driver *i2cdrv`
:   the I2C driver to register

**Description**

This function registers both **i2cdev** and **i3cdev**, and fails if one of these
registrations fails. This is mainly useful for devices that support both I2C
and I3C modes.
Note that when CONFIG\_I3C is not enabled, this function only registers the
I2C driver.

**Return**

0 if both registrations succeeds, a negative error code otherwise.

void i3c\_i2c\_driver\_unregister(struct [i3c\_driver](#c.i3c_driver "i3c_driver") \*i3cdrv, struct [i2c\_driver](../i2c.html#c.i2c_driver "i2c_driver") \*i2cdrv)
:   Unregister an i2c and an i3c driver

**Parameters**

`struct i3c_driver *i3cdrv`
:   the I3C driver to register

`struct i2c_driver *i2cdrv`
:   the I2C driver to register

**Description**

This function unregisters both **i3cdrv** and **i2cdrv**.
Note that when CONFIG\_I3C is not enabled, this function only unregisters the
**i2cdrv**.

module\_i3c\_i2c\_driver

`module_i3c_i2c_driver (__i3cdrv, __i2cdrv)`

> Register a module providing an I3C and an I2C driver

**Parameters**

`__i3cdrv`
:   the I3C driver to register

`__i2cdrv`
:   the I2C driver to register

**Description**

Provide generic init/exit functions that simply register/unregister an I3C
and an I2C driver.
This macro can be used even if CONFIG\_I3C is disabled, in this case, only
the I2C driver will be registered.
Should be used by any driver that does not require extra init/cleanup steps.

struct i3c\_ibi\_setup
:   IBI setup object

**Definition**:

```
struct i3c_ibi_setup {
    unsigned int max_payload_len;
    unsigned int num_slots;
    void (*handler)(struct i3c_device *dev, const struct i3c_ibi_payload *payload);
};
```

**Members**

`max_payload_len`
:   maximum length of the payload associated to an IBI. If one
    IBI appears to have a payload that is bigger than this
    number, the IBI will be rejected.

`num_slots`
:   number of pre-allocated IBI slots. This should be chosen so that
    the system never runs out of IBI slots, otherwise you’ll lose
    IBIs.

`handler`
:   IBI handler, every time an IBI is received. This handler is called
    in a workqueue context. It is allowed to sleep and send new
    messages on the bus, though it’s recommended to keep the
    processing done there as fast as possible to avoid delaying
    processing of other queued on the same workqueue.

**Description**

Temporary structure used to pass information to [`i3c_device_request_ibi()`](#c.i3c_device_request_ibi "i3c_device_request_ibi").
This object can be allocated on the stack since [`i3c_device_request_ibi()`](#c.i3c_device_request_ibi "i3c_device_request_ibi")
copies every bit of information and do not use it after
[`i3c_device_request_ibi()`](#c.i3c_device_request_ibi "i3c_device_request_ibi") has returned.

int i3c\_device\_do\_xfers(struct [i3c\_device](master-driver-api.html#c.i3c_device "i3c_device") \*dev, struct [i3c\_xfer](#c.i3c_xfer "i3c_xfer") \*xfers, int nxfers, enum [i3c\_xfer\_mode](#c.i3c_xfer_mode "i3c_xfer_mode") mode)
:   do I3C transfers directed to a specific device

**Parameters**

`struct i3c_device *dev`
:   device with which the transfers should be done

`struct i3c_xfer *xfers`
:   array of transfers

`int nxfers`
:   number of transfers

`enum i3c_xfer_mode mode`
:   transfer mode

**Description**

Initiate one or several private SDR transfers with **dev**.

This function can sleep and thus cannot be called in atomic context.

**Return**

* 0 in case of success, a negative error core otherwise.
* -EAGAIN: controller lost address arbitration. Target (IBI, HJ or
  controller role request) win the bus. Client driver needs to resend the
  ‘xfers’ some time later. See I3C spec ver 1.1.1 09-Jun-2021. Section:
  5.1.2.2.3.

int i3c\_device\_do\_setdasa(struct [i3c\_device](master-driver-api.html#c.i3c_device "i3c_device") \*dev)
:   do I3C dynamic address assignement with static address

**Parameters**

`struct i3c_device *dev`
:   device with which the DAA should be done

**Return**

0 in case of success, a negative error core otherwise.

void i3c\_device\_get\_info(const struct [i3c\_device](master-driver-api.html#c.i3c_device "i3c_device") \*dev, struct [i3c\_device\_info](#c.i3c_device_info "i3c_device_info") \*info)
:   get I3C device information

**Parameters**

`const struct i3c_device *dev`
:   device we want information on

`struct i3c_device_info *info`
:   the information object to fill in

**Description**

Retrieve I3C dev info.

int i3c\_device\_disable\_ibi(struct [i3c\_device](master-driver-api.html#c.i3c_device "i3c_device") \*dev)
:   Disable IBIs coming from a specific device

**Parameters**

`struct i3c_device *dev`
:   device on which IBIs should be disabled

**Description**

This function disable IBIs coming from a specific device and wait for
all pending IBIs to be processed.

**Return**

0 in case of success, a negative error core otherwise.

int i3c\_device\_enable\_ibi(struct [i3c\_device](master-driver-api.html#c.i3c_device "i3c_device") \*dev)
:   Enable IBIs coming from a specific device

**Parameters**

`struct i3c_device *dev`
:   device on which IBIs should be enabled

**Description**

This function enable IBIs coming from a specific device and wait for
all pending IBIs to be processed. This should be called on a device
where [`i3c_device_request_ibi()`](#c.i3c_device_request_ibi "i3c_device_request_ibi") has succeeded.

Note that IBIs from this device might be received before this function
returns to its caller.

**Return**

0 in case of success, a negative error core otherwise.

int i3c\_device\_request\_ibi(struct [i3c\_device](master-driver-api.html#c.i3c_device "i3c_device") \*dev, const struct [i3c\_ibi\_setup](#c.i3c_ibi_setup "i3c_ibi_setup") \*req)
:   Request an IBI

**Parameters**

`struct i3c_device *dev`
:   device for which we should enable IBIs

`const struct i3c_ibi_setup *req`
:   setup requested for this IBI

**Description**

This function is responsible for pre-allocating all resources needed to
process IBIs coming from **dev**. When this function returns, the IBI is not
enabled until [`i3c_device_enable_ibi()`](#c.i3c_device_enable_ibi "i3c_device_enable_ibi") is called.

**Return**

0 in case of success, a negative error core otherwise.

void i3c\_device\_free\_ibi(struct [i3c\_device](master-driver-api.html#c.i3c_device "i3c_device") \*dev)
:   Free all resources needed for IBI handling

**Parameters**

`struct i3c_device *dev`
:   device on which you want to release IBI resources

**Description**

This function is responsible for de-allocating resources previously
allocated by [`i3c_device_request_ibi()`](#c.i3c_device_request_ibi "i3c_device_request_ibi"). It should be called after disabling
IBIs with [`i3c_device_disable_ibi()`](#c.i3c_device_disable_ibi "i3c_device_disable_ibi").

struct [device](../infrastructure.html#c.device "device") \*i3cdev\_to\_dev(struct [i3c\_device](master-driver-api.html#c.i3c_device "i3c_device") \*i3cdev)
:   Returns the device embedded in **i3cdev**

**Parameters**

`struct i3c_device *i3cdev`
:   I3C device

**Return**

a pointer to a device object.

const struct i3c\_device\_id \*i3c\_device\_match\_id(struct [i3c\_device](master-driver-api.html#c.i3c_device "i3c_device") \*i3cdev, const struct i3c\_device\_id \*id\_table)
:   Returns the i3c\_device\_id entry matching **i3cdev**

**Parameters**

`struct i3c_device *i3cdev`
:   I3C device

`const struct i3c_device_id *id_table`
:   I3C device match table

**Return**

a pointer to an i3c\_device\_id object or NULL if there’s no match.

u32 i3c\_device\_get\_supported\_xfer\_mode(struct [i3c\_device](master-driver-api.html#c.i3c_device "i3c_device") \*dev)
:   Returns the supported transfer mode by connected master controller.

**Parameters**

`struct i3c_device *dev`
:   I3C device

**Return**

a bit mask, which supported transfer mode, bit position is defined at
`enum i3c_hdr_mode`

int i3c\_driver\_register\_with\_owner(struct [i3c\_driver](#c.i3c_driver "i3c_driver") \*drv, struct module \*owner)
:   register an I3C device driver

**Parameters**

`struct i3c_driver *drv`
:   driver to register

`struct module *owner`
:   module that owns this driver

**Description**

Register **drv** to the core.

**Return**

0 in case of success, a negative error core otherwise.

void i3c\_driver\_unregister(struct [i3c\_driver](#c.i3c_driver "i3c_driver") \*drv)
:   unregister an I3C device driver

**Parameters**

`struct i3c_driver *drv`
:   driver to unregister

**Description**

Unregister **drv**.
