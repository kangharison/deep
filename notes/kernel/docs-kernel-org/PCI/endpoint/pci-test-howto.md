# 9.5.PCI Test User Guide

> 출처(원문): https://docs.kernel.org/PCI/endpoint/pci-test-howto.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 9.5. PCI Test User Guide

Author:
:   Kishon Vijay Abraham I <[kishon@ti.com](mailto:kishon%40ti.com)>

This document is a guide to help users use pci-epf-test function driver
and pci\_endpoint\_test host driver for testing PCI. The list of steps to
be followed in the host side and EP side is given below.

## 9.5.1. Endpoint Device

### 9.5.1.1. Endpoint Controller Devices

To find the list of endpoint controller devices in the system:

```
# ls /sys/class/pci_epc/
  51000000.pcie_ep
```

If PCI\_ENDPOINT\_CONFIGFS is enabled:

```
# ls /sys/kernel/config/pci_ep/controllers
  51000000.pcie_ep
```

### 9.5.1.2. Endpoint Function Drivers

To find the list of endpoint function drivers in the system:

```
# ls /sys/bus/pci-epf/drivers
  pci_epf_test
```

If PCI\_ENDPOINT\_CONFIGFS is enabled:

```
# ls /sys/kernel/config/pci_ep/functions
  pci_epf_test
```

### 9.5.1.3. Creating pci-epf-test Device

PCI endpoint function device can be created using the configfs. To create
pci-epf-test device, the following commands can be used:

```
# mount -t configfs none /sys/kernel/config
# cd /sys/kernel/config/pci_ep/
# mkdir functions/pci_epf_test/func1
```

The “mkdir func1” above creates the pci-epf-test function device that will
be probed by pci\_epf\_test driver.

The PCI endpoint framework populates the directory with the following
configurable fields:

```
# ls functions/pci_epf_test/func1
  baseclass_code        interrupt_pin   progif_code     subsys_id
  cache_line_size       msi_interrupts  revid           subsys_vendorid
  deviceid              msix_interrupts subclass_code   vendorid
```

The PCI endpoint function driver populates these entries with default values
when the device is bound to the driver. The pci-epf-test driver populates
vendorid with 0xffff and interrupt\_pin with 0x0001:

```
# cat functions/pci_epf_test/func1/vendorid
  0xffff
# cat functions/pci_epf_test/func1/interrupt_pin
  0x0001
```

### 9.5.1.4. Configuring pci-epf-test Device

The user can configure the pci-epf-test device using configfs entry. In order
to change the vendorid and the number of MSI interrupts used by the function
device, the following commands can be used:

```
# echo 0x104c > functions/pci_epf_test/func1/vendorid
# echo 0xb500 > functions/pci_epf_test/func1/deviceid
# echo 32 > functions/pci_epf_test/func1/msi_interrupts
# echo 2048 > functions/pci_epf_test/func1/msix_interrupts
```

By default, pci-epf-test uses the following BAR sizes:

```
# grep . functions/pci_epf_test/func1/pci_epf_test.0/bar?_size
  functions/pci_epf_test/func1/pci_epf_test.0/bar0_size:131072
  functions/pci_epf_test/func1/pci_epf_test.0/bar1_size:131072
  functions/pci_epf_test/func1/pci_epf_test.0/bar2_size:131072
  functions/pci_epf_test/func1/pci_epf_test.0/bar3_size:131072
  functions/pci_epf_test/func1/pci_epf_test.0/bar4_size:131072
  functions/pci_epf_test/func1/pci_epf_test.0/bar5_size:1048576
```

The user can override a default value using e.g.::
:   # echo 1048576 > functions/pci\_epf\_test/func1/pci\_epf\_test.0/bar1\_size

Overriding the default BAR sizes can only be done before binding the
pci-epf-test device to a PCI endpoint controller driver.

Note: Some endpoint controllers might have fixed-size BARs or reserved BARs;
for such controllers, the corresponding BAR size in configfs will be ignored.

### 9.5.1.5. Binding pci-epf-test Device to EP Controller

In order for the endpoint function device to be useful, it has to be bound to
a PCI endpoint controller driver. Use the configfs to bind the function
device to one of the controller driver present in the system:

```
# ln -s functions/pci_epf_test/func1 controllers/51000000.pcie_ep/
```

Once the above step is completed, the PCI endpoint is ready to establish a link
with the host.

### 9.5.1.6. Start the Link

In order for the endpoint device to establish a link with the host, the \_start\_
field should be populated with ‘1’:

```
# echo 1 > controllers/51000000.pcie_ep/start
```

## 9.5.2. RootComplex Device

### 9.5.2.1. lspci Output

Note that the devices listed here correspond to the value populated in 1.4
above:

```
00:00.0 PCI bridge: Texas Instruments Device 8888 (rev 01)
01:00.0 Unassigned class [ff00]: Texas Instruments Device b500
```

### 9.5.2.2. Using Endpoint Test function Device

Kselftest added in tools/testing/selftests/pci\_endpoint can be used to run all
the default PCI endpoint tests. To build the Kselftest for PCI endpoint
subsystem, the following commands should be used:

```
# cd <kernel-dir>
# make -C tools/testing/selftests/pci_endpoint
```

or if you desire to compile and install in your system:

```
# cd <kernel-dir>
# make -C tools/testing/selftests/pci_endpoint INSTALL_PATH=/usr/bin install
```

The test will be located in <rootfs>/usr/bin/

#### 9.5.2.2.1. Kselftest Output

```
# pci_endpoint_test
TAP version 13
1..16
# Starting 16 tests from 9 test cases.
#  RUN           pci_ep_bar.BAR0.BAR_TEST ...
#            OK  pci_ep_bar.BAR0.BAR_TEST
ok 1 pci_ep_bar.BAR0.BAR_TEST
#  RUN           pci_ep_bar.BAR1.BAR_TEST ...
#            OK  pci_ep_bar.BAR1.BAR_TEST
ok 2 pci_ep_bar.BAR1.BAR_TEST
#  RUN           pci_ep_bar.BAR2.BAR_TEST ...
#            OK  pci_ep_bar.BAR2.BAR_TEST
ok 3 pci_ep_bar.BAR2.BAR_TEST
#  RUN           pci_ep_bar.BAR3.BAR_TEST ...
#            OK  pci_ep_bar.BAR3.BAR_TEST
ok 4 pci_ep_bar.BAR3.BAR_TEST
#  RUN           pci_ep_bar.BAR4.BAR_TEST ...
#            OK  pci_ep_bar.BAR4.BAR_TEST
ok 5 pci_ep_bar.BAR4.BAR_TEST
#  RUN           pci_ep_bar.BAR5.BAR_TEST ...
#            OK  pci_ep_bar.BAR5.BAR_TEST
ok 6 pci_ep_bar.BAR5.BAR_TEST
#  RUN           pci_ep_basic.CONSECUTIVE_BAR_TEST ...
#            OK  pci_ep_basic.CONSECUTIVE_BAR_TEST
ok 7 pci_ep_basic.CONSECUTIVE_BAR_TEST
#  RUN           pci_ep_basic.LEGACY_IRQ_TEST ...
#            OK  pci_ep_basic.LEGACY_IRQ_TEST
ok 8 pci_ep_basic.LEGACY_IRQ_TEST
#  RUN           pci_ep_basic.MSI_TEST ...
#            OK  pci_ep_basic.MSI_TEST
ok 9 pci_ep_basic.MSI_TEST
#  RUN           pci_ep_basic.MSIX_TEST ...
#            OK  pci_ep_basic.MSIX_TEST
ok 10 pci_ep_basic.MSIX_TEST
#  RUN           pci_ep_data_transfer.memcpy.READ_TEST ...
#            OK  pci_ep_data_transfer.memcpy.READ_TEST
ok 11 pci_ep_data_transfer.memcpy.READ_TEST
#  RUN           pci_ep_data_transfer.memcpy.WRITE_TEST ...
#            OK  pci_ep_data_transfer.memcpy.WRITE_TEST
ok 12 pci_ep_data_transfer.memcpy.WRITE_TEST
#  RUN           pci_ep_data_transfer.memcpy.COPY_TEST ...
#            OK  pci_ep_data_transfer.memcpy.COPY_TEST
ok 13 pci_ep_data_transfer.memcpy.COPY_TEST
#  RUN           pci_ep_data_transfer.dma.READ_TEST ...
#            OK  pci_ep_data_transfer.dma.READ_TEST
ok 14 pci_ep_data_transfer.dma.READ_TEST
#  RUN           pci_ep_data_transfer.dma.WRITE_TEST ...
#            OK  pci_ep_data_transfer.dma.WRITE_TEST
ok 15 pci_ep_data_transfer.dma.WRITE_TEST
#  RUN           pci_ep_data_transfer.dma.COPY_TEST ...
#            OK  pci_ep_data_transfer.dma.COPY_TEST
ok 16 pci_ep_data_transfer.dma.COPY_TEST
# PASSED: 16 / 16 tests passed.
# Totals: pass:16 fail:0 xfail:0 xpass:0 skip:0 error:0
```

Testcase 16 (pci\_ep\_data\_transfer.dma.COPY\_TEST) will fail for most of the DMA
capable endpoint controllers due to the absence of the MEMCPY over DMA. For such
controllers, it is advisable to skip this testcase using this
command:

```
# pci_endpoint_test -f pci_ep_bar -f pci_ep_basic -v memcpy -T COPY_TEST -v dma
```

#### 9.5.2.2.2. Kselftest EP Doorbell

If the Endpoint MSI controller is used for the doorbell usecase, run below
command for testing it:

> # pci\_endpoint\_test -f pcie\_ep\_doorbell
>
> # Starting 1 tests from 1 test cases.
> # RUN pcie\_ep\_doorbell.DOORBELL\_TEST ...
> # OK pcie\_ep\_doorbell.DOORBELL\_TEST
> ok 1 pcie\_ep\_doorbell.DOORBELL\_TEST
> # PASSED: 1 / 1 tests passed.
> # Totals: pass:1 fail:0 xfail:0 xpass:0 skip:0 error:0
