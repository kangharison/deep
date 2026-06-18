# ALPS HID Touchpad Protocol

> 출처(원문): https://docs.kernel.org/hid/hid-alps.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# ALPS HID Touchpad Protocol

## Introduction

Currently ALPS HID driver supports U1 Touchpad device.

U1 device basic information.

|  |  |
| --- | --- |
| Vendor ID | 0x044E |
| Product ID | 0x120B |
| Version ID | 0x0121 |

## HID Descriptor

| Byte | Field | Value | Notes |
| --- | --- | --- | --- |
| 0 | wHIDDescLength | 001E | Length of HID Descriptor : 30 bytes |
| 2 | bcdVersion | 0100 | Compliant with Version 1.00 |
| 4 | wReportDescLength | 00B2 | Report Descriptor is 178 Bytes (0x00B2) |
| 6 | wReportDescRegister | 0002 | Identifier to read Report Descriptor |
| 8 | wInputRegister | 0003 | Identifier to read Input Report |
| 10 | wMaxInputLength | 0053 | Input Report is 80 Bytes + 2 |
| 12 | wOutputRegister | 0000 | Identifier to read Output Report |
| 14 | wMaxOutputLength | 0000 | No Output Reports |
| 16 | wCommandRegister | 0005 | Identifier for Command Register |
| 18 | wDataRegister | 0006 | Identifier for Data Register |
| 20 | wVendorID | 044E | Vendor ID 0x044E |
| 22 | wProductID | 120B | Product ID 0x120B |
| 24 | wVersionID | 0121 | Version 01.21 |
| 26 | RESERVED | 0000 | RESERVED |

## Report ID

|  |  |  |
| --- | --- | --- |
| ReportID-1 | (Input Reports) | (HIDUsage-Mouse) for TP&SP |
| ReportID-2 | (Input Reports) | (HIDUsage-keyboard) for TP |
| ReportID-3 | (Input Reports) | (Vendor Usage: Max 10 finger data) for TP |
| ReportID-4 | (Input Reports) | (Vendor Usage: ON bit data) for GP |
| ReportID-5 | (Feature Reports) | Feature Reports |
| ReportID-6 | (Input Reports) | (Vendor Usage: StickPointer data) for SP |
| ReportID-7 | (Feature Reports) | Flash update (Bootloader) |

## Data pattern

|  |  |  |  |
| --- | --- | --- | --- |
| Case1 | ReportID\_1 | TP/SP | Relative/Relative |
| Case2 | ReportID\_3 ReportID\_6 | TP SP | Absolute Absolute |

## Command Read/Write

To read/write to RAM, need to send a command to the device.

The command format is as below.

### DataByte(SET\_REPORT)

|  |  |
| --- | --- |
| Byte1 | Command Byte |
| Byte2 | Address - Byte 0 (LSB) |
| Byte3 | Address - Byte 1 |
| Byte4 | Address - Byte 2 |
| Byte5 | Address - Byte 3 (MSB) |
| Byte6 | Value Byte |
| Byte7 | Checksum |

Command Byte is read=0xD1/write=0xD2.

Address is read/write RAM address.

Value Byte is writing data when you send the write commands.

When you read RAM, there is no meaning.

### DataByte(GET\_REPORT)

|  |  |
| --- | --- |
| Byte1 | Response Byte |
| Byte2 | Address - Byte 0 (LSB) |
| Byte3 | Address - Byte 1 |
| Byte4 | Address - Byte 2 |
| Byte5 | Address - Byte 3 (MSB) |
| Byte6 | Value Byte |
| Byte7 | Checksum |

Read value is stored in Value Byte.

## Packet Format

### Touchpad data byte

|  | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 0 | 0 | SW6 | SW5 | SW4 | SW3 | SW2 | SW1 |
| 2 | 0 | 0 | 0 | Fcv | Fn3 | Fn2 | Fn1 | Fn0 |
| 3 | Xa0\_7 | Xa0\_6 | Xa0\_5 | Xa0\_4 | Xa0\_3 | Xa0\_2 | Xa0\_1 | Xa0\_0 |
| 4 | Xa0\_15 | Xa0\_14 | Xa0\_13 | Xa0\_12 | Xa0\_11 | Xa0\_10 | Xa0\_9 | Xa0\_8 |
| 5 | Ya0\_7 | Ya0\_6 | Ya0\_5 | Ya0\_4 | Ya0\_3 | Ya0\_2 | Ya0\_1 | Ya0\_0 |
| 6 | Ya0\_15 | Ya0\_14 | Ya0\_13 | Ya0\_12 | Ya0\_11 | Ya0\_10 | Ya0\_9 | Ya0\_8 |
| 7 | LFB0 | Zs0\_6 | Zs0\_5 | Zs0\_4 | Zs0\_3 | Zs0\_2 | Zs0\_1 | Zs0\_0 |
| 8 | Xa1\_7 | Xa1\_6 | Xa1\_5 | Xa1\_4 | Xa1\_3 | Xa1\_2 | Xa1\_1 | Xa1\_0 |
| 9 | Xa1\_15 | Xa1\_14 | Xa1\_13 | Xa1\_12 | Xa1\_11 | Xa1\_10 | Xa1\_9 | Xa1\_8 |
| 10 | Ya1\_7 | Ya1\_6 | Ya1\_5 | Ya1\_4 | Ya1\_3 | Ya1\_2 | Ya1\_1 | Ya1\_0 |
| 11 | Ya1\_15 | Ya1\_14 | Ya1\_13 | Ya1\_12 | Ya1\_11 | Ya1\_10 | Ya1\_9 | Ya1\_8 |
| 12 | LFB1 | Zs1\_6 | Zs1\_5 | Zs1\_4 | Zs1\_3 | Zs1\_2 | Zs1\_1 | Zs1\_0 |
| 13 | Xa2\_7 | Xa2\_6 | Xa2\_5 | Xa2\_4 | Xa2\_3 | Xa2\_2 | Xa2\_1 | Xa2\_0 |
| 14 | Xa2\_15 | Xa2\_14 | Xa2\_13 | Xa2\_12 | Xa2\_11 | Xa2\_10 | Xa2\_9 | Xa2\_8 |
| 15 | Ya2\_7 | Ya2\_6 | Ya2\_5 | Ya2\_4 | Ya2\_3 | Ya2\_2 | Ya2\_1 | Ya2\_0 |
| 16 | Ya2\_15 | Ya2\_14 | Ya2\_13 | Ya2\_12 | Ya2\_11 | Ya2\_10 | Ya2\_9 | Ya2\_8 |
| 17 | LFB2 | Zs2\_6 | Zs2\_5 | Zs2\_4 | Zs2\_3 | Zs2\_2 | Zs2\_1 | Zs2\_0 |
| 18 | Xa3\_7 | Xa3\_6 | Xa3\_5 | Xa3\_4 | Xa3\_3 | Xa3\_2 | Xa3\_1 | Xa3\_0 |
| 19 | Xa3\_15 | Xa3\_14 | Xa3\_13 | Xa3\_12 | Xa3\_11 | Xa3\_10 | Xa3\_9 | Xa3\_8 |
| 20 | Ya3\_7 | Ya3\_6 | Ya3\_5 | Ya3\_4 | Ya3\_3 | Ya3\_2 | Ya3\_1 | Ya3\_0 |
| 21 | Ya3\_15 | Ya3\_14 | Ya3\_13 | Ya3\_12 | Ya3\_11 | Ya3\_10 | Ya3\_9 | Ya3\_8 |
| 22 | LFB3 | Zs3\_6 | Zs3\_5 | Zs3\_4 | Zs3\_3 | Zs3\_2 | Zs3\_1 | Zs3\_0 |
| 23 | Xa4\_7 | Xa4\_6 | Xa4\_5 | Xa4\_4 | Xa4\_3 | Xa4\_2 | Xa4\_1 | Xa4\_0 |
| 24 | Xa4\_15 | Xa4\_14 | Xa4\_13 | Xa4\_12 | Xa4\_11 | Xa4\_10 | Xa4\_9 | Xa4\_8 |
| 25 | Ya4\_7 | Ya4\_6 | Ya4\_5 | Ya4\_4 | Ya4\_3 | Ya4\_2 | Ya4\_1 | Ya4\_0 |
| 26 | Ya4\_15 | Ya4\_14 | Ya4\_13 | Ya4\_12 | Ya4\_11 | Ya4\_10 | Ya4\_9 | Ya4\_8 |
| 27 | LFB4 | Zs4\_6 | Zs4\_5 | Zs4\_4 | Zs4\_3 | Zs4\_2 | Zs4\_1 | Zs4\_0 |

SW1-SW6:
:   SW ON/OFF status

Xan\_15-0(16bit):
:   X Absolute data of the “n”th finger

Yan\_15-0(16bit):
:   Y Absolute data of the “n”th finger

Zsn\_6-0(7bit):
:   Operation area of the “n”th finger

### StickPointer data byte

|  | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Byte1 | 1 | 1 | 1 | 0 | 1 | SW3 | SW2 | SW1 |
| Byte2 | X7 | X6 | X5 | X4 | X3 | X2 | X1 | X0 |
| Byte3 | X15 | X14 | X13 | X12 | X11 | X10 | X9 | X8 |
| Byte4 | Y7 | Y6 | Y5 | Y4 | Y3 | Y2 | Y1 | Y0 |
| Byte5 | Y15 | Y14 | Y13 | Y12 | Y11 | Y10 | Y9 | Y8 |
| Byte6 | Z7 | Z6 | Z5 | Z4 | Z3 | Z2 | Z1 | Z0 |
| Byte7 | T&P | Z14 | Z13 | Z12 | Z11 | Z10 | Z9 | Z8 |

SW1-SW3:
:   SW ON/OFF status

Xn\_15-0(16bit):
:   X Absolute data

Yn\_15-0(16bit):
:   Y Absolute data

Zn\_14-0(15bit):
:   Z
