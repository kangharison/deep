# Intel(R) Management Engine (ME) Client bus API

> 출처(원문): https://docs.kernel.org/driver-api/mei/mei-client-bus.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Intel(R) Management Engine (ME) Client bus API

## Rationale

The MEI character device is useful for dedicated applications to send and receive
data to the many FW appliance found in Intel’s ME from the user space.
However, for some of the ME functionalities it makes sense to leverage existing software
stack and expose them through existing kernel subsystems.

In order to plug seamlessly into the kernel device driver model we add kernel virtual
bus abstraction on top of the MEI driver. This allows implementing Linux kernel drivers
for the various MEI features as a stand alone entities found in their respective subsystem.
Existing device drivers can even potentially be re-used by adding an MEI CL bus layer to
the existing code.

## MEI CL bus API

A driver implementation for an MEI Client is very similar to any other existing bus
based device drivers. The driver registers itself as an MEI CL bus driver through
the `struct mei_cl_driver` structure defined in `include/linux/mei_cl_bus.c`

```
struct mei_cl_driver {
        struct device_driver driver;
        const char *name;

        const struct mei_cl_device_id *id_table;

        int (*probe)(struct mei_cl_device *dev, const struct mei_cl_id *id);
        int (*remove)(struct mei_cl_device *dev);
};
```

The mei\_cl\_device\_id structure defined in `include/linux/mod_devicetable.h` allows a
driver to bind itself against a device name.

```
struct mei_cl_device_id {
        char name[MEI_CL_NAME_SIZE];
        uuid_le uuid;
        __u8    version;
        kernel_ulong_t driver_info;
};
```

To actually register a driver on the ME Client bus one must call the `mei_cl_add_driver()`
API. This is typically called at module initialization time.

Once the driver is registered and bound to the device, a driver will typically
try to do some I/O on this bus and this should be done through the `mei_cl_send()`
and `mei_cl_recv()` functions. More detailed information is in [API:](#api) section.

In order for a driver to be notified about pending traffic or event, the driver
should register a callback via `mei_cl_devev_register_rx_cb()` and
`mei_cldev_register_notify_cb()` function respectively.

### API:

ssize\_t mei\_cldev\_send\_vtag(struct mei\_cl\_device \*cldev, const u8 \*buf, size\_t length, u8 vtag)
:   me device send with vtag (write)

**Parameters**

`struct mei_cl_device *cldev`
:   me client device

`const u8 *buf`
:   buffer to send

`size_t length`
:   buffer length

`u8 vtag`
:   virtual tag

**Return**

* written size in bytes
* < 0 on error

ssize\_t mei\_cldev\_send\_vtag\_timeout(struct mei\_cl\_device \*cldev, const u8 \*buf, size\_t length, u8 vtag, unsigned long timeout)
:   me device send with vtag and timeout (write)

**Parameters**

`struct mei_cl_device *cldev`
:   me client device

`const u8 *buf`
:   buffer to send

`size_t length`
:   buffer length

`u8 vtag`
:   virtual tag

`unsigned long timeout`
:   send timeout in milliseconds, 0 for infinite timeout

**Return**

* written size in bytes
* < 0 on error

ssize\_t mei\_cldev\_recv\_vtag(struct mei\_cl\_device \*cldev, u8 \*buf, size\_t length, u8 \*vtag)
:   client receive with vtag (read)

**Parameters**

`struct mei_cl_device *cldev`
:   me client device

`u8 *buf`
:   buffer to receive

`size_t length`
:   buffer length

`u8 *vtag`
:   virtual tag

**Return**

* read size in bytes
* < 0 on error

ssize\_t mei\_cldev\_recv\_timeout(struct mei\_cl\_device \*cldev, u8 \*buf, size\_t length, unsigned long timeout)
:   client receive with timeout (read)

**Parameters**

`struct mei_cl_device *cldev`
:   me client device

`u8 *buf`
:   buffer to receive

`size_t length`
:   buffer length

`unsigned long timeout`
:   send timeout in milliseconds, 0 for infinite timeout

**Return**

* read size in bytes
* < 0 on error

ssize\_t mei\_cldev\_recv\_vtag\_timeout(struct mei\_cl\_device \*cldev, u8 \*buf, size\_t length, u8 \*vtag, unsigned long timeout)
:   client receive with vtag (read)

**Parameters**

`struct mei_cl_device *cldev`
:   me client device

`u8 *buf`
:   buffer to receive

`size_t length`
:   buffer length

`u8 *vtag`
:   virtual tag

`unsigned long timeout`
:   recv timeout in milliseconds, 0 for infinite timeout

**Return**

* read size in bytes
* < 0 on error

ssize\_t mei\_cldev\_send(struct mei\_cl\_device \*cldev, const u8 \*buf, size\_t length)
:   me device send (write)

**Parameters**

`struct mei_cl_device *cldev`
:   me client device

`const u8 *buf`
:   buffer to send

`size_t length`
:   buffer length

**Return**

* written size in bytes
* < 0 on error

ssize\_t mei\_cldev\_send\_timeout(struct mei\_cl\_device \*cldev, const u8 \*buf, size\_t length, unsigned long timeout)
:   me device send with timeout (write)

**Parameters**

`struct mei_cl_device *cldev`
:   me client device

`const u8 *buf`
:   buffer to send

`size_t length`
:   buffer length

`unsigned long timeout`
:   send timeout in milliseconds, 0 for infinite timeout

**Return**

* written size in bytes
* < 0 on error

ssize\_t mei\_cldev\_recv(struct mei\_cl\_device \*cldev, u8 \*buf, size\_t length)
:   client receive (read)

**Parameters**

`struct mei_cl_device *cldev`
:   me client device

`u8 *buf`
:   buffer to receive

`size_t length`
:   buffer length

**Return**

read size in bytes of < 0 on error

int mei\_cldev\_register\_rx\_cb(struct mei\_cl\_device \*cldev, mei\_cldev\_cb\_t rx\_cb)
:   register Rx event callback

**Parameters**

`struct mei_cl_device *cldev`
:   me client devices

`mei_cldev_cb_t rx_cb`
:   callback function

**Return**

0 on success
-EALREADY if an callback is already registered
<0 on other errors

int mei\_cldev\_register\_notif\_cb(struct mei\_cl\_device \*cldev, mei\_cldev\_cb\_t notif\_cb)
:   register FW notification event callback

**Parameters**

`struct mei_cl_device *cldev`
:   me client devices

`mei_cldev_cb_t notif_cb`
:   callback function

**Return**

0 on success
-EALREADY if an callback is already registered
<0 on other errors

void \*mei\_cldev\_get\_drvdata(const struct mei\_cl\_device \*cldev)
:   driver data getter

**Parameters**

`const struct mei_cl_device *cldev`
:   mei client device

**Return**

driver private data

void mei\_cldev\_set\_drvdata(struct mei\_cl\_device \*cldev, void \*data)
:   driver data setter

**Parameters**

`struct mei_cl_device *cldev`
:   mei client device

`void *data`
:   data to store

const uuid\_le \*mei\_cldev\_uuid(const struct mei\_cl\_device \*cldev)
:   return uuid of the underlying me client

**Parameters**

`const struct mei_cl_device *cldev`
:   mei client device

**Return**

me client uuid

u8 mei\_cldev\_ver(const struct mei\_cl\_device \*cldev)
:   return protocol version of the underlying me client

**Parameters**

`const struct mei_cl_device *cldev`
:   mei client device

**Return**

me client protocol version

size\_t mei\_cldev\_mtu(const struct mei\_cl\_device \*cldev)
:   max message that client can send and receive

**Parameters**

`const struct mei_cl_device *cldev`
:   mei client device

**Return**

mtu or 0 if client is not connected

bool mei\_cldev\_enabled(const struct mei\_cl\_device \*cldev)
:   check whether the device is enabled

**Parameters**

`const struct mei_cl_device *cldev`
:   mei client device

**Return**

true if me client is initialized and connected

int mei\_cldev\_enable(struct mei\_cl\_device \*cldev)
:   enable me client device create connection with me client

**Parameters**

`struct mei_cl_device *cldev`
:   me client device

**Return**

0 on success and < 0 on error

int mei\_cldev\_disable(struct mei\_cl\_device \*cldev)
:   disable me client device disconnect form the me client

**Parameters**

`struct mei_cl_device *cldev`
:   me client device

**Return**

0 on success and < 0 on error

ssize\_t mei\_cldev\_send\_gsc\_command(struct mei\_cl\_device \*cldev, u8 client\_id, u32 fence\_id, struct scatterlist \*sg\_in, size\_t total\_in\_len, struct scatterlist \*sg\_out)
:   sends a gsc command, by sending a gsl mei message to gsc and receiving reply from gsc

**Parameters**

`struct mei_cl_device *cldev`
:   me client device

`u8 client_id`
:   client id to send the command to

`u32 fence_id`
:   fence id to send the command to

`struct scatterlist *sg_in`
:   scatter gather list containing addresses for rx message buffer

`size_t total_in_len`
:   total length of data in ‘in’ sg, can be less than the sum of buffers sizes

`struct scatterlist *sg_out`
:   scatter gather list containing addresses for tx message buffer

**Return**

* written size in bytes
* < 0 on error

## Example

As a theoretical example let’s pretend the ME comes with a “contact” NFC IP.
The driver init and exit routines for this device would look like:

```
#define CONTACT_DRIVER_NAME "contact"

static struct mei_cl_device_id contact_mei_cl_tbl[] = {
        { CONTACT_DRIVER_NAME, },

        /* required last entry */
        { }
};
MODULE_DEVICE_TABLE(mei_cl, contact_mei_cl_tbl);

static struct mei_cl_driver contact_driver = {
        .id_table = contact_mei_tbl,
        .name = CONTACT_DRIVER_NAME,

        .probe = contact_probe,
        .remove = contact_remove,
};

static int contact_init(void)
{
        int r;

        r = mei_cl_driver_register(&contact_driver);
        if (r) {
                pr_err(CONTACT_DRIVER_NAME ": driver registration failed\n");
                return r;
        }

        return 0;
}

static void __exit contact_exit(void)
{
        mei_cl_driver_unregister(&contact_driver);
}

module_init(contact_init);
module_exit(contact_exit);
```

And the driver’s simplified probe routine would look like that:

```
int contact_probe(struct mei_cl_device *dev, struct mei_cl_device_id *id)
{
        [...]
        mei_cldev_enable(dev);

        mei_cldev_register_rx_cb(dev, contact_rx_cb);

        return 0;
}
```

In the probe routine the driver first enable the MEI device and then registers
an rx handler which is as close as it can get to registering a threaded IRQ handler.
The handler implementation will typically call [`mei_cldev_recv()`](#c.mei_cldev_recv "mei_cldev_recv") and then
process received data.

```
#define MAX_PAYLOAD 128
#define HDR_SIZE 4
static void conntact_rx_cb(struct mei_cl_device *cldev)
{
        struct contact *c = mei_cldev_get_drvdata(cldev);
        unsigned char payload[MAX_PAYLOAD];
        ssize_t payload_sz;

        payload_sz = mei_cldev_recv(cldev, payload,  MAX_PAYLOAD)
        if (reply_size < HDR_SIZE) {
                return;
        }

        c->process_rx(payload);

}
```

## MEI Client Bus Drivers

* [HDCP:](hdcp.html)
  + [mei\_hdcp driver](hdcp.html#mei-hdcp-driver)
  + [mei\_hdcp api](hdcp.html#mei-hdcp-api)
* [MEI NFC](nfc.html)
