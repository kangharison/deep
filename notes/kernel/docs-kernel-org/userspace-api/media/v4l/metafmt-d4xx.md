# 2.13.3.V4L2_META_FMT_D4XX (‘D4XX’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/metafmt-d4xx.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.13.3. V4L2\_META\_FMT\_D4XX (‘D4XX’)

Intel D4xx UVC Cameras Metadata

## 2.13.3.1. Description

Intel D4xx (D435, D455 and others) cameras include per-frame metadata in their UVC
payload headers, following the Microsoft(R) UVC extension proposal [[1](#id1)]. That
means, that the private D4XX metadata, following the standard UVC header, is
organised in blocks. D4XX cameras implement several standard block types,
proposed by Microsoft, and several proprietary ones. Supported standard metadata
types are MetadataId\_CaptureStats (ID 3), MetadataId\_CameraExtrinsics (ID 4),
and MetadataId\_CameraIntrinsics (ID 5). For their description see [[1](#id1)]. This
document describes proprietary metadata types, used by D4xx cameras.

V4L2\_META\_FMT\_D4XX buffers follow the metadata buffer layout of
V4L2\_META\_FMT\_UVC with the only difference, that it also includes proprietary
payload header data. D4xx cameras use bulk transfers and only send one payload
per frame, therefore their headers cannot be larger than 255 bytes.

This document implements Intel Configuration version 3 [[9](#id9)].

Below are proprietary Microsoft style metadata types, used by D4xx cameras,
where all fields are in little endian order:

D4xx metadata

| **Field** | **Description** |
| --- | --- |
| *Depth Control* | |
| \_\_u32 ID | 0x80000000 |
| \_\_u32 Size | Size in bytes, include ID (all protocol versions: 60) |
| \_\_u32 Version | Version of this structure. The documentation herein covers versions 1, 2 and 3. The version number will be incremented when new fields are added. |
| \_\_u32 Flags | A bitmask of flags: see [[2](#id2)] below |
| \_\_u32 Gain | Gain value in internal units, same as the V4L2\_CID\_GAIN control, used to capture the frame |
| \_\_u32 Exposure | Exposure time (in microseconds) used to capture the frame |
| \_\_u32 Laser power | Power of the laser LED 0-360, used for depth measurement |
| \_\_u32 AE mode | 0: manual; 1: automatic exposure |
| \_\_u32 Exposure priority | Exposure priority value: 0 - constant frame rate |
| \_\_u32 AE ROI left | Left border of the AE Region of Interest (all ROI values are in pixels and lie between 0 and maximum width or height respectively) |
| \_\_u32 AE ROI right | Right border of the AE Region of Interest |
| \_\_u32 AE ROI top | Top border of the AE Region of Interest |
| \_\_u32 AE ROI bottom | Bottom border of the AE Region of Interest |
| \_\_u32 Preset | Preset selector value, default: 0, unless changed by the user |
| \_\_u8 Emitter mode (v3 only) (\_\_u32 Laser mode for v1) [[8](#id8)] | 0: off, 1: on, same as \_\_u32 Laser mode for v1 |
| \_\_u8 RFU byte (v3 only) | Spare byte for future use |
| \_\_u16 LED Power (v3 only) | Led power value 0-360 (F416 SKU) |
| *Capture Timing* | |
| \_\_u32 ID | 0x80000001 |
| \_\_u32 Size | Size in bytes, include ID (all protocol versions: 40) |
| \_\_u32 Version | Version of this structure. The documentation herein corresponds to version xxx. The version number will be incremented when new fields are added. |
| \_\_u32 Flags | A bitmask of flags: see [[3](#id3)] below |
| \_\_u32 Frame counter | Monotonically increasing counter |
| \_\_u32 Optical time | Time in microseconds from the beginning of a frame till its middle |
| \_\_u32 Readout time | Time, used to read out a frame in microseconds |
| \_\_u32 Exposure time | Frame exposure time in microseconds |
| \_\_u32 Frame interval | In microseconds = 1000000 / framerate |
| \_\_u32 Pipe latency | Time in microseconds from start of frame to data in USB buffer |
| *Configuration* | |
| \_\_u32 ID | 0x80000002 |
| \_\_u32 Size | Size in bytes, include ID (v1:36, v3:40) |
| \_\_u32 Version | Version of this structure. The documentation herein corresponds to version xxx. The version number will be incremented when new fields are added. |
| \_\_u32 Flags | A bitmask of flags: see [[4](#id4)] below |
| \_\_u8 Hardware type | Camera hardware version [[5](#id5)] |
| \_\_u8 SKU ID | Camera hardware configuration [[6](#id6)] |
| \_\_u32 Cookie | Internal synchronisation |
| \_\_u16 Format | Image format code [[7](#id7)] |
| \_\_u16 Width | Width in pixels |
| \_\_u16 Height | Height in pixels |
| \_\_u16 Framerate | Requested frame rate per second |
| \_\_u16 Trigger | Byte 0: bit 0: depth and RGB are synchronised, bit 1: external trigger |
| \_\_u16 Calibration count (v3 only) | Calibration counter, see [[4](#id4)] below |
| \_\_u8 GPIO input data (v3 only) | GPIO readout, see [[4](#id4)] below (Supported from FW 5.12.7.0) |
| \_\_u32 Sub-preset info (v3 only) | Sub-preset choice information, see [[4](#id4)] below |
| \_\_u8 reserved (v3 only) | RFU byte. |

[1] <https://docs.microsoft.com/en-us/windows-hardware/drivers/stream/uvc-extensions-1-5>

[2] Depth Control flags specify which fields are valid:

```
0x00000001 Gain
0x00000002 Exposure
0x00000004 Laser power
0x00000008 AE mode
0x00000010 Exposure priority
0x00000020 AE ROI
0x00000040 Preset
0x00000080 Emitter mode
0x00000100 LED Power
```

[3] Capture Timing flags specify which fields are valid:

```
0x00000001 Frame counter
0x00000002 Optical time
0x00000004 Readout time
0x00000008 Exposure time
0x00000010 Frame interval
0x00000020 Pipe latency
```

[4] Configuration flags specify which fields are valid:

```
0x00000001 Hardware type
0x00000002 SKU ID
0x00000004 Cookie
0x00000008 Format
0x00000010 Width
0x00000020 Height
0x00000040 Framerate
0x00000080 Trigger
0x00000100 Cal count
0x00000200 GPIO Input Data
0x00000400 Sub-preset Info
```

[5] Camera model:

```
0 DS5
1 IVCAM2
```

[6] 8-bit camera hardware configuration bitfield:

```
[1:0] depthCamera
      00: no depth
      01: standard depth
      10: wide depth
      11: reserved
[2]   depthIsActive - has a laser projector
[3]   RGB presence
[4]   Inertial Measurement Unit (IMU) presence
[5]   projectorType
      0: HPTG
      1: Princeton
[6]   0: a projector, 1: an LED
[7]   reserved
```

[7] Image format codes per video streaming interface:

Depth:

```
1 Z16
2 Z
```

Left sensor:

```
1 Y8
2 UYVY
3 R8L8
4 Calibration
5 W10
```

Fish Eye sensor:

```
1 RAW8
```

[8] The “Laser mode” has been replaced in version 3 by three different fields.
“Laser” has been renamed to “Emitter” as there are multiple technologies for
camera projectors. As we have another field for “Laser Power” we introduced
“LED Power” for extra emitter.

The “Laser mode” \_\_u32 fields has been split into: ::
:   1 \_\_u8 Emitter mode
    2 \_\_u8 RFU byte
    3 \_\_u16 LED Power

This is a change between versions 1 and 3. All versions 1, 2 and 3 are backward
compatible with the same data format and they are supported. See [[2](#id2)] for which
attributes are valid.

[9] LibRealSense SDK metadata source:
<https://github.com/IntelRealSense/librealsense/blob/master/src/metadata.h>
