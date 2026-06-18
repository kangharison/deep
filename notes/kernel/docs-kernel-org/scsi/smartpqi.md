# SMARTPQI - Microchip Smart Storage SCSI driver

> 출처(원문): https://docs.kernel.org/scsi/smartpqi.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# SMARTPQI - Microchip Smart Storage SCSI driver

This file describes the smartpqi SCSI driver for Microchip
(<http://www.microchip.com>) PQI controllers. The smartpqi driver
is the next generation SCSI driver for Microchip Corp. The smartpqi
driver is the first SCSI driver to implement the PQI queuing model.

The smartpqi driver will replace the aacraid driver for Adaptec Series 9
controllers. Customers running an older kernel (Pre-4.9) using an Adaptec
Series 9 controller will have to configure the smartpqi driver or their
volumes will not be added to the OS.

For Microchip smartpqi controller support, enable the smartpqi driver
when configuring the kernel.

For more information on the PQI Queuing Interface, please see:

* <http://www.t10.org/drafts.htm>
* <http://www.t10.org/members/w_pqi2.htm>

## Supported devices

<Controller names to be added as they become publicly available.>

## smartpqi specific entries in /sys

### smartpqi host attributes

> * /sys/class/scsi\_host/host\*/rescan
> * /sys/class/scsi\_host/host\*/driver\_version
>
> The host rescan attribute is a write only attribute. Writing to this
> attribute will trigger the driver to scan for new, changed, or removed
> devices and notify the SCSI mid-layer of any changes detected.
>
> The version attribute is read-only and will return the driver version
> and the controller firmware version.
> For example:
>
> ```
> driver: 0.9.13-370
> firmware: 0.01-522
> ```

### smartpqi sas device attributes

> HBA devices are added to the SAS transport layer. These attributes are
> automatically added by the SAS transport layer.
>
> /sys/class/sas\_device/end\_device-X:X/sas\_address
> /sys/class/sas\_device/end\_device-X:X/enclosure\_identifier
> /sys/class/sas\_device/end\_device-X:X/scsi\_target\_id

## smartpqi specific ioctls

> For compatibility with applications written for the cciss protocol.
>
> CCISS\_DEREGDISK, CCISS\_REGNEWDISK, CCISS\_REGNEWD
> :   The above three ioctls all do exactly the same thing, which is to cause the driver
>     to rescan for new devices. This does exactly the same thing as writing to the
>     smartpqi specific host “rescan” attribute.
>
> CCISS\_GETPCIINFO
> :   Returns PCI domain, bus, device and function and “board ID” (PCI subsystem ID).
>
> CCISS\_GETDRIVVER
> :   Returns driver version in three bytes encoded as:
>
>     ```
>     (DRIVER_MAJOR << 28) | (DRIVER_MINOR << 24) | (DRIVER_RELEASE << 16) | DRIVER_REVISION;
>     ```
>
> CCISS\_PASSTHRU
> :   Allows “BMIC” and “CISS” commands to be passed through to the Smart Storage Array.
>     These are used extensively by the SSA Array Configuration Utility, SNMP storage
>     agents, etc.
