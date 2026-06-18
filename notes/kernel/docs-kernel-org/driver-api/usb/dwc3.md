# Synopsys DesignWare Core SuperSpeed USB 3.0 Controller

> 출처(원문): https://docs.kernel.org/driver-api/usb/dwc3.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Synopsys DesignWare Core SuperSpeed USB 3.0 Controller

Author:
:   Felipe Balbi <[felipe.balbi@linux.intel.com](mailto:felipe.balbi%40linux.intel.com)>

Date:
:   April 2017

## Introduction

The *Synopsys DesignWare Core SuperSpeed USB 3.0 Controller*
(hereinafter referred to as *DWC3*) is a USB SuperSpeed compliant
controller which can be configured in one of 4 ways:

> 1. Peripheral-only configuration
> 2. Host-only configuration
> 3. Dual-Role configuration
> 4. Hub configuration

Linux currently supports several versions of this controller. In all
likelihood, the version in your SoC is already supported. At the time
of this writing, known tested versions range from 2.02a to 3.10a. As a
rule of thumb, anything above 2.02a should work reliably well.

Currently, we have many known users for this driver. In alphabetical
order:

> 1. Cavium
> 2. Intel Corporation
> 3. Qualcomm
> 4. Rockchip
> 5. ST
> 6. Samsung
> 7. Texas Instruments
> 8. Xilinx

## Summary of Features

For details about features supported by your version of DWC3, consult
your IP team and/or *Synopsys DesignWare Core SuperSpeed USB 3.0
Controller Databook*. Following is a list of features supported by the
driver at the time of this writing:

> 1. Up to 16 bidirectional endpoints (including the control
>    pipe - ep0)
> 2. Flexible endpoint configuration
> 3. Simultaneous IN and OUT transfer support
> 4. Scatter-list support
> 5. Up to 256 TRBs [[1]](#trb) per endpoint
> 6. Support for all transfer types (*Control*, *Bulk*,
>    *Interrupt*, and *Isochronous*)
> 7. SuperSpeed Bulk Streams
> 8. Link Power Management
> 9. Trace Events for debugging
> 10. DebugFS [[3]](#id9) interface

These features have all been exercised with many of the **in-tree**
gadget drivers. We have verified both *ConfigFS* [[4]](#configfs) and
legacy gadget drivers.

## Driver Design

The DWC3 driver sits on the *drivers/usb/dwc3/* directory. All files
related to this driver are in this one directory. This makes it easy
for new-comers to read the code and understand how it behaves.

Because of DWC3’s configuration flexibility, the driver is a little
complex in some places but it should be rather straightforward to
understand.

The biggest part of the driver refers to the Gadget API.

## Known Limitations

Like any other HW, DWC3 has its own set of limitations. To avoid
constant questions about such problems, we decided to document them
here and have a single location to where we could point users.

### OUT Transfer Size Requirements

According to Synopsys Databook, all OUT transfer TRBs [[1]](#trb) must
have their *size* field set to a value which is integer divisible by
the endpoint’s *wMaxPacketSize*. This means that *e.g.* in order to
receive a Mass Storage *CBW* [[5]](#cbw), req->length must either be set
to a value that’s divisible by *wMaxPacketSize* (1024 on SuperSpeed,
512 on HighSpeed, etc), or DWC3 driver must add a Chained TRB pointing
to a throw-away buffer for the remaining length. Without this, OUT
transfers will **NOT** start.

Note that as of this writing, this won’t be a problem because DWC3 is
fully capable of appending a chained TRB for the remaining length and
completely hide this detail from the gadget driver. It’s still worth
mentioning because this seems to be the largest source of queries
about DWC3 and *non-working transfers*.

### TRB Ring Size Limitation

We, currently, have a hard limit of 256 TRBs [[1]](#trb) per endpoint,
with the last TRB being a Link TRB [[2]](#link-trb) pointing back to the
first. This limit is arbitrary but it has the benefit of adding up to
exactly 4096 bytes, or 1 Page.

DWC3 driver will try its best to cope with more than 255 requests and,
for the most part, it should work normally. However this is not
something that has been exercised very frequently. If you experience
any problems, see section **Reporting Bugs** below.

## Reporting Bugs

Whenever you encounter a problem with DWC3, first and foremost you
should make sure that:

> 1. You’re running latest tag from [Linus’ tree](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/)
> 2. You can reproduce the error without any out-of-tree changes
>    to DWC3
> 3. You have checked that it’s not a fault on the host machine

After all these are verified, then here’s how to capture enough
information so we can be of any help to you.

### Required Information

DWC3 relies exclusively on Trace Events for debugging. Everything is
exposed there, with some extra bits being exposed to DebugFS
[[3]](#id9).

In order to capture DWC3’s Trace Events you should run the following
commands **before** plugging the USB cable to a host machine:

```
# mkdir -p /d
# mkdir -p /t
# mount -t debugfs none /d
# mount -t tracefs none /t
# echo 81920 > /t/buffer_size_kb
# echo 1 > /t/events/dwc3/enable
```

After this is done, you can connect your USB cable and reproduce the
problem. As soon as the fault is reproduced, make a copy of files
`trace` and `regdump`, like so:

```
# cp /t/trace /root/trace.txt
# cat /d/*dwc3*/regdump > /root/regdump.txt
```

Make sure to compress `trace.txt` and `regdump.txt` in a tarball
and email it to [me](mailto:felipe.balbi%40linux.intel.com) with [linux-usb](mailto:linux-usb%40vger.kernel.org) in Cc. If you want to be extra
sure that I’ll help you, write your subject line in the following
format:

> **[BUG REPORT] usb: dwc3: Bug while doing XYZ**

On the email body, make sure to detail what you doing, which gadget
driver you were using, how to reproduce the problem, what SoC you’re
using, which OS (and its version) was running on the Host machine.

With all this information, we should be able to understand what’s
going on and be helpful to you.

## Debugging

First and foremost a disclaimer:

```
DISCLAIMER: The information available on DebugFS and/or TraceFS can
change at any time at any Major Linux Kernel Release. If writing
scripts, do **NOT** assume information to be available in the
current format.
```

With that out of the way, let’s carry on.

If you’re willing to debug your own problem, you deserve a round of
applause :-)

Anyway, there isn’t much to say here other than Trace Events will be
really helpful in figuring out issues with DWC3. Also, access to
Synopsys Databook will be **really** valuable in this case.

A USB Sniffer can be helpful at times but it’s not entirely required,
there’s a lot that can be understood without looking at the wire.

Feel free to email [me](mailto:felipe.balbi%40linux.intel.com) and Cc [linux-usb](mailto:linux-usb%40vger.kernel.org) if you need any help.

### `DebugFS`

`DebugFS` is very good for gathering snapshots of what’s going on
with DWC3 and/or any endpoint.

On DWC3’s `DebugFS` directory, you will find the following files and
directories:

`ep[0..15]{in,out}/`
`link_state`
`regdump`
`testmode`

#### `link_state`

When read, `link_state` will print out one of `U0`, `U1`,
`U2`, `U3`, `SS.Disabled`, `RX.Detect`, `SS.Inactive`,
`Polling`, `Recovery`, `Hot Reset`, `Compliance`,
`Loopback`, `Reset`, `Resume` or `UNKNOWN link state`.

This file can also be written to in order to force link to one of the
states above.

#### `regdump`

File name is self-explanatory. When read, `regdump` will print out a
register dump of DWC3. Note that this file can be grepped to find the
information you want.

#### `testmode`

When read, `testmode` will print out a name of one of the specified
USB 2.0 Testmodes (`test_j`, `test_k`, `test_se0_nak`,
`test_packet`, `test_force_enable`) or the string `no test` in
case no tests are currently being executed.

In order to start any of these test modes, the same strings can be
written to the file and DWC3 will enter the requested test mode.

#### `ep[0..15]{in,out}`

For each endpoint we expose one directory following the naming
convention `ep$num$dir` *(ep0in, ep0out, ep1in, ...)*. Inside each
of these directories you will find the following files:

`descriptor_fetch_queue`
`event_queue`
`rx_fifo_queue`
`rx_info_queue`
`rx_request_queue`
`transfer_type`
`trb_ring`
`tx_fifo_queue`
`tx_request_queue`

With access to Synopsys Databook, you can decode the information on
them.

##### `transfer_type`

When read, `transfer_type` will print out one of `control`,
`bulk`, `interrupt` or `isochronous` depending on what the
endpoint descriptor says. If the endpoint hasn’t been enabled yet, it
will print `--`.

##### `trb_ring`

When read, `trb_ring` will print out details about all TRBs on the
ring. It will also tell you where our enqueue and dequeue pointers are
located in the ring:

```
buffer_addr,size,type,ioc,isp_imi,csp,chn,lst,hwo
000000002c754000,481,normal,1,0,1,0,0,0
000000002c75c000,481,normal,1,0,1,0,0,0
000000002c780000,481,normal,1,0,1,0,0,0
000000002c788000,481,normal,1,0,1,0,0,0
000000002c78c000,481,normal,1,0,1,0,0,0
000000002c754000,481,normal,1,0,1,0,0,0
000000002c75c000,481,normal,1,0,1,0,0,0
000000002c784000,481,normal,1,0,1,0,0,0
000000002c788000,481,normal,1,0,1,0,0,0
000000002c78c000,481,normal,1,0,1,0,0,0
000000002c790000,481,normal,1,0,1,0,0,0
000000002c758000,481,normal,1,0,1,0,0,0
000000002c780000,481,normal,1,0,1,0,0,0
000000002c788000,481,normal,1,0,1,0,0,0
000000002c790000,481,normal,1,0,1,0,0,0
000000002c758000,481,normal,1,0,1,0,0,0
000000002c780000,481,normal,1,0,1,0,0,0
000000002c784000,481,normal,1,0,1,0,0,0
000000002c788000,481,normal,1,0,1,0,0,0
000000002c78c000,481,normal,1,0,1,0,0,0
000000002c754000,481,normal,1,0,1,0,0,0
000000002c758000,481,normal,1,0,1,0,0,0
000000002c780000,481,normal,1,0,1,0,0,0
000000002c784000,481,normal,1,0,1,0,0,0
000000002c78c000,481,normal,1,0,1,0,0,0
000000002c790000,481,normal,1,0,1,0,0,0
000000002c758000,481,normal,1,0,1,0,0,0
000000002c780000,481,normal,1,0,1,0,0,0
000000002c788000,481,normal,1,0,1,0,0,0
000000002c790000,481,normal,1,0,1,0,0,0
000000002c758000,481,normal,1,0,1,0,0,0
000000002c780000,481,normal,1,0,1,0,0,0
000000002c788000,481,normal,1,0,1,0,0,0
000000002c790000,481,normal,1,0,1,0,0,0
000000002c758000,481,normal,1,0,1,0,0,0
000000002c780000,481,normal,1,0,1,0,0,0
000000002c788000,481,normal,1,0,1,0,0,0
000000002c790000,481,normal,1,0,1,0,0,0
000000002c758000,481,normal,1,0,1,0,0,0
000000002c780000,481,normal,1,0,1,0,0,0
000000002c788000,481,normal,1,0,1,0,0,0
000000002c790000,481,normal,1,0,1,0,0,0
000000002c758000,481,normal,1,0,1,0,0,0
000000002c780000,481,normal,1,0,1,0,0,0
000000002c788000,481,normal,1,0,1,0,0,0
000000002c790000,481,normal,1,0,1,0,0,0
000000002c758000,481,normal,1,0,1,0,0,0
000000002c780000,481,normal,1,0,1,0,0,0
000000002c788000,481,normal,1,0,1,0,0,0
000000002c790000,481,normal,1,0,1,0,0,0
000000002c758000,481,normal,1,0,1,0,0,0
000000002c780000,481,normal,1,0,1,0,0,0
000000002c788000,481,normal,1,0,1,0,0,0
000000002c790000,481,normal,1,0,1,0,0,0
000000002c758000,481,normal,1,0,1,0,0,0
000000002c780000,481,normal,1,0,1,0,0,0
000000002c78c000,481,normal,1,0,1,0,0,0
000000002c784000,481,normal,1,0,1,0,0,0
000000002c788000,481,normal,1,0,1,0,0,0
000000002c78c000,481,normal,1,0,1,0,0,0
000000002c754000,481,normal,1,0,1,0,0,0
000000002c758000,481,normal,1,0,1,0,0,0
000000002c780000,481,normal,1,0,1,0,0,0
000000002c788000,481,normal,1,0,1,0,0,0
000000002c790000,481,normal,1,0,1,0,0,0
000000002c758000,481,normal,1,0,1,0,0,0
000000002c780000,481,normal,1,0,1,0,0,0
000000002c758000,481,normal,1,0,1,0,0,0
000000002c780000,481,normal,1,0,1,0,0,0
000000002c78c000,481,normal,1,0,1,0,0,0
000000002c75c000,481,normal,1,0,1,0,0,0
000000002c78c000,481,normal,1,0,1,0,0,0
000000002c780000,481,normal,1,0,1,0,0,0
000000002c754000,481,normal,1,0,1,0,0,0
000000002c788000,481,normal,1,0,1,0,0,0
000000002c754000,481,normal,1,0,1,0,0,0
000000002c780000,481,normal,1,0,1,0,0,0
000000002c788000,481,normal,1,0,1,0,0,0
000000002c78c000,481,normal,1,0,1,0,0,0
000000002c790000,481,normal,1,0,1,0,0,0
000000002c754000,481,normal,1,0,1,0,0,0
000000002c758000,481,normal,1,0,1,0,0,0
000000002c75c000,481,normal,1,0,1,0,0,0
000000002c780000,481,normal,1,0,1,0,0,0
000000002c784000,481,normal,1,0,1,0,0,0
000000002c788000,481,normal,1,0,1,0,0,0
000000002c78c000,481,normal,1,0,1,0,0,0
000000002c790000,481,normal,1,0,1,0,0,0
000000002c754000,481,normal,1,0,1,0,0,0
000000002c758000,481,normal,1,0,1,0,0,0
000000002c75c000,512,normal,1,0,1,0,0,1        D
0000000000000000,0,UNKNOWN,0,0,0,0,0,0       E
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
0000000000000000,0,UNKNOWN,0,0,0,0,0,0
00000000381ab000,0,link,0,0,0,0,0,1
```

### Trace Events

DWC3 also provides several trace events which help us gathering
information about the behavior of the driver during runtime.

In order to use these events, you must enable `CONFIG_FTRACE` in
your kernel config.

For details about how enable DWC3 events, see section **Reporting
Bugs**.

The following subsections will give details about each Event Class and
each Event defined by DWC3.

#### MMIO

It is sometimes useful to look at every MMIO access when looking for
bugs. Because of that, DWC3 offers two Trace Events (one for
`dwc3_readl()` and one for `dwc3_writel()`). `TP_printk` follows:

```
TP_printk("addr %p value %08x", __entry->base + __entry->offset,
              __entry->value)
```

#### Interrupt Events

Every IRQ event can be logged and decoded into a human readable
string. Because every event will be different, we don’t give an
example other than the `TP_printk` format used:

```
TP_printk("event (%08x): %s", __entry->event,
              dwc3_decode_event(__entry->event, __entry->ep0state))
```

#### Control Request

Every USB Control Request can be logged to the trace buffer. The
output format is:

```
TP_printk("%s", dwc3_decode_ctrl(__entry->bRequestType,
                              __entry->bRequest, __entry->wValue,
                              __entry->wIndex, __entry->wLength)
)
```

Note that Standard Control Requests will be decoded into
human-readable strings with their respective arguments. Class and
Vendor requests will be printed out a sequence of 8 bytes in hex
format.

#### Lifetime of a `struct usb_request`

The entire lifetime of a `struct usb_request` can be tracked on the
trace buffer. We have one event for each of allocation, free,
queueing, dequeueing, and giveback. Output format is:

```
TP_printk("%s: req %p length %u/%u %s%s%s ==> %d",
      __get_str(name), __entry->req, __entry->actual, __entry->length,
      __entry->zero ? "Z" : "z",
      __entry->short_not_ok ? "S" : "s",
      __entry->no_interrupt ? "i" : "I",
      __entry->status
)
```

#### Generic Commands

We can log and decode every Generic Command with its completion
code. Format is:

```
TP_printk("cmd '%s' [%x] param %08x --> status: %s",
      dwc3_gadget_generic_cmd_string(__entry->cmd),
      __entry->cmd, __entry->param,
      dwc3_gadget_generic_cmd_status_string(__entry->status)
)
```

#### Endpoint Commands

Endpoints commands can also be logged together with completion
code. Format is:

```
TP_printk("%s: cmd '%s' [%d] params %08x %08x %08x --> status: %s",
      __get_str(name), dwc3_gadget_ep_cmd_string(__entry->cmd),
      __entry->cmd, __entry->param0,
      __entry->param1, __entry->param2,
      dwc3_ep_cmd_status_string(__entry->cmd_status)
)
```

#### Lifetime of a `TRB`

A `TRB` Lifetime is simple. We are either preparing a `TRB` or
completing it. With these two events, we can see how a `TRB` changes
over time. Format is:

```
TP_printk("%s: %d/%d trb %p buf %08x%08x size %s%d ctrl %08x (%c%c%c%c:%c%c:%s)",
      __get_str(name), __entry->queued, __entry->allocated,
      __entry->trb, __entry->bph, __entry->bpl,
      ({char *s;
      int pcm = ((__entry->size >> 24) & 3) + 1;
      switch (__entry->type) {
      case USB_ENDPOINT_XFER_INT:
      case USB_ENDPOINT_XFER_ISOC:
              switch (pcm) {
              case 1:
                      s = "1x ";
                      break;
              case 2:
                      s = "2x ";
                      break;
              case 3:
                      s = "3x ";
                      break;
              }
      default:
              s = "";
      } s; }),
      DWC3_TRB_SIZE_LENGTH(__entry->size), __entry->ctrl,
      __entry->ctrl & DWC3_TRB_CTRL_HWO ? 'H' : 'h',
      __entry->ctrl & DWC3_TRB_CTRL_LST ? 'L' : 'l',
      __entry->ctrl & DWC3_TRB_CTRL_CHN ? 'C' : 'c',
      __entry->ctrl & DWC3_TRB_CTRL_CSP ? 'S' : 's',
      __entry->ctrl & DWC3_TRB_CTRL_ISP_IMI ? 'S' : 's',
      __entry->ctrl & DWC3_TRB_CTRL_IOC ? 'C' : 'c',
    dwc3_trb_type_string(DWC3_TRBCTL_TYPE(__entry->ctrl))
)
```

#### Lifetime of an Endpoint

And endpoint’s lifetime is summarized with enable and disable
operations, both of which can be traced. Format is:

```
TP_printk("%s: mps %d/%d streams %d burst %d ring %d/%d flags %c:%c%c%c%c%c:%c:%c",
      __get_str(name), __entry->maxpacket,
      __entry->maxpacket_limit, __entry->max_streams,
      __entry->maxburst, __entry->trb_enqueue,
      __entry->trb_dequeue,
      __entry->flags & DWC3_EP_ENABLED ? 'E' : 'e',
      __entry->flags & DWC3_EP_STALL ? 'S' : 's',
      __entry->flags & DWC3_EP_WEDGE ? 'W' : 'w',
      __entry->flags & DWC3_EP_TRANSFER_STARTED ? 'B' : 'b',
      __entry->flags & DWC3_EP_PENDING_REQUEST ? 'P' : 'p',
      __entry->flags & DWC3_EP_END_TRANSFER_PENDING ? 'E' : 'e',
      __entry->direction ? '<' : '>'
)
```

## Structures, Methods and Definitions

struct dwc3\_event\_buffer
:   Software event buffer representation

**Definition**:

```
struct dwc3_event_buffer {
    void *buf;
    void *cache;
    unsigned int            length;
    unsigned int            lpos;
    unsigned int            count;
    unsigned int            flags;
#define DWC3_EVENT_PENDING      BIT(0);
    dma_addr_t dma;
    struct dwc3             *dwc;
};
```

**Members**

`buf`
:   \_THE\_ buffer

`cache`
:   The buffer cache used in the threaded interrupt

`length`
:   size of this buffer

`lpos`
:   event offset

`count`
:   cache of last read event count register

`flags`
:   flags related to this event buffer

`dma`
:   dma\_addr\_t

`dwc`
:   pointer to DWC controller

struct dwc3\_ep
:   device side endpoint representation

**Definition**:

```
struct dwc3_ep {
    struct usb_ep           endpoint;
    struct delayed_work     nostream_work;
    struct list_head        cancelled_list;
    struct list_head        pending_list;
    struct list_head        started_list;
    struct dwc3_trb         *trb_pool;
    dma_addr_t trb_pool_dma;
    struct dwc3             *dwc;
    u32 saved_state;
    unsigned int            flags;
#define DWC3_EP_ENABLED                 BIT(0);
#define DWC3_EP_STALL                   BIT(1);
#define DWC3_EP_WEDGE                   BIT(2);
#define DWC3_EP_TRANSFER_STARTED        BIT(3);
#define DWC3_EP_END_TRANSFER_PENDING    BIT(4);
#define DWC3_EP_PENDING_REQUEST         BIT(5);
#define DWC3_EP_DELAY_START             BIT(6);
#define DWC3_EP_WAIT_TRANSFER_COMPLETE  BIT(7);
#define DWC3_EP_IGNORE_NEXT_NOSTREAM    BIT(8);
#define DWC3_EP_FORCE_RESTART_STREAM    BIT(9);
#define DWC3_EP_STREAM_PRIMED           BIT(10);
#define DWC3_EP_PENDING_CLEAR_STALL     BIT(11);
#define DWC3_EP_TXFIFO_RESIZED          BIT(12);
#define DWC3_EP_DELAY_STOP             BIT(13);
#define DWC3_EP_RESOURCE_ALLOCATED      BIT(14);
#define DWC3_EP0_DIR_IN                 BIT(31);
    u8 trb_enqueue;
    u8 trb_dequeue;
    u8 number;
    u8 type;
    u8 resource_index;
    u32 frame_number;
    u32 interval;
    char name[20];
    unsigned direction:1;
    unsigned stream_capable:1;
    u8 combo_num;
    int start_cmd_status;
};
```

**Members**

`endpoint`
:   usb endpoint

`nostream_work`
:   work for handling bulk NoStream

`cancelled_list`
:   list of cancelled requests for this endpoint

`pending_list`
:   list of pending requests for this endpoint

`started_list`
:   list of started requests on this endpoint

`trb_pool`
:   array of transaction buffers

`trb_pool_dma`
:   dma address of **trb\_pool**

`dwc`
:   pointer to DWC controller

`saved_state`
:   ep state saved during hibernation

`flags`
:   endpoint flags (wedged, stalled, ...)

`trb_enqueue`
:   enqueue ‘pointer’ into TRB array

`trb_dequeue`
:   dequeue ‘pointer’ into TRB array

`number`
:   endpoint number (1 - 15)

`type`
:   set to bmAttributes & USB\_ENDPOINT\_XFERTYPE\_MASK

`resource_index`
:   Resource transfer index

`frame_number`
:   set to the frame number we want this transfer to start (ISOC)

`interval`
:   the interval on which the ISOC transfer is started

`name`
:   a human readable name e.g. ep1out-bulk

`direction`
:   true for TX, false for RX

`stream_capable`
:   true when streams are enabled

`combo_num`
:   the test combination BIT[15:14] of the frame number to test
    isochronous START TRANSFER command failure workaround

`start_cmd_status`
:   the status of testing START TRANSFER command with
    combo\_num = ‘b00

struct dwc3\_trb
:   transfer request block (hw format)

**Definition**:

```
struct dwc3_trb {
    u32 bpl;
    u32 bph;
    u32 size;
    u32 ctrl;
};
```

**Members**

`bpl`
:   DW0-3

`bph`
:   DW4-7

`size`
:   DW8-B

`ctrl`
:   DWC-F

struct dwc3\_hwparams
:   copy of HWPARAMS registers

**Definition**:

```
struct dwc3_hwparams {
    u32 hwparams0;
    u32 hwparams1;
    u32 hwparams2;
    u32 hwparams3;
    u32 hwparams4;
    u32 hwparams5;
    u32 hwparams6;
    u32 hwparams7;
    u32 hwparams8;
    u32 hwparams9;
};
```

**Members**

`hwparams0`
:   GHWPARAMS0

`hwparams1`
:   GHWPARAMS1

`hwparams2`
:   GHWPARAMS2

`hwparams3`
:   GHWPARAMS3

`hwparams4`
:   GHWPARAMS4

`hwparams5`
:   GHWPARAMS5

`hwparams6`
:   GHWPARAMS6

`hwparams7`
:   GHWPARAMS7

`hwparams8`
:   GHWPARAMS8

`hwparams9`
:   GHWPARAMS9

struct dwc3\_request
:   representation of a transfer request

**Definition**:

```
struct dwc3_request {
    struct usb_request      request;
    struct list_head        list;
    struct dwc3_ep          *dep;
    struct scatterlist      *start_sg;
    unsigned int            num_pending_sgs;
    unsigned int            remaining;
    unsigned int            status;
#define DWC3_REQUEST_STATUS_QUEUED              0;
#define DWC3_REQUEST_STATUS_STARTED             1;
#define DWC3_REQUEST_STATUS_DISCONNECTED        2;
#define DWC3_REQUEST_STATUS_DEQUEUED            3;
#define DWC3_REQUEST_STATUS_STALLED             4;
#define DWC3_REQUEST_STATUS_COMPLETED           5;
#define DWC3_REQUEST_STATUS_UNKNOWN             -1;
    u8 epnum;
    struct dwc3_trb         *trb;
    dma_addr_t trb_dma;
    unsigned int            num_trbs;
    unsigned int            direction:1;
    unsigned int            mapped:1;
};
```

**Members**

`request`
:   [`struct usb_request`](gadget.html#c.usb_request "usb_request") to be transferred

`list`
:   a list\_head used for request queueing

`dep`
:   [`struct dwc3_ep`](#c.dwc3_ep "dwc3_ep") owning this request

`start_sg`
:   pointer to the sg which should be queued next

`num_pending_sgs`
:   counter to pending sgs

`remaining`
:   amount of data remaining

`status`
:   internal dwc3 request status tracking

`epnum`
:   endpoint number to which this request refers

`trb`
:   pointer to [`struct dwc3_trb`](#c.dwc3_trb "dwc3_trb")

`trb_dma`
:   DMA address of **trb**

`num_trbs`
:   number of TRBs used by this request

`direction`
:   IN or OUT direction flag

`mapped`
:   true when request has been dma-mapped

struct dwc3\_glue\_ops
:   The ops indicate the notifications that need to be passed on to glue layer

**Definition**:

```
struct dwc3_glue_ops {
    void (*pre_set_role)(struct dwc3 *dwc, enum usb_role role);
    void (*pre_run_stop)(struct dwc3 *dwc, bool is_on);
};
```

**Members**

`pre_set_role`
:   Notify glue of role switch notifications

`pre_run_stop`
:   Notify run stop enable/disable information to glue

struct dwc3
:   representation of our controller

**Definition**:

```
struct dwc3 {
    struct work_struct      drd_work;
    struct dwc3_trb         *ep0_trb;
    void *bounce;
    u8 *setup_buf;
    dma_addr_t ep0_trb_addr;
    dma_addr_t bounce_addr;
    struct dwc3_request     ep0_usb_req;
    struct completion       ep0_in_setup;
    spinlock_t lock;
    struct mutex            mutex;
    struct device           *dev;
    struct device           *sysdev;
    struct platform_device  *xhci;
    struct resource         xhci_resources[DWC3_XHCI_RESOURCES_NUM];
    struct dwc3_event_buffer *ev_buf;
    struct dwc3_ep          *eps[DWC3_ENDPOINTS_NUM];
    struct usb_gadget       *gadget;
    struct usb_gadget_driver *gadget_driver;
    const struct dwc3_glue_ops      *glue_ops;
    struct clk              *bus_clk;
    struct clk              *ref_clk;
    struct clk              *susp_clk;
    struct clk              *utmi_clk;
    struct clk              *pipe_clk;
    struct reset_control    *reset;
    struct usb_phy          *usb2_phy;
    struct usb_phy          *usb3_phy;
    struct phy              *usb2_generic_phy[DWC3_USB2_MAX_PORTS];
    struct phy              *usb3_generic_phy[DWC3_USB3_MAX_PORTS];
    u8 num_usb2_ports;
    u8 num_usb3_ports;
    bool phys_ready;
    struct ulpi             *ulpi;
    bool ulpi_ready;
    void __iomem            *regs;
    size_t regs_size;
    enum usb_dr_mode        dr_mode;
    u32 current_dr_role;
    u32 desired_dr_role;
    struct extcon_dev       *edev;
    struct notifier_block   edev_nb;
    enum usb_phy_interface  hsphy_mode;
    struct usb_role_switch  *role_sw;
    enum usb_dr_mode        role_switch_default_mode;
    struct power_supply     *usb_psy;
    struct work_struct      vbus_draw_work;
    unsigned int            current_limit;
    u32 fladj;
    u32 ref_clk_per;
    u32 irq_gadget;
    u32 otg_irq;
    u32 current_otg_role;
    u32 desired_otg_role;
    bool otg_restart_host;
    u32 u1u2;
    u32 maximum_speed;
    u32 gadget_max_speed;
    enum usb_ssp_rate       max_ssp_rate;
    enum usb_ssp_rate       gadget_ssp_rate;
    u32 ip;
#define DWC3_IP                 0x5533;
#define DWC31_IP                0x3331;
#define DWC32_IP                0x3332;
#define DWC4_IP                 0x3430;
    u32 revision;
#define DWC3_REVISION_ANY       0x0;
#define DWC3_REVISION_173A      0x5533173a;
#define DWC3_REVISION_175A      0x5533175a;
#define DWC3_REVISION_180A      0x5533180a;
#define DWC3_REVISION_183A      0x5533183a;
#define DWC3_REVISION_185A      0x5533185a;
#define DWC3_REVISION_187A      0x5533187a;
#define DWC3_REVISION_188A      0x5533188a;
#define DWC3_REVISION_190A      0x5533190a;
#define DWC3_REVISION_194A      0x5533194a;
#define DWC3_REVISION_200A      0x5533200a;
#define DWC3_REVISION_202A      0x5533202a;
#define DWC3_REVISION_210A      0x5533210a;
#define DWC3_REVISION_220A      0x5533220a;
#define DWC3_REVISION_230A      0x5533230a;
#define DWC3_REVISION_240A      0x5533240a;
#define DWC3_REVISION_250A      0x5533250a;
#define DWC3_REVISION_260A      0x5533260a;
#define DWC3_REVISION_270A      0x5533270a;
#define DWC3_REVISION_280A      0x5533280a;
#define DWC3_REVISION_290A      0x5533290a;
#define DWC3_REVISION_300A      0x5533300a;
#define DWC3_REVISION_310A      0x5533310a;
#define DWC3_REVISION_320A      0x5533320a;
#define DWC3_REVISION_330A      0x5533330a;
#define DWC31_REVISION_ANY      0x0;
#define DWC31_REVISION_110A     0x3131302a;
#define DWC31_REVISION_120A     0x3132302a;
#define DWC31_REVISION_160A     0x3136302a;
#define DWC31_REVISION_170A     0x3137302a;
#define DWC31_REVISION_180A     0x3138302a;
#define DWC31_REVISION_190A     0x3139302a;
#define DWC31_REVISION_200A     0x3230302a;
#define DWC32_REVISION_ANY      0x0;
#define DWC32_REVISION_100A     0x3130302a;
    u32 version_type;
#define DWC31_VERSIONTYPE_ANY           0x0;
#define DWC31_VERSIONTYPE_EA01          0x65613031;
#define DWC31_VERSIONTYPE_EA02          0x65613032;
#define DWC31_VERSIONTYPE_EA03          0x65613033;
#define DWC31_VERSIONTYPE_EA04          0x65613034;
#define DWC31_VERSIONTYPE_EA05          0x65613035;
#define DWC31_VERSIONTYPE_EA06          0x65613036;
    enum dwc3_ep0_next      ep0_next_event;
    enum dwc3_ep0_state     ep0state;
    enum dwc3_link_state    link_state;
    u16 u2sel;
    u16 u2pel;
    u8 u1sel;
    u8 u1pel;
    u8 speed;
    u8 num_eps;
    struct dwc3_hwparams    hwparams;
    struct debugfs_regset32 *regset;
    u32 dbg_lsp_select;
    u8 test_mode;
    u8 test_mode_nr;
    u8 lpm_nyet_threshold;
    u8 hird_threshold;
    u8 rx_thr_num_pkt;
    u8 rx_max_burst;
    u8 tx_thr_num_pkt;
    u8 tx_max_burst;
    u8 rx_thr_num_pkt_prd;
    u8 rx_max_burst_prd;
    u8 tx_thr_num_pkt_prd;
    u8 tx_max_burst_prd;
    u8 tx_fifo_resize_max_num;
    u8 clear_stall_protocol;
    u16 num_hc_interrupters;
    const char              *hsphy_interface;
    unsigned connected:1;
    unsigned softconnect:1;
    unsigned delayed_status:1;
    unsigned ep0_bounced:1;
    unsigned ep0_expect_in:1;
    unsigned sysdev_is_parent:1;
    unsigned has_lpm_erratum:1;
    unsigned is_utmi_l1_suspend:1;
    unsigned is_fpga:1;
    unsigned pending_events:1;
    unsigned do_fifo_resize:1;
    unsigned pullups_connected:1;
    unsigned setup_packet_pending:1;
    unsigned three_stage_setup:1;
    unsigned dis_start_transfer_quirk:1;
    unsigned usb3_lpm_capable:1;
    unsigned usb2_lpm_disable:1;
    unsigned usb2_gadget_lpm_disable:1;
    unsigned needs_full_reinit:1;
    unsigned disable_scramble_quirk:1;
    unsigned u2exit_lfps_quirk:1;
    unsigned u2ss_inp3_quirk:1;
    unsigned req_p1p2p3_quirk:1;
    unsigned del_p1p2p3_quirk:1;
    unsigned del_phy_power_chg_quirk:1;
    unsigned lfps_filter_quirk:1;
    unsigned rx_detect_poll_quirk:1;
    unsigned dis_u3_susphy_quirk:1;
    unsigned dis_u2_susphy_quirk:1;
    unsigned dis_enblslpm_quirk:1;
    unsigned dis_u1_entry_quirk:1;
    unsigned dis_u2_entry_quirk:1;
    unsigned dis_rxdet_inp3_quirk:1;
    unsigned dis_u2_freeclk_exists_quirk:1;
    unsigned dis_del_phy_power_chg_quirk:1;
    unsigned dis_tx_ipgap_linecheck_quirk:1;
    unsigned resume_hs_terminations:1;
    unsigned ulpi_ext_vbus_drv:1;
    unsigned parkmode_disable_ss_quirk:1;
    unsigned parkmode_disable_hs_quirk:1;
    unsigned gfladj_refclk_lpm_sel:1;
    unsigned tx_de_emphasis_quirk:1;
    unsigned tx_de_emphasis:2;
    unsigned dis_metastability_quirk:1;
    unsigned dis_split_quirk:1;
    unsigned enable_usb2_transceiver_delay:1;
    unsigned async_callbacks:1;
    unsigned sys_wakeup:1;
    unsigned wakeup_configured:1;
    unsigned suspended:1;
    unsigned susphy_state:1;
    u16 imod_interval;
    int max_cfg_eps;
    int last_fifo_depth;
    int num_ep_resized;
    struct dentry           *debug_root;
    u32 gsbuscfg0_reqinfo;
    u32 wakeup_pending_funcs;
};
```

**Members**

`drd_work`
:   workqueue used for role swapping

`ep0_trb`
:   trb which is used for the ctrl\_req

`bounce`
:   address of bounce buffer

`setup_buf`
:   used while precessing STD USB requests

`ep0_trb_addr`
:   dma address of **ep0\_trb**

`bounce_addr`
:   dma address of **bounce**

`ep0_usb_req`
:   dummy req used while handling STD USB requests

`ep0_in_setup`
:   one control transfer is completed and enter setup phase

`lock`
:   for synchronizing

`mutex`
:   for mode switching

`dev`
:   pointer to our [`struct device`](../infrastructure.html#c.device "device")

`sysdev`
:   pointer to the DMA-capable device

`xhci`
:   pointer to our xHCI child

`xhci_resources`
:   `struct resources` for our **xhci** child

`ev_buf`
:   [`struct dwc3_event_buffer`](#c.dwc3_event_buffer "dwc3_event_buffer") pointer

`eps`
:   endpoint array

`gadget`
:   device side representation of the peripheral controller

`gadget_driver`
:   pointer to the gadget driver

`glue_ops`
:   Vendor callbacks for flattened device implementations.

`bus_clk`
:   clock for accessing the registers

`ref_clk`
:   reference clock

`susp_clk`
:   clock used when the SS phy is in low power (S3) state

`utmi_clk`
:   clock used for USB2 PHY communication

`pipe_clk`
:   clock used for USB3 PHY communication

`reset`
:   reset control

`usb2_phy`
:   pointer to USB2 PHY

`usb3_phy`
:   pointer to USB3 PHY

`usb2_generic_phy`
:   pointer to array of USB2 PHYs

`usb3_generic_phy`
:   pointer to array of USB3 PHYs

`num_usb2_ports`
:   number of USB2 ports

`num_usb3_ports`
:   number of USB3 ports

`phys_ready`
:   flag to indicate that PHYs are ready

`ulpi`
:   pointer to ulpi interface

`ulpi_ready`
:   flag to indicate that ULPI is initialized

`regs`
:   base address for our registers

`regs_size`
:   address space size

`dr_mode`
:   requested mode of operation

`current_dr_role`
:   current role of operation when in dual-role mode

`desired_dr_role`
:   desired role of operation when in dual-role mode

`edev`
:   extcon handle

`edev_nb`
:   extcon notifier

`hsphy_mode`
:   UTMI phy mode, one of following:
    - USBPHY\_INTERFACE\_MODE\_UTMI
    - USBPHY\_INTERFACE\_MODE\_UTMIW

`role_sw`
:   usb\_role\_switch handle

`role_switch_default_mode`
:   default operation mode of controller while
    usb role is USB\_ROLE\_NONE.

`usb_psy`
:   pointer to power supply interface.

`vbus_draw_work`
:   Work to set the vbus drawing limit

`current_limit`
:   How much current to draw from vbus, in milliAmperes.

`fladj`
:   frame length adjustment

`ref_clk_per`
:   reference clock period configuration

`irq_gadget`
:   peripheral controller’s IRQ number

`otg_irq`
:   IRQ number for OTG IRQs

`current_otg_role`
:   current role of operation while using the OTG block

`desired_otg_role`
:   desired role of operation while using the OTG block

`otg_restart_host`
:   flag that OTG controller needs to restart host

`u1u2`
:   only used on revisions <1.83a for workaround

`maximum_speed`
:   maximum speed requested (mainly for testing purposes)

`gadget_max_speed`
:   maximum gadget speed requested

`max_ssp_rate`
:   SuperSpeed Plus maximum signaling rate and lane count

`gadget_ssp_rate`
:   Gadget driver’s maximum supported SuperSpeed Plus signaling
    rate and lane count.

`ip`
:   controller’s ID

`revision`
:   controller’s version of an IP

`version_type`
:   VERSIONTYPE register contents, a sub release of a revision

`ep0_next_event`
:   hold the next expected event

`ep0state`
:   state of endpoint zero

`link_state`
:   link state

`u2sel`
:   parameter from Set SEL request.

`u2pel`
:   parameter from Set SEL request.

`u1sel`
:   parameter from Set SEL request.

`u1pel`
:   parameter from Set SEL request.

`speed`
:   device speed (super, high, full, low)

`num_eps`
:   number of endpoints

`hwparams`
:   copy of hwparams registers

`regset`
:   debugfs pointer to regdump file

`dbg_lsp_select`
:   current debug lsp mux register selection

`test_mode`
:   true when we’re entering a USB test mode

`test_mode_nr`
:   test feature selector

`lpm_nyet_threshold`
:   LPM NYET response threshold

`hird_threshold`
:   HIRD threshold

`rx_thr_num_pkt`
:   USB receive packet count

`rx_max_burst`
:   max USB receive burst size

`tx_thr_num_pkt`
:   USB transmit packet count

`tx_max_burst`
:   max USB transmit burst size

`rx_thr_num_pkt_prd`
:   periodic ESS receive packet count

`rx_max_burst_prd`
:   max periodic ESS receive burst size

`tx_thr_num_pkt_prd`
:   periodic ESS transmit packet count

`tx_max_burst_prd`
:   max periodic ESS transmit burst size

`tx_fifo_resize_max_num`
:   max number of fifos allocated during txfifo resize

`clear_stall_protocol`
:   endpoint number that requires a delayed status phase

`num_hc_interrupters`
:   number of host controller interrupters

`hsphy_interface`
:   “utmi” or “ulpi”

`connected`
:   true when we’re connected to a host, false otherwise

`softconnect`
:   true when gadget connect is called, false when disconnect runs

`delayed_status`
:   true when gadget driver asks for delayed status

`ep0_bounced`
:   true when we used bounce buffer

`ep0_expect_in`
:   true when we expect a DATA IN transfer

`sysdev_is_parent`
:   true when dwc3 device has a parent driver

`has_lpm_erratum`
:   true when core was configured with LPM Erratum. Note that
    there’s now way for software to detect this in runtime.

`is_utmi_l1_suspend`
:   the core asserts output signal
    0 - utmi\_sleep\_n
    1 - utmi\_l1\_suspend\_n

`is_fpga`
:   true when we are using the FPGA board

`pending_events`
:   true when we have pending IRQs to be handled

`do_fifo_resize`
:   true when txfifo resizing is enabled for dwc3 endpoints

`pullups_connected`
:   true when Run/Stop bit is set

`setup_packet_pending`
:   true when there’s a Setup Packet in FIFO. Workaround

`three_stage_setup`
:   set if we perform a three phase setup

`dis_start_transfer_quirk`
:   set if start\_transfer failure SW workaround is
    not needed for DWC\_usb31 version 1.70a-ea06 and below

`usb3_lpm_capable`
:   set if hadrware supports Link Power Management

`usb2_lpm_disable`
:   set to disable usb2 lpm for host

`usb2_gadget_lpm_disable`
:   set to disable usb2 lpm for gadget

`needs_full_reinit`
:   set to indicate the core may lose power and need full
    initialization during system pm

`disable_scramble_quirk`
:   set if we enable the disable scramble quirk

`u2exit_lfps_quirk`
:   set if we enable u2exit lfps quirk

`u2ss_inp3_quirk`
:   set if we enable P3 OK for U2/SS Inactive quirk

`req_p1p2p3_quirk`
:   set if we enable request p1p2p3 quirk

`del_p1p2p3_quirk`
:   set if we enable delay p1p2p3 quirk

`del_phy_power_chg_quirk`
:   set if we enable delay phy power change quirk

`lfps_filter_quirk`
:   set if we enable LFPS filter quirk

`rx_detect_poll_quirk`
:   set if we enable rx\_detect to polling lfps quirk

`dis_u3_susphy_quirk`
:   set if we disable usb3 suspend phy

`dis_u2_susphy_quirk`
:   set if we disable usb2 suspend phy

`dis_enblslpm_quirk`
:   set if we clear enblslpm in GUSB2PHYCFG,
    disabling the suspend signal to the PHY.

`dis_u1_entry_quirk`
:   set if link entering into U1 state needs to be disabled.

`dis_u2_entry_quirk`
:   set if link entering into U2 state needs to be disabled.

`dis_rxdet_inp3_quirk`
:   set if we disable Rx.Detect in P3

`dis_u2_freeclk_exists_quirk`
:   set if we clear u2\_freeclk\_exists
    in GUSB2PHYCFG, specify that USB2 PHY doesn’t
    provide a free-running PHY clock.

`dis_del_phy_power_chg_quirk`
:   set if we disable delay phy power
    change quirk.

`dis_tx_ipgap_linecheck_quirk`
:   set if we disable u2mac linestate
    check during HS transmit.

`resume_hs_terminations`
:   Set if we enable quirk for fixing improper crc
    generation after resume from suspend.

`ulpi_ext_vbus_drv`
:   Set to confiure the upli chip to drives CPEN pin
    VBUS with an external supply.

`parkmode_disable_ss_quirk`
:   set if we need to disable all SuperSpeed
    instances in park mode.

`parkmode_disable_hs_quirk`
:   set if we need to disable all HighSpeed
    instances in park mode.

`gfladj_refclk_lpm_sel`
:   set if we need to enable SOF/ITP counter
    running based on ref\_clk

`tx_de_emphasis_quirk`
:   set if we enable Tx de-emphasis quirk

`tx_de_emphasis`
:   Tx de-emphasis value
    0 - -6dB de-emphasis
    1 - -3.5dB de-emphasis
    2 - No de-emphasis
    3 - Reserved

`dis_metastability_quirk`
:   set to disable metastability quirk.

`dis_split_quirk`
:   set to disable split boundary.

`enable_usb2_transceiver_delay`
:   Set to insert a delay before the
    assertion of the TxValid signal during a HS Chirp.

`async_callbacks`
:   if set, indicate that async callbacks will be used.

`sys_wakeup`
:   set if the device may do system wakeup.

`wakeup_configured`
:   set if the device is configured for remote wakeup.

`suspended`
:   set to track suspend event due to U3/L2.

`susphy_state`
:   state of DWC3\_GUSB2PHYCFG\_SUSPHY + DWC3\_GUSB3PIPECTL\_SUSPHY
    before PM suspend.

`imod_interval`
:   set the interrupt moderation interval in 250ns
    increments or 0 to disable.

`max_cfg_eps`
:   current max number of IN eps used across all USB configs.

`last_fifo_depth`
:   last fifo depth used to determine next fifo ram start
    address.

`num_ep_resized`
:   carries the current number endpoints which have had its tx
    fifo resized.

`debug_root`
:   root debugfs directory for this device to put its files in.

`gsbuscfg0_reqinfo`
:   store GSBUSCFG0.DATRDREQINFO, DESRDREQINFO,
    DATWRREQINFO, and DESWRREQINFO value passed from
    glue driver.

`wakeup_pending_funcs`
:   Indicates whether any interface has requested for
    function wakeup in bitmap format where bit position
    represents interface\_id.

struct dwc3\_event\_depevt
:   Device Endpoint Events

**Definition**:

```
struct dwc3_event_depevt {
    u32 one_bit:1;
    u32 endpoint_number:5;
    u32 endpoint_event:4;
    u32 reserved11_10:2;
    u32 status:4;
#define DEPEVT_STATUS_TRANSFER_ACTIVE   BIT(3);
#define DEPEVT_STATUS_BUSERR    BIT(0);
#define DEPEVT_STATUS_SHORT     BIT(1);
#define DEPEVT_STATUS_IOC       BIT(2);
#define DEPEVT_STATUS_LST       BIT(3) ;
#define DEPEVT_STATUS_MISSED_ISOC BIT(3) ;
#define DEPEVT_STREAMEVT_FOUND          1;
#define DEPEVT_STREAMEVT_NOTFOUND       2;
#define DEPEVT_STREAM_PRIME             0xfffe;
#define DEPEVT_STREAM_NOSTREAM          0x0;
#define DEPEVT_STATUS_CONTROL_DATA      1;
#define DEPEVT_STATUS_CONTROL_STATUS    2;
#define DEPEVT_STATUS_CONTROL_PHASE(n)  ((n) & 3);
#define DEPEVT_TRANSFER_NO_RESOURCE     1;
#define DEPEVT_TRANSFER_BUS_EXPIRY      2;
    u32 parameters:16;
#define DEPEVT_PARAMETER_CMD(n) (((n) & (0xf << 8)) >> 8);
};
```

**Members**

`one_bit`
:   indicates this is an endpoint event (not used)

`endpoint_number`
:   number of the endpoint

`endpoint_event`
:   The event we have:
    0x00 - Reserved
    0x01 - XferComplete
    0x02 - XferInProgress
    0x03 - XferNotReady
    0x04 - RxTxFifoEvt (IN->Underrun, OUT->Overrun)
    0x05 - Reserved
    0x06 - StreamEvt
    0x07 - EPCmdCmplt

`reserved11_10`
:   Reserved, don’t use.

`status`
:   Indicates the status of the event. Refer to databook for
    more information.

`parameters`
:   Parameters of the current event. Refer to databook for
    more information.

struct dwc3\_event\_devt
:   Device Events

**Definition**:

```
struct dwc3_event_devt {
    u32 one_bit:1;
    u32 device_event:7;
    u32 type:4;
    u32 reserved15_12:4;
    u32 event_info:9;
    u32 reserved31_25:7;
};
```

**Members**

`one_bit`
:   indicates this is a non-endpoint event (not used)

`device_event`
:   indicates it’s a device event. Should read as 0x00

`type`
:   indicates the type of device event.
    0 - DisconnEvt
    1 - USBRst
    2 - ConnectDone
    3 - ULStChng
    4 - WkUpEvt
    5 - Reserved
    6 - Suspend (EOPF on revisions 2.10a and prior)
    7 - SOF
    8 - Reserved
    9 - ErrticErr
    10 - CmdCmplt
    11 - EvntOverflow
    12 - VndrDevTstRcved

`reserved15_12`
:   Reserved, not used

`event_info`
:   Information about this event

`reserved31_25`
:   Reserved, not used

struct dwc3\_event\_gevt
:   Other Core Events

**Definition**:

```
struct dwc3_event_gevt {
    u32 one_bit:1;
    u32 device_event:7;
    u32 phy_port_number:4;
    u32 reserved31_12:20;
};
```

**Members**

`one_bit`
:   indicates this is a non-endpoint event (not used)

`device_event`
:   indicates it’s (0x03) Carkit or (0x04) I2C event.

`phy_port_number`
:   self-explanatory

`reserved31_12`
:   Reserved, not used.

union dwc3\_event
:   representation of Event Buffer contents

**Definition**:

```
union dwc3_event {
    u32 raw;
    struct dwc3_event_type          type;
    struct dwc3_event_depevt        depevt;
    struct dwc3_event_devt          devt;
    struct dwc3_event_gevt          gevt;
};
```

**Members**

`raw`
:   raw 32-bit event

`type`
:   the type of the event

`depevt`
:   Device Endpoint Event

`devt`
:   Device Event

`gevt`
:   Global Event

struct dwc3\_gadget\_ep\_cmd\_params
:   representation of endpoint command parameters

**Definition**:

```
struct dwc3_gadget_ep_cmd_params {
    u32 param2;
    u32 param1;
    u32 param0;
};
```

**Members**

`param2`
:   third parameter

`param1`
:   second parameter

`param0`
:   first parameter

u32 dwc3\_mdwidth(struct [dwc3](#c.dwc3 "dwc3") \*dwc)
:   get MDWIDTH value in bits

**Parameters**

`struct dwc3 *dwc`
:   pointer to our context structure

**Description**

Return MDWIDTH configuration value in bits.

struct [dwc3\_request](#c.dwc3_request "dwc3_request") \*next\_request(struct list\_head \*list)
:   gets the next request on the given list

**Parameters**

`struct list_head *list`
:   the request list to operate on

**Description**

Caller should take care of locking. This function return `NULL` or the first
request available on **list**.

void dwc3\_gadget\_move\_started\_request(struct [dwc3\_request](#c.dwc3_request "dwc3_request") \*req)
:   move **req** to the started\_list

**Parameters**

`struct dwc3_request *req`
:   the request to be moved

**Description**

Caller should take care of locking. This function will move **req** from its
current list to the endpoint’s started\_list.

void dwc3\_gadget\_move\_cancelled\_request(struct [dwc3\_request](#c.dwc3_request "dwc3_request") \*req, unsigned int reason)
:   move **req** to the cancelled\_list

**Parameters**

`struct dwc3_request *req`
:   the request to be moved

`unsigned int reason`
:   cancelled reason for the dwc3 request

**Description**

Caller should take care of locking. This function will move **req** from its
current list to the endpoint’s cancelled\_list.

void dwc3\_gadget\_ep\_get\_transfer\_index(struct [dwc3\_ep](#c.dwc3_ep "dwc3_ep") \*dep)
:   Gets transfer index from HW

**Parameters**

`struct dwc3_ep *dep`
:   dwc3 endpoint

**Description**

Caller should take care of locking. Returns the transfer resource
index for a given endpoint.

void dwc3\_gadget\_dctl\_write\_safe(struct [dwc3](#c.dwc3 "dwc3") \*dwc, u32 value)
:   write to DCTL safe from link state change

**Parameters**

`struct dwc3 *dwc`
:   pointer to our context structure

`u32 value`
:   value to write to DCTL

**Description**

Use this function when doing read-modify-write to DCTL. It will not
send link state change request.

int dwc3\_gadget\_set\_test\_mode(struct [dwc3](#c.dwc3 "dwc3") \*dwc, int mode)
:   enables usb2 test modes

**Parameters**

`struct dwc3 *dwc`
:   pointer to our context structure

`int mode`
:   the mode to set (J, K SE0 NAK, Force Enable)

**Description**

Caller should take care of locking. This function will return 0 on
success or -EINVAL if wrong Test Selector is passed.

int dwc3\_gadget\_get\_link\_state(struct [dwc3](#c.dwc3 "dwc3") \*dwc)
:   gets current state of usb link

**Parameters**

`struct dwc3 *dwc`
:   pointer to our context structure

**Description**

Caller should take care of locking. This function will
return the link state on success (>= 0) or -ETIMEDOUT.

int dwc3\_gadget\_set\_link\_state(struct [dwc3](#c.dwc3 "dwc3") \*dwc, enum dwc3\_link\_state state)
:   sets usb link to a particular state

**Parameters**

`struct dwc3 *dwc`
:   pointer to our context structure

`enum dwc3_link_state state`
:   the state to put link into

**Description**

Caller should take care of locking. This function will
return 0 on success or -ETIMEDOUT.

void dwc3\_ep\_inc\_trb(u8 \*index)
:   increment a trb index.

**Parameters**

`u8 *index`
:   Pointer to the TRB index to increment.

**Description**

The index should never point to the link TRB. After incrementing,
if it is point to the link TRB, wrap around to the beginning. The
link TRB is always at the last TRB entry.

void dwc3\_ep\_inc\_enq(struct [dwc3\_ep](#c.dwc3_ep "dwc3_ep") \*dep)
:   increment endpoint’s enqueue pointer

**Parameters**

`struct dwc3_ep *dep`
:   The endpoint whose enqueue pointer we’re incrementing

void dwc3\_ep\_inc\_deq(struct [dwc3\_ep](#c.dwc3_ep "dwc3_ep") \*dep)
:   increment endpoint’s dequeue pointer

**Parameters**

`struct dwc3_ep *dep`
:   The endpoint whose enqueue pointer we’re incrementing

void dwc3\_gadget\_giveback(struct [dwc3\_ep](#c.dwc3_ep "dwc3_ep") \*dep, struct [dwc3\_request](#c.dwc3_request "dwc3_request") \*req, int status)
:   call [`struct usb_request`](gadget.html#c.usb_request "usb_request")’s ->complete callback

**Parameters**

`struct dwc3_ep *dep`
:   The endpoint to whom the request belongs to

`struct dwc3_request *req`
:   The request we’re giving back

`int status`
:   completion code for the request

**Description**

Must be called with controller’s lock held and interrupts disabled. This
function will unmap **req** and call its ->`complete()` callback to notify upper
layers that it has completed.

int dwc3\_send\_gadget\_generic\_command(struct [dwc3](#c.dwc3 "dwc3") \*dwc, unsigned int cmd, u32 param)
:   issue a generic command for the controller

**Parameters**

`struct dwc3 *dwc`
:   pointer to the controller context

`unsigned int cmd`
:   the command to be issued

`u32 param`
:   command parameter

**Description**

Caller should take care of locking. Issue **cmd** with a given **param** to **dwc**
and wait for its completion.

int dwc3\_send\_gadget\_ep\_cmd(struct [dwc3\_ep](#c.dwc3_ep "dwc3_ep") \*dep, unsigned int cmd, struct [dwc3\_gadget\_ep\_cmd\_params](#c.dwc3_gadget_ep_cmd_params "dwc3_gadget_ep_cmd_params") \*params)
:   issue an endpoint command

**Parameters**

`struct dwc3_ep *dep`
:   the endpoint to which the command is going to be issued

`unsigned int cmd`
:   the command to be issued

`struct dwc3_gadget_ep_cmd_params *params`
:   parameters to the command

**Description**

Caller should handle locking. This function will issue **cmd** with given
**params** to **dep** and wait for its completion.

According to the programming guide, if the link state is in L1/L2/U3,
then sending the Start Transfer command may not complete. The
programming guide suggested to bring the link state back to ON/U0 by
performing remote wakeup prior to sending the command. However, don’t
initiate remote wakeup when the user/function does not send wakeup
request via wakeup ops. Send the command when it’s allowed.

**Notes**

For L1 link state, issuing a command requires the clearing of
GUSB2PHYCFG.SUSPENDUSB2, which turns on the signal required to complete
the given command (usually within 50us). This should happen within the
command timeout set by driver. No additional step is needed.

For L2 or U3 link state, the gadget is in USB suspend. Care should be
taken when sending Start Transfer command to ensure that it’s done after
USB resume.

int dwc3\_gadget\_start\_config(struct [dwc3](#c.dwc3 "dwc3") \*dwc, unsigned int resource\_index)
:   reset endpoint resources

**Parameters**

`struct dwc3 *dwc`
:   pointer to the DWC3 context

`unsigned int resource_index`
:   DEPSTARTCFG.XferRscIdx value (must be 0 or 2)

**Description**

Set resource\_index=0 to reset all endpoints’ resources allocation. Do this as
part of the power-on/soft-reset initialization.

Set resource\_index=2 to reset only non-control endpoints’ resources. Do this
on receiving the SET\_CONFIGURATION request or hibernation resume.

int dwc3\_gadget\_calc\_tx\_fifo\_size(struct [dwc3](#c.dwc3 "dwc3") \*dwc, int mult)
:   calculates the txfifo size value

**Parameters**

`struct dwc3 *dwc`
:   pointer to the DWC3 context

`int mult`
:   multiplier to be used when calculating the fifo\_size

**Description**

Calculates the size value based on the equation below:

DWC3 revision 280A and prior:
fifo\_size = mult \* (max\_packet / mdwidth) + 1;

DWC3 revision 290A and onwards:
fifo\_size = mult \* ((max\_packet + mdwidth)/mdwidth + 1) + 1

The max packet size is set to 1024, as the txfifo requirements mainly apply
to super speed USB use cases. However, it is safe to overestimate the fifo
allocations for other scenarios, i.e. high speed USB.

int dwc3\_gadget\_calc\_ram\_depth(struct [dwc3](#c.dwc3 "dwc3") \*dwc)
:   calculates the ram depth for txfifo

**Parameters**

`struct dwc3 *dwc`
:   pointer to the DWC3 context

void dwc3\_gadget\_clear\_tx\_fifos(struct [dwc3](#c.dwc3 "dwc3") \*dwc)
:   Clears txfifo allocation

**Parameters**

`struct dwc3 *dwc`
:   pointer to the DWC3 context

**Description**

Iterates through all the endpoint registers and clears the previous txfifo
allocations.

int \_\_dwc3\_gadget\_ep\_enable(struct [dwc3\_ep](#c.dwc3_ep "dwc3_ep") \*dep, unsigned int action)
:   initializes a hw endpoint

**Parameters**

`struct dwc3_ep *dep`
:   endpoint to be initialized

`unsigned int action`
:   one of INIT, MODIFY or RESTORE

**Description**

Caller should take care of locking. Execute all necessary commands to
initialize a HW endpoint so it can be used by a gadget driver.

int \_\_dwc3\_gadget\_ep\_disable(struct [dwc3\_ep](#c.dwc3_ep "dwc3_ep") \*dep)
:   disables a hw endpoint

**Parameters**

`struct dwc3_ep *dep`
:   the endpoint to disable

**Description**

This function undoes what \_\_dwc3\_gadget\_ep\_enable did and also removes
requests which are currently being processed by the hardware and those which
are not yet scheduled.

Caller should take care of locking.

struct [dwc3\_trb](#c.dwc3_trb "dwc3_trb") \*dwc3\_ep\_prev\_trb(struct [dwc3\_ep](#c.dwc3_ep "dwc3_ep") \*dep, u8 index)
:   returns the previous TRB in the ring

**Parameters**

`struct dwc3_ep *dep`
:   The endpoint with the TRB ring

`u8 index`
:   The index of the current TRB in the ring

**Description**

Returns the TRB prior to the one pointed to by the index. If the
index is 0, we will wrap backwards, skip the link TRB, and return
the one just before that.

void dwc3\_prepare\_one\_trb(struct [dwc3\_ep](#c.dwc3_ep "dwc3_ep") \*dep, struct [dwc3\_request](#c.dwc3_request "dwc3_request") \*req, unsigned int trb\_length, unsigned int chain, unsigned int node, bool use\_bounce\_buffer, bool must\_interrupt)
:   setup one TRB from one request

**Parameters**

`struct dwc3_ep *dep`
:   endpoint for which this request is prepared

`struct dwc3_request *req`
:   dwc3\_request pointer

`unsigned int trb_length`
:   buffer size of the TRB

`unsigned int chain`
:   should this TRB be chained to the next?

`unsigned int node`
:   only for isochronous endpoints. First TRB needs different type.

`bool use_bounce_buffer`
:   set to use bounce buffer

`bool must_interrupt`
:   set to interrupt on TRB completion

int dwc3\_prepare\_last\_sg(struct [dwc3\_ep](#c.dwc3_ep "dwc3_ep") \*dep, struct [dwc3\_request](#c.dwc3_request "dwc3_request") \*req, unsigned int entry\_length, unsigned int node)
:   prepare TRBs for the last SG entry

**Parameters**

`struct dwc3_ep *dep`
:   The endpoint that the request belongs to

`struct dwc3_request *req`
:   The request to prepare

`unsigned int entry_length`
:   The last SG entry size

`unsigned int node`
:   Indicates whether this is not the first entry (for isoc only)

**Description**

Return the number of TRBs prepared.

int \_\_dwc3\_stop\_active\_transfer(struct [dwc3\_ep](#c.dwc3_ep "dwc3_ep") \*dep, bool force, bool interrupt)
:   stop the current active transfer

**Parameters**

`struct dwc3_ep *dep`
:   isoc endpoint

`bool force`
:   set forcerm bit in the command

`bool interrupt`
:   command complete interrupt after End Transfer command

**Description**

When setting force, the ForceRM bit will be set. In that case
the controller won’t update the TRB progress on command
completion. It also won’t clear the HWO bit in the TRB.
The command will also not complete immediately in that case.

int dwc3\_gadget\_start\_isoc\_quirk(struct [dwc3\_ep](#c.dwc3_ep "dwc3_ep") \*dep)
:   workaround invalid frame number

**Parameters**

`struct dwc3_ep *dep`
:   isoc endpoint

**Description**

This function tests for the correct combination of BIT[15:14] from the 16-bit
microframe number reported by the XferNotReady event for the future frame
number to start the isoc transfer.

In DWC\_usb31 version 1.70a-ea06 and prior, for highspeed and fullspeed
isochronous IN, BIT[15:14] of the 16-bit microframe number reported by the
XferNotReady event are invalid. The driver uses this number to schedule the
isochronous transfer and passes it to the START TRANSFER command. Because
this number is invalid, the command may fail. If BIT[15:14] matches the
internal 16-bit microframe, the START TRANSFER command will pass and the
transfer will start at the scheduled time, if it is off by 1, the command
will still pass, but the transfer will start 2 seconds in the future. For all
other conditions, the START TRANSFER command will fail with bus-expiry.

In order to workaround this issue, we can test for the correct combination of
BIT[15:14] by sending START TRANSFER commands with different values of
BIT[15:14]: ‘b00, ‘b01, ‘b10, and ‘b11. Each combination is 2^14 uframe apart
(or 2 seconds). 4 seconds into the future will result in a bus-expiry status.
As the result, within the 4 possible combinations for BIT[15:14], there will
be 2 successful and 2 failure START COMMAND status. One of the 2 successful
command status will result in a 2-second delay start. The smaller BIT[15:14]
value is the correct combination.

Since there are only 4 outcomes and the results are ordered, we can simply
test 2 START TRANSFER commands with BIT[15:14] combinations ‘b00 and ‘b01 to
deduce the smaller successful combination.

Let test0 = test status for combination ‘b00 and test1 = test status for ‘b01
of BIT[15:14]. The correct combination is as follow:

if test0 fails and test1 passes, BIT[15:14] is ‘b01
if test0 fails and test1 fails, BIT[15:14] is ‘b10
if test0 passes and test1 fails, BIT[15:14] is ‘b11
if test0 passes and test1 passes, BIT[15:14] is ‘b00

Synopsys STAR 9001202023: Wrong microframe number for isochronous IN
endpoints.

void dwc3\_gadget\_setup\_nump(struct [dwc3](#c.dwc3 "dwc3") \*dwc)
:   calculate and initialize NUMP field of `DWC3_DCFG`

**Parameters**

`struct dwc3 *dwc`
:   pointer to our context structure

**Description**

The following looks like complex but it’s actually very simple. In order to
calculate the number of packets we can burst at once on OUT transfers, we’re
gonna use RxFIFO size.

To calculate RxFIFO size we need two numbers:
MDWIDTH = size, in bits, of the internal memory bus
RAM2\_DEPTH = depth, in MDWIDTH, of internal RAM2 (where RxFIFO sits)

Given these two numbers, the formula is simple:

RxFIFO Size = (RAM2\_DEPTH \* MDWIDTH / 8) - 24 - 16;

24 bytes is for 3x SETUP packets
16 bytes is a clock domain crossing tolerance

Given RxFIFO Size, NUMP = RxFIFOSize / 1024;

int dwc3\_gadget\_check\_config(struct [usb\_gadget](gadget.html#c.usb_gadget "usb_gadget") \*g)
:   ensure dwc3 can support the USB configuration

**Parameters**

`struct usb_gadget *g`
:   pointer to the USB gadget

**Description**

Used to record the maximum number of endpoints being used in a USB composite
device. (across all configurations) This is to be used in the calculation
of the TXFIFO sizes when resizing internal memory for individual endpoints.
It will help ensured that the resizing logic reserves enough space for at
least one max packet.

int dwc3\_get\_dr\_mode(struct [dwc3](#c.dwc3 "dwc3") \*dwc)
:   Validates and sets dr\_mode

**Parameters**

`struct dwc3 *dwc`
:   pointer to our context structure

int dwc3\_core\_soft\_reset(struct [dwc3](#c.dwc3 "dwc3") \*dwc)
:   Issues core soft reset and PHY reset

**Parameters**

`struct dwc3 *dwc`
:   pointer to our context structure

void dwc3\_ref\_clk\_period(struct [dwc3](#c.dwc3 "dwc3") \*dwc)
:   Reference clock period configuration Default reference clock period depends on hardware configuration. For systems with reference clock that differs from the default, this will set clock period in DWC3\_GUCTL register.

**Parameters**

`struct dwc3 *dwc`
:   Pointer to our controller context structure

void dwc3\_free\_one\_event\_buffer(struct [dwc3](#c.dwc3 "dwc3") \*dwc, struct [dwc3\_event\_buffer](#c.dwc3_event_buffer "dwc3_event_buffer") \*evt)
:   Frees one event buffer

**Parameters**

`struct dwc3 *dwc`
:   Pointer to our controller context structure

`struct dwc3_event_buffer *evt`
:   Pointer to event buffer to be freed

struct [dwc3\_event\_buffer](#c.dwc3_event_buffer "dwc3_event_buffer") \*dwc3\_alloc\_one\_event\_buffer(struct [dwc3](#c.dwc3 "dwc3") \*dwc, unsigned int length)
:   Allocates one event buffer structure

**Parameters**

`struct dwc3 *dwc`
:   Pointer to our controller context structure

`unsigned int length`
:   size of the event buffer

**Description**

Returns a pointer to the allocated event buffer structure on success
otherwise ERR\_PTR(errno).

void dwc3\_free\_event\_buffers(struct [dwc3](#c.dwc3 "dwc3") \*dwc)
:   frees all allocated event buffers

**Parameters**

`struct dwc3 *dwc`
:   Pointer to our controller context structure

int dwc3\_alloc\_event\_buffers(struct [dwc3](#c.dwc3 "dwc3") \*dwc, unsigned int length)
:   Allocates **num** event buffers of size **length**

**Parameters**

`struct dwc3 *dwc`
:   pointer to our controller context structure

`unsigned int length`
:   size of event buffer

**Description**

Returns 0 on success otherwise negative errno. In the error case, dwc
may contain some buffers allocated but not all which were requested.

int dwc3\_event\_buffers\_setup(struct [dwc3](#c.dwc3 "dwc3") \*dwc)
:   setup our allocated event buffers

**Parameters**

`struct dwc3 *dwc`
:   pointer to our controller context structure

**Description**

Returns 0 on success otherwise negative errno.

int dwc3\_phy\_setup(struct [dwc3](#c.dwc3 "dwc3") \*dwc)
:   Configure USB PHY Interface of DWC3 Core

**Parameters**

`struct dwc3 *dwc`
:   Pointer to our controller context structure

**Description**

Returns 0 on success. The USB PHY interfaces are configured but not
initialized. The PHY interfaces and the PHYs get initialized together with
the core in dwc3\_core\_init.

[1]
([1](#id1),[2](#id4),[3](#id6))

Transfer Request Block


[[2](#id7)]

Transfer Request Block pointing to another Transfer
Request Block.


[3]
([1](#id2),[2](#id8))

The Debug File System


[[4](#id3)]

The Config File System


[[5](#id5)]

Command Block Wrapper
