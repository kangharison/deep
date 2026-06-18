# 4.5.Digital TV Network kABI

> 출처(원문): https://docs.kernel.org/driver-api/media/dtv-net.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.5. Digital TV Network kABI

struct dvb\_net
:   describes a DVB network interface

**Definition**:

```
struct dvb_net {
    struct dvb_device *dvbdev;
    struct net_device *device[DVB_NET_DEVICES_MAX];
    int state[DVB_NET_DEVICES_MAX];
    unsigned int exit:1;
    struct dmx_demux *demux;
    struct mutex ioctl_mutex;
    struct mutex remove_mutex;
};
```

**Members**

`dvbdev`
:   pointer to [`struct dvb_device`](dtv-common.html#c.dvb_device "dvb_device").

`device`
:   array of pointers to [`struct net_device`](../../networking/kapi.html#c.net_device "net_device").

`state`
:   array of integers to each net device. A value
    different than zero means that the interface is
    in usage.

`exit`
:   flag to indicate when the device is being removed.

`demux`
:   pointer to [`struct dmx_demux`](dtv-demux.html#c.dmx_demux "dmx_demux").

`ioctl_mutex`
:   protect access to this struct.

`remove_mutex`
:   mutex that avoids a race condition between a callback
    called when the hardware is disconnected and the
    file\_operations of dvb\_net.

**Description**

Currently, the core supports up to `DVB_NET_DEVICES_MAX` (10) network
devices.

int dvb\_net\_init(struct [dvb\_adapter](dtv-common.html#c.dvb_adapter "dvb_adapter") \*adap, struct [dvb\_net](#c.dvb_net "dvb_net") \*dvbnet, struct [dmx\_demux](dtv-demux.html#c.dmx_demux "dmx_demux") \*dmxdemux)
:   nitializes a digital TV network device and registers it.

**Parameters**

`struct dvb_adapter *adap`
:   pointer to [`struct dvb_adapter`](dtv-common.html#c.dvb_adapter "dvb_adapter").

`struct dvb_net *dvbnet`
:   pointer to [`struct dvb_net`](#c.dvb_net "dvb_net").

`struct dmx_demux *dmxdemux`
:   pointer to [`struct dmx_demux`](dtv-demux.html#c.dmx_demux "dmx_demux").

void dvb\_net\_release(struct [dvb\_net](#c.dvb_net "dvb_net") \*dvbnet)
:   releases a digital TV network device and unregisters it.

**Parameters**

`struct dvb_net *dvbnet`
:   pointer to [`struct dvb_net`](#c.dvb_net "dvb_net").
