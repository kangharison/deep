# tcp_sock struct fast path usage breakdown

> 출처(원문): https://docs.kernel.org/networking/net_cachelines/tcp_sock.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# tcp\_sock struct fast path usage breakdown

| Type | Name | fastpath\_tx\_access | fastpath\_rx\_access | Comments |
| --- | --- | --- | --- | --- |
| `struct inet_connection_sock` | inet\_conn |  |  |  |
| u16 | tcp\_header\_len | read\_mostly | read\_mostly | tcp\_bound\_to\_half\_wnd,tcp\_current\_mss(tx);tcp\_rcv\_established(rx) |
| u16 | gso\_segs | read\_mostly |  | tcp\_xmit\_size\_goal |
| \_\_be32 | pred\_flags | read\_write | read\_mostly | tcp\_select\_window(tx);tcp\_rcv\_established(rx) |
| u64 | bytes\_received |  | read\_write | tcp\_rcv\_nxt\_update(rx) |
| u32 | segs\_in |  | read\_write | tcp\_v6\_rcv(rx) |
| u32 | data\_segs\_in |  | read\_write | tcp\_v6\_rcv(rx) |
| u32 | rcv\_nxt | read\_mostly | read\_write | tcp\_cleanup\_rbuf,tcp\_send\_ack,tcp\_inq\_hint,tcp\_transmit\_skb,tcp\_receive\_window(tx);tcp\_v6\_do\_rcv,tcp\_rcv\_established,tcp\_data\_queue,tcp\_receive\_window,tcp\_rcv\_nxt\_update(write)(rx) |
| u32 | copied\_seq |  | read\_mostly | tcp\_cleanup\_rbuf,tcp\_rcv\_space\_adjust,tcp\_inq\_hint |
| u32 | rcv\_wup |  | read\_write | \_\_tcp\_cleanup\_rbuf,tcp\_receive\_window,tcp\_receive\_established |
| u32 | snd\_nxt | read\_write | read\_mostly | tcp\_rate\_check\_app\_limited,\_\_tcp\_transmit\_skb,tcp\_event\_new\_data\_sent(write)(tx);tcp\_rcv\_established,tcp\_ack,tcp\_clean\_rtx\_queue(rx) |
| u32 | segs\_out | read\_write |  | \_\_tcp\_transmit\_skb |
| u32 | data\_segs\_out | read\_write |  | \_\_tcp\_transmit\_skb,tcp\_update\_skb\_after\_send |
| u64 | bytes\_sent | read\_write |  | \_\_tcp\_transmit\_skb |
| u64 | bytes\_acked |  | read\_write | tcp\_snd\_una\_update/tcp\_ack |
| u32 | dsack\_dups |  |  |  |
| u32 | snd\_una | read\_mostly | read\_write | tcp\_wnd\_end,tcp\_urg\_mode,tcp\_minshall\_check,tcp\_cwnd\_validate(tx);tcp\_ack,tcp\_may\_update\_window,tcp\_clean\_rtx\_queue(write),tcp\_ack\_tstamp(rx) |
| u32 | snd\_sml | read\_write |  | tcp\_minshall\_check,tcp\_minshall\_update |
| u32 | rcv\_tstamp | read\_write | read\_write | tcp\_ack |
| void \* | tcp\_clean\_acked | read\_mostly |  | tcp\_ack |
| u32 | lsndtime | read\_write |  | tcp\_slow\_start\_after\_idle\_check,tcp\_event\_data\_sent |
| u32 | last\_oow\_ack\_time |  |  |  |
| u32 | compressed\_ack\_rcv\_nxt |  |  |  |
| u32 | tsoffset | read\_mostly | read\_mostly | tcp\_established\_options(tx);tcp\_fast\_parse\_options(rx) |
| `struct list_head` | tsq\_node |  |  |  |
| `struct list_head` | tsorted\_sent\_queue | read\_write |  | tcp\_update\_skb\_after\_send |
| u32 | snd\_wl1 |  | read\_mostly | tcp\_may\_update\_window |
| u32 | snd\_wnd | read\_mostly | read\_mostly | tcp\_wnd\_end,tcp\_tso\_should\_defer(tx);tcp\_fast\_path\_on(rx) |
| u32 | max\_window | read\_mostly |  | tcp\_bound\_to\_half\_wnd,forced\_push |
| u32 | mss\_cache | read\_mostly | read\_mostly | tcp\_rate\_check\_app\_limited,tcp\_current\_mss,tcp\_sync\_mss,tcp\_sndbuf\_expand,tcp\_tso\_should\_defer(tx);tcp\_update\_pacing\_rate,tcp\_clean\_rtx\_queue(rx) |
| u32 | window\_clamp | read\_mostly | read\_write | tcp\_rcv\_space\_adjust,\_\_tcp\_select\_window |
| u32 | rcv\_ssthresh | read\_mostly |  | \_\_tcp\_select\_window |
| u8 | scaling\_ratio | read\_mostly | read\_mostly | tcp\_win\_from\_space |
| struct | tcp\_rack |  |  |  |
| u16 | advmss |  | read\_mostly | tcp\_rcv\_space\_adjust |
| u8 | compressed\_ack |  |  |  |
| u8:2 | dup\_ack\_counter |  |  |  |
| u8:1 | tlp\_retrans |  |  |  |
| u8:1 | tcp\_usec\_ts | read\_mostly | read\_mostly |  |
| u32 | chrono\_start | read\_write |  | tcp\_chrono\_start/stop(tcp\_write\_xmit,tcp\_cwnd\_validate,tcp\_send\_syn\_data) |
| u32[3] | chrono\_stat | read\_write |  | tcp\_chrono\_start/stop(tcp\_write\_xmit,tcp\_cwnd\_validate,tcp\_send\_syn\_data) |
| u8:2 | chrono\_type | read\_write |  | tcp\_chrono\_start/stop(tcp\_write\_xmit,tcp\_cwnd\_validate,tcp\_send\_syn\_data) |
| u8:1 | rate\_app\_limited |  | read\_write | tcp\_rate\_gen |
| u8:1 | fastopen\_connect |  |  |  |
| u8:1 | fastopen\_no\_cookie |  |  |  |
| u8:1 | is\_sack\_reneg |  | read\_mostly | tcp\_skb\_entail,tcp\_ack |
| u8:2 | fastopen\_client\_fail |  |  |  |
| u8:4 | nonagle | read\_write |  | tcp\_skb\_entail,tcp\_push\_pending\_frames |
| u8:1 | thin\_lto |  |  |  |
| u8:1 | recvmsg\_inq |  | read\_mostly | tcp\_recvmsg |
| u8:1 | repair | read\_mostly |  | tcp\_write\_xmit |
| u8:1 | frto |  |  |  |
| u8 | repair\_queue |  |  |  |
| u8:2 | save\_syn |  |  |  |
| u8:1 | syn\_data |  |  |  |
| u8:1 | syn\_fastopen |  |  |  |
| u8:1 | syn\_fastopen\_exp |  |  |  |
| u8:1 | syn\_fastopen\_ch |  |  |  |
| u8:1 | syn\_data\_acked |  |  |  |
| u8:1 | is\_cwnd\_limited | read\_mostly |  | tcp\_cwnd\_validate,tcp\_is\_cwnd\_limited |
| u32 | tlp\_high\_seq |  | read\_mostly | tcp\_ack |
| u32 | tcp\_tx\_delay |  |  |  |
| u64 | tcp\_wstamp\_ns | read\_write |  | tcp\_pacing\_check,tcp\_tso\_should\_defer,tcp\_update\_skb\_after\_send |
| u64 | tcp\_clock\_cache | read\_write | read\_write | tcp\_mstamp\_refresh(tcp\_write\_xmit/tcp\_rcv\_space\_adjust),\_\_tcp\_transmit\_skb,tcp\_tso\_should\_defer;timer |
| u64 | tcp\_mstamp | read\_write | read\_write | tcp\_mstamp\_refresh(tcp\_write\_xmit/tcp\_rcv\_space\_adjust)(tx);tcp\_rcv\_space\_adjust,tcp\_rate\_gen,tcp\_clean\_rtx\_queue,tcp\_ack\_update\_rtt/tcp\_time\_stamp(rx);timer |
| u32 | srtt\_us | read\_mostly | read\_write | tcp\_tso\_should\_defer(tx);tcp\_update\_pacing\_rate,\_\_tcp\_set\_rto,tcp\_rtt\_estimator(rx) |
| u32 | mdev\_us | read\_write |  | tcp\_rtt\_estimator |
| u32 | mdev\_max\_us |  |  |  |
| u32 | rttvar\_us |  | read\_mostly | \_\_tcp\_set\_rto |
| u32 | rtt\_seq | read\_write |  | tcp\_rtt\_estimator |
| `struct minmax` | rtt\_min |  | read\_mostly | tcp\_min\_rtt/tcp\_rate\_gen,tcp\_min\_rtttcp\_update\_rtt\_min |
| u32 | packets\_out | read\_write | read\_write | tcp\_packets\_in\_flight(tx/rx);tcp\_slow\_start\_after\_idle\_check,tcp\_nagle\_check,tcp\_rate\_skb\_sent,tcp\_event\_new\_data\_sent,tcp\_cwnd\_validate,tcp\_write\_xmit(tx);tcp\_ack,tcp\_clean\_rtx\_queue,tcp\_update\_pacing\_rate(rx) |
| u32 | retrans\_out |  | read\_mostly | tcp\_packets\_in\_flight,tcp\_rate\_check\_app\_limited |
| u32 | max\_packets\_out |  | read\_write | tcp\_cwnd\_validate |
| u32 | cwnd\_usage\_seq |  | read\_write | tcp\_cwnd\_validate |
| u16 | urg\_data |  | read\_mostly | tcp\_fast\_path\_check |
| u8 | ecn\_flags | read\_write |  | tcp\_ecn\_send |
| u8 | keepalive\_probes |  |  |  |
| u32 | reordering | read\_mostly |  | tcp\_sndbuf\_expand |
| u32 | reord\_seen |  |  |  |
| u32 | snd\_up | read\_write | read\_mostly | tcp\_mark\_urg,tcp\_urg\_mode,\_\_tcp\_transmit\_skb(tx);tcp\_clean\_rtx\_queue(rx) |
| `struct tcp_options_received` | rx\_opt | read\_mostly | read\_write | tcp\_established\_options(tx);tcp\_fast\_path\_on,tcp\_ack\_update\_window,tcp\_is\_sack,tcp\_data\_queue,tcp\_rcv\_established,tcp\_ack\_update\_rtt(rx) |
| u32 | snd\_ssthresh |  | read\_mostly | tcp\_update\_pacing\_rate |
| u32 | snd\_cwnd | read\_mostly | read\_mostly | tcp\_snd\_cwnd,tcp\_rate\_check\_app\_limited,tcp\_tso\_should\_defer(tx);tcp\_update\_pacing\_rate |
| u32 | snd\_cwnd\_cnt |  |  |  |
| u32 | snd\_cwnd\_clamp |  |  |  |
| u32 | snd\_cwnd\_used |  |  |  |
| u32 | snd\_cwnd\_stamp |  |  |  |
| u32 | prior\_cwnd |  |  |  |
| u32 | prr\_delivered |  |  |  |
| u32 | prr\_out | read\_mostly | read\_mostly | tcp\_rate\_skb\_sent,tcp\_newly\_delivered(tx);tcp\_ack,tcp\_rate\_gen,tcp\_clean\_rtx\_queue(rx) |
| u32 | delivered | read\_mostly | read\_write | tcp\_rate\_skb\_sent, tcp\_newly\_delivered(tx);tcp\_ack, tcp\_rate\_gen, tcp\_clean\_rtx\_queue (rx) |
| u32 | delivered\_ce | read\_mostly | read\_write | tcp\_rate\_skb\_sent(tx);tcp\_rate\_gen(rx) |
| u32 | received\_ce | read\_mostly | read\_write |  |
| u32[3] | received\_ecn\_bytes | read\_mostly | read\_write |  |
| u8:4 | received\_ce\_pending | read\_mostly | read\_write |  |
| u32[3] | delivered\_ecn\_bytes |  | read\_write |  |
| u16 | pkts\_acked\_ewma |  | read\_write |  |
| u8:2 | syn\_ect\_snt | write\_mostly | read\_write |  |
| u8:2 | syn\_ect\_rcv | read\_mostly | read\_write |  |
| u8:2 | accecn\_minlen | write\_mostly | read\_write |  |
| u8:2 | est\_ecnfield |  | read\_write |  |
| u8:2 | accecn\_opt\_demand | read\_mostly | read\_write |  |
| u8:2 | prev\_ecnfield |  | read\_write |  |
| u64 | accecn\_opt\_tstamp | read\_write |  |  |
| u8:4 | accecn\_fail\_mode |  |  |  |
| u32 | lost |  | read\_mostly | tcp\_ack |
| u32 | app\_limited | read\_write | read\_mostly | tcp\_rate\_check\_app\_limited,tcp\_rate\_skb\_sent(tx);tcp\_rate\_gen(rx) |
| u64 | first\_tx\_mstamp | read\_write |  | tcp\_rate\_skb\_sent |
| u64 | delivered\_mstamp | read\_write |  | tcp\_rate\_skb\_sent |
| u32 | rate\_delivered |  | read\_mostly | tcp\_rate\_gen |
| u32 | rate\_interval\_us |  | read\_mostly | rate\_delivered,rate\_app\_limited |
| u32 | rcv\_wnd | read\_write | read\_mostly | tcp\_select\_window,tcp\_receive\_window,tcp\_fast\_path\_check |
| u32 | rcv\_mwnd\_seq | read\_write |  | tcp\_select\_window |
| u32 | write\_seq | read\_write |  | tcp\_rate\_check\_app\_limited,tcp\_write\_queue\_empty,tcp\_skb\_entail,forced\_push,tcp\_mark\_push |
| u32 | notsent\_lowat | read\_mostly |  | tcp\_stream\_memory\_free |
| u32 | pushed\_seq | read\_write |  | tcp\_mark\_push,forced\_push |
| u32 | lost\_out | read\_mostly | read\_mostly | tcp\_left\_out(tx);tcp\_packets\_in\_flight(tx/rx);tcp\_rate\_check\_app\_limited(rx) |
| u32 | sacked\_out | read\_mostly | read\_mostly | tcp\_left\_out(tx);tcp\_packets\_in\_flight(tx/rx);tcp\_clean\_rtx\_queue(rx) |
| `struct hrtimer` | pacing\_timer |  |  |  |
| `struct hrtimer` | compressed\_ack\_timer |  |  |  |
| [`struct sk_buff`](../kapi.html#c.sk_buff "sk_buff")\* | retransmit\_skb\_hint | read\_mostly |  | tcp\_clean\_rtx\_queue |
| `struct rb_root` | out\_of\_order\_queue |  | read\_mostly | tcp\_data\_queue,tcp\_fast\_path\_check |
| [`struct sk_buff`](../kapi.html#c.sk_buff "sk_buff")\* | ooo\_last\_skb |  |  |  |
| `struct tcp_sack_block`[1] | duplicate\_sack |  |  |  |
| `struct tcp_sack_block`[4] | selective\_acks |  |  |  |
| `struct tcp_sack_block`[4] | recv\_sack\_cache |  |  |  |
| [`struct sk_buff`](../kapi.html#c.sk_buff "sk_buff")\* | highest\_sack | read\_write |  | tcp\_event\_new\_data\_sent |
| u32 | prior\_ssthresh |  |  |  |
| u32 | high\_seq |  |  |  |
| u32 | retrans\_stamp |  |  |  |
| u32 | undo\_marker |  |  |  |
| int | undo\_retrans |  |  |  |
| u64 | bytes\_retrans |  |  |  |
| u32 | total\_retrans |  |  |  |
| u32 | rto\_stamp |  |  |  |
| u16 | total\_rto |  |  |  |
| u16 | total\_rto\_recoveries |  |  |  |
| u32 | total\_rto\_time |  |  |  |
| u32 | urg\_seq |  |  |  |
| unsigned\_int | keepalive\_time |  |  |  |
| unsigned\_int | keepalive\_intvl |  |  |  |
| int | linger2 |  |  |  |
| u8 | bpf\_sock\_ops\_cb\_flags |  |  |  |
| u8:1 | bpf\_chg\_cc\_inprogress |  |  |  |
| u16 | timeout\_rehash |  |  |  |
| u32 | rcv\_ooopack |  |  |  |
| u32 | rcv\_rtt\_last\_tsecr |  |  |  |
| struct | rcv\_rtt\_est |  | read\_write | tcp\_rcv\_space\_adjust,tcp\_rcv\_established |
| struct | rcvq\_space |  | read\_write | tcp\_rcv\_space\_adjust |
| struct | mtu\_probe |  |  |  |
| u32 | plb\_rehash |  |  |  |
| u32 | mtu\_info |  |  |  |
| bool | is\_mptcp |  |  |  |
| bool | smc\_hs\_congested |  |  |  |
| bool | syn\_smc |  |  |  |
| `struct tcp_sock_af_ops`\* | af\_specific |  |  |  |
| `struct tcp_md5sig_info`\* | md5sig\_info |  |  |  |
| `struct tcp_fastopen_request`\* | fastopen\_req |  |  |  |
| `struct request_sock`\* | fastopen\_rsk |  |  |  |
| `struct saved_syn`\* | saved\_syn |  |  |  |
