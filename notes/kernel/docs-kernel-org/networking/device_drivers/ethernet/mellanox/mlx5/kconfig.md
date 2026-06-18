# Enabling the driver and kconfig options

> 출처(원문): https://docs.kernel.org/networking/device_drivers/ethernet/mellanox/mlx5/kconfig.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Enabling the driver and kconfig options

Copyright:
:   © 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

mlx5 core is modular and most of the major mlx5 core driver features can be selected (compiled in/out)

at build time via kernel Kconfig flags.

Basic features, ethernet net device rx/tx offloads and XDP, are available with the most basic flags

CONFIG\_MLX5\_CORE=y/m and CONFIG\_MLX5\_CORE\_EN=y.

For the list of advanced features, please see below.

**CONFIG\_MLX5\_BRIDGE=(y/n)**

Enable [Ethernet Bridging (BRIDGE) offloading support](switchdev.html#mlx5-bridge-offload).

This will provide the ability to add representors of mlx5 uplink and VF

ports to Bridge and offloading rules for traffic between such ports.

Supports VLANs (trunk and access modes).

**CONFIG\_MLX5\_CORE=(y/m/n)** (module mlx5\_core.ko)

The driver can be enabled by choosing CONFIG\_MLX5\_CORE=y/m in kernel config.

This will provide mlx5 core driver for mlx5 ulps to interface with (mlx5e, mlx5\_ib).

**CONFIG\_MLX5\_CORE\_EN=(y/n)**

Choosing this option will allow basic ethernet netdevice support with all of the standard rx/tx offloads.

mlx5e is the mlx5 ulp driver which provides netdevice kernel interface, when chosen, mlx5e will be

built-in into mlx5\_core.ko.

**CONFIG\_MLX5\_CORE\_EN\_DCB=(y/n)**:

Enables [Data Center Bridging (DCB) Support](https://enterprise-support.nvidia.com/s/article/howto-auto-config-pfc-and-ets-on-connectx-4-via-lldp-dcbx).

**CONFIG\_MLX5\_CORE\_IPOIB=(y/n)**

IPoIB offloads & acceleration support.

Requires CONFIG\_MLX5\_CORE\_EN to provide an accelerated interface for the rdma

IPoIB ulp netdevice.

**CONFIG\_MLX5\_CLS\_ACT=(y/n)**

Enables offload support for TC classifier action (NET\_CLS\_ACT).

Works in both native NIC mode and Switchdev SRIOV mode.

Flow-based classifiers, such as those registered through

tc-flower(8), are processed by the device, rather than the

host. Actions that would then overwrite matching classification

results would then be instant due to the offload.

**CONFIG\_MLX5\_EN\_ARFS=(y/n)**

Enables Hardware-accelerated receive flow steering (arfs) support, and ntuple filtering.

<https://enterprise-support.nvidia.com/s/article/howto-configure-arfs-on-connectx-4>

**CONFIG\_MLX5\_EN\_IPSEC=(y/n)**

Enables [IPSec XFRM cryptography-offload acceleration](../../../../xfrm/xfrm_device.html#xfrm-device).

**CONFIG\_MLX5\_MACSEC=(y/n)**

Build support for MACsec cryptography-offload acceleration in the NIC.

**CONFIG\_MLX5\_EN\_RXNFC=(y/n)**

Enables ethtool receive network flow classification, which allows user defined

flow rules to direct traffic into arbitrary rx queue via ethtool set/get\_rxnfc API.

**CONFIG\_MLX5\_EN\_TLS=(y/n)**

TLS cryptography-offload acceleration.

**CONFIG\_MLX5\_ESWITCH=(y/n)**

Ethernet SRIOV E-Switch support in ConnectX NIC. E-Switch provides internal SRIOV packet steering

and switching for the enabled VFs and PF in two available modes:

1) [Legacy SRIOV mode (L2 mac vlan steering based)](https://enterprise-support.nvidia.com/s/article/HowTo-Configure-SR-IOV-for-ConnectX-4-ConnectX-5-ConnectX-6-with-KVM-Ethernet).

2) [Switchdev mode (eswitch offloads)](../../../../switchdev.html#switchdev).

**CONFIG\_MLX5\_FPGA=(y/n)**

Build support for the Innova family of network cards by Mellanox Technologies.

Innova network cards are comprised of a ConnectX chip and an FPGA chip on one board.

If you select this option, the mlx5\_core driver will include the Innova FPGA core and allow

building sandbox-specific client drivers.

**CONFIG\_MLX5\_INFINIBAND=(y/n/m)** (module mlx5\_ib.ko)

Provides low-level InfiniBand/RDMA and [RoCE](https://enterprise-support.nvidia.com/s/article/recommended-network-configuration-examples-for-roce-deployment) support.

**CONFIG\_MLX5\_MPFS=(y/n)**

Ethernet Multi-Physical Function Switch (MPFS) support in ConnectX NIC.

MPFs is required for when [Multi-Host](https://www.nvidia.com/en-us/networking/multi-host/) configuration is enabled to allow passing

user configured unicast MAC addresses to the requesting PF.

**CONFIG\_MLX5\_SF=(y/n)**

Build support for subfunction.

Subfunctions are more light weight than PCI SRIOV VFs. Choosing this option

will enable support for creating subfunction devices.

**CONFIG\_MLX5\_SF\_MANAGER=(y/n)**

Build support for subfunction port in the NIC. A Mellanox subfunction

port is managed through devlink. A subfunction supports RDMA, netdevice

and vdpa device. It is similar to a SRIOV VF but it doesn’t require

SRIOV support.

**CONFIG\_MLX5\_SW\_STEERING=(y/n)**

Build support for software-managed steering in the NIC.

**CONFIG\_MLX5\_HW\_STEERING=(y/n)**

Build support for hardware-managed steering in the NIC.

**CONFIG\_MLX5\_TC\_CT=(y/n)**

Support offloading connection tracking rules via tc ct action.

**CONFIG\_MLX5\_TC\_SAMPLE=(y/n)**

Support offloading sample rules via tc sample action.

**CONFIG\_MLX5\_VDPA=(y/n)**

Support library for Mellanox VDPA drivers. Provides code that is

common for all types of VDPA drivers. The following drivers are planned:

net, block.

**CONFIG\_MLX5\_VDPA\_NET=(y/n)**

VDPA network driver for ConnectX6 and newer. Provides offloading

of virtio net datapath such that descriptors put on the ring will

be executed by the hardware. It also supports a variety of stateless

offloads depending on the actual device used and firmware version.

**CONFIG\_MLX5\_VFIO\_PCI=(y/n)**

This provides migration support for MLX5 devices using the VFIO framework.

**External options** ( Choose if the corresponding mlx5 feature is required )

* CONFIG\_MLXFW: When chosen, mlx5 firmware flashing support will be enabled (via devlink and ethtool).
* CONFIG\_PTP\_1588\_CLOCK: When chosen, mlx5 ptp support will be enabled
* CONFIG\_VXLAN: When chosen, mlx5 vxlan support will be enabled.
