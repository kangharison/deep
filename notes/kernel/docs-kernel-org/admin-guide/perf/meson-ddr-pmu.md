# Amlogic SoC DDR Bandwidth Performance Monitoring Unit (PMU)

> 출처(원문): https://docs.kernel.org/admin-guide/perf/meson-ddr-pmu.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Amlogic SoC DDR Bandwidth Performance Monitoring Unit (PMU)

The Amlogic Meson G12 SoC contains a bandwidth monitor inside DRAM controller.
The monitor includes 4 channels. Each channel can count the request accessing
DRAM. The channel can count up to 3 AXI port simultaneously. It can be helpful
to show if the performance bottleneck is on DDR bandwidth.

Currently, this driver supports the following 5 perf events:

* meson\_ddr\_bw/total\_rw\_bytes/
* meson\_ddr\_bw/chan\_1\_rw\_bytes/
* meson\_ddr\_bw/chan\_2\_rw\_bytes/
* meson\_ddr\_bw/chan\_3\_rw\_bytes/
* meson\_ddr\_bw/chan\_4\_rw\_bytes/

meson\_ddr\_bw/chan\_{1,2,3,4}\_rw\_bytes/ events are channel-specific events.
Each channel support filtering, which can let the channel to monitor
individual IP module in SoC.

Below are DDR access request event filter keywords:

* arm - from CPU
* vpu\_read1 - from OSD + VPP read
* gpu - from 3D GPU
* pcie - from PCIe controller
* hdcp - from HDCP controller
* hevc\_front - from HEVC codec front end
* usb3\_0 - from USB3.0 controller
* hevc\_back - from HEVC codec back end
* h265enc - from HEVC encoder
* vpu\_read2 - from DI read
* vpu\_write1 - from VDIN write
* vpu\_write2 - from di write
* vdec - from legacy codec video decoder
* hcodec - from H264 encoder
* ge2d - from ge2d
* spicc1 - from SPI controller 1
* usb0 - from USB2.0 controller 0
* dma - from system DMA controller 1
* arb0 - from arb0
* sd\_emmc\_b - from SD eMMC b controller
* usb1 - from USB2.0 controller 1
* audio - from Audio module
* sd\_emmc\_c - from SD eMMC c controller
* spicc2 - from SPI controller 2
* ethernet - from Ethernet controller

Examples:

> * Show the total DDR bandwidth per seconds:
>
>   ```
>   perf stat -a -e meson_ddr_bw/total_rw_bytes/ -I 1000 sleep 10
>   ```
> * Show individual DDR bandwidth from CPU and GPU respectively, as well as
>   sum of them:
>
>   ```
>   perf stat -a -e meson_ddr_bw/chan_1_rw_bytes,arm=1/ -I 1000 sleep 10
>   perf stat -a -e meson_ddr_bw/chan_2_rw_bytes,gpu=1/ -I 1000 sleep 10
>   perf stat -a -e meson_ddr_bw/chan_3_rw_bytes,arm=1,gpu=1/ -I 1000 sleep 10
>   ```
