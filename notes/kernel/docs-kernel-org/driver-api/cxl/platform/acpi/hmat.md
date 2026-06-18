# HMAT - Heterogeneous Memory Attribute Table

> 출처(원문): https://docs.kernel.org/driver-api/cxl/platform/acpi/hmat.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# HMAT - Heterogeneous Memory Attribute Table

The Heterogeneous Memory Attributes Table contains information such as cache
attributes and bandwidth and latency details for memory proximity domains.
For the purpose of this document, we will only discuss the SSLIB entry.

## SLLBI

The System Locality Latency and Bandwidth Information records latency and
bandwidth information for proximity domains.

This table is used by Linux to configure interleave weights and memory tiers.

Example (Heavily truncated for brevity)

```
              Structure Type : 0001 [SLLBI]
                   Data Type : 00         <- Latency
Target Proximity Domain List : 00000000
Target Proximity Domain List : 00000001
                       Entry : 0080       <- DRAM LTC
                       Entry : 0100       <- CXL LTC

              Structure Type : 0001 [SLLBI]
                   Data Type : 03         <- Bandwidth
Target Proximity Domain List : 00000000
Target Proximity Domain List : 00000001
                       Entry : 1200       <- DRAM BW
                       Entry : 0200       <- CXL BW
```
