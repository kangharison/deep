# FunctionFS Descriptors

> 출처(원문): https://docs.kernel.org/usb/functionfs-desc.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# FunctionFS Descriptors

Some of the descriptors that can be written to the FFS gadget are
described below. Device and configuration descriptors are handled
by the composite gadget and are not written by the user to the
FFS gadget.

Descriptors are written to the “ep0” file in the FFS gadget
following the descriptor header.

Descriptors format:

|  |  |  |  |
| --- | --- | --- | --- |
| off | name | type | description |
| 0 | magic | LE32 | FUNCTIONFS\_DESCRIPTORS\_MAGIC\_V2 |
| 4 | length | LE32 | length of the whole data chunk |
| 8 | flags | LE32 | combination of functionfs\_flags |
|  | eventfd | LE32 | eventfd file descriptor |
|  | fs\_count | LE32 | number of full-speed descriptors |
|  | hs\_count | LE32 | number of high-speed descriptors |
|  | ss\_count | LE32 | number of super-speed descriptors |
|  | os\_count | LE32 | number of MS OS descriptors |
|  | fs\_descrs | Descriptor[] | list of full-speed descriptors |
|  | hs\_descrs | Descriptor[] | list of high-speed descriptors |
|  | ss\_descrs | Descriptor[] | list of super-speed descriptors |
|  | os\_descrs | OSDesc[] | list of MS OS descriptors |

Depending on which flags are set, various fields may be missing in the
structure. Any flags that are not recognised cause the whole block to be
rejected with -ENOSYS.

Legacy descriptors format (deprecated as of 3.14):

|  |  |  |  |
| --- | --- | --- | --- |
| off | name | type | description |
| 0 | magic | LE32 | FUNCTIONFS\_DESCRIPTORS\_MAGIC |
| 4 | length | LE32 | length of the whole data chunk |
| 8 | fs\_count | LE32 | number of full-speed descriptors |
| 12 | hs\_count | LE32 | number of high-speed descriptors |
| 16 | fs\_descrs | Descriptor[] | list of full-speed descriptors |
|  | hs\_descrs | Descriptor[] | list of high-speed descriptors |

All numbers must be in little endian order.

Descriptor[] is an array of valid USB descriptors which have the following
format:

|  |  |  |  |
| --- | --- | --- | --- |
| off | name | type | description |
| 0 | bLength | U8 | length of the descriptor |
| 1 | bDescriptorType | U8 | descriptor type |
| 2 | payload |  | descriptor’s payload |

OSDesc[] is an array of valid MS OS Feature Descriptors which have one of
the following formats:

|  |  |  |  |
| --- | --- | --- | --- |
| off | name | type | description |
| 0 | interface | U8 | related interface number |
| 1 | dwLength | U32 | length of the descriptor |
| 5 | bcdVersion | U16 | currently supported: 1 |
| 7 | wIndex | U16 | currently supported: 4 |
| 9 | bCount | U8 | number of ext. compat. |
| 10 | Reserved | U8 | 0 |
| 11 | ExtCompat[] |  | list of ext. compat. d. |

|  |  |  |  |
| --- | --- | --- | --- |
| off | name | type | description |
| 0 | interface | U8 | related interface number |
| 1 | dwLength | U32 | length of the descriptor |
| 5 | bcdVersion | U16 | currently supported: 1 |
| 7 | wIndex | U16 | currently supported: 5 |
| 9 | wCount | U16 | number of ext. compat. |
| 11 | ExtProp[] |  | list of ext. prop. d. |

ExtCompat[] is an array of valid Extended Compatibility descriptors
which have the following format:

|  |  |  |  |
| --- | --- | --- | --- |
| off | name | type | description |
| 0 | bFirstInterfaceNumber | U8 | index of the interface or of the 1st |
|  |  |  | interface in an IAD group |
| 1 | Reserved | U8 | 1 |
| 2 | CompatibleID | U8[8] | compatible ID string |
| 10 | SubCompatibleID | U8[8] | subcompatible ID string |
| 18 | Reserved | U8[6] | 0 |

ExtProp[] is an array of valid Extended Properties descriptors
which have the following format:

|  |  |  |  |
| --- | --- | --- | --- |
| off | name | type | description |
| 0 | dwSize | U32 | length of the descriptor |
| 4 | dwPropertyDataType | U32 | 1..7 |
| 8 | wPropertyNameLength | U16 | bPropertyName length (NL) |
| 10 | bPropertyName | U8[NL] | name of this property |
| 10+NL | dwPropertyDataLength | U32 | bPropertyData length (DL) |
| 14+NL | bProperty | U8[DL] | payload of this property |

## Interface Descriptors

Standard USB interface descriptors may be written. The class/subclass of the
most recent interface descriptor determines what type of class-specific
descriptors are accepted.

## Class-Specific Descriptors

Class-specific descriptors are accepted only for the class/subclass of the
most recent interface descriptor. The following are some of the
class-specific descriptors that are supported.

### DFU Functional Descriptor

When the interface class is USB\_CLASS\_APP\_SPEC and the interface subclass
is USB\_SUBCLASS\_DFU, a DFU functional descriptor can be provided.
The DFU functional descriptor is a described in the USB specification for
Device Firmware Upgrade (DFU), version 1.1 as of this writing.

struct usb\_dfu\_functional\_descriptor
:   DFU Functional descriptor

**Definition**:

```
struct usb_dfu_functional_descriptor {
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 bmAttributes;
    __le16 wDetachTimeOut;
    __le16 wTransferSize;
    __le16 bcdDFUVersion;
};
```

**Members**

`bLength`
:   Size of the descriptor (bytes)

`bDescriptorType`
:   USB\_DT\_DFU\_FUNCTIONAL

`bmAttributes`
:   DFU attributes

`wDetachTimeOut`
:   Maximum time to wait after DFU\_DETACH (ms, le16)

`wTransferSize`
:   Maximum number of bytes per control-write (le16)

`bcdDFUVersion`
:   DFU Spec version (BCD, le16)
