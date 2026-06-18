# 5.1.1.Net Data Types

> 출처(원문): https://docs.kernel.org/userspace-api/media/dvb/net-types.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 5.1.1. Net Data Types

struct dvb\_net\_if
:   describes a DVB network interface

**Definition**:

```
struct dvb_net_if {
    __u16 pid;
    __u16 if_num;
    __u8 feedtype;
#define DVB_NET_FEEDTYPE_MPE 0  ;
#define DVB_NET_FEEDTYPE_ULE 1  ;
};
```

**Members**

`pid`
:   Packet ID (PID) of the MPEG-TS that contains data

`if_num`
:   number of the Digital TV interface.

`feedtype`
:   Encapsulation type of the feed.

**Description**

A MPEG-TS stream may contain packet IDs with IP packages on it.
This `struct describes` it, and the type of encoding.

**feedtype** can be:

> * `DVB_NET_FEEDTYPE_MPE` for MPE encoding
> * `DVB_NET_FEEDTYPE_ULE` for ULE encoding.
