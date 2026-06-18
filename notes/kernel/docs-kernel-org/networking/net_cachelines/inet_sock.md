# inet_sock struct fast path usage breakdown

> 출처(원문): https://docs.kernel.org/networking/net_cachelines/inet_sock.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# inet\_sock struct fast path usage breakdown

| Type | Name | fastpath\_tx\_access | fastpath\_rx\_access | comment |
| --- | --- | --- | --- | --- |
| [`struct sock`](../kapi.html#c.sock "sock") | sk | read\_mostly | read\_mostly | tcp\_init\_buffer\_space,tcp\_init\_transfer,tcp\_finish\_connect,tcp\_connect,tcp\_send\_rcvq,tcp\_send\_syn\_data |
| `struct ipv6_pinfo`\* | pinet6 |  |  |  |
| `struct ipv6_fl_socklist`\* | ipv6\_fl\_list | read\_mostly |  | tcp\_v6\_connect,\_\_ip6\_datagram\_connect,udpv6\_sendmsg,rawv6\_sendmsg |
| be16 | inet\_sport | read\_mostly |  | \_\_tcp\_transmit\_skb |
| be32 | inet\_daddr | read\_mostly |  | ip\_select\_ident\_segs |
| be32 | inet\_rcv\_saddr |  |  |  |
| be16 | inet\_dport | read\_mostly |  | \_\_tcp\_transmit\_skb |
| u16 | inet\_num |  |  |  |
| be32 | inet\_saddr |  |  |  |
| s16 | uc\_ttl | read\_mostly |  | \_\_ip\_queue\_xmit/ip\_select\_ttl |
| u16 | cmsg\_flags |  |  |  |
| `struct ip_options_rcu`\* | inet\_opt | read\_mostly |  | \_\_ip\_queue\_xmit |
| u16 | inet\_id | read\_mostly |  | ip\_select\_ident\_segs |
| u8 | tos | read\_mostly |  | ip\_queue\_xmit |
| u8 | min\_ttl |  |  |  |
| u8 | mc\_ttl |  |  |  |
| u8 | pmtudisc |  |  |  |
| u8:1 | recverr |  |  |  |
| u8:1 | is\_icsk |  |  |  |
| u8:1 | freebind |  |  |  |
| u8:1 | hdrincl |  |  |  |
| u8:1 | mc\_loop |  |  |  |
| u8:1 | transparent |  |  |  |
| u8:1 | mc\_all |  |  |  |
| u8:1 | nodefrag |  |  |  |
| u8:1 | bind\_address\_no\_port |  |  |  |
| u8:1 | recverr\_rfc4884 |  |  |  |
| u8:1 | defer\_connect | read\_mostly |  | tcp\_sendmsg\_fastopen |
| u8 | rcv\_tos |  |  |  |
| u8 | convert\_csum |  |  |  |
| int | uc\_index |  |  |  |
| int | mc\_index |  |  |  |
| be32 | mc\_addr |  |  |  |
| `struct ip_mc_socklist`\* | mc\_list |  |  |  |
| `struct inet_cork_full` | cork | read\_mostly |  | \_\_tcp\_transmit\_skb |
| struct | local\_port\_range |  |  |  |
