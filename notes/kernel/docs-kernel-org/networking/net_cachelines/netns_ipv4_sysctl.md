# netns_ipv4 struct fast path usage breakdown

> 출처(원문): https://docs.kernel.org/networking/net_cachelines/netns_ipv4_sysctl.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# netns\_ipv4 struct fast path usage breakdown

| Type | Name | fastpath\_tx\_access | fastpath\_rx\_access | comment |
| --- | --- | --- | --- | --- |
| struct\_inet\_timewait\_death\_row | tcp\_death\_row |  |  |  |
| struct\_udp\_table\* | udp\_table |  |  |  |
| struct\_ctl\_table\_header\* | forw\_hdr |  |  |  |
| struct\_ctl\_table\_header\* | frags\_hdr |  |  |  |
| struct\_ctl\_table\_header\* | ipv4\_hdr |  |  |  |
| struct\_ctl\_table\_header\* | route\_hdr |  |  |  |
| struct\_ctl\_table\_header\* | xfrm4\_hdr |  |  |  |
| struct\_ipv4\_devconf\* | devconf\_all |  |  |  |
| struct\_ipv4\_devconf\* | devconf\_dflt |  |  |  |
| struct\_ip\_ra\_chain | ra\_chain |  |  |  |
| struct\_mutex | ra\_mutex |  |  |  |
| struct\_fib\_rules\_ops\* | rules\_ops |  |  |  |
| struct\_fib\_table | fib\_main |  |  |  |
| struct\_fib\_table | fib\_default |  |  |  |
| unsigned\_int | fib\_rules\_require\_fldissect |  |  |  |
| bool | fib\_has\_custom\_rules |  |  |  |
| bool | fib\_has\_custom\_local\_routes |  |  |  |
| bool | fib\_offload\_disabled |  |  |  |
| atomic\_t | fib\_num\_tclassid\_users |  |  |  |
| struct\_hlist\_head\* | fib\_table\_hash |  |  |  |
| struct\_sock\* | fibnl |  |  |  |
| struct\_sock\* | mc\_autojoin\_sk |  |  |  |
| struct\_inet\_peer\_base\* | peers |  |  |  |
| struct\_fqdir\* | fqdir |  |  |  |
| u8 | sysctl\_icmp\_echo\_ignore\_all |  |  |  |
| u8 | sysctl\_icmp\_echo\_enable\_probe |  |  |  |
| u8 | sysctl\_icmp\_echo\_ignore\_broadcasts |  |  |  |
| u8 | sysctl\_icmp\_ignore\_bogus\_error\_responses |  |  |  |
| u8 | sysctl\_icmp\_errors\_use\_inbound\_ifaddr |  |  |  |
| int | sysctl\_icmp\_ratelimit |  |  |  |
| int | sysctl\_icmp\_ratemask |  |  |  |
| u32 | ip\_rt\_min\_pmtu |  |  |  |
| int | ip\_rt\_mtu\_expires |  |  |  |
| int | ip\_rt\_min\_advmss |  |  |  |
| struct\_local\_ports | ip\_local\_ports |  |  |  |
| u8 | sysctl\_tcp\_ecn |  |  |  |
| u8 | sysctl\_tcp\_ecn\_fallback |  |  |  |
| u8 | sysctl\_ip\_default\_ttl |  |  | ip4\_dst\_hoplimit/ip\_select\_ttl |
| u8 | sysctl\_ip\_no\_pmtu\_disc |  |  |  |
| u8 | sysctl\_ip\_fwd\_use\_pmtu | read\_mostly |  | ip\_dst\_mtu\_maybe\_forward/ip\_skb\_dst\_mtu |
| u8 | sysctl\_ip\_fwd\_update\_priority |  |  | ip\_forward |
| u8 | sysctl\_ip\_nonlocal\_bind |  |  |  |
| u8 | sysctl\_ip\_autobind\_reuse |  |  |  |
| u8 | sysctl\_ip\_dynaddr |  |  |  |
| u32 | sysctl\_ip\_local\_port\_step\_width |  |  |  |
| u8 | sysctl\_ip\_early\_demux |  | read\_mostly | ip(6)\_rcv\_finish\_core |
| u8 | sysctl\_raw\_l3mdev\_accept |  |  |  |
| u8 | sysctl\_tcp\_early\_demux |  | read\_mostly | ip(6)\_rcv\_finish\_core |
| u8 | sysctl\_udp\_early\_demux |  |  |  |
| u8 | sysctl\_nexthop\_compat\_mode |  |  |  |
| u8 | sysctl\_fwmark\_reflect |  |  |  |
| u8 | sysctl\_tcp\_fwmark\_accept |  |  |  |
| u8 | sysctl\_tcp\_l3mdev\_accept |  | read\_mostly | \_\_inet6\_lookup\_established/inet\_request\_bound\_dev\_if |
| u8 | sysctl\_tcp\_mtu\_probing |  |  |  |
| int | sysctl\_tcp\_mtu\_probe\_floor |  |  |  |
| int | sysctl\_tcp\_base\_mss |  |  |  |
| int | sysctl\_tcp\_min\_snd\_mss | read\_mostly |  | \_\_tcp\_mtu\_to\_mss(tcp\_write\_xmit) |
| int | sysctl\_tcp\_probe\_threshold |  |  | tcp\_mtu\_probe(tcp\_write\_xmit) |
| u32 | sysctl\_tcp\_probe\_interval |  |  | tcp\_mtu\_check\_reprobe(tcp\_write\_xmit) |
| int | sysctl\_tcp\_keepalive\_time |  |  |  |
| int | sysctl\_tcp\_keepalive\_intvl |  |  |  |
| u8 | sysctl\_tcp\_keepalive\_probes |  |  |  |
| u8 | sysctl\_tcp\_syn\_retries |  |  |  |
| u8 | sysctl\_tcp\_synack\_retries |  |  |  |
| u8 | sysctl\_tcp\_syncookies |  |  | generated\_on\_syn |
| u8 | sysctl\_tcp\_migrate\_req |  |  | reuseport |
| u8 | sysctl\_tcp\_comp\_sack\_nr |  |  | \_\_tcp\_ack\_snd\_check |
| int | sysctl\_tcp\_reordering |  | read\_mostly | tcp\_may\_raise\_cwnd/tcp\_cong\_control |
| u8 | sysctl\_tcp\_retries1 |  |  |  |
| u8 | sysctl\_tcp\_retries2 |  |  |  |
| u8 | sysctl\_tcp\_orphan\_retries |  |  |  |
| u8 | sysctl\_tcp\_tw\_reuse |  |  | timewait\_sock\_ops |
| unsigned\_int | sysctl\_tcp\_tw\_reuse\_delay |  |  | timewait\_sock\_ops |
| int | sysctl\_tcp\_fin\_timeout |  |  | TCP\_LAST\_ACK/tcp\_rcv\_state\_process |
| unsigned\_int | sysctl\_tcp\_notsent\_lowat | read\_mostly |  | tcp\_notsent\_lowat/tcp\_stream\_memory\_free |
| u8 | sysctl\_tcp\_sack |  |  | tcp\_syn\_options |
| u8 | sysctl\_tcp\_window\_scaling |  |  | tcp\_syn\_options,tcp\_parse\_options |
| u8 | sysctl\_tcp\_timestamps |  |  |  |
| u8 | sysctl\_tcp\_early\_retrans | read\_mostly |  | tcp\_schedule\_loss\_probe(tcp\_write\_xmit) |
| u32 | sysctl\_tcp\_rto\_max\_ms |  |  |  |
| u8 | sysctl\_tcp\_recovery |  |  | tcp\_fastretrans\_alert |
| u8 | sysctl\_tcp\_thin\_linear\_timeouts |  |  | tcp\_retrans\_timer(on\_thin\_streams) |
| u8 | sysctl\_tcp\_slow\_start\_after\_idle |  |  | unlikely(tcp\_cwnd\_validate-network-not-starved) |
| u8 | sysctl\_tcp\_retrans\_collapse |  |  |  |
| u8 | sysctl\_tcp\_stdurg |  |  | unlikely(tcp\_check\_urg) |
| u8 | sysctl\_tcp\_rfc1337 |  |  |  |
| u8 | sysctl\_tcp\_abort\_on\_overflow |  |  |  |
| u8 | sysctl\_tcp\_fack |  |  |  |
| int | sysctl\_tcp\_max\_reordering |  |  | tcp\_check\_sack\_reordering |
| int | sysctl\_tcp\_adv\_win\_scale |  |  | tcp\_init\_buffer\_space |
| u8 | sysctl\_tcp\_dsack |  |  | partial\_packet\_or\_retrans\_in\_tcp\_data\_queue |
| u8 | sysctl\_tcp\_app\_win |  |  | tcp\_win\_from\_space |
| u8 | sysctl\_tcp\_frto |  |  | tcp\_enter\_loss |
| u8 | sysctl\_tcp\_nometrics\_save |  |  | TCP\_LAST\_ACK/tcp\_update\_metrics |
| u8 | sysctl\_tcp\_no\_ssthresh\_metrics\_save |  |  | TCP\_LAST\_ACK/tcp\_(update/init)\_metrics |
| u8 | sysctl\_tcp\_moderate\_rcvbuf |  | read\_mostly | `tcp_rcvbuf_grow()` |
| u32 | sysctl\_tcp\_rcvbuf\_low\_rtt |  | read\_mostly | `tcp_rcvbuf_grow()` |
| u8 | sysctl\_tcp\_shrink\_window | read\_mostly | read\_mostly | `__tcp_select_window()` |
| u8 | sysctl\_tcp\_tso\_win\_divisor | read\_mostly |  | tcp\_tso\_should\_defer(tcp\_write\_xmit) |
| u8 | sysctl\_tcp\_workaround\_signed\_windows |  |  | tcp\_select\_window |
| int | sysctl\_tcp\_limit\_output\_bytes | read\_mostly |  | tcp\_small\_queue\_check(tcp\_write\_xmit) |
| int | sysctl\_tcp\_challenge\_ack\_limit |  |  |  |
| int | sysctl\_tcp\_min\_rtt\_wlen | read\_mostly |  | tcp\_ack\_update\_rtt |
| u8 | sysctl\_tcp\_min\_tso\_segs |  |  | unlikely(icsk\_ca\_ops-written) |
| u8 | sysctl\_tcp\_tso\_rtt\_log | read\_mostly |  | tcp\_tso\_autosize |
| u8 | sysctl\_tcp\_autocorking | read\_mostly |  | tcp\_push/tcp\_should\_autocork |
| u8 | sysctl\_tcp\_reflect\_tos |  |  | tcp\_v(4/6)\_send\_synack |
| int | sysctl\_tcp\_invalid\_ratelimit |  |  |  |
| int | sysctl\_tcp\_pacing\_ss\_ratio |  |  | default\_cong\_cont(tcp\_update\_pacing\_rate) |
| int | sysctl\_tcp\_pacing\_ca\_ratio |  |  | default\_cong\_cont(tcp\_update\_pacing\_rate) |
| int | sysctl\_tcp\_wmem[3] | read\_mostly |  | tcp\_wmem\_schedule(sendmsg/sendpage) |
| int | sysctl\_tcp\_rmem[3] |  | read\_mostly | \_\_tcp\_grow\_window(tx),tcp\_rcv\_space\_adjust(rx) |
| unsigned\_int | sysctl\_tcp\_child\_ehash\_entries |  |  |  |
| unsigned\_long | sysctl\_tcp\_comp\_sack\_delay\_ns |  |  | \_\_tcp\_ack\_snd\_check |
| unsigned\_long | sysctl\_tcp\_comp\_sack\_slack\_ns |  |  | \_\_tcp\_ack\_snd\_check |
| int | sysctl\_max\_syn\_backlog |  |  |  |
| int | sysctl\_tcp\_fastopen |  |  |  |
| struct\_tcp\_congestion\_ops | tcp\_congestion\_control |  |  | init\_cc |
| struct\_tcp\_fastopen\_context | tcp\_fastopen\_ctx |  |  |  |
| unsigned\_int | sysctl\_tcp\_fastopen\_blackhole\_timeout |  |  |  |
| atomic\_t | tfo\_active\_disable\_times |  |  |  |
| unsigned\_long | tfo\_active\_disable\_stamp |  |  |  |
| u32 | tcp\_challenge\_timestamp |  |  |  |
| u32 | tcp\_challenge\_count |  |  |  |
| u8 | sysctl\_tcp\_plb\_enabled |  |  |  |
| u8 | sysctl\_tcp\_plb\_idle\_rehash\_rounds |  |  |  |
| u8 | sysctl\_tcp\_plb\_rehash\_rounds |  |  |  |
| u8 | sysctl\_tcp\_plb\_suspend\_rto\_sec |  |  |  |
| int | sysctl\_tcp\_plb\_cong\_thresh |  |  |  |
| int | sysctl\_udp\_wmem\_min |  |  |  |
| int | sysctl\_udp\_rmem\_min |  |  |  |
| u8 | sysctl\_fib\_notify\_on\_flag\_change |  |  |  |
| u8 | sysctl\_udp\_l3mdev\_accept |  |  |  |
| u8 | sysctl\_igmp\_llm\_reports |  |  |  |
| int | sysctl\_igmp\_max\_memberships |  |  |  |
| int | sysctl\_igmp\_max\_msf |  |  |  |
| int | sysctl\_igmp\_qrv |  |  |  |
| struct\_ping\_group\_range | ping\_group\_range |  |  |  |
| atomic\_t | dev\_addr\_genid |  |  |  |
| unsigned\_int | sysctl\_udp\_child\_hash\_entries |  |  |  |
| unsigned\_long\* | sysctl\_local\_reserved\_ports |  |  |  |
| int | sysctl\_ip\_prot\_sock |  |  |  |
| struct\_mr\_table\* | mrt |  |  |  |
| struct\_list\_head | mr\_tables |  |  |  |
| struct\_fib\_rules\_ops\* | mr\_rules\_ops |  |  |  |
| u32 | sysctl\_fib\_multipath\_hash\_fields |  |  |  |
| u8 | sysctl\_fib\_multipath\_use\_neigh |  |  |  |
| u8 | sysctl\_fib\_multipath\_hash\_policy |  |  |  |
| struct\_fib\_notifier\_ops\* | notifier\_ops |  |  |  |
| unsigned\_int | fib\_seq |  |  |  |
| struct\_fib\_notifier\_ops\* | ipmr\_notifier\_ops |  |  |  |
| unsigned\_int | ipmr\_seq |  |  |  |
| atomic\_t | rt\_genid |  |  |  |
| siphash\_key\_t | ip\_id\_key |  |  |  |
