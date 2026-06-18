# Network Flow Processor (NFP) Kernel Drivers

> 출처(원문): https://docs.kernel.org/networking/device_drivers/ethernet/netronome/nfp.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Network Flow Processor (NFP) Kernel Drivers

Copyright:
:   © 2019, Netronome Systems, Inc.

Copyright:
:   © 2022, Corigine, Inc.

## Contents

* [Overview](#overview)
* [Acquiring Firmware](#acquiring-firmware)
* [Devlink Info](#devlink-info)
* [Configure Device](#configure-device)
* [Statistics](#statistics)

## Overview

This driver supports Netronome and Corigine’s line of Network Flow Processor
devices, including the NFP3800, NFP4000, NFP5000, and NFP6000 models, which
are also incorporated in the companies’ family of Agilio SmartNICs. The SR-IOV
physical and virtual functions for these devices are supported by the driver.

## Acquiring Firmware

The NFP3800, NFP4000 and NFP6000 devices require application specific firmware
to function. Application firmware can be located either on the host file system
or in the device flash (if supported by management firmware).

Firmware files on the host filesystem contain card type (AMDA-\* string), media
config etc. They should be placed in /lib/firmware/netronome directory to
load firmware from the host file system.

Firmware for basic NIC operation is available in the upstream
linux-firmware.git repository.

A more comprehensive list of firmware can be downloaded from the
[Corigine Support site](https://www.corigine.com/DPUDownload.html).

### Firmware in NVRAM

Recent versions of management firmware supports loading application
firmware from flash when the host driver gets probed. The firmware loading
policy configuration may be used to configure this feature appropriately.

Devlink or ethtool can be used to update the application firmware on the device
flash by providing the appropriate nic\_AMDA\*.nffw file to the respective
command. Users need to take care to write the correct firmware image for the
card and media configuration to flash.

Available storage space in flash depends on the card being used.

### Dealing with multiple projects

NFP hardware is fully programmable therefore there can be different
firmware images targeting different applications.

When using application firmware from host, we recommend placing
actual firmware files in application-named subdirectories in
/lib/firmware/netronome and linking the desired files, e.g.:

```
$ tree /lib/firmware/netronome/
/lib/firmware/netronome/
├── bpf
│   ├── nic_AMDA0081-0001_1x40.nffw
│   └── nic_AMDA0081-0001_4x10.nffw
├── flower
│   ├── nic_AMDA0081-0001_1x40.nffw
│   └── nic_AMDA0081-0001_4x10.nffw
├── nic
│   ├── nic_AMDA0081-0001_1x40.nffw
│   └── nic_AMDA0081-0001_4x10.nffw
├── nic_AMDA0081-0001_1x40.nffw -> bpf/nic_AMDA0081-0001_1x40.nffw
└── nic_AMDA0081-0001_4x10.nffw -> bpf/nic_AMDA0081-0001_4x10.nffw

3 directories, 8 files
```

You may need to use hard instead of symbolic links on distributions
which use old mkinitrd command instead of dracut (e.g. Ubuntu).

After changing firmware files you may need to regenerate the initramfs
image. Initramfs contains drivers and firmware files your system may
need to boot. Refer to the documentation of your distribution to find
out how to update initramfs. Good indication of stale initramfs
is system loading wrong driver or firmware on boot, but when driver is
later reloaded manually everything works correctly.

### Selecting firmware per device

Most commonly all cards on the system use the same type of firmware.
If you want to load a specific firmware image for a specific card, you
can use either the PCI bus address or serial number. The driver will
print which files it’s looking for when it recognizes a NFP device:

```
nfp: Looking for firmware file in order of priority:
nfp:  netronome/serial-00-12-34-aa-bb-cc-10-ff.nffw: not found
nfp:  netronome/pci-0000:02:00.0.nffw: not found
nfp:  netronome/nic_AMDA0081-0001_1x40.nffw: found, loading...
```

In this case if file (or link) called *serial-00-12-34-aa-bb-5d-10-ff.nffw*
or *pci-0000:02:00.0.nffw* is present in /lib/firmware/netronome this
firmware file will take precedence over nic\_AMDA\* files.

Note that serial-\* and pci-\* files are **not** automatically included
in initramfs, you will have to refer to documentation of appropriate tools
to find out how to include them.

### Running firmware version

The version of the loaded firmware for a particular <netdev> interface,
(e.g. enp4s0), or an interface’s port <netdev port> (e.g. enp4s0np0) can
be displayed with the ethtool command:

```
$ ethtool -i <netdev>
```

### Firmware loading policy

Firmware loading policy is controlled via three HWinfo parameters
stored as key value pairs in the device flash:

app\_fw\_from\_flash
:   Defines which firmware should take precedence, ‘Disk’ (0), ‘Flash’ (1) or
    the ‘Preferred’ (2) firmware. When ‘Preferred’ is selected, the management
    firmware makes the decision over which firmware will be loaded by comparing
    versions of the flash firmware and the host supplied firmware.
    This variable is configurable using the ‘fw\_load\_policy’
    devlink parameter.

abi\_drv\_reset
:   Defines if the driver should reset the firmware when
    the driver is probed, either ‘Disk’ (0) if firmware was found on disk,
    ‘Always’ (1) reset or ‘Never’ (2) reset. Note that the device is always
    reset on driver unload if firmware was loaded when the driver was probed.
    This variable is configurable using the ‘reset\_dev\_on\_drv\_probe’
    devlink parameter.

abi\_drv\_load\_ifc
:   Defines a list of PF devices allowed to load FW on the device.
    This variable is not currently user configurable.

## Devlink Info

The devlink info command displays the running and stored firmware versions
on the device, serial number and board information.

Devlink info command example (replace PCI address):

```
$ devlink dev info pci/0000:03:00.0
  pci/0000:03:00.0:
    driver nfp
    serial_number CSAAMDA2001-1003000111
    versions:
        fixed:
          board.id AMDA2001-1003
          board.rev 01
          board.manufacture CSA
          board.model mozart
        running:
          fw.mgmt 22.10.0-rc3
          fw.cpld 0x1000003
          fw.app nic-22.09.0
          chip.init AMDA-2001-1003  1003000111
        stored:
          fw.bundle_id bspbundle_1003000111
          fw.mgmt 22.10.0-rc3
          fw.cpld 0x0
          chip.init AMDA-2001-1003  1003000111
```

## Configure Device

This section explains how to use Agilio SmartNICs running basic NIC firmware.

### Configure interface link-speed

The following steps explains how to change between 10G mode and 25G mode on
Agilio CX 2x25GbE cards. The changing of port speed must be done in order,
port 0 (p0) must be set to 10G before port 1 (p1) may be set to 10G.

Down the respective interface(s):

```
$ ip link set dev <netdev port 0> down
$ ip link set dev <netdev port 1> down
```

Set interface link-speed to 10G:

```
$ ethtool -s <netdev port 0> speed 10000
$ ethtool -s <netdev port 1> speed 10000
```

Set interface link-speed to 25G:

```
$ ethtool -s <netdev port 0> speed 25000
$ ethtool -s <netdev port 1> speed 25000
```

Reload driver for changes to take effect:

```
$ rmmod nfp; modprobe nfp
```

### Configure interface Maximum Transmission Unit (MTU)

The MTU of interfaces can temporarily be set using the iproute2, ip link or
ifconfig tools. Note that this change will not persist. Setting this via
Network Manager, or another appropriate OS configuration tool, is
recommended as changes to the MTU using Network Manager can be made to
persist.

Set interface MTU to 9000 bytes:

```
$ ip link set dev <netdev port> mtu 9000
```

It is the responsibility of the user or the orchestration layer to set
appropriate MTU values when handling jumbo frames or utilizing tunnels. For
example, if packets sent from a VM are to be encapsulated on the card and
egress a physical port, then the MTU of the VF should be set to lower than
that of the physical port to account for the extra bytes added by the
additional header. If a setup is expected to see fallback traffic between
the SmartNIC and the kernel then the user should also ensure that the PF MTU
is appropriately set to avoid unexpected drops on this path.

### Configure Forward Error Correction (FEC) modes

Agilio SmartNICs support FEC mode configuration, e.g. Auto, Firecode Base-R,
ReedSolomon and Off modes. Each physical port’s FEC mode can be set
independently using ethtool. The supported FEC modes for an interface can
be viewed using:

```
$ ethtool <netdev>
```

The currently configured FEC mode can be viewed using:

```
$ ethtool --show-fec <netdev>
```

To force the FEC mode for a particular port, auto-negotiation must be disabled
(see the [Auto-negotiation](#auto-negotiation) section). An example of how to set the FEC mode
to Reed-Solomon is:

```
$ ethtool --set-fec <netdev> encoding rs
```

### Auto-negotiation

To change auto-negotiation settings, the link must first be put down. After the
link is down, auto-negotiation can be enabled or disabled using:

```
ethtool -s <netdev> autoneg <on|off>
```

## Statistics

Following device statistics are available through the `ethtool -S` interface:

NFP device statistics

| Name | ID | Meaning |
| --- | --- | --- |
| dev\_rx\_discards | 1 | Packet can be discarded on the RX path for one of the following reasons:  * The NIC is not in promisc mode, and the destination MAC address   doesn’t match the interfaces’ MAC address. * The received packet is larger than the max buffer size on the host.   I.e. it exceeds the Layer 3 MRU. * There is no freelist descriptor available on the host for the packet.   It is likely that the NIC couldn’t cache one in time. * A BPF program discarded the packet. * The datapath drop action was executed. * The MAC discarded the packet due to lack of ingress buffer space   on the NIC. |
| dev\_rx\_errors | 2 | A packet can be counted (and dropped) as RX error for the following reasons:   * A problem with the VEB lookup (only when SR-IOV is used). * A physical layer problem that causes Ethernet errors, like FCS or   alignment errors. The cause is usually faulty cables or SFPs. |
| dev\_rx\_bytes | 3 | Total number of bytes received. |
| dev\_rx\_uc\_bytes | 4 | Unicast bytes received. |
| dev\_rx\_mc\_bytes | 5 | Multicast bytes received. |
| dev\_rx\_bc\_bytes | 6 | Broadcast bytes received. |
| dev\_rx\_pkts | 7 | Total number of packets received. |
| dev\_rx\_mc\_pkts | 8 | Multicast packets received. |
| dev\_rx\_bc\_pkts | 9 | Broadcast packets received. |
| dev\_tx\_discards | 10 | A packet can be discarded in the TX direction if the MAC is being flow controlled and the NIC runs out of TX queue space. |
| dev\_tx\_errors | 11 | A packet can be counted as TX error (and dropped) for one for the following reasons:   * The packet is an LSO segment, but the Layer 3 or Layer 4 offset   could not be determined. Therefore LSO could not continue. * An invalid packet descriptor was received over PCIe. * The packet Layer 3 length exceeds the device MTU. * An error on the MAC/physical layer. Usually due to faulty cables or   SFPs. * A CTM buffer could not be allocated. * The packet offset was incorrect and could not be fixed by the NIC. |
| dev\_tx\_bytes | 12 | Total number of bytes transmitted. |
| dev\_tx\_uc\_bytes | 13 | Unicast bytes transmitted. |
| dev\_tx\_mc\_bytes | 14 | Multicast bytes transmitted. |
| dev\_tx\_bc\_bytes | 15 | Broadcast bytes transmitted. |
| dev\_tx\_pkts | 16 | Total number of packets transmitted. |
| dev\_tx\_mc\_pkts | 17 | Multicast packets transmitted. |
| dev\_tx\_bc\_pkts | 18 | Broadcast packets transmitted. |

Note that statistics unknown to the driver will be displayed as
`dev_unknown_stat$ID`, where `$ID` refers to the second column
above.
