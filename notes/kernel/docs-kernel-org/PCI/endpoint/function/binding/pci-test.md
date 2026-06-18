# 9.11.PCI Test Endpoint Function

> 출처(원문): https://docs.kernel.org/PCI/endpoint/function/binding/pci-test.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 9.11. PCI Test Endpoint Function

name: Should be “pci\_epf\_test” to bind to the pci\_epf\_test driver.

Configurable Fields:

|  |  |
| --- | --- |
| vendorid | should be 0x104c |
| deviceid | should be 0xb500 for DRA74x and 0xb501 for DRA72x |
| revid | don’t care |
| progif\_code | don’t care |
| subclass\_code | don’t care |
| baseclass\_code | should be 0xff |
| cache\_line\_size | don’t care |
| subsys\_vendor\_id | don’t care |
| subsys\_id | don’t care |
| interrupt\_pin | Should be 1 - INTA, 2 - INTB, 3 - INTC, 4 -INTD |
| msi\_interrupts | Should be 1 to 32 depending on the number of MSI interrupts to test |
| msix\_interrupts | Should be 1 to 2048 depending on the number of MSI-X interrupts to test |
