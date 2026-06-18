# 4.9.RDS Interface

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/dev-rds.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.9. RDS Interface

The Radio Data System transmits supplementary information in binary
format, for example the station name or travel information, on an
inaudible audio subcarrier of a radio program. This interface is aimed
at devices capable of receiving and/or transmitting RDS information.

For more information see the core RDS standard [IEC 62106](biblio.html#iec62106) and the
RBDS standard [NRSC-4-B](biblio.html#nrsc4).

Note

Note that the RBDS standard as is used in the USA is almost
identical to the RDS standard. Any RDS decoder/encoder can also handle
RBDS. Only some of the fields have slightly different meanings. See the
RBDS standard for more information.

The RBDS standard also specifies support for MMBS (Modified Mobile
Search). This is a proprietary format which seems to be discontinued.
The RDS interface does not support this format. Should support for MMBS
(or the so-called ‘E blocks’ in general) be needed, then please contact
the linux-media mailing list:
<https://linuxtv.org/lists.php>.

## 4.9.1. Querying Capabilities

Devices supporting the RDS capturing API set the
`V4L2_CAP_RDS_CAPTURE` flag in the `capabilities` field of struct
[`v4l2_capability`](vidioc-querycap.html#c.V4L.v4l2_capability "v4l2_capability") returned by the
[ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap) ioctl. Any tuner that
supports RDS will set the `V4L2_TUNER_CAP_RDS` flag in the
`capability` field of struct [`v4l2_tuner`](vidioc-g-tuner.html#c.V4L.v4l2_tuner "v4l2_tuner"). If the
driver only passes RDS blocks without interpreting the data the
`V4L2_TUNER_CAP_RDS_BLOCK_IO` flag has to be set, see
[Reading RDS data](#reading-rds-data). For future use the flag
`V4L2_TUNER_CAP_RDS_CONTROLS` has also been defined. However, a driver
for a radio tuner with this capability does not yet exist, so if you are
planning to write such a driver you should discuss this on the
linux-media mailing list:
<https://linuxtv.org/lists.php>.

Whether an RDS signal is present can be detected by looking at the
`rxsubchans` field of struct [`v4l2_tuner`](vidioc-g-tuner.html#c.V4L.v4l2_tuner "v4l2_tuner"): the
`V4L2_TUNER_SUB_RDS` will be set if RDS data was detected.

Devices supporting the RDS output API set the `V4L2_CAP_RDS_OUTPUT`
flag in the `capabilities` field of struct
[`v4l2_capability`](vidioc-querycap.html#c.V4L.v4l2_capability "v4l2_capability") returned by the
[ioctl VIDIOC\_QUERYCAP](vidioc-querycap.html#vidioc-querycap) ioctl. Any modulator that
supports RDS will set the `V4L2_TUNER_CAP_RDS` flag in the
`capability` field of struct
[`v4l2_modulator`](vidioc-g-modulator.html#c.V4L.v4l2_modulator "v4l2_modulator"). In order to enable the RDS
transmission one must set the `V4L2_TUNER_SUB_RDS` bit in the
`txsubchans` field of struct
[`v4l2_modulator`](vidioc-g-modulator.html#c.V4L.v4l2_modulator "v4l2_modulator"). If the driver only passes RDS
blocks without interpreting the data the `V4L2_TUNER_CAP_RDS_BLOCK_IO`
flag has to be set. If the tuner is capable of handling RDS entities
like program identification codes and radio text, the flag
`V4L2_TUNER_CAP_RDS_CONTROLS` should be set, see
[Writing RDS data](#writing-rds-data) and
[FM Transmitter Control Reference](ext-ctrls-fm-tx.html#fm-tx-controls).

## 4.9.2. Reading RDS data

RDS data can be read from the radio device with the
[`read()`](func-read.html#c.V4L.read "read") function. The data is packed in groups of
three bytes.

## 4.9.3. Writing RDS data

RDS data can be written to the radio device with the
[`write()`](func-write.html#c.V4L.write "write") function. The data is packed in groups of
three bytes, as follows:

## 4.9.4. RDS datastructures

type v4l2\_rds\_data

struct v4l2\_rds\_data

|  |  |  |
| --- | --- | --- |
| \_\_u8 | `lsb` | Least Significant Byte of RDS Block |
| \_\_u8 | `msb` | Most Significant Byte of RDS Block |
| \_\_u8 | `block` | Block description |

Block description

|  |  |
| --- | --- |
| Bits 0-2 | Block (aka offset) of the received data. |
| Bits 3-5 | Deprecated. Currently identical to bits 0-2. Do not use these bits. |
| Bit 6 | Corrected bit. Indicates that an error was corrected for this data block. |
| Bit 7 | Error bit. Indicates that an uncorrectable error occurred during reception of this block. |

Block defines

|  |  |  |  |
| --- | --- | --- | --- |
| V4L2\_RDS\_BLOCK\_MSK |  | 7 | Mask for bits 0-2 to get the block ID. |
| V4L2\_RDS\_BLOCK\_A |  | 0 | Block A. |
| V4L2\_RDS\_BLOCK\_B |  | 1 | Block B. |
| V4L2\_RDS\_BLOCK\_C |  | 2 | Block C. |
| V4L2\_RDS\_BLOCK\_D |  | 3 | Block D. |
| V4L2\_RDS\_BLOCK\_C\_ALT |  | 4 | Block C’. |
| V4L2\_RDS\_BLOCK\_INVALID | read-only | 7 | An invalid block. |
| V4L2\_RDS\_BLOCK\_CORRECTED | read-only | 0x40 | A bit error was detected but corrected. |
| V4L2\_RDS\_BLOCK\_ERROR | read-only | 0x80 | An uncorrectable error occurred. |
