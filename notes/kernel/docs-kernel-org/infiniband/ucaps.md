# Infiniband Userspace Capabilities

> 출처(원문): https://docs.kernel.org/infiniband/ucaps.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Infiniband Userspace Capabilities

> User CAPabilities (UCAPs) provide fine-grained control over specific
> firmware features in Infiniband (IB) devices. This approach offers
> more granular capabilities than the existing Linux capabilities,
> which may be too generic for certain FW features.
>
> Each user capability is represented as a character device with root
> read-write access. Root processes can grant users special privileges
> by allowing access to these character devices (e.g., using chown).

## Usage

> UCAPs allow control over specific features of an IB device using file
> descriptors of UCAP character devices. Here is how a user enables
> specific features of an IB device:
>
> > * A root process grants the user access to the UCAP files that
> >   represents the capabilities (e.g., using chown).
> > * The user opens the UCAP files, obtaining file descriptors.
> > * When opening an IB device, include an array of the UCAP file
> >   descriptors as an attribute.
> > * The ib\_uverbs driver recognizes the UCAP file descriptors and enables
> >   the corresponding capabilities for the IB device.

## Creating UCAPs

> To create a new UCAP, drivers must first define a type in the
> rdma\_user\_cap `enum in` rdma/ib\_ucaps.h. The name of the UCAP character
> device should be added to the ucap\_names array in
> drivers/infiniband/core/ucaps.c. Then, the driver can create the UCAP
> character device by calling the ib\_create\_ucap API with the UCAP
> type.
>
> A reference count is stored for each UCAP to track creations and
> removals of the UCAP device. If multiple creation calls are made with
> the same type (e.g., for two IB devices), the UCAP character device
> is created during the first call and subsequent calls increment the
> reference count.
>
> The UCAP character device is created under /dev/infiniband, and its
> permissions are set to allow root read and write access only.

## Removing UCAPs

> Each removal decrements the reference count of the UCAP. The UCAP
> character device is removed from the filesystem only when the
> reference count is decreased to 0.

## /dev and /sys/class files

> The class:
>
> ```
> /sys/class/infiniband_ucaps
> ```
>
> is created when the first UCAP character device is created.
>
> The UCAP character device is created under /dev/infiniband.
>
> For example, if mlx5\_ib adds the rdma\_user\_cap
> RDMA\_UCAP\_MLX5\_CTRL\_LOCAL with name “mlx5\_perm\_ctrl\_local”, this will
> create the device node:
>
> ```
> /dev/infiniband/mlx5_perm_ctrl_local
> ```
