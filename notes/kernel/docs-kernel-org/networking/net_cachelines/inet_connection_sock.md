# inet_connection_sock struct fast path usage breakdown

> 출처(원문): https://docs.kernel.org/networking/net_cachelines/inet_connection_sock.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# inet\_connection\_sock struct fast path usage breakdown

| Type | Name | fastpath\_tx\_access | fastpath\_rx\_access | comment |
| --- | --- | --- | --- | --- |
| `struct inet_sock` | icsk\_inet | read\_mostly | read\_mostly | tcp\_init\_buffer\_space,tcp\_init\_transfer,tcp\_finish\_connect,tcp\_connect,tcp\_send\_rcvq,tcp\_send\_syn\_data |
| `struct request_sock_queue` | icsk\_accept\_queue |  |  |  |
| `struct inet_bind_bucket` | icsk\_bind\_hash | read\_mostly |  | tcp\_set\_state |
| `struct inet_bind2_bucket` | icsk\_bind2\_hash | read\_mostly |  | tcp\_set\_state,inet\_put\_port |
| `struct timer_list` | icsk\_delack\_timer | read\_mostly |  | inet\_csk\_reset\_xmit\_timer,tcp\_connect |
| `struct timer_list` | icsk\_keepalive\_timer |  |  |  |
| u32 | icsk\_rto | read\_write |  | tcp\_cwnd\_validate,tcp\_schedule\_loss\_probe,tcp\_connect\_init,tcp\_connect,tcp\_write\_xmit,tcp\_push\_one |
| u32 | icsk\_rto\_min |  |  |  |
| u32 | icsk\_rto\_max | read\_mostly |  | tcp\_reset\_xmit\_timer |
| u32 | icsk\_delack\_max |  |  |  |
| u32 | icsk\_pmtu\_cookie | read\_write |  | tcp\_sync\_mss,tcp\_current\_mss,tcp\_send\_syn\_data,tcp\_connect\_init,tcp\_connect |
| `struct tcp_congestion_ops` | icsk\_ca\_ops | read\_write |  | tcp\_cwnd\_validate,tcp\_tso\_segs,tcp\_ca\_dst\_init,tcp\_connect\_init,tcp\_connect,tcp\_write\_xmit |
| `struct inet_connection_sock_af_ops` | icsk\_af\_ops | read\_mostly |  | tcp\_finish\_connect,tcp\_send\_syn\_data,tcp\_mtup\_init,tcp\_mtu\_check\_reprobe,tcp\_mtu\_probe,tcp\_connect\_init,tcp\_connect,\_\_tcp\_transmit\_skb |
| `struct tcp_ulp_ops`\* | icsk\_ulp\_ops |  |  |  |
| void\* | icsk\_ulp\_data |  |  |  |
| u8:5 | icsk\_ca\_state | read\_write |  | tcp\_cwnd\_application\_limited,tcp\_set\_ca\_state,tcp\_enter\_cwr,tcp\_tso\_should\_defer,tcp\_mtu\_probe,tcp\_schedule\_loss\_probe,tcp\_write\_xmit,\_\_tcp\_transmit\_skb |
| u8:1 | icsk\_ca\_initialized | read\_write |  | tcp\_init\_transfer,tcp\_init\_congestion\_control,tcp\_init\_transfer,tcp\_finish\_connect,tcp\_connect |
| u8:1 | icsk\_ca\_setsockopt |  |  |  |
| u8:1 | icsk\_ca\_dst\_locked | write\_mostly |  | tcp\_ca\_dst\_init,tcp\_connect\_init,tcp\_connect |
| u8 | icsk\_retransmits | write\_mostly |  | tcp\_connect\_init,tcp\_connect |
| u8 | icsk\_pending | read\_write |  | inet\_csk\_reset\_xmit\_timer,tcp\_connect,tcp\_check\_probe\_timer,\_\_tcp\_push\_pending\_frames,tcp\_rearm\_rto,tcp\_event\_new\_data\_sent,tcp\_event\_new\_data\_sent |
| u8 | icsk\_backoff | write\_mostly |  | tcp\_write\_queue\_purge,tcp\_connect\_init |
| u8 | icsk\_syn\_retries |  |  |  |
| u8 | icsk\_probes\_out |  |  |  |
| u16 | icsk\_ext\_hdr\_len | read\_mostly |  | \_\_tcp\_mtu\_to\_mss,tcp\_mtu\_to\_rss,tcp\_mtu\_probe,tcp\_write\_xmit,tcp\_mtu\_to\_mss, |
| `struct icsk_ack_u8` | pending | read\_write | read\_write | inet\_csk\_ack\_scheduled,\_\_tcp\_cleanup\_rbuf,tcp\_cleanup\_rbuf,inet\_csk\_clear\_xmit\_timer,tcp\_event\_ack-sent,inet\_csk\_reset\_xmit\_timer |
| `struct icsk_ack_u8` | quick | read\_write | write\_mostly | tcp\_dec\_quickack\_mode,tcp\_event\_ack\_sent,\_\_tcp\_transmit\_skb,\_\_tcp\_select\_window,\_\_tcp\_cleanup\_rbuf |
| `struct icsk_ack_u8` | pingpong |  |  |  |
| `struct icsk_ack_u8` | retry | write\_mostly | read\_write | inet\_csk\_clear\_xmit\_timer,tcp\_rearm\_rto,tcp\_event\_new\_data\_sent,tcp\_write\_xmit,\_\_tcp\_send\_ack,tcp\_send\_ack, |
| `struct icsk_ack_u8` | ato | read\_mostly | write\_mostly | tcp\_dec\_quickack\_mode,tcp\_event\_ack\_sent,\_\_tcp\_transmit\_skb,\_\_tcp\_send\_ack,tcp\_send\_ack |
| `struct icsk_ack_u32` | lrcvtime | read\_write |  | tcp\_finish\_connect,tcp\_connect,tcp\_event\_data\_sent,\_\_tcp\_transmit\_skb |
| `struct icsk_ack_u16` | rcv\_mss | write\_mostly | read\_mostly | \_\_tcp\_select\_window,\_\_tcp\_cleanup\_rbuf,tcp\_initialize\_rcv\_mss,tcp\_connect\_init |
| `struct icsk_mtup_int` | search\_high | read\_write |  | tcp\_mtup\_init,tcp\_sync\_mss,tcp\_connect\_init,tcp\_mtu\_check\_reprobe,tcp\_write\_xmit |
| `struct icsk_mtup_int` | search\_low | read\_write |  | tcp\_mtu\_probe,tcp\_mtu\_check\_reprobe,tcp\_write\_xmit,tcp\_sync\_mss,tcp\_connect\_init,tcp\_mtup\_init |
| `struct icsk_mtup_u32`:31 | probe\_size | read\_write |  | tcp\_mtup\_init,tcp\_connect\_init,\_\_tcp\_transmit\_skb |
| `struct icsk_mtup_u32`:1 | enabled | read\_write |  | tcp\_mtup\_init,tcp\_sync\_mss,tcp\_connect\_init,tcp\_mtu\_probe,tcp\_write\_xmit |
| `struct icsk_mtup_u32` | probe\_timestamp | read\_write |  | tcp\_mtup\_init,tcp\_connect\_init,tcp\_mtu\_check\_reprobe,tcp\_mtu\_probe |
| u32 | icsk\_probes\_tstamp |  |  |  |
| u32 | icsk\_user\_timeout |  |  |  |
| u64[104/sizeof(u64)] | icsk\_ca\_priv |  |  |  |
