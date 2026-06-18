# Block layer support for Persistent Reservations

> 출처(원문): https://docs.kernel.org/block/pr.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Block layer support for Persistent Reservations

The Linux kernel supports a user space interface for simplified
Persistent Reservations which map to block devices that support
these (like SCSI). Persistent Reservations allow restricting
access to block devices to specific initiators in a shared storage
setup.

This document gives a general overview of the support ioctl commands.
For a more detailed reference please refer to the SCSI Primary
Commands standard, specifically the section on Reservations and the
“PERSISTENT RESERVE IN” and “PERSISTENT RESERVE OUT” commands.

All implementations are expected to ensure the reservations survive
a power loss and cover all connections in a multi path environment.
These behaviors are optional in SPC but will be automatically applied
by Linux.

## The following types of reservations are supported:

> * PR\_WRITE\_EXCLUSIVE
>   :   Only the initiator that owns the reservation can write to the
>       device. Any initiator can read from the device.
> * PR\_EXCLUSIVE\_ACCESS
>   :   Only the initiator that owns the reservation can access the
>       device.
> * PR\_WRITE\_EXCLUSIVE\_REG\_ONLY
>   :   Only initiators with a registered key can write to the device,
>       Any initiator can read from the device.
> * PR\_EXCLUSIVE\_ACCESS\_REG\_ONLY
>   :   Only initiators with a registered key can access the device.
> * PR\_WRITE\_EXCLUSIVE\_ALL\_REGS
>
>   > Only initiators with a registered key can write to the device,
>   > Any initiator can read from the device.
>   > All initiators with a registered key are considered reservation
>   > holders.
>   > Please reference the SPC spec on the meaning of a reservation
>   > holder if you want to use this type.
> * PR\_EXCLUSIVE\_ACCESS\_ALL\_REGS
>   :   Only initiators with a registered key can access the device.
>       All initiators with a registered key are considered reservation
>       holders.
>       Please reference the SPC spec on the meaning of a reservation
>       holder if you want to use this type.

## The following ioctl are supported:

### 1. IOC\_PR\_REGISTER

This ioctl command registers a new reservation if the new\_key argument
is non-null. If no existing reservation exists old\_key must be zero,
if an existing reservation should be replaced old\_key must contain
the old reservation key.

If the new\_key argument is 0 it unregisters the existing reservation passed
in old\_key.

### 2. IOC\_PR\_RESERVE

This ioctl command reserves the device and thus restricts access for other
devices based on the type argument. The key argument must be the existing
reservation key for the device as acquired by the IOC\_PR\_REGISTER,
IOC\_PR\_REGISTER\_IGNORE, IOC\_PR\_PREEMPT or IOC\_PR\_PREEMPT\_ABORT commands.

### 3. IOC\_PR\_RELEASE

This ioctl command releases the reservation specified by key and flags
and thus removes any access restriction implied by it.

### 4. IOC\_PR\_PREEMPT

This ioctl command releases the existing reservation referred to by
old\_key and replaces it with a new reservation of type for the
reservation key new\_key.

### 5. IOC\_PR\_PREEMPT\_ABORT

This ioctl command works like IOC\_PR\_PREEMPT except that it also aborts
any outstanding command sent over a connection identified by old\_key.

### 6. IOC\_PR\_CLEAR

This ioctl command unregisters both key and any other reservation key
registered with the device and drops any existing reservation.

## Flags

All the ioctls have a flag field. Currently only one flag is supported:

> * PR\_FL\_IGNORE\_KEY
>   :   Ignore the existing reservation key. This is commonly supported for
>       IOC\_PR\_REGISTER, and some implementation may support the flag for
>       IOC\_PR\_RESERVE.

For all unknown flags the kernel will return -EOPNOTSUPP.
