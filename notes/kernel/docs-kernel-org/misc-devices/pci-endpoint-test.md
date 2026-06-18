# Driver for PCI Endpoint Test Function

> 출처(원문): https://docs.kernel.org/misc-devices/pci-endpoint-test.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Driver for PCI Endpoint Test Function

This driver should be used as a host side driver if the root complex is
connected to a configurable PCI endpoint running `pci_epf_test` function
driver configured according to [[1]](#id2).

The “pci\_endpoint\_test” driver can be used to perform the following tests.

The PCI driver for the test device performs the following tests:

> 1. verifying addresses programmed in BAR
> 2. raise legacy IRQ
> 3. raise MSI IRQ
> 4. raise MSI-X IRQ
> 5. read data
> 6. write data
> 7. copy data

This misc driver creates /dev/pci-endpoint-test.<num> for every
`pci_epf_test` function connected to the root complex and “ioctls”
should be used to perform the above tests.

## ioctl

> PCITEST\_BAR:
> :   Tests the BAR. The number of the BAR to be tested
>     should be passed as argument.
>
> PCITEST\_LEGACY\_IRQ:
> :   Tests legacy IRQ
>
> PCITEST\_MSI:
> :   Tests message signalled interrupts. The MSI number
>     to be tested should be passed as argument.
>
> PCITEST\_MSIX:
> :   Tests message signalled interrupts. The MSI-X number
>     to be tested should be passed as argument.
>
> PCITEST\_SET\_IRQTYPE:
> :   Changes driver IRQ type configuration. The IRQ type
>     should be passed as argument (0: Legacy, 1:MSI, 2:MSI-X).
>
> PCITEST\_GET\_IRQTYPE:
> :   Gets driver IRQ type configuration.
>
> PCITEST\_WRITE:
> :   Perform write tests. The size of the buffer should be passed
>     as argument.
>
> PCITEST\_READ:
> :   Perform read tests. The size of the buffer should be passed
>     as argument.
>
> PCITEST\_COPY:
> :   Perform read tests. The size of the buffer should be passed
>     as argument.

[[1](#id1)]

[PCI Test Endpoint Function](../PCI/endpoint/function/binding/pci-test.html)
