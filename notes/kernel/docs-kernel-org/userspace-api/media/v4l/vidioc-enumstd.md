# 7.20.ioctl VIDIOC_ENUMSTD, VIDIOC_SUBDEV_ENUMSTD

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/vidioc-enumstd.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7.20. ioctl VIDIOC\_ENUMSTD, VIDIOC\_SUBDEV\_ENUMSTD

## 7.20.1. Name

VIDIOC\_ENUMSTD - VIDIOC\_SUBDEV\_ENUMSTD - Enumerate supported video standards

## 7.20.2. Synopsis

VIDIOC\_ENUMSTD

`int ioctl(int fd, VIDIOC_ENUMSTD, struct v4l2_standard *argp)`

VIDIOC\_SUBDEV\_ENUMSTD

`int ioctl(int fd, VIDIOC_SUBDEV_ENUMSTD, struct v4l2_standard *argp)`

## 7.20.3. Arguments

`fd`
:   File descriptor returned by [`open()`](func-open.html#c.V4L.open "open").

`argp`
:   Pointer to struct [`v4l2_standard`](#c.V4L.v4l2_standard "v4l2_standard").

## 7.20.4. Description

To query the attributes of a video standard, especially a custom (driver
defined) one, applications initialize the `index` field of struct
[`v4l2_standard`](#c.V4L.v4l2_standard "v4l2_standard") and call the [ioctl VIDIOC\_ENUMSTD, VIDIOC\_SUBDEV\_ENUMSTD](#vidioc-enumstd)
ioctl with a pointer to this structure. Drivers fill the rest of the
structure or return an `EINVAL` error code when the index is out of
bounds. To enumerate all standards applications shall begin at index
zero, incrementing by one until the driver returns `EINVAL`. Drivers may
enumerate a different set of standards after switching the video input
or output. [[1]](#f1)

type v4l2\_standard

struct v4l2\_standard

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `index` | Number of the video standard, set by the application. |
| [v4l2\_std\_id](#v4l2-std-id) | `id` | The bits in this field identify the standard as one of the common standards listed in [typedef v4l2\_std\_id](#v4l2-std-id), or if bits 32 to 63 are set as custom standards. Multiple bits can be set if the hardware does not distinguish between these standards, however separate indices do not indicate the opposite. The `id` must be unique. No other enumerated struct [`v4l2_standard`](#c.V4L.v4l2_standard "v4l2_standard") structure, for this input or output anyway, can contain the same set of bits. |
| \_\_u8 | `name`[24] | Name of the standard, a NUL-terminated ASCII string, for example: “PAL-B/G”, “NTSC Japan”. This information is intended for the user. |
| struct [`v4l2_fract`](#c.V4L.v4l2_fract "v4l2_fract") | `frameperiod` | The frame period (not field period) is numerator / denominator. For example M/NTSC has a frame period of 1001 / 30000 seconds. |
| \_\_u32 | `framelines` | Total lines per frame including blanking, e. g. 625 for B/PAL. |
| \_\_u32 | `reserved`[4] | Reserved for future extensions. Drivers must set the array to zero. |

type v4l2\_fract

struct v4l2\_fract

|  |  |  |
| --- | --- | --- |
| \_\_u32 | `numerator` |  |
| \_\_u32 | `denominator` |  |

typedef v4l2\_std\_id

|  |  |  |
| --- | --- | --- |
| \_\_u64 | `v4l2_std_id` | This type is a set, each bit representing another video standard as listed below and in [Video Standards (based on itu470)](#video-standards). The 32 most significant bits are reserved for custom (driver defined) video standards. |

```
#define V4L2_STD_PAL_B          ((v4l2_std_id)0x00000001)
#define V4L2_STD_PAL_B1         ((v4l2_std_id)0x00000002)
#define V4L2_STD_PAL_G          ((v4l2_std_id)0x00000004)
#define V4L2_STD_PAL_H          ((v4l2_std_id)0x00000008)
#define V4L2_STD_PAL_I          ((v4l2_std_id)0x00000010)
#define V4L2_STD_PAL_D          ((v4l2_std_id)0x00000020)
#define V4L2_STD_PAL_D1         ((v4l2_std_id)0x00000040)
#define V4L2_STD_PAL_K          ((v4l2_std_id)0x00000080)

#define V4L2_STD_PAL_M          ((v4l2_std_id)0x00000100)
#define V4L2_STD_PAL_N          ((v4l2_std_id)0x00000200)
#define V4L2_STD_PAL_Nc         ((v4l2_std_id)0x00000400)
#define V4L2_STD_PAL_60         ((v4l2_std_id)0x00000800)
```

`V4L2_STD_PAL_60` is a hybrid standard with 525 lines, 60 Hz refresh
rate, and PAL color modulation with a 4.43 MHz color subcarrier. Some
PAL video recorders can play back NTSC tapes in this mode for display on
a 50/60 Hz agnostic PAL TV.

```
#define V4L2_STD_NTSC_M         ((v4l2_std_id)0x00001000)
#define V4L2_STD_NTSC_M_JP      ((v4l2_std_id)0x00002000)
#define V4L2_STD_NTSC_443       ((v4l2_std_id)0x00004000)
```

`V4L2_STD_NTSC_443` is a hybrid standard with 525 lines, 60 Hz refresh
rate, and NTSC color modulation with a 4.43 MHz color subcarrier.

```
#define V4L2_STD_NTSC_M_KR      ((v4l2_std_id)0x00008000)

#define V4L2_STD_SECAM_B        ((v4l2_std_id)0x00010000)
#define V4L2_STD_SECAM_D        ((v4l2_std_id)0x00020000)
#define V4L2_STD_SECAM_G        ((v4l2_std_id)0x00040000)
#define V4L2_STD_SECAM_H        ((v4l2_std_id)0x00080000)
#define V4L2_STD_SECAM_K        ((v4l2_std_id)0x00100000)
#define V4L2_STD_SECAM_K1       ((v4l2_std_id)0x00200000)
#define V4L2_STD_SECAM_L        ((v4l2_std_id)0x00400000)
#define V4L2_STD_SECAM_LC       ((v4l2_std_id)0x00800000)

/* ATSC/HDTV */
#define V4L2_STD_ATSC_8_VSB     ((v4l2_std_id)0x01000000)
#define V4L2_STD_ATSC_16_VSB    ((v4l2_std_id)0x02000000)
```

`V4L2_STD_ATSC_8_VSB` and `V4L2_STD_ATSC_16_VSB` are U.S.
terrestrial digital TV standards. Presently the V4L2 API does not
support digital TV. See also the Linux DVB API at
<https://linuxtv.org>.

```
#define V4L2_STD_PAL_BG         (V4L2_STD_PAL_B         |
                 V4L2_STD_PAL_B1        |
                 V4L2_STD_PAL_G)
#define V4L2_STD_B              (V4L2_STD_PAL_B         |
                 V4L2_STD_PAL_B1        |
                 V4L2_STD_SECAM_B)
#define V4L2_STD_GH             (V4L2_STD_PAL_G         |
                 V4L2_STD_PAL_H         |
                 V4L2_STD_SECAM_G       |
                 V4L2_STD_SECAM_H)
#define V4L2_STD_PAL_DK         (V4L2_STD_PAL_D         |
                 V4L2_STD_PAL_D1        |
                 V4L2_STD_PAL_K)
#define V4L2_STD_PAL            (V4L2_STD_PAL_BG        |
                 V4L2_STD_PAL_DK        |
                 V4L2_STD_PAL_H         |
                 V4L2_STD_PAL_I)
#define V4L2_STD_NTSC           (V4L2_STD_NTSC_M        |
                 V4L2_STD_NTSC_M_JP     |
                 V4L2_STD_NTSC_M_KR)
#define V4L2_STD_MN             (V4L2_STD_PAL_M         |
                 V4L2_STD_PAL_N         |
                 V4L2_STD_PAL_Nc        |
                 V4L2_STD_NTSC)
#define V4L2_STD_SECAM_DK       (V4L2_STD_SECAM_D       |
                 V4L2_STD_SECAM_K       |
                 V4L2_STD_SECAM_K1)
#define V4L2_STD_DK             (V4L2_STD_PAL_DK        |
                 V4L2_STD_SECAM_DK)

#define V4L2_STD_SECAM          (V4L2_STD_SECAM_B       |
                 V4L2_STD_SECAM_G       |
                 V4L2_STD_SECAM_H       |
                 V4L2_STD_SECAM_DK      |
                 V4L2_STD_SECAM_L       |
                 V4L2_STD_SECAM_LC)

#define V4L2_STD_525_60         (V4L2_STD_PAL_M         |
                 V4L2_STD_PAL_60        |
                 V4L2_STD_NTSC          |
                 V4L2_STD_NTSC_443)
#define V4L2_STD_625_50         (V4L2_STD_PAL           |
                 V4L2_STD_PAL_N         |
                 V4L2_STD_PAL_Nc        |
                 V4L2_STD_SECAM)

#define V4L2_STD_UNKNOWN        0
#define V4L2_STD_ALL            (V4L2_STD_525_60        |
                 V4L2_STD_625_50)
```

Video Standards (based on [ITU BT.470](biblio.html#itu470))

| Characteristics | M/NTSC [[2]](#f2) | M/PAL | N/PAL [[3]](#f3) | B, B1, G/PAL | D, D1, K/PAL | H/PAL | I/PAL | B, G/SECAM | D, K/SECAM | K1/SECAM | L/SECAM |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Frame lines | 525 | | 625 | | | | | | | | |
| Frame period (s) | 1001/30000 | | 1/25 | | | | | | | | |
| Chrominance sub-carrier frequency (Hz) | 3579545 ± 10 | 3579611.49 ± 10 | 4433618.75 ± 5  (3582056.25 ± 5) | 4433618.75 ± 5 | | | | 4433618.75 ± 1 | fOR = 4406250 ± 2000,  fOB = 4250000 ± 2000 | | |
| Nominal radio-frequency channel bandwidth (MHz) | 6 | 6 | 6 | B: 7; B1, G: 8 | 8 | 8 | 8 | 8 | 8 | 8 | 8 |
| Sound carrier relative to vision carrier (MHz) | 4.5 | 4.5 | 4.5 | 5.5 ± 0.001 [[4]](#f4) [[5]](#f5) [[6]](#f6) [[7]](#f7) | 6.5 ± 0.001 | 5.5 | 5.9996 ± 0.0005 | 5.5 ± 0.001 | 6.5 ± 0.001 | 6.5 | 6.5 [[8]](#f8) |

## 7.20.5. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

EINVAL
:   The struct [`v4l2_standard`](#c.V4L.v4l2_standard "v4l2_standard") `index` is out
    of bounds.

ENODATA
:   Standard video timings are not supported for this input or output.

[[1](#id1)]

The supported standards may overlap and we need an unambiguous set to
find the current standard returned by [VIDIOC\_G\_STD](vidioc-g-std.html#vidioc-g-std).


[[2](#id2)]

Japan uses a standard similar to M/NTSC (V4L2\_STD\_NTSC\_M\_JP).


[[3](#id3)]

The values in brackets apply to the combination N/PAL a.k.a.
NC used in Argentina (V4L2\_STD\_PAL\_Nc).


[[4](#id4)]

In the Federal Republic of Germany, Austria, Italy, the Netherlands,
Slovakia and Switzerland a system of two sound carriers is used, the
frequency of the second carrier being 242.1875 kHz above the
frequency of the first sound carrier. For stereophonic sound
transmissions a similar system is used in Australia.


[[5](#id5)]

New Zealand uses a sound carrier displaced 5.4996 ± 0.0005 MHz from
the vision carrier.


[[6](#id6)]

In Denmark, Finland, New Zealand, Sweden and Spain a system of two
sound carriers is used. In Iceland, Norway and Poland the same system
is being introduced. The second carrier is 5.85 MHz above the vision
carrier and is DQPSK modulated with 728 kbit/s sound and data
multiplex. (NICAM system)


[[7](#id7)]

In the United Kingdom, a system of two sound carriers is used. The
second sound carrier is 6.552 MHz above the vision carrier and is
DQPSK modulated with a 728 kbit/s sound and data multiplex able to
carry two sound channels. (NICAM system)


[[8](#id8)]

In France, a digital carrier 5.85 MHz away from the vision carrier
may be used in addition to the main sound carrier. It is modulated in
differentially encoded QPSK with a 728 kbit/s sound and data
multiplexer capable of carrying two sound channels. (NICAM system)
