# struct sk_buff

> 출처(원문): https://docs.kernel.org/networking/skbuff.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# struct sk\_buff

[`sk_buff`](kapi.html#c.sk_buff "sk_buff") is the main networking structure representing
a packet.

## Basic sk\_buff geometry

[`struct sk_buff`](kapi.html#c.sk_buff "sk_buff") itself is a metadata structure and does not hold any packet
data. All the data is held in associated buffers.

[`sk_buff.head`](kapi.html#c.sk_buff "sk_buff") points to the main “head” buffer. The head buffer is divided
into two parts:

> * data buffer, containing headers and sometimes payload;
>   this is the part of the skb operated on by the common helpers
>   such as [`skb_put()`](kapi.html#c.skb_put "skb_put") or [`skb_pull()`](kapi.html#c.skb_pull "skb_pull");
> * shared info (`struct skb_shared_info`) which holds an array of pointers
>   to read-only data in the (page, offset, length) format.

Optionally `skb_shared_info.frag_list` may point to another skb.

Basic diagram may look like this:

```
                                ---------------
                               | sk_buff       |
                                ---------------
   ,---------------------------  + head
  /          ,-----------------  + data
 /          /      ,-----------  + tail
|          |      |            , + end
|          |      |           |
v          v      v           v
 -----------------------------------------------
| headroom | data |  tailroom | skb_shared_info |
 -----------------------------------------------
                               + [page frag]
                               + [page frag]
                               + [page frag]
                               + [page frag]       ---------
                               + frag_list    --> | sk_buff |
                                                   ---------
```

## Shared skbs and skb clones

`sk_buff.users` is a simple refcount allowing multiple entities
to keep a [`struct sk_buff`](kapi.html#c.sk_buff "sk_buff") alive. skbs with a `sk_buff.users != 1` are referred
to as shared skbs (see [`skb_shared()`](kapi.html#c.skb_shared "skb_shared")).

[`skb_clone()`](kapi.html#c.skb_clone "skb_clone") allows for fast duplication of skbs. None of the data buffers
get copied, but caller gets a new metadata struct ([`struct sk_buff`](kapi.html#c.sk_buff "sk_buff")).
&skb\_shared\_info.refcount indicates the number of skbs pointing at the same
packet data (i.e. clones).

## dataref and headerless skbs

Transport layers send out clones of payload skbs they hold for
retransmissions. To allow lower layers of the stack to prepend their headers
we split `skb_shared_info.dataref` into two halves.
The lower 16 bits count the overall number of references.
The higher 16 bits indicate how many of the references are payload-only.
[`skb_header_cloned()`](kapi.html#c.skb_header_cloned "skb_header_cloned") checks if skb is allowed to add / write the headers.

The creator of the skb (e.g. TCP) marks its skb as [`sk_buff.nohdr`](kapi.html#c.sk_buff "sk_buff")
(via [`__skb_header_release()`](kapi.html#c.__skb_header_release "__skb_header_release")). Any clone created from marked skb will get
[`sk_buff.hdr_len`](kapi.html#c.sk_buff "sk_buff") populated with the available headroom.
If there’s the only clone in existence it’s able to modify the headroom
at will. The sequence of calls inside the transport layer is:

```
<alloc skb>
skb_reserve()
__skb_header_release()
skb_clone()
// send the clone down the stack
```

This is not a very generic construct and it depends on the transport layers
doing the right thing. In practice there’s usually only one payload-only skb.
Having multiple payload-only skbs with different lengths of hdr\_len is not
possible. The payload-only skbs should never leave their owner.

## Checksum information

The interface for checksum offload between the stack and networking drivers
is as follows...

### IP checksum related features

Drivers advertise checksum offload capabilities in the features of a device.
From the stack’s point of view these are capabilities offered by the driver.
A driver typically only advertises features that it is capable of offloading
to its device.

Checksum related device features

|  |  |
| --- | --- |
| `NETIF_F_HW_CSUM` | The driver (or its device) is able to compute one IP (one’s complement) checksum for any combination of protocols or protocol layering. The checksum is computed and set in a packet per the CHECKSUM\_PARTIAL interface (see below). |
| `NETIF_F_IP_CSUM` | Driver (device) is only able to checksum plain TCP or UDP packets over IPv4. These are specifically unencapsulated packets of the form IPv4|TCP or IPv4|UDP where the Protocol field in the IPv4 header is TCP or UDP. The IPv4 header may contain IP options. This feature cannot be set in features for a device with NETIF\_F\_HW\_CSUM also set. This feature is being DEPRECATED (see below). |
| `NETIF_F_IPV6_CSUM` | Driver (device) is only able to checksum plain TCP or UDP packets over IPv6. These are specifically unencapsulated packets of the form IPv6|TCP or IPv6|UDP where the Next Header field in the IPv6 header is either TCP or UDP. IPv6 extension headers are not supported with this feature. This feature cannot be set in features for a device with NETIF\_F\_HW\_CSUM also set. This feature is being DEPRECATED (see below). |
| `NETIF_F_RXCSUM` | Driver (device) performs receive checksum offload. This flag is only used to disable the RX checksum feature for a device. The stack will accept receive checksum indication in packets received on a device regardless of whether NETIF\_F\_RXCSUM is set. |

### Checksumming of received packets by device

Indication of checksum verification is set in [`sk_buff.ip_summed`](kapi.html#c.sk_buff "sk_buff").
Possible values are:

* `CHECKSUM_NONE`

  Device did not checksum this packet e.g. due to lack of capabilities.
  The packet contains full (though not verified) checksum in packet but
  not in skb->csum. Thus, skb->csum is undefined in this case.
* `CHECKSUM_UNNECESSARY`

  The hardware you’re dealing with doesn’t calculate the full checksum
  (as in `CHECKSUM_COMPLETE`), but it does parse headers and verify checksums
  for specific protocols. For such packets it will set `CHECKSUM_UNNECESSARY`
  if their checksums are okay. [`sk_buff.csum`](kapi.html#c.sk_buff "sk_buff") is still undefined in this case
  though. A driver or device must never modify the checksum field in the
  packet even if checksum is verified.

  `CHECKSUM_UNNECESSARY` is applicable to following protocols:

  > + TCP: IPv6 and IPv4.
  > + UDP: IPv4 and IPv6. A device may apply CHECKSUM\_UNNECESSARY to a
  >   zero UDP checksum for either IPv4 or IPv6, the networking stack
  >   may perform further validation in this case.
  > + GRE: only if the checksum is present in the header.
  > + SCTP: indicates the CRC in SCTP header has been validated.
  > + FCOE: indicates the CRC in FC frame has been validated.

  [`sk_buff.csum_level`](kapi.html#c.sk_buff "sk_buff") indicates the number of consecutive checksums found in
  the packet minus one that have been verified as `CHECKSUM_UNNECESSARY`.
  For instance if a device receives an IPv6->UDP->GRE->IPv4->TCP packet
  and a device is able to verify the checksums for UDP (possibly zero),
  GRE (checksum flag is set) and TCP, [`sk_buff.csum_level`](kapi.html#c.sk_buff "sk_buff") would be set to
  two. If the device were only able to verify the UDP checksum and not
  GRE, either because it doesn’t support GRE checksum or because GRE
  checksum is bad, skb->csum\_level would be set to zero (TCP checksum is
  not considered in this case).
* `CHECKSUM_COMPLETE`

  This is the most generic way. The device supplied checksum of the \_whole\_
  packet as seen by [`netif_rx()`](kapi.html#c.netif_rx "netif_rx") and fills in [`sk_buff.csum`](kapi.html#c.sk_buff "sk_buff"). This means the
  hardware doesn’t need to parse L3/L4 headers to implement this.

  Notes:

  + Even if device supports only some protocols, but is able to produce
    skb->csum, it MUST use CHECKSUM\_COMPLETE, not CHECKSUM\_UNNECESSARY.
  + CHECKSUM\_COMPLETE is not applicable to SCTP and FCoE protocols.
* `CHECKSUM_PARTIAL`

  A checksum is set up to be offloaded to a device as described in the
  output description for CHECKSUM\_PARTIAL. This may occur on a packet
  received directly from another Linux OS, e.g., a virtualized Linux kernel
  on the same host, or it may be set in the input path in GRO or remote
  checksum offload. For the purposes of checksum verification, the checksum
  referred to by skb->csum\_start + skb->csum\_offset and any preceding
  checksums in the packet are considered verified. Any checksums in the
  packet that are after the checksum being offloaded are not considered to
  be verified.

### Checksumming on transmit for non-GSO

The stack requests checksum offload in the [`sk_buff.ip_summed`](kapi.html#c.sk_buff "sk_buff") for a packet.
Values are:

* `CHECKSUM_PARTIAL`

  The driver is required to checksum the packet as seen by `hard_start_xmit()`
  from [`sk_buff.csum_start`](kapi.html#c.sk_buff "sk_buff") up to the end, and to record/write the checksum at
  offset [`sk_buff.csum_start`](kapi.html#c.sk_buff "sk_buff") + [`sk_buff.csum_offset`](kapi.html#c.sk_buff "sk_buff").
  A driver may verify that the
  csum\_start and csum\_offset values are valid values given the length and
  offset of the packet, but it should not attempt to validate that the
  checksum refers to a legitimate transport layer checksum -- it is the
  purview of the stack to validate that csum\_start and csum\_offset are set
  correctly.

  When the stack requests checksum offload for a packet, the driver MUST
  ensure that the checksum is set correctly. A driver can either offload the
  checksum calculation to the device, or call skb\_checksum\_help (in the case
  that the device does not support offload for a particular checksum).

  `NETIF_F_IP_CSUM` and `NETIF_F_IPV6_CSUM` are being deprecated in favor of
  `NETIF_F_HW_CSUM`. New devices should use `NETIF_F_HW_CSUM` to indicate
  checksum offload capability.
  `skb_csum_hwoffload_help()` can be called to resolve `CHECKSUM_PARTIAL` based
  on network device checksumming capabilities: if a packet does not match
  them, `skb_checksum_help()` or `skb_crc32c_help()` (depending on the value of
  [`sk_buff.csum_not_inet`](kapi.html#c.sk_buff "sk_buff"), see [Non-IP checksum (CRC) offloads](#crc))
  is called to resolve the checksum.
* `CHECKSUM_NONE`

  The skb was already checksummed by the protocol, or a checksum is not
  required.
* `CHECKSUM_UNNECESSARY`

  This has the same meaning as CHECKSUM\_NONE for checksum offload on
  output.
* `CHECKSUM_COMPLETE`

  Not used in checksum output. If a driver observes a packet with this value
  set in skbuff, it should treat the packet as if `CHECKSUM_NONE` were set.

### Non-IP checksum (CRC) offloads

|  |  |
| --- | --- |
| `NETIF_F_SCTP_CRC` | This feature indicates that a device is capable of offloading the SCTP CRC in a packet. To perform this offload the stack will set csum\_start and csum\_offset accordingly, set ip\_summed to `CHECKSUM_PARTIAL` and set csum\_not\_inet to 1, to provide an indication in the skbuff that the `CHECKSUM_PARTIAL` refers to CRC32c. A driver that supports both IP checksum offload and SCTP CRC32c offload must verify which offload is configured for a packet by testing the value of [`sk_buff.csum_not_inet`](kapi.html#c.sk_buff "sk_buff"); `skb_crc32c_csum_help()` is provided to resolve `CHECKSUM_PARTIAL` on skbs where csum\_not\_inet is set to 1. |
| `NETIF_F_FCOE_CRC` | This feature indicates that a device is capable of offloading the FCOE CRC in a packet. To perform this offload the stack will set ip\_summed to `CHECKSUM_PARTIAL` and set csum\_start and csum\_offset accordingly. Note that there is no indication in the skbuff that the `CHECKSUM_PARTIAL` refers to an FCOE checksum, so a driver that supports both IP checksum offload and FCOE CRC offload must verify which offload is configured for a packet, presumably by inspecting packet headers. |

### Checksumming on output with GSO

In the case of a GSO packet (`skb_is_gso()` is true), checksum offload
is implied by the SKB\_GSO\_\* flags in gso\_type. Most obviously, if the
gso\_type is `SKB_GSO_TCPV4` or `SKB_GSO_TCPV6`, TCP checksum offload as
part of the GSO operation is implied. If a checksum is being offloaded
with GSO then ip\_summed is `CHECKSUM_PARTIAL`, and both csum\_start and
csum\_offset are set to refer to the outermost checksum being offloaded
(two offloaded checksums are possible with UDP encapsulation).
