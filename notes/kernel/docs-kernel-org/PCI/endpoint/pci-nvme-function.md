# 9.10.PCI NVMe Function

> 출처(원문): https://docs.kernel.org/PCI/endpoint/pci-nvme-function.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 9.10. PCI NVMe Function

Author:
:   Damien Le Moal <[dlemoal@kernel.org](mailto:dlemoal%40kernel.org)>

The PCI NVMe endpoint function implements a PCI NVMe controller using the NVMe
subsystem target core code. The driver for this function resides with the NVMe
subsystem as drivers/nvme/target/pci-epf.c.

See [NVMe PCI Endpoint Function Target](../../nvme/nvme-pci-endpoint-target.html) for more details.
