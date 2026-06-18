# Ethernet ports

> 출처(원문): https://docs.kernel.org/networking/phy-port.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Ethernet ports

This document is a basic description of the phy\_port infrastructure,
introduced to represent physical interfaces of Ethernet devices.

Without phy\_port, we already have quite a lot of information about what the
media-facing interface of a NIC can do and looks like, through the
`struct ethtool_link_ksettings` attributes,
which includes :

> * What the NIC can do through the `supported` field
> * What the Link Partner advertises through `lp_advertising`
> * Which features we’re advertising through `advertising`

We also have info about the number of pairs and the PORT type. These settings
are built by aggregating together information reported by various devices that
are sitting on the link :

> * The NIC itself, through the `get_link_ksettings` callback
> * Precise information from the MAC and PCS by using phylink in the MAC driver
> * Information reported by the PHY device
> * Information reported by an SFP module (which can itself include a PHY)

This model however starts showing its limitations when we consider devices that
have more than one media interface. In such a case, only information about the
actively used interface is reported, and it’s not possible to know what the
other interfaces can do. In fact, we have very little information about whether
or not there are any other media interfaces.

The goal of the phy\_port representation is to provide a way of representing a
physical interface of a NIC, regardless of what is driving the port (NIC through
a firmware, SFP module, Ethernet PHY).

## Multi-port interfaces examples

Several cases of multi-interface NICs have been observed so far :

Internal MII Mux:

```
+------------------+
| SoC              |
|          +-----+ |           +-----+
| +-----+  |     |-------------| PHY |
| | MAC |--| Mux | |   +-----+ +-----+
| +-----+  |     |-----| SFP |
|          +-----+ |   +-----+
+------------------+
```

Internal Mux with internal PHY:

```
+------------------------+
| SoC                    |
|          +-----+ +-----+
| +-----+  |     |-| PHY |
| | MAC |--| Mux | +-----+   +-----+
| +-----+  |     |-----------| SFP |
|          +-----+       |   +-----+
+------------------------+
```

External Mux:

```
+---------+
| SoC     |  +-----+  +-----+
|         |  |     |--| PHY |
| +-----+ |  |     |  +-----+
| | MAC |----| Mux |  +-----+
| +-----+ |  |     |--| PHY |
|         |  +-----+  +-----+
|         |     |
|    GPIO-------+
+---------+
```

Double-port PHY:

```
+---------+
| SoC     | +-----+
|         | |     |--- RJ45
| +-----+ | |     |
| | MAC |---| PHY |   +-----+
| +-----+ | |     |---| SFP |
+---------+ +-----+   +-----+
```

phy\_port aims at providing a path to support all the above topologies, by
representing the media interfaces in a way that’s agnostic to what’s driving
the interface. the `struct phy_port` object has its own set of callback ops, and
will eventually be able to report its own ksettings:

```
            _____      +------+
           (     )-----| Port |
+-----+   (       )    +------+
| MAC |--(   ???   )
+-----+   (       )    +------+
           (_____)-----| Port |
                       +------+
```

## Next steps

As of writing this documentation, only ports controlled by PHY devices are
supported. The next steps will be to add the Netlink API to expose these
to userspace and add support for raw ports (controlled by some firmware, and directly
managed by the NIC driver).

Another parallel task is the introduction of a MII muxing framework to allow the
control of non-PHY driver multi-port setups.
