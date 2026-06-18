# Bare UDP Tunnelling Module Documentation

> 출처(원문): https://docs.kernel.org/networking/bareudp.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Bare UDP Tunnelling Module Documentation

There are various L3 encapsulation standards using UDP being discussed to
leverage the UDP based load balancing capability of different networks.
MPLSoUDP (<https://tools.ietf.org/html/rfc7510>) is one among them.

The Bareudp tunnel module provides a generic L3 encapsulation support for
tunnelling different L3 protocols like MPLS, IP, NSH etc. inside a UDP tunnel.

## Special Handling

The bareudp device supports special handling for MPLS & IP as they can have
multiple ethertypes.
The MPLS protocol can have ethertypes ETH\_P\_MPLS\_UC (unicast) & ETH\_P\_MPLS\_MC (multicast).
IP protocol can have ethertypes ETH\_P\_IP (v4) & ETH\_P\_IPV6 (v6).
This special handling can be enabled only for ethertypes ETH\_P\_IP & ETH\_P\_MPLS\_UC
with a flag called multiproto mode.

## Usage

1. Device creation & deletion

   > 1. ip link add dev bareudp0 type bareudp dstport 6635 ethertype mpls\_uc
   >
   >    This creates a bareudp tunnel device which tunnels L3 traffic with ethertype
   >    0x8847 (MPLS traffic). The destination port of the UDP header will be set to
   >    6635.The device will listen on UDP port 6635 to receive traffic.
   > 2. ip link delete bareudp0
2. Device creation with multiproto mode enabled

The multiproto mode allows bareudp tunnels to handle several protocols of the
same family. It is currently only available for IP and MPLS. This mode has to
be enabled explicitly with the “multiproto” flag.

> 1. ip link add dev bareudp0 type bareudp dstport 6635 ethertype ipv4 multiproto
>
>    For an IPv4 tunnel the multiproto mode allows the tunnel to also handle
>    IPv6.
> 2. ip link add dev bareudp0 type bareudp dstport 6635 ethertype mpls\_uc multiproto
>
>    For MPLS, the multiproto mode allows the tunnel to handle both unicast
>    and multicast MPLS packets.

3. Device Usage

The bareudp device could be used along with OVS or flower filter in TC.
The OVS or TC flower layer must set the tunnel information in the SKB dst field before
sending the packet buffer to the bareudp device for transmission. On reception, the
bareUDP device extracts and stores the tunnel information in the SKB dst field before
passing the packet buffer to the network stack.
