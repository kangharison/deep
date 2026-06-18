# net_device struct fast path usage breakdown

> 출처(원문): https://docs.kernel.org/networking/net_cachelines/net_device.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# net\_device struct fast path usage breakdown

| Type | Name | fastpath\_tx\_access | fastpath\_rx\_access | Comments |
| --- | --- | --- | --- | --- |
| unsigned\_long:32 | priv\_flags | read\_mostly |  | \_\_dev\_queue\_xmit(tx) |
| unsigned\_long:1 | lltx | read\_mostly |  | HARD\_TX\_LOCK,HARD\_TX\_TRYLOCK,HARD\_TX\_UNLOCK(tx) |
| unsigned long:1 | netmem\_tx:1; | read\_mostly |  |  |
| char | name[16] |  |  |  |
| `struct netdev_name_node`\* | name\_node |  |  |  |
| `struct dev_ifalias`\* | ifalias |  |  |  |
| unsigned\_long | mem\_end |  |  |  |
| unsigned\_long | mem\_start |  |  |  |
| unsigned\_long | base\_addr |  |  |  |
| unsigned\_long | state | read\_mostly | read\_mostly | netif\_running(dev) |
| `struct list_head` | dev\_list |  |  |  |
| `struct list_head` | napi\_list |  |  |  |
| `struct list_head` | unreg\_list |  |  |  |
| `struct list_head` | close\_list |  |  |  |
| `struct list_head` | ptype\_all | read\_mostly |  | dev\_nit\_active(tx) |
| `struct list_head` | ptype\_specific |  | read\_mostly | deliver\_ptype\_list\_skb/\_\_netif\_receive\_skb\_core(rx) |
| struct | adj\_list |  |  |  |
| unsigned\_int | flags | read\_mostly | read\_mostly | \_\_dev\_queue\_xmit,\_\_dev\_xmit\_skb,ip6\_output,\_\_ip6\_finish\_output(tx);ip6\_rcv\_core(rx) |
| xdp\_features\_t | xdp\_features |  |  |  |
| `struct net_device_ops`\* | netdev\_ops | read\_mostly |  | netdev\_core\_pick\_tx,netdev\_start\_xmit(tx) |
| `struct xdp_metadata_ops`\* | xdp\_metadata\_ops |  |  |  |
| int | ifindex |  | read\_mostly | ip6\_rcv\_core |
| unsigned\_short | gflags |  |  |  |
| unsigned\_short | hard\_header\_len | read\_mostly | read\_mostly | ip6\_xmit(tx);gro\_list\_prepare(rx) |
| unsigned\_int | mtu | read\_mostly |  | ip\_finish\_output2 |
| unsigned\_short | needed\_headroom | read\_mostly |  | LL\_RESERVED\_SPACE/ip\_finish\_output2 |
| unsigned\_short | needed\_tailroom |  |  |  |
| netdev\_features\_t | features | read\_mostly | read\_mostly | HARD\_TX\_LOCK,netif\_skb\_features,sk\_setup\_caps(tx);netif\_elide\_gro(rx) |
| netdev\_features\_t | hw\_features |  |  |  |
| netdev\_features\_t | wanted\_features |  |  |  |
| netdev\_features\_t | vlan\_features |  |  |  |
| netdev\_features\_t | hw\_enc\_features |  |  | netif\_skb\_features |
| netdev\_features\_t | mpls\_features |  |  |  |
| netdev\_features\_t | gso\_partial\_features | read\_mostly |  | gso\_features\_check |
| unsigned\_int | min\_mtu |  |  |  |
| unsigned\_int | max\_mtu |  |  |  |
| unsigned\_short | type |  |  |  |
| unsigned\_char | min\_header\_len |  |  |  |
| unsigned\_char | name\_assign\_type |  |  |  |
| int | group |  |  |  |
| `struct net_device_stats` | stats |  |  |  |
| `struct net_device_core_stats`\* | core\_stats |  |  |  |
| atomic\_t | carrier\_up\_count |  |  |  |
| atomic\_t | carrier\_down\_count |  |  |  |
| `struct iw_handler_def`\* | wireless\_handlers |  |  |  |
| `struct ethtool_ops`\* | ethtool\_ops |  |  |  |
| `struct l3mdev_ops`\* | l3mdev\_ops |  |  |  |
| `struct ndisc_ops`\* | ndisc\_ops |  |  |  |
| `struct xfrmdev_ops`\* | xfrmdev\_ops |  |  |  |
| `struct tlsdev_ops`\* | tlsdev\_ops |  |  |  |
| `struct header_ops`\* | header\_ops | read\_mostly |  | ip\_finish\_output2,ip6\_finish\_output2(tx) |
| unsigned\_char | operstate |  |  |  |
| unsigned\_char | link\_mode |  |  |  |
| unsigned\_char | if\_port |  |  |  |
| unsigned\_char | dma |  |  |  |
| unsigned\_char | perm\_addr[32] |  |  |  |
| unsigned\_char | addr\_assign\_type |  |  |  |
| unsigned\_char | addr\_len |  |  |  |
| unsigned\_char | upper\_level |  |  |  |
| unsigned\_char | lower\_level |  |  |  |
| u8 | threaded |  |  | napi\_poll(napi\_enable,netif\_set\_threaded) |
| unsigned\_short | neigh\_priv\_len |  |  |  |
| unsigned\_short | padded |  |  |  |
| unsigned\_short | dev\_id |  |  |  |
| unsigned\_short | dev\_port |  |  |  |
| spinlock\_t | addr\_list\_lock |  |  |  |
| int | irq |  |  |  |
| `struct netdev_hw_addr_list` | uc |  |  |  |
| `struct netdev_hw_addr_list` | mc |  |  |  |
| `struct netdev_hw_addr_list` | dev\_addrs |  |  |  |
| `struct kset`\* | queues\_kset |  |  |  |
| `struct list_head` | unlink\_list |  |  |  |
| unsigned\_int | promiscuity |  |  |  |
| unsigned\_int | allmulti |  |  |  |
| bool | uc\_promisc |  |  |  |
| unsigned\_char | nested\_level |  |  |  |
| `struct in_device`\* | ip\_ptr | read\_mostly | read\_mostly | \_\_in\_dev\_get |
| `struct hlist_head` | fib\_nh\_head |  |  |  |
| `struct inet6_dev`\* | ip6\_ptr | read\_mostly | read\_mostly | \_\_in6\_dev\_get |
| `struct vlan_info`\* | vlan\_info |  |  |  |
| `struct dsa_port`\* | dsa\_ptr |  |  |  |
| [`struct tipc_bearer`](../tipc.html#c.tipc_bearer "tipc_bearer")\* | tipc\_ptr |  |  |  |
| void\* | atalk\_ptr |  |  |  |
| void\* | ax25\_ptr |  |  |  |
| [`struct wireless_dev`](../../driver-api/80211/cfg80211.html#c.wireless_dev "wireless_dev")\* | ieee80211\_ptr |  |  |  |
| `struct wpan_dev`\* | ieee802154\_ptr |  |  |  |
| `struct mpls_dev`\* | mpls\_ptr |  |  |  |
| `struct mctp_dev`\* | mctp\_ptr |  |  |  |
| unsigned\_char\* | dev\_addr |  |  |  |
| `struct netdev_queue`\* | \_rx | read\_mostly |  | netdev\_get\_rx\_queue(rx) |
| unsigned\_int | num\_rx\_queues |  |  |  |
| unsigned\_int | real\_num\_rx\_queues |  | read\_mostly | get\_rps\_cpu |
| `struct bpf_prog`\* | xdp\_prog |  | read\_mostly | `netif_elide_gro()` |
| unsigned\_long | gro\_flush\_timeout |  | read\_mostly | napi\_complete\_done |
| u32 | napi\_defer\_hard\_irqs |  | read\_mostly | napi\_complete\_done |
| unsigned\_int | gro\_max\_size |  | read\_mostly | skb\_gro\_receive |
| unsigned\_int | gro\_ipv4\_max\_size |  | read\_mostly | skb\_gro\_receive |
| rx\_handler\_func\_t\* | rx\_handler | read\_mostly |  | \_\_netif\_receive\_skb\_core |
| void\* | rx\_handler\_data | read\_mostly |  |  |
| `struct netdev_queue`\* | ingress\_queue | read\_mostly |  |  |
| `struct bpf_mprog_entry` | tcx\_ingress |  | read\_mostly | sch\_handle\_ingress |
| `struct nf_hook_entries`\* | nf\_hooks\_ingress |  |  |  |
| unsigned\_char | broadcast[32] |  |  |  |
| `struct cpu_rmap`\* | rx\_cpu\_rmap |  |  |  |
| `struct hlist_node` | index\_hlist |  |  |  |
| `struct netdev_queue`\* | \_tx | read\_mostly |  | netdev\_get\_tx\_queue(tx) |
| unsigned\_int | num\_tx\_queues |  |  |  |
| unsigned\_int | real\_num\_tx\_queues | read\_mostly |  | skb\_tx\_hash,netdev\_core\_pick\_tx(tx) |
| unsigned\_int | tx\_queue\_len |  |  |  |
| spinlock\_t | tx\_global\_lock |  |  |  |
| `struct xdp_dev_bulk_queue__percpu`\* | xdp\_bulkq |  |  |  |
| `struct xps_dev_maps`\* | xps\_maps[2] | read\_mostly |  | \_\_netif\_set\_xps\_queue |
| `struct bpf_mprog_entry` | tcx\_egress | read\_mostly |  | sch\_handle\_egress |
| `struct nf_hook_entries`\* | nf\_hooks\_egress | read\_mostly |  |  |
| `struct hlist_head` | qdisc\_hash[16] |  |  |  |
| `struct timer_list` | watchdog\_timer |  |  |  |
| int | watchdog\_timeo |  |  |  |
| u32 | proto\_down\_reason |  |  |  |
| `struct list_head` | todo\_list |  |  |  |
| int\_\_percpu\* | pcpu\_refcnt |  |  |  |
| refcount\_t | dev\_refcnt |  |  |  |
| `struct ref_tracker_dir` | refcnt\_tracker |  |  |  |
| `struct list_head` | link\_watch\_list |  |  |  |
| enum:8 | reg\_state |  |  |  |
| bool | dismantle |  |  |  |
| bool | rtnl\_link\_initilizing |  |  |  |
| bool | needs\_free\_netdev |  |  |  |
| void\*priv\_destructor | [`struct net_device`](../kapi.html#c.net_device "net_device") |  |  |  |
| `struct netpoll_info`\* | npinfo |  | read\_mostly | napi\_poll/napi\_poll\_lock |
| possible\_net\_t | nd\_net |  | read\_mostly | (dev\_net)napi\_busy\_loop,tcp\_v(4/6)\_rcv,ip(v6)\_rcv,ip(6)\_input,ip(6)\_input\_finish |
| void\* | ml\_priv |  |  |  |
| enum\_netdev\_ml\_priv\_type | ml\_priv\_type |  |  |  |
| `struct pcpu_lstats__percpu`\* | lstats | read\_mostly |  | `dev_lstats_add()` |
| `struct pcpu_sw_netstats__percpu`\* | tstats | read\_mostly |  | `dev_sw_netstats_tx_add()` |
| `struct pcpu_dstats__percpu`\* | dstats |  |  |  |
| `struct garp_port`\* | garp\_port |  |  |  |
| `struct mrp_port`\* | mrp\_port |  |  |  |
| `struct dm_hw_stat_delta`\* | dm\_private |  |  |  |
| [`struct device`](../../driver-api/infrastructure.html#c.device "device") | dev |  |  |  |
| `struct attribute_group`\* | sysfs\_groups[4] |  |  |  |
| `struct attribute_group`\* | sysfs\_rx\_queue\_group |  |  |  |
| `struct rtnl_link_ops`\* | rtnl\_link\_ops |  |  |  |
| unsigned\_int | gso\_max\_size | read\_mostly |  | sk\_dst\_gso\_max\_size |
| unsigned\_int | tso\_max\_size |  |  |  |
| u16 | gso\_max\_segs | read\_mostly |  | gso\_max\_segs |
| u16 | tso\_max\_segs |  |  |  |
| unsigned\_int | gso\_ipv4\_max\_size | read\_mostly |  | sk\_dst\_gso\_max\_size |
| `struct dcbnl_rtnl_ops`\* | dcbnl\_ops |  |  |  |
| s16 | num\_tc | read\_mostly |  | skb\_tx\_hash |
| `struct netdev_tc_txq` | tc\_to\_txq[16] | read\_mostly |  | skb\_tx\_hash |
| u8 | prio\_tc\_map[16] |  |  |  |
| unsigned\_int | fcoe\_ddp\_xid |  |  |  |
| `struct netprio_map`\* | priomap |  |  |  |
| [`struct phy_device`](../kapi.html#c.phy_device "phy_device")\* | phydev |  |  |  |
| [`struct sfp_bus`](../kapi.html#c.sfp_bus "sfp_bus")\* | sfp\_bus |  |  |  |
| `struct lock_class_key`\* | qdisc\_tx\_busylock |  |  |  |
| bool | proto\_down |  |  |  |
| unsigned:1 | wol\_enabled |  |  |  |
| unsigned\_long:1 | see\_all\_hwtstamp\_requests |  |  |  |
| unsigned\_long:1 | change\_proto\_down |  |  |  |
| unsigned\_long:1 | netns\_immutable |  |  |  |
| unsigned\_long:1 | fcoe\_mtu |  |  |  |
| `struct list_head` | net\_notifier\_list |  |  |  |
| `struct macsec_ops`\* | macsec\_ops |  |  |  |
| `struct udp_tunnel_nic_info`\* | udp\_tunnel\_nic\_info |  |  |  |
| `struct udp_tunnel_nic`\* | udp\_tunnel\_nic |  |  |  |
| unsigned\_int | xdp\_zc\_max\_segs |  |  |  |
| `struct bpf_xdp_entity` | xdp\_state[3] |  |  |  |
| u8 | dev\_addr\_shadow[32] |  |  |  |
| netdevice\_tracker | linkwatch\_dev\_tracker |  |  |  |
| netdevice\_tracker | watchdog\_dev\_tracker |  |  |  |
| netdevice\_tracker | dev\_registered\_tracker |  |  |  |
| `struct rtnl_hw_stats64`\* | offload\_xstats\_l3 |  |  |  |
| `struct devlink_port`\* | devlink\_port |  |  |  |
| `struct dpll_pin`\* | dpll\_pin |  |  |  |
| `struct hlist_head` | page\_pools |  |  |  |
| [`struct dim_irq_moder`](../net_dim.html#c.dim_irq_moder "dim_irq_moder")\* | irq\_moder |  |  |  |
| u64 | max\_pacing\_offload\_horizon |  |  |  |
| struct\_napi\_config\* | napi\_config |  |  |  |
| unsigned\_long | gro\_flush\_timeout |  |  |  |
| u32 | napi\_defer\_hard\_irqs |  |  |  |
| `struct hlist_head` | neighbours[2] |  |  |  |
