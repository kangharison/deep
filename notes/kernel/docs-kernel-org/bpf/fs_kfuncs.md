# BPF filesystem kfuncs

> 출처(원문): https://docs.kernel.org/bpf/fs_kfuncs.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# BPF filesystem kfuncs

BPF LSM programs need to access filesystem data from LSM hooks. The following
BPF kfuncs can be used to get these data.

> * `bpf_get_file_xattr()`
> * `bpf_get_fsverity_digest()`

To avoid recursions, these kfuncs follow the following rules:

1. These kfuncs are only permitted from BPF LSM function.
2. These kfuncs should not call into other LSM hooks, i.e. security\_\*(). For
   example, `bpf_get_file_xattr()` does not use `vfs_getxattr()`, because
   the latter calls LSM hook `security_inode_getxattr`.
