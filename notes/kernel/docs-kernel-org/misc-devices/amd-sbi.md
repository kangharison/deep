# AMD SIDE BAND interface

> 출처(원문): https://docs.kernel.org/misc-devices/amd-sbi.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# AMD SIDE BAND interface

Some AMD Zen based processors supports system management
functionality via side-band interface (SBI) called
Advanced Platform Management Link (APML). APML is an I2C/I3C
based 2-wire processor target interface. APML is used to
communicate with the Remote Management Interface
(SB Remote Management Interface (SB-RMI)
and SB Temperature Sensor Interface (SB-TSI)).

More details on the interface can be found in chapter
“5 Advanced Platform Management Link (APML)” of the family/model PPR [[1]](#id2).

[[1](#id1)]

<https://docs.amd.com/v/u/en-US/55898_B1_pub_0_50>

## SBRMI device

apml\_sbrmi driver under the drivers/misc/amd-sbi creates miscdevice
/dev/sbrmi-\* to let user space programs run APML mailbox, CPUID,
MCAMSR and register xfer commands.

Register sets is common across APML protocols. IOCTL is providing synchronization
among protocols as transactions may create race condition.

```
$ ls -al /dev/sbrmi-3c
crw-------    1 root     root       10,  53 Jul 10 11:13 /dev/sbrmi-3c
```

apml\_sbrmi driver registers hwmon sensors for monitoring power\_cap\_max,
current power consumption and managing power\_cap.

Characteristics of the dev node:
:   * Differnet xfer protocols are defined:
      :   + Mailbox
          + CPUID
          + MCA\_MSR
          + Register xfer

Access restrictions:
:   * Only root user is allowed to open the file.
    * APML Mailbox messages and Register xfer access are read-write,
    * CPUID and MCA\_MSR access is read-only.

## Driver IOCTLs

SBRMI\_IOCTL\_MBOX\_CMD

**Parameters**

**struct** apml\_mbox\_msg
:   Pointer to the `struct apml_mbox_msg` that will contain the protocol
    information

**Description**
IOCTL command for APML messages using generic \_IOWR
The IOCTL provides userspace access to AMD sideband mailbox protocol
- Mailbox message read/write(0x0~0xFF)
- returning “-EFAULT” if none of the above
“-EPROTOTYPE” error is returned to provide additional error details

SBRMI\_IOCTL\_CPUID\_CMD

**Parameters**

**struct** apml\_cpuid\_msg
:   Pointer to the `struct apml_cpuid_msg` that will contain the protocol
    information

**Description**
IOCTL command for APML messages using generic \_IOWR
The IOCTL provides userspace access to AMD sideband cpuid protocol
- CPUID protocol to get CPU details for Function/Ext Function
at thread level
- returning “-EFAULT” if none of the above
“-EPROTOTYPE” error is returned to provide additional error details

SBRMI\_IOCTL\_MCAMSR\_CMD

**Parameters**

**struct** apml\_mcamsr\_msg
:   Pointer to the `struct apml_mcamsr_msg` that will contain the protocol
    information

**Description**
IOCTL command for APML messages using generic \_IOWR
The IOCTL provides userspace access to AMD sideband MCAMSR protocol
- MCAMSR protocol to get MCA bank details for Function at thread level
- returning “-EFAULT” if none of the above
“-EPROTOTYPE” error is returned to provide additional error details

SBRMI\_IOCTL\_REG\_XFER\_CMD

**Parameters**

**struct** apml\_reg\_xfer\_msg
:   Pointer to the `struct apml_reg_xfer_msg` that will contain the protocol
    information

**Description**
IOCTL command for APML messages using generic \_IOWR
The IOCTL provides userspace access to AMD sideband register xfer protocol
- Register xfer protocol to get/set hardware register for given offset

## User-space usage

To access side band interface from a C program.
First, user need to include the headers:

```
#include <uapi/misc/amd-apml.h>
```

Which defines the supported IOCTL and data structure to be passed
from the user space.

Next thing, open the device file, as follows:

```
int file;

file = open("/dev/sbrmi-*", O_RDWR);
if (file < 0) {
  /* ERROR HANDLING */
  exit(1);
}
```

The following IOCTLs are defined:

`#define SB_BASE_IOCTL_NR              0xF9`
`#define SBRMI_IOCTL_MBOX_CMD          _IOWR(SB_BASE_IOCTL_NR, 0, struct apml_mbox_msg)`
`#define SBRMI_IOCTL_CPUID_CMD         _IOWR(SB_BASE_IOCTL_NR, 1, struct apml_cpuid_msg)`
`#define SBRMI_IOCTL_MCAMSR_CMD        _IOWR(SB_BASE_IOCTL_NR, 2, struct apml_mcamsr_msg)`
`#define SBRMI_IOCTL_REG_XFER_CMD      _IOWR(SB_BASE_IOCTL_NR, 3, struct apml_reg_xfer_msg)`

User space C-APIs are made available by esmi\_oob\_library, hosted at
[[2]](#id5) which is provided by the E-SMS project [[3]](#id6).

[[2](#id3)]

<https://github.com/amd/esmi_oob_library>


[[3](#id4)]

<https://www.amd.com/en/developer/e-sms.html>
