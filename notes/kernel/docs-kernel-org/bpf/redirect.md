# Redirect

> 출처(원문): https://docs.kernel.org/bpf/redirect.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Redirect

## XDP\_REDIRECT

### Supported maps

XDP\_REDIRECT works with the following map types:

* `BPF_MAP_TYPE_DEVMAP`
* `BPF_MAP_TYPE_DEVMAP_HASH`
* `BPF_MAP_TYPE_CPUMAP`
* `BPF_MAP_TYPE_XSKMAP`

For more information on these maps, please see the specific map documentation.

### Process

XDP\_REDIRECT works by a three-step process, implemented in the functions
below:

1. The `bpf_redirect()` and `bpf_redirect_map()` helpers will lookup the target
   of the redirect and store it (along with some other metadata) in a per-CPU
   `struct bpf_redirect_info`.
2. When the program returns the XDP\_REDIRECT return code, the driver will
   call `xdp_do_redirect()` which will use the information in `struct
   bpf_redirect_info` to actually enqueue the frame into a map type-specific
   bulk queue structure.
3. Before exiting its NAPI poll loop, the driver will call
   `xdp_do_flush()`, which will flush all the different bulk queues,
   thus completing the redirect. Note that `xdp_do_flush()` must be
   called before [`napi_complete_done()`](../networking/kapi.html#c.napi_complete_done "napi_complete_done") in the driver, as the
   XDP\_REDIRECT logic relies on being inside a single NAPI instance
   through to the `xdp_do_flush()` call for RCU protection of all
   in-kernel data structures.

Note

Not all drivers support transmitting frames after a redirect, and for
those that do, not all of them support non-linear frames. Non-linear xdp
bufs/frames are bufs/frames that contain more than one fragment.

### Debugging packet drops

Silent packet drops for XDP\_REDIRECT can be debugged using:

* bpf\_trace
* perf\_record

#### bpf\_trace

The following bpftrace command can be used to capture and count all XDP tracepoints:

```
sudo bpftrace -e 'tracepoint:xdp:* { @cnt[probe] = count(); }'
Attaching 12 probes...
^C

@cnt[tracepoint:xdp:mem_connect]: 18
@cnt[tracepoint:xdp:mem_disconnect]: 18
@cnt[tracepoint:xdp:xdp_exception]: 19605
@cnt[tracepoint:xdp:xdp_devmap_xmit]: 1393604
@cnt[tracepoint:xdp:xdp_redirect]: 22292200
```

Note

The various xdp tracepoints can be found in `source/include/trace/events/xdp.h`

The following bpftrace command can be used to extract the `ERRNO` being returned as
part of the err parameter:

```
sudo bpftrace -e \
'tracepoint:xdp:xdp_redirect*_err {@redir_errno[-args->err] = count();}
tracepoint:xdp:xdp_devmap_xmit {@devmap_errno[-args->err] = count();}'
```

#### perf record

The perf tool also supports recording tracepoints:

```
perf record -a -e xdp:xdp_redirect_err \
    -e xdp:xdp_redirect_map_err \
    -e xdp:xdp_exception \
    -e xdp:xdp_devmap_xmit
```

##### References

* <https://github.com/xdp-project/xdp-tutorial/tree/master/tracing02-xdp-monitor>
