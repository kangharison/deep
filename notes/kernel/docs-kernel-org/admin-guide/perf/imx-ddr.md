# Freescale i.MX8 DDR Performance Monitoring Unit (PMU)

> 출처(원문): https://docs.kernel.org/admin-guide/perf/imx-ddr.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Freescale i.MX8 DDR Performance Monitoring Unit (PMU)

There are no performance counters inside the DRAM controller, so performance
signals are brought out to the edge of the controller where a set of 4 x 32 bit
counters is implemented. This is controlled by the CSV modes programmed in counter
control register which causes a large number of PERF signals to be generated.

Selection of the value for each counter is done via the config registers. There
is one register for each counter. Counter 0 is special in that it always counts
“time” and when expired causes a lock on itself and the other counters and an
interrupt is raised. If any other counter overflows, it continues counting, and
no interrupt is raised.

The “format” directory describes format of the config (event ID) and config1/2
(AXI filter setting) fields of the perf\_event\_attr structure, see /sys/bus/event\_source/
devices/imx8\_ddr0/format/. The “events” directory describes the events types
hardware supported that can be used with perf tool, see /sys/bus/event\_source/
devices/imx8\_ddr0/events/. The “caps” directory describes filter features implemented
in DDR PMU, see /sys/bus/events\_source/devices/imx8\_ddr0/caps/.

> ```
> perf stat -a -e imx8_ddr0/cycles/ cmd
> perf stat -a -e imx8_ddr0/read/,imx8_ddr0/write/ cmd
> ```

AXI filtering is only used by CSV modes 0x41 (axid-read) and 0x42 (axid-write)
to count reading or writing matches filter setting. Filter setting is various
from different DRAM controller implementations, which is distinguished by quirks
in the driver. You also can dump info from userspace, “caps” directory show the
type of AXI filter (filter, enhanced\_filter and super\_filter). Value 0 for
un-supported, and value 1 for supported.

* With DDR\_CAP\_AXI\_ID\_FILTER quirk(filter: 1, enhanced\_filter: 0, super\_filter: 0).
  Filter is defined with two configuration parts:
  --AXI\_ID defines AxID matching value.
  --AXI\_MASKING defines which bits of AxID are meaningful for the matching.

  > + 0: corresponding bit is masked.
  > + 1: corresponding bit is not masked, i.e. used to do the matching.

  AXI\_ID and AXI\_MASKING are mapped on DPCR1 register in performance counter.
  When non-masked bits are matching corresponding AXI\_ID bits then counter is
  incremented. Perf counter is incremented if:

  ```
  AxID && AXI_MASKING == AXI_ID && AXI_MASKING
  ```

  This filter doesn’t support filter different AXI ID for axid-read and axid-write
  event at the same time as this filter is shared between counters.

  ```
  perf stat -a -e imx8_ddr0/axid-read,axi_mask=0xMMMM,axi_id=0xDDDD/ cmd
  perf stat -a -e imx8_ddr0/axid-write,axi_mask=0xMMMM,axi_id=0xDDDD/ cmd
  ```

  Note

  axi\_mask is inverted in userspace(i.e. set bits are bits to mask), and
  it will be reverted in driver automatically. so that the user can just specify
  axi\_id to monitor a specific id, rather than having to specify axi\_mask.

  ```
  perf stat -a -e imx8_ddr0/axid-read,axi_id=0x12/ cmd, which will monitor ARID=0x12
  ```
* With DDR\_CAP\_AXI\_ID\_FILTER\_ENHANCED quirk(filter: 1, enhanced\_filter: 1, super\_filter: 0).
  This is an extension to the DDR\_CAP\_AXI\_ID\_FILTER quirk which permits
  counting the number of bytes (as opposed to the number of bursts) from DDR
  read and write transactions concurrently with another set of data counters.
* With DDR\_CAP\_AXI\_ID\_PORT\_CHANNEL\_FILTER quirk(filter: 0, enhanced\_filter: 0, super\_filter: 1).
  There is a limitation in previous AXI filter, it cannot filter different IDs
  at the same time as the filter is shared between counters. This quirk is the
  extension of AXI ID filter. One improvement is that counter 1-3 has their own
  filter, means that it supports concurrently filter various IDs. Another
  improvement is that counter 1-3 supports AXI PORT and CHANNEL selection. Support
  selecting address channel or data channel.

  Filter is defined with 2 configuration registers per counter 1-3.
  --Counter N MASK COMP register - including AXI\_ID and AXI\_MASKING.
  --Counter N MUX CNTL register - including AXI CHANNEL and AXI PORT.

  > + 0: address channel
  > + 1: data channel

  PMU in DDR subsystem, only one single port0 exists, so axi\_port is reserved
  which should be 0.

  ```
  perf stat -a -e imx8_ddr0/axid-read,axi_mask=0xMMMM,axi_id=0xDDDD,axi_channel=0xH/ cmd
  perf stat -a -e imx8_ddr0/axid-write,axi_mask=0xMMMM,axi_id=0xDDDD,axi_channel=0xH/ cmd
  ```

  Note

  axi\_channel is inverted in userspace, and it will be reverted in driver
  automatically. So that users do not need specify axi\_channel if want to
  monitor data channel from DDR transactions, since data channel is more
  meaningful.
