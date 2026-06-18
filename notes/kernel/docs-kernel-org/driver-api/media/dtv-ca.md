# 4.4.Digital TV Conditional Access kABI

> 출처(원문): https://docs.kernel.org/driver-api/media/dtv-ca.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.4. Digital TV Conditional Access kABI

struct dvb\_ca\_en50221
:   Structure describing a CA interface

**Definition**:

```
struct dvb_ca_en50221 {
    struct module *owner;
    int (*read_attribute_mem)(struct dvb_ca_en50221 *ca, int slot, int address);
    int (*write_attribute_mem)(struct dvb_ca_en50221 *ca, int slot, int address, u8 value);
    int (*read_cam_control)(struct dvb_ca_en50221 *ca, int slot, u8 address);
    int (*write_cam_control)(struct dvb_ca_en50221 *ca, int slot, u8 address, u8 value);
    int (*read_data)(struct dvb_ca_en50221 *ca, int slot, u8 *ebuf, int ecount);
    int (*write_data)(struct dvb_ca_en50221 *ca, int slot, u8 *ebuf, int ecount);
    int (*slot_reset)(struct dvb_ca_en50221 *ca, int slot);
    int (*slot_shutdown)(struct dvb_ca_en50221 *ca, int slot);
    int (*slot_ts_enable)(struct dvb_ca_en50221 *ca, int slot);
    int (*poll_slot_status)(struct dvb_ca_en50221 *ca, int slot, int open);
    void *data;
    void *private;
};
```

**Members**

`owner`
:   the module owning this structure

`read_attribute_mem`
:   function for reading attribute memory on the CAM

`write_attribute_mem`
:   function for writing attribute memory on the CAM

`read_cam_control`
:   function for reading the control interface on the CAM

`write_cam_control`
:   function for reading the control interface on the CAM

`read_data`
:   function for reading data (block mode)

`write_data`
:   function for writing data (block mode)

`slot_reset`
:   function to reset the CAM slot

`slot_shutdown`
:   function to shutdown a CAM slot

`slot_ts_enable`
:   function to enable the Transport Stream on a CAM slot

`poll_slot_status`
:   function to poll slot status. Only necessary if
    DVB\_CA\_FLAG\_EN50221\_IRQ\_CAMCHANGE is not set.

`data`
:   private data, used by caller.

`private`
:   Opaque data used by the dvb\_ca core. Do not modify!

**NOTE**

the read\_\*, write\_\* and poll\_slot\_status functions will be
called for different slots concurrently and need to use locks where
and if appropriate. There will be no concurrent access to one slot.

void dvb\_ca\_en50221\_camchange\_irq(struct [dvb\_ca\_en50221](#c.dvb_ca_en50221 "dvb_ca_en50221") \*pubca, int slot, int change\_type)
:   A CAMCHANGE IRQ has occurred.

**Parameters**

`struct dvb_ca_en50221 *pubca`
:   CA instance.

`int slot`
:   Slot concerned.

`int change_type`
:   One of the DVB\_CA\_CAMCHANGE\_\* values

void dvb\_ca\_en50221\_camready\_irq(struct [dvb\_ca\_en50221](#c.dvb_ca_en50221 "dvb_ca_en50221") \*pubca, int slot)
:   A CAMREADY IRQ has occurred.

**Parameters**

`struct dvb_ca_en50221 *pubca`
:   CA instance.

`int slot`
:   Slot concerned.

void dvb\_ca\_en50221\_frda\_irq(struct [dvb\_ca\_en50221](#c.dvb_ca_en50221 "dvb_ca_en50221") \*ca, int slot)
:   An FR or a DA IRQ has occurred.

**Parameters**

`struct dvb_ca_en50221 *ca`
:   CA instance.

`int slot`
:   Slot concerned.

int dvb\_ca\_en50221\_init(struct [dvb\_adapter](#c.dvb_ca_en50221_init "dvb_adapter") \*dvb\_adapter, struct [dvb\_ca\_en50221](#c.dvb_ca_en50221 "dvb_ca_en50221") \*ca, int flags, int slot\_count)
:   Initialise a new DVB CA device.

**Parameters**

`struct dvb_adapter *dvb_adapter`
:   DVB adapter to attach the new CA device to.

`struct dvb_ca_en50221 *ca`
:   The dvb\_ca instance.

`int flags`
:   Flags describing the CA device (DVB\_CA\_EN50221\_FLAG\_\*).

`int slot_count`
:   Number of slots supported.

**Description**

**return** 0 on success, nonzero on failure

void dvb\_ca\_en50221\_release(struct [dvb\_ca\_en50221](#c.dvb_ca_en50221 "dvb_ca_en50221") \*ca)
:   Release a DVB CA device.

**Parameters**

`struct dvb_ca_en50221 *ca`
:   The associated dvb\_ca instance.
