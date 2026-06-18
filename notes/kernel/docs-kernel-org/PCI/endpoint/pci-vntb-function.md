# 9.8.PCI vNTB Function

> 출처(원문): https://docs.kernel.org/PCI/endpoint/pci-vntb-function.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 9.8. PCI vNTB Function

Author:
:   Frank Li <[Frank.Li@nxp.com](mailto:Frank.Li%40nxp.com)>

The difference between PCI NTB function and PCI vNTB function is

PCI NTB function need at two endpoint instances and connect HOST1
and HOST2.

PCI vNTB function only use one host and one endpoint(EP), use NTB
connect EP and PCI host

```
+------------+         +---------------------------------------+
|            |         |                                       |
+------------+         |                        +--------------+
| NTB        |         |                        | NTB          |
| NetDev     |         |                        | NetDev       |
+------------+         |                        +--------------+
| NTB        |         |                        | NTB          |
| Transfer   |         |                        | Transfer     |
+------------+         |                        +--------------+
|            |         |                        |              |
|  PCI NTB   |         |                        |              |
|    EPF     |         |                        |              |
|   Driver   |         |                        | PCI Virtual  |
|            |         +---------------+        | NTB Driver   |
|            |         | PCI EP NTB    |<------>|              |
|            |         |  FN Driver    |        |              |
+------------+         +---------------+        +--------------+
|            |         |               |        |              |
|  PCI BUS   | <-----> |  PCI EP BUS   |        |  Virtual PCI |
|            |  PCI    |               |        |     BUS      |
+------------+         +---------------+--------+--------------+
    PCI RC                        PCI EP
```

## 9.8.1. Constructs used for Implementing vNTB

> 1. Config Region
> 2. Self Scratchpad Registers
> 3. Peer Scratchpad Registers
> 4. Doorbell (DB) Registers
> 5. Memory Window (MW)

### 9.8.1.1. Config Region:

It is same as PCI NTB Function driver

### 9.8.1.2. Scratchpad Registers:

It is appended after Config region.

```
+--------------------------------------------------+ Base
|                                                  |
|                                                  |
|                                                  |
|          Common Config Register                  |
|                                                  |
|                                                  |
|                                                  |
+-----------------------+--------------------------+ Base + span_offset
|                       |                          |
|    Peer Span Space    |    Span Space            |
|                       |                          |
|                       |                          |
+-----------------------+--------------------------+ Base + span_offset
|                       |                          |      + span_count * 4
|                       |                          |
|     Span Space        |   Peer Span Space        |
|                       |                          |
+-----------------------+--------------------------+
      Virtual PCI             Pcie Endpoint
      NTB Driver               NTB Driver
```

### 9.8.1.3. Doorbell Registers:

> Doorbell Registers are used by the hosts to interrupt each other.

### 9.8.1.4. Memory Window:

> Actual transfer of data between the two hosts will happen using the
> memory window.

## 9.8.2. Modeling Constructs:

32-bit BARs.

| BAR NO | CONSTRUCTS USED |
| --- | --- |
| BAR0 | Config Region |
| BAR1 | Doorbell |
| BAR2 | Memory Window 1 |
| BAR3 | Memory Window 2 |
| BAR4 | Memory Window 3 |
| BAR5 | Memory Window 4 |

64-bit BARs.

| BAR NO | CONSTRUCTS USED |
| --- | --- |
| BAR0 | Config Region + Scratchpad |
| BAR1 |  |
| BAR2 | Doorbell |
| BAR3 |  |
| BAR4 | Memory Window 1 |
| BAR5 |  |
