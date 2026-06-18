# 4.1.CA Data Types

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/ca_data_types.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 4.1. CA Data Types

struct ca\_slot\_info
:   CA slot interface types and info.

**Definition**:

```
struct ca_slot_info {
    int num;
    int type;
#define CA_CI            1;
#define CA_CI_LINK       2;
#define CA_CI_PHYS       4;
#define CA_DESCR         8;
#define CA_SC          128;
    unsigned int flags;
#define CA_CI_MODULE_PRESENT 1;
#define CA_CI_MODULE_READY   2;
};
```

**Members**

`num`
:   slot number.

`type`
:   slot type.

`flags`
:   flags applicable to the slot.

**Description**

This `struct stores` the CA slot information.

**type** can be:

> * `CA_CI` - CI high level interface;
> * `CA_CI_LINK` - CI link layer level interface;
> * `CA_CI_PHYS` - CI physical layer level interface;
> * `CA_DESCR` - built-in descrambler;
> * `CA_SC` -simple smart card interface.

**flags** can be:

> * `CA_CI_MODULE_PRESENT` - module (or card) inserted;
> * `CA_CI_MODULE_READY` - module is ready for usage.

struct ca\_descr\_info
:   descrambler types and info.

**Definition**:

```
struct ca_descr_info {
    unsigned int num;
    unsigned int type;
#define CA_ECD           1;
#define CA_NDS           2;
#define CA_DSS           4;
};
```

**Members**

`num`
:   number of available descramblers (keys).

`type`
:   type of supported scrambling system.

**Description**

Identifies the number of descramblers and their type.

**type** can be:

> * `CA_ECD` - European Common Descrambler (ECD) hardware;
> * `CA_NDS` - Videoguard (NDS) hardware;
> * `CA_DSS` - Distributed Sample Scrambling (DSS) hardware.

struct ca\_caps
:   CA slot interface capabilities.

**Definition**:

```
struct ca_caps {
    unsigned int slot_num;
    unsigned int slot_type;
    unsigned int descr_num;
    unsigned int descr_type;
};
```

**Members**

`slot_num`
:   total number of CA card and module slots.

`slot_type`
:   bitmap with all supported types as defined at
    [`struct ca_slot_info`](#c.ca_slot_info "ca_slot_info") (e. g. `CA_CI`, `CA_CI_LINK`, etc).

`descr_num`
:   total number of descrambler slots (keys)

`descr_type`
:   bitmap with all supported types as defined at
    [`struct ca_descr_info`](#c.ca_descr_info "ca_descr_info") (e. g. `CA_ECD`, `CA_NDS`, etc).

struct ca\_msg
:   a message to/from a CI-CAM

**Definition**:

```
struct ca_msg {
    unsigned int index;
    unsigned int type;
    unsigned int length;
    unsigned char msg[256];
};
```

**Members**

`index`
:   unused

`type`
:   unused

`length`
:   length of the message

`msg`
:   message

**Description**

This `struct carries` a message to be send/received from a CI CA module.

struct ca\_descr
:   CA descrambler control words info

**Definition**:

```
struct ca_descr {
    unsigned int index;
    unsigned int parity;
    unsigned char cw[8];
};
```

**Members**

`index`
:   CA Descrambler slot

`parity`
:   control words parity, where 0 means even and 1 means odd

`cw`
:   CA Descrambler control words
