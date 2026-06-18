# Program Types and ELF Sections

> 출처(원문): https://docs.kernel.org/bpf/libbpf/program_types.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Program Types and ELF Sections

The table below lists the program types, their attach types where relevant and the ELF section
names supported by libbpf for them. The ELF section names follow these rules:

* `type` is an exact match, e.g. `SEC("socket")`
* `type+` means it can be either exact `SEC("type")` or well-formed `SEC("type/extras")`
  with a ‘`/`’ separator between `type` and `extras`.

When `extras` are specified, they provide details of how to auto-attach the BPF program. The
format of `extras` depends on the program type, e.g. `SEC("tracepoint/<category>/<name>")`
for tracepoints or `SEC("usdt/<path>:<provider>:<name>")` for USDT probes. The extras are
described in more detail in the footnotes.

| Program Type | Attach Type | ELF Section Name | Sleepable |
| --- | --- | --- | --- |
| `BPF_PROG_TYPE_CGROUP_DEVICE` | `BPF_CGROUP_DEVICE` | `cgroup/dev` |  |
| `BPF_PROG_TYPE_CGROUP_SKB` |  | `cgroup/skb` |  |
| `BPF_CGROUP_INET_EGRESS` | `cgroup_skb/egress` |  |
| `BPF_CGROUP_INET_INGRESS` | `cgroup_skb/ingress` |  |
| `BPF_PROG_TYPE_CGROUP_SOCKOPT` | `BPF_CGROUP_GETSOCKOPT` | `cgroup/getsockopt` |  |
| `BPF_CGROUP_SETSOCKOPT` | `cgroup/setsockopt` |  |
| `BPF_PROG_TYPE_CGROUP_SOCK_ADDR` | `BPF_CGROUP_INET4_BIND` | `cgroup/bind4` |  |
| `BPF_CGROUP_INET4_CONNECT` | `cgroup/connect4` |  |
| `BPF_CGROUP_INET4_GETPEERNAME` | `cgroup/getpeername4` |  |
| `BPF_CGROUP_INET4_GETSOCKNAME` | `cgroup/getsockname4` |  |
| `BPF_CGROUP_INET6_BIND` | `cgroup/bind6` |  |
| `BPF_CGROUP_INET6_CONNECT` | `cgroup/connect6` |  |
| `BPF_CGROUP_INET6_GETPEERNAME` | `cgroup/getpeername6` |  |
| `BPF_CGROUP_INET6_GETSOCKNAME` | `cgroup/getsockname6` |  |
| `BPF_CGROUP_UDP4_RECVMSG` | `cgroup/recvmsg4` |  |
| `BPF_CGROUP_UDP4_SENDMSG` | `cgroup/sendmsg4` |  |
| `BPF_CGROUP_UDP6_RECVMSG` | `cgroup/recvmsg6` |  |
| `BPF_CGROUP_UDP6_SENDMSG` | `cgroup/sendmsg6` |  |
| `BPF_CGROUP_UNIX_CONNECT` | `cgroup/connect_unix` |  |
| `BPF_CGROUP_UNIX_SENDMSG` | `cgroup/sendmsg_unix` |  |
| `BPF_CGROUP_UNIX_RECVMSG` | `cgroup/recvmsg_unix` |  |
| `BPF_CGROUP_UNIX_GETPEERNAME` | `cgroup/getpeername_unix` |  |
| `BPF_CGROUP_UNIX_GETSOCKNAME` | `cgroup/getsockname_unix` |  |
| `BPF_PROG_TYPE_CGROUP_SOCK` | `BPF_CGROUP_INET4_POST_BIND` | `cgroup/post_bind4` |  |
| `BPF_CGROUP_INET6_POST_BIND` | `cgroup/post_bind6` |  |
| `BPF_CGROUP_INET_SOCK_CREATE` | `cgroup/sock_create` |  |
| `cgroup/sock` |  |
| `BPF_CGROUP_INET_SOCK_RELEASE` | `cgroup/sock_release` |  |
| `BPF_PROG_TYPE_CGROUP_SYSCTL` | `BPF_CGROUP_SYSCTL` | `cgroup/sysctl` |  |
| `BPF_PROG_TYPE_EXT` |  | `freplace+` [[1]](#fentry) |  |
| `BPF_PROG_TYPE_FLOW_DISSECTOR` | `BPF_FLOW_DISSECTOR` | `flow_dissector` |  |
| `BPF_PROG_TYPE_KPROBE` |  | `kprobe+` [[2]](#kprobe) |  |
| `kretprobe+` [[2]](#kprobe) |  |
| `ksyscall+` [[3]](#ksyscall) |  |
| `kretsyscall+` [[3]](#ksyscall) |  |
| `uprobe+` [[4]](#uprobe) |  |
| `uprobe.s+` [[4]](#uprobe) | Yes |
| `uretprobe+` [[4]](#uprobe) |  |
| `uretprobe.s+` [[4]](#uprobe) | Yes |
| `usdt+` [[6]](#usdt) |  |
| `usdt.s+` [[6]](#usdt) | Yes |
| `BPF_TRACE_KPROBE_MULTI` | `kprobe.multi+` [[7]](#kpmulti) |  |
| `kretprobe.multi+` [[7]](#kpmulti) |  |
| `BPF_TRACE_KPROBE_SESSION` | `kprobe.session+` [[7]](#kpmulti) |  |
| `BPF_TRACE_UPROBE_MULTI` | `uprobe.multi+` [[5]](#upmul) |  |
| `uprobe.multi.s+` [[5]](#upmul) | Yes |
| `uretprobe.multi+` [[5]](#upmul) |  |
| `uretprobe.multi.s+` [[5]](#upmul) | Yes |
| `BPF_TRACE_UPROBE_SESSION` | `uprobe.session+` [[5]](#upmul) |  |
| `uprobe.session.s+` [[5]](#upmul) | Yes |
| `BPF_PROG_TYPE_LIRC_MODE2` | `BPF_LIRC_MODE2` | `lirc_mode2` |  |
| `BPF_PROG_TYPE_LSM` | `BPF_LSM_CGROUP` | `lsm_cgroup+` |  |
| `BPF_LSM_MAC` | `lsm+` [[8]](#lsm) |  |
| `lsm.s+` [[8]](#lsm) | Yes |
| `BPF_PROG_TYPE_LWT_IN` |  | `lwt_in` |  |
| `BPF_PROG_TYPE_LWT_OUT` |  | `lwt_out` |  |
| `BPF_PROG_TYPE_LWT_SEG6LOCAL` |  | `lwt_seg6local` |  |
| `BPF_PROG_TYPE_LWT_XMIT` |  | `lwt_xmit` |  |
| `BPF_PROG_TYPE_NETFILTER` |  | `netfilter` |  |
| `BPF_PROG_TYPE_PERF_EVENT` |  | `perf_event` |  |
| `BPF_PROG_TYPE_RAW_TRACEPOINT_WRITABLE` |  | `raw_tp.w+` [[9]](#rawtp) |  |
| `raw_tracepoint.w+` |  |
| `BPF_PROG_TYPE_RAW_TRACEPOINT` |  | `raw_tp+` [[9]](#rawtp) |  |
| `raw_tracepoint+` |  |
| `BPF_PROG_TYPE_SCHED_ACT` |  | `action` [[10]](#tc-legacy) |  |
| `BPF_PROG_TYPE_SCHED_CLS` |  | `classifier` [[10]](#tc-legacy) |  |
| `tc` [[10]](#tc-legacy) |  |
| `BPF_NETKIT_PRIMARY` | `netkit/primary` |  |
| `BPF_NETKIT_PEER` | `netkit/peer` |  |
| `BPF_TCX_INGRESS` | `tc/ingress` |  |
| `BPF_TCX_EGRESS` | `tc/egress` |  |
| `BPF_TCX_INGRESS` | `tcx/ingress` |  |
| `BPF_TCX_EGRESS` | `tcx/egress` |  |
| `BPF_PROG_TYPE_SK_LOOKUP` | `BPF_SK_LOOKUP` | `sk_lookup` |  |
| `BPF_PROG_TYPE_SK_MSG` | `BPF_SK_MSG_VERDICT` | `sk_msg` |  |
| `BPF_PROG_TYPE_SK_REUSEPORT` | `BPF_SK_REUSEPORT_SELECT_OR_MIGRATE` | `sk_reuseport/migrate` |  |
| `BPF_SK_REUSEPORT_SELECT` | `sk_reuseport` |  |
| `BPF_PROG_TYPE_SK_SKB` |  | `sk_skb` |  |
| `BPF_SK_SKB_STREAM_PARSER` | `sk_skb/stream_parser` |  |
| `BPF_SK_SKB_STREAM_VERDICT` | `sk_skb/stream_verdict` |  |
| `BPF_PROG_TYPE_SOCKET_FILTER` |  | `socket` |  |
| `BPF_PROG_TYPE_SOCK_OPS` | `BPF_CGROUP_SOCK_OPS` | `sockops` |  |
| `BPF_PROG_TYPE_STRUCT_OPS` |  | `struct_ops+` [[11]](#struct-ops) |  |
| `struct_ops.s+` [[11]](#struct-ops) | Yes |
| `BPF_PROG_TYPE_SYSCALL` |  | `syscall` | Yes |
| `BPF_PROG_TYPE_TRACEPOINT` |  | `tp+` [[12]](#tp) |  |
| `tracepoint+` [[12]](#tp) |  |
| `BPF_PROG_TYPE_TRACING` | `BPF_MODIFY_RETURN` | `fmod_ret+` [[1]](#fentry) |  |
| `fmod_ret.s+` [[1]](#fentry) | Yes |
| `BPF_TRACE_FENTRY` | `fentry+` [[1]](#fentry) |  |
| `fentry.s+` [[1]](#fentry) | Yes |
| `BPF_TRACE_FEXIT` | `fexit+` [[1]](#fentry) |  |
| `fexit.s+` [[1]](#fentry) | Yes |
| `BPF_TRACE_FSESSION` | `fsession+` [[1]](#fentry) |  |
| `fsession.s+` [[1]](#fentry) | Yes |
| `BPF_TRACE_ITER` | `iter+` [[13]](#iter) |  |
| `iter.s+` [[13]](#iter) | Yes |
| `BPF_TRACE_RAW_TP` | `tp_btf+` [[1]](#fentry) |  |
| `BPF_PROG_TYPE_XDP` | `BPF_XDP_CPUMAP` | `xdp.frags/cpumap` |  |
| `xdp/cpumap` |  |
| `BPF_XDP_DEVMAP` | `xdp.frags/devmap` |  |
| `xdp/devmap` |  |
| `BPF_XDP` | `xdp.frags` |  |
| `xdp` |  |

Footnotes

[1]
([1](#id1),[2](#id32),[3](#id33),[4](#id34),[5](#id35),[6](#id36),[7](#id37),[8](#id38),[9](#id39),[10](#id42))

The `fentry` attach format is `fentry[.s]/<function>`.


[2]
([1](#id2),[2](#id3))

The `kprobe` attach format is `kprobe/<function>[+<offset>]`. Valid
characters for `function` are `a-zA-Z0-9_.` and `offset` must be a valid
non-negative integer.


[3]
([1](#id4),[2](#id5))

The `ksyscall` attach format is `ksyscall/<syscall>`.


[4]
([1](#id6),[2](#id7),[3](#id8),[4](#id9))

The `uprobe` attach format is `uprobe[.s]/<path>:<function>[+<offset>]`.


[5]
([1](#id15),[2](#id16),[3](#id17),[4](#id18),[5](#id19),[6](#id20))

The `uprobe.multi` attach format is `uprobe.multi[.s]/<path>:<function-pattern>`
where `function-pattern` supports `*` and `?` wildcards.


[6]
([1](#id10),[2](#id11))

The `usdt` attach format is `usdt/<path>:<provider>:<name>`.


[7]
([1](#id12),[2](#id13),[3](#id14))

The `kprobe.multi` attach format is `kprobe.multi/<pattern>` where `pattern`
supports `*` and `?` wildcards. Valid characters for pattern are
`a-zA-Z0-9_.*?`.


[8]
([1](#id21),[2](#id22))

The `lsm` attachment format is `lsm[.s]/<hook>`.


[9]
([1](#id23),[2](#id24))

The `raw_tp` attach format is `raw_tracepoint[.w]/<tracepoint>`.


[10]
([1](#id25),[2](#id26),[3](#id27))

The `tc`, `classifier` and `action` attach types are deprecated, use
`tcx/*` instead.


[11]
([1](#id28),[2](#id29))

The `struct_ops` attach format supports `struct_ops[.s]/<name>` convention,
but `name` is ignored and it is recommended to just use plain
`SEC("struct_ops[.s]")`. The attachments are defined in a `struct initializer`
that is tagged with `SEC(".struct_ops[.link]")`.


[12]
([1](#id30),[2](#id31))

The `tracepoint` attach format is `tracepoint/<category>/<name>`.


[13]
([1](#id40),[2](#id41))

The `iter` attach format is `iter[.s]/<struct-name>`.
