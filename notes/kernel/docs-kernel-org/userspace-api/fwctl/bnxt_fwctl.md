# fwctl bnxt driver

> 출처(원문): https://docs.kernel.org/userspace-api/fwctl/bnxt_fwctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# fwctl bnxt driver

Author:
:   Pavan Chebbi

## Overview

BNXT driver makes a fwctl service available through an auxiliary\_device.
The bnxt\_fwctl driver binds to this device and registers itself with the
fwctl subsystem.

The bnxt\_fwctl driver is agnostic to the device firmware internals. It
uses the Upper Layer Protocol (ULP) conduit provided by bnxt to send
HardWare Resource Manager (HWRM) commands to firmware.

These commands can query or change firmware driven device configurations
and read/write registers that are useful for debugging.

## bnxt\_fwctl User API

Each RPC request contains the HWRM input structure in the fwctl\_rpc
‘in’ buffer while ‘out’ will contain the response.

A typical user application can send a FWCTL\_INFO command using ioctl()
to discover bnxt\_fwctl’s RPC capabilities as shown below:

> ioctl(fd, FWCTL\_INFO, &fwctl\_info\_msg);

where fwctl\_info\_msg (of type [`struct fwctl_info`](fwctl.html#c.fwctl_info "fwctl_info")) describes bnxt\_info\_msg
(of type [`struct fwctl_info_bnxt`](fwctl.html#c.fwctl_info_bnxt "fwctl_info_bnxt")). fwctl\_info\_msg is set up as follows:

> size = sizeof([`struct fwctl_info`](fwctl.html#c.fwctl_info "fwctl_info"));
> flags = 0;
> device\_data\_len = sizeof(bnxt\_info\_msg);
> out\_device\_data = (\_\_aligned\_u64)&bnxt\_info\_msg;

The uctx\_caps of bnxt\_info\_msg represents the capabilities as described
in fwctl\_bnxt\_commands of include/uapi/fwctl/bnxt.h

The FW RPC itself, FWCTL\_RPC can be sent using ioctl() as:

> ioctl(fd, FWCTL\_RPC, &fwctl\_rpc\_msg);

where fwctl\_rpc\_msg (of type [`struct fwctl_rpc`](fwctl.html#c.fwctl_rpc "fwctl_rpc")) carries the HWRM command
in its ‘in’ buffer. The HWRM input structures are described in
include/linux/bnxt/hsi.h. An example for HWRM\_VER\_GET is shown below:

> `struct hwrm_ver_get_output` resp;
> [`struct fwctl_rpc`](fwctl.html#c.fwctl_rpc "fwctl_rpc") fwctl\_rpc\_msg;
> `struct hwrm_ver_get_input` req;
>
> req.req\_type = HWRM\_VER\_GET;
> req.hwrm\_intf\_maj = HWRM\_VERSION\_MAJOR;
> req.hwrm\_intf\_min = HWRM\_VERSION\_MINOR;
> req.hwrm\_intf\_upd = HWRM\_VERSION\_UPDATE;
> req.cmpl\_ring = -1;
> req.target\_id = -1;
>
> fwctl\_rpc\_msg.size = sizeof([`struct fwctl_rpc`](fwctl.html#c.fwctl_rpc "fwctl_rpc"));
> fwctl\_rpc\_msg.scope = FWCTL\_RPC\_DEBUG\_READ\_ONLY;
> fwctl\_rpc\_msg.in\_len = sizeof(req);
> fwctl\_rpc\_msg.out\_len = sizeof(resp);
> fwctl\_rpc\_msg.in = (\_\_aligned\_u64)&req;
> fwctl\_rpc\_msg.out = (\_\_aligned\_u64)&resp;

An example python3 program that can exercise this interface can be found in
the following git repository:

<https://github.com/Broadcom/fwctl-tools>
