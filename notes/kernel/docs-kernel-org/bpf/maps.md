# BPF maps

> 출처(원문): https://docs.kernel.org/bpf/maps.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# BPF maps

BPF ‘maps’ provide generic storage of different types for sharing data between
kernel and user space. There are several storage types available, including
hash, array, bloom filter and radix-tree. Several of the map types exist to
support specific BPF helpers that perform actions based on the map contents. The
maps are accessed from BPF programs via BPF helpers which are documented in the
[man-pages](https://www.kernel.org/doc/man-pages/) for [bpf-helpers(7)](https://man7.org/linux/man-pages/man7/bpf-helpers.7.html).

BPF maps are accessed from user space via the `bpf` syscall, which provides
commands to create maps, lookup elements, update elements and delete elements.
More details of the BPF syscall are available in [ebpf-syscall](https://docs.kernel.org/userspace-api/ebpf/syscall.html) and in the
[man-pages](https://www.kernel.org/doc/man-pages/) for [bpf(2)](https://man7.org/linux/man-pages/man2/bpf.2.html).

## Map Types

* [BPF\_MAP\_TYPE\_ARRAY and BPF\_MAP\_TYPE\_PERCPU\_ARRAY](map_array.html)
* [BPF\_MAP\_TYPE\_BLOOM\_FILTER](map_bloom_filter.html)
* [BPF\_MAP\_TYPE\_CGROUP\_STORAGE](map_cgroup_storage.html)
* [BPF\_MAP\_TYPE\_CGRP\_STORAGE](map_cgrp_storage.html)
* [BPF\_MAP\_TYPE\_CPUMAP](map_cpumap.html)
* [BPF\_MAP\_TYPE\_DEVMAP and BPF\_MAP\_TYPE\_DEVMAP\_HASH](map_devmap.html)
* [BPF\_MAP\_TYPE\_HASH, with PERCPU and LRU Variants](map_hash.html)
* [BPF\_MAP\_TYPE\_LPM\_TRIE](map_lpm_trie.html)
* [BPF\_MAP\_TYPE\_ARRAY\_OF\_MAPS and BPF\_MAP\_TYPE\_HASH\_OF\_MAPS](map_of_maps.html)
* [BPF\_MAP\_TYPE\_QUEUE and BPF\_MAP\_TYPE\_STACK](map_queue_stack.html)
* [BPF\_MAP\_TYPE\_SK\_STORAGE](map_sk_storage.html)
* [BPF\_MAP\_TYPE\_SOCKMAP and BPF\_MAP\_TYPE\_SOCKHASH](map_sockmap.html)
* [BPF\_MAP\_TYPE\_XSKMAP](map_xskmap.html)

## Usage Notes

int bpf(int command, union bpf\_attr \*attr, u32 size)

Use the `bpf()` system call to perform the operation specified by
`command`. The operation takes parameters provided in `attr`. The `size`
argument is the size of the `union bpf_attr` in `attr`.

**BPF\_MAP\_CREATE**

Create a map with the desired type and attributes in `attr`:

```
int fd;
union bpf_attr attr = {
        .map_type = BPF_MAP_TYPE_ARRAY;  /* mandatory */
        .key_size = sizeof(__u32);       /* mandatory */
        .value_size = sizeof(__u32);     /* mandatory */
        .max_entries = 256;              /* mandatory */
        .map_flags = BPF_F_MMAPABLE;
        .map_name = "example_array";
};

fd = bpf(BPF_MAP_CREATE, &attr, sizeof(attr));
```

Returns a process-local file descriptor on success, or negative error in case of
failure. The map can be deleted by calling `close(fd)`. Maps held by open
file descriptors will be deleted automatically when a process exits.

Note

Valid characters for `map_name` are `A-Z`, `a-z`, `0-9`,
`'_'` and `'.'`.

**BPF\_MAP\_LOOKUP\_ELEM**

Lookup key in a given map using `attr->map_fd`, `attr->key`,
`attr->value`. Returns zero and stores found elem into `attr->value` on
success, or negative error on failure.

**BPF\_MAP\_UPDATE\_ELEM**

Create or update key/value pair in a given map using `attr->map_fd`, `attr->key`,
`attr->value`. Returns zero on success or negative error on failure.

**BPF\_MAP\_DELETE\_ELEM**

Find and delete element by key in a given map using `attr->map_fd`,
`attr->key`. Returns zero on success or negative error on failure.
