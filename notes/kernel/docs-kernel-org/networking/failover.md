# FAILOVER

> 출처(원문): https://docs.kernel.org/networking/failover.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# FAILOVER

## Overview

The failover module provides a generic interface for paravirtual drivers
to register a netdev and a set of ops with a failover instance. The ops
are used as event handlers that get called to handle netdev register/
unregister/link change/name change events on slave pci ethernet devices
with the same mac address as the failover netdev.

This enables paravirtual drivers to use a VF as an accelerated low latency
datapath. It also allows live migration of VMs with direct attached VFs by
failing over to the paravirtual datapath when the VF is unplugged.
