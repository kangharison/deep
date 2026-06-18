# 13.pxrc - PhoenixRC Flight Controller Adapter

> 출처(원문): https://docs.kernel.org/input/devices/pxrc.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 13. pxrc - PhoenixRC Flight Controller Adapter

Author:
:   Marcus Folkesson <[marcus.folkesson@gmail.com](mailto:marcus.folkesson%40gmail.com)>

This driver let you use your own RC controller plugged into the
adapter that comes with PhoenixRC or other compatible adapters.

The adapter supports 7 analog channels and 1 digital input switch.

## 13.1. Notes

Many RC controllers is able to configure which stick goes to which channel.
This is also configurable in most simulators, so a matching is not necessary.

The driver is generating the following input event for analog channels:

| Channel | Event |
| --- | --- |
| 1 | ABS\_X |
| 2 | ABS\_Y |
| 3 | ABS\_RX |
| 4 | ABS\_RY |
| 5 | ABS\_RUDDER |
| 6 | ABS\_THROTTLE |
| 7 | ABS\_MISC |

The digital input switch is generated as an BTN\_A event.

## 13.2. Manual Testing

To test this driver’s functionality you may use input-event which is part of
the input layer utilities suite [[1]](#id2).

For example:

```
> modprobe pxrc
> input-events <devnr>
```

To print all input events from input devnr.

## 13.3. References

[[1](#id1)]

<https://www.kraxel.org/cgit/input/>
