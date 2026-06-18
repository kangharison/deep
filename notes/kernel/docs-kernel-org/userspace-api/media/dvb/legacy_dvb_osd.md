# 6.2.3.3.DVB OSD Device

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/legacy_dvb_osd.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6.2.3.3. DVB OSD Device

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

The DVB OSD device controls the OnScreen-Display of the AV7110 based
DVB-cards with hardware MPEG2 decoder. It can be accessed through
`/dev/dvb/adapter?/osd0`.
Data types and ioctl definitions can be accessed by including
`linux/dvb/osd.h` in your application.

The OSD is not a frame-buffer like on many other cards.
It is a kind of canvas one can draw on.
The color-depth is limited depending on the memory size installed.
An appropriate palette of colors has to be set up.
The installed memory size can be identified with the [OSD\_GET\_CAPABILITY](#osd-get-capability)
ioctl.

## 6.2.3.3.1. OSD Data Types

### 6.2.3.3.1.1. OSD\_Command

#### 6.2.3.3.1.1.1. Synopsis

```
typedef enum {
    /* All functions return -2 on "not open" */
    OSD_Close = 1,
    OSD_Open,
    OSD_Show,
    OSD_Hide,
    OSD_Clear,
    OSD_Fill,
    OSD_SetColor,
    OSD_SetPalette,
    OSD_SetTrans,
    OSD_SetPixel,
    OSD_GetPixel,
    OSD_SetRow,
    OSD_SetBlock,
    OSD_FillRow,
    OSD_FillBlock,
    OSD_Line,
    OSD_Query,
    OSD_Test,
    OSD_Text,
    OSD_SetWindow,
    OSD_MoveWindow,
    OSD_OpenRaw,
} OSD_Command;
```

#### 6.2.3.3.1.1.2. Commands

Note

All functions return -2 on “not open”

| Command | Used variables of `struct` [osd\_cmd\_t](#osd-cmd-t).  Usage{variable} if alternative use. | Description | | |
| --- | --- | --- | --- | --- |
| `OSD_Close` |  | Disables OSD and releases the buffers.  Returns 0 on success. | | |
| `OSD_Open` | x0,y0,x1,y1,  BitPerPixel[2/4/8]{color&0x0F},  mix[0..15]{color&0xF0} | Opens OSD with this size and bit depth  Returns 0 on success,  -1 on DRAM allocation error,  -2 on “already open”. | | |
| `OSD_Show` |  | Enables OSD mode.  Returns 0 on success. | | |
| `OSD_Hide` |  | Disables OSD mode.  Returns 0 on success. | | |
| `OSD_Clear` |  | Sets all pixel to color 0.  Returns 0 on success. | | |
| `OSD_Fill` | color | Sets all pixel to color <color>.  Returns 0 on success. | | |
| `OSD_SetColor` | color,  R{x0},G{y0},B{x1},  opacity{y1} | Set palette entry <num> to <r,g,b>, <mix> and <trans> apply  R,G,B: 0..255  R=Red, G=Green, B=Blue  opacity=0: pixel opacity 0% (only video pixel shows)  opacity=1..254: pixel opacity as specified in header  opacity=255: pixel opacity 100% (only OSD pixel shows)  Returns 0 on success, -1 on error. | | |
| `OSD_SetPalette` | firstcolor{color},  lastcolor{x0},data | Set a number of entries in the palette.  Sets the entries “firstcolor” through “lastcolor” from the array “data”.  Data has 4 byte for each color:  R,G,B, and a opacity value: 0->transparent, 1..254->mix, 255->pixel | | |
| `OSD_SetTrans` | transparency{color} | Sets transparency of mixed pixel (0..15).  Returns 0 on success. | | |
| `OSD_SetPixel` | x0,y0,color | Sets pixel <x>,<y> to color number <color>.  Returns 0 on success, -1 on error. | | |
| `OSD_GetPixel` | x0,y0 | Returns color number of pixel <x>,<y>, or -1.  Command currently not supported by the AV7110! | | |
| `OSD_SetRow` | x0,y0,x1,data | Fills pixels x0,y through x1,y with the content of data[].  Returns 0 on success, -1 on clipping all pixel (no pixel drawn). | | |
| `OSD_SetBlock` | x0,y0,x1,y1,  increment{color},  data | Fills pixels x0,y0 through x1,y1 with the content of data[].  Inc contains the width of one line in the data block,  inc<=0 uses block width as line width.  Returns 0 on success, -1 on clipping all pixel. | | |
| `OSD_FillRow` | x0,y0,x1,color | Fills pixels x0,y through x1,y with the color <color>.  Returns 0 on success, -1 on clipping all pixel. | | |
| `OSD_FillBlock` | x0,y0,x1,y1,color | Fills pixels x0,y0 through x1,y1 with the color <color>.  Returns 0 on success, -1 on clipping all pixel. | | |
| `OSD_Line` | x0,y0,x1,y1,color | Draw a line from x0,y0 to x1,y1 with the color <color>.  Returns 0 on success. | | |
| `OSD_Query` | x0,y0,x1,y1,  xasp{color}; yasp=11 | Fills parameters with the picture dimensions and the pixel aspect ratio.  Returns 0 on success.  Command currently not supported by the AV7110! | | |
| `OSD_Test` |  | Draws a test picture.  For debugging purposes only.  Returns 0 on success. | | |
| `OSD_Text` | x0,y0,size,color,text | Draws a text at position x0,y0 with the color <color>. | | |
| `OSD_SetWindow` | x0 | Set window with number 0<x0<8 as current. | | |
| `OSD_MoveWindow` | x0,y0 | Move current window to (x0, y0). | | |
| `OSD_OpenRaw` | x0,y0,x1,y1,  [osd\_raw\_window\_t](#osd-raw-window-t) {color} | Open other types of OSD windows. | | |

#### 6.2.3.3.1.1.3. Description

The `OSD_Command` data type is used with the [OSD\_SEND\_CMD](#osd-send-cmd) ioctl to
tell the driver which OSD\_Command to execute.

---

### 6.2.3.3.1.2. osd\_cmd\_t

#### 6.2.3.3.1.2.1. Synopsis

```
typedef struct osd_cmd_s {
    OSD_Command cmd;
    int x0;
    int y0;
    int x1;
    int y1;
    int color;
    void __user *data;
} osd_cmd_t;
```

#### 6.2.3.3.1.2.2. Variables

|  |  |
| --- | --- |
| `OSD_Command cmd` | [OSD\_Command](#osd-command) to be executed. |
| `int x0` | First horizontal position. |
| `int y0` | First vertical position. |
| `int x1` | Second horizontal position. |
| `int y1` | Second vertical position. |
| `int color` | Number of the color in the palette. |
| `void __user *data` | Command specific Data. |

#### 6.2.3.3.1.2.3. Description

The `osd_cmd_t` data type is used with the [OSD\_SEND\_CMD](#osd-send-cmd) ioctl.
It contains the data for the OSD\_Command and the [OSD\_Command](#osd-command) itself.
The structure has to be passed to the driver and the components may be
modified by it.

---

### 6.2.3.3.1.3. osd\_raw\_window\_t

#### 6.2.3.3.1.3.1. Synopsis

```
typedef enum {
    OSD_BITMAP1,
    OSD_BITMAP2,
    OSD_BITMAP4,
    OSD_BITMAP8,
    OSD_BITMAP1HR,
    OSD_BITMAP2HR,
    OSD_BITMAP4HR,
    OSD_BITMAP8HR,
    OSD_YCRCB422,
    OSD_YCRCB444,
    OSD_YCRCB444HR,
    OSD_VIDEOTSIZE,
    OSD_VIDEOHSIZE,
    OSD_VIDEOQSIZE,
    OSD_VIDEODSIZE,
    OSD_VIDEOTHSIZE,
    OSD_VIDEOTQSIZE,
    OSD_VIDEOTDSIZE,
    OSD_VIDEONSIZE,
    OSD_CURSOR
} osd_raw_window_t;
```

#### 6.2.3.3.1.3.2. Constants

|  |  |  |
| --- | --- | --- |
| `OSD_BITMAP1` | 1 bit bitmap | |
| `OSD_BITMAP2` | 2 bit bitmap | |
| `OSD_BITMAP4` | 4 bit bitmap | |
| `OSD_BITMAP8` | 8 bit bitmap | |
| `OSD_BITMAP1HR` | 1 Bit bitmap half resolution | |
| `OSD_BITMAP2HR` | 2 Bit bitmap half resolution | |
| `OSD_BITMAP4HR` | 4 Bit bitmap half resolution | |
| `OSD_BITMAP8HR` | 8 Bit bitmap half resolution | |
| `OSD_YCRCB422` | 4:2:2 YCRCB Graphic Display | |
| `OSD_YCRCB444` | 4:4:4 YCRCB Graphic Display | |
| `OSD_YCRCB444HR` | 4:4:4 YCRCB graphic half resolution | |
| `OSD_VIDEOTSIZE` | True Size Normal MPEG Video Display | |
| `OSD_VIDEOHSIZE` | MPEG Video Display Half Resolution | |
| `OSD_VIDEOQSIZE` | MPEG Video Display Quarter Resolution | |
| `OSD_VIDEODSIZE` | MPEG Video Display Double Resolution | |
| `OSD_VIDEOTHSIZE` | True Size MPEG Video Display Half Resolution | |
| `OSD_VIDEOTQSIZE` | True Size MPEG Video Display Quarter Resolution | |
| `OSD_VIDEOTDSIZE` | True Size MPEG Video Display Double Resolution | |
| `OSD_VIDEONSIZE` | Full Size MPEG Video Display | |
| `OSD_CURSOR` | Cursor | |

#### 6.2.3.3.1.3.3. Description

The `osd_raw_window_t` data type is used with the [OSD\_Command](#osd-command)
OSD\_OpenRaw to tell the driver which type of OSD to open.

---

### 6.2.3.3.1.4. osd\_cap\_t

#### 6.2.3.3.1.4.1. Synopsis

```
typedef struct osd_cap_s {
    int  cmd;
#define OSD_CAP_MEMSIZE         1
    long val;
} osd_cap_t;
```

#### 6.2.3.3.1.4.2. Variables

|  |  |
| --- | --- |
| `int  cmd` | Capability to query. |
| `long val` | Used to store the Data. |

#### 6.2.3.3.1.4.3. Supported capabilities

|  |  |
| --- | --- |
| `OSD_CAP_MEMSIZE` | Memory size installed on the card. |

#### 6.2.3.3.1.4.4. Description

This structure of data used with the [OSD\_GET\_CAPABILITY](#osd-get-capability) call.

---

## 6.2.3.3.2. OSD Function Calls

### 6.2.3.3.2.1. OSD\_SEND\_CMD

#### 6.2.3.3.2.1.1. Synopsis

OSD\_SEND\_CMD

```
int ioctl(int fd, int request = OSD_SEND_CMD, enum osd_cmd_t *cmd)
```

#### 6.2.3.3.2.1.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Pointer to the location of the structure [osd\_cmd\_t](#osd-cmd-t) for this command. | |

#### 6.2.3.3.2.1.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl sends the [OSD\_Command](#osd-command) to the card.

#### 6.2.3.3.2.1.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

|  |  |
| --- | --- |
| `EINVAL` | Command is out of range. |

---

### 6.2.3.3.2.2. OSD\_GET\_CAPABILITY

#### 6.2.3.3.2.2.1. Synopsis

OSD\_GET\_CAPABILITY

```
int ioctl(int fd, int request = OSD_GET_CAPABILITY,
struct osd_cap_t *cap)
```

#### 6.2.3.3.2.2.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open). | |
| `int request` | Equals `OSD_GET_CAPABILITY` for this command. | |
| `unsigned int *cap` | Pointer to the location of the structure [osd\_cap\_t](#osd-cap-t) for this command. | |

#### 6.2.3.3.2.2.3. Description

Attention

Do **not** use in new drivers!
See: [General Notes](legacy_dvb_decoder_api.html#legacy-dvb-decoder-notes)

This ioctl is used to get the capabilities of the OSD of the AV7110 based
DVB-decoder-card in use.

Note

The structure osd\_cap\_t has to be setup by the user and passed to the
driver.

#### 6.2.3.3.2.2.4. Return Value

On success 0 is returned, on error -1 and the `errno` variable is set
appropriately. The generic error codes are described at the
[Generic Error Codes](../gen-errors.html#id1) chapter.

|  |  |
| --- | --- |
| `EINVAL` | Unsupported capability. |

---

### 6.2.3.3.2.3. open()

#### 6.2.3.3.2.3.1. Synopsis

```
#include <fcntl.h>
```

int open(const char \*deviceName, int flags)

#### 6.2.3.3.2.3.2. Arguments

|  |  |  |
| --- | --- | --- |
| `const char *deviceName` | Name of specific OSD device. | |
| `int flags` | A bit-wise OR of the following flags: | |
| `O_RDONLY` | read-only access |
| `O_RDWR` | read/write access |
| `O_NONBLOCK` | Open in non-blocking mode  (blocking mode is the default) |

#### 6.2.3.3.2.3.3. Description

This system call opens a named OSD device (e.g.
`/dev/dvb/adapter?/osd0`) for subsequent use.

#### 6.2.3.3.2.3.4. Return Value

|  |  |
| --- | --- |
| `ENODEV` | Device driver not loaded/available. |
| `EINTERNAL` | Internal error. |
| `EBUSY` | Device or resource busy. |
| `EINVAL` | Invalid argument. |

---

### 6.2.3.3.2.4. close()

#### 6.2.3.3.2.4.1. Synopsis

int close(int fd)

#### 6.2.3.3.2.4.2. Arguments

|  |  |  |
| --- | --- | --- |
| `int fd` | File descriptor returned by a previous call to [open()](#open) . | |

#### 6.2.3.3.2.4.3. Description

This system call closes a previously opened OSD device.

#### 6.2.3.3.2.4.4. Return Value

|  |  |
| --- | --- |
| `EBADF` | fd is not a valid open file descriptor. |
