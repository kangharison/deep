# FUSE (Filesystem in Userspace) Technical Documentation

> 출처(원문): https://docs.kernel.org/filesystems/fuse/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# FUSE (Filesystem in Userspace) Technical Documentation

* [1. FUSE Overview](fuse.html)
  + [1.1. Definitions](fuse.html#definitions)
  + [1.2. What is FUSE?](fuse.html#what-is-fuse)
  + [1.3. Filesystem type](fuse.html#filesystem-type)
  + [1.4. Mount options](fuse.html#mount-options)
  + [1.5. Control filesystem](fuse.html#control-filesystem)
  + [1.6. Aborting a filesystem connection](fuse.html#aborting-a-filesystem-connection)
  + [1.7. How do non-privileged mounts work?](fuse.html#how-do-non-privileged-mounts-work)
  + [1.8. How are requirements fulfilled?](fuse.html#how-are-requirements-fulfilled)
  + [1.9. I think these limitations are unacceptable?](fuse.html#i-think-these-limitations-are-unacceptable)
  + [1.10. Kernel - userspace interface](fuse.html#kernel-userspace-interface)
* [2. FUSE I/O Modes](fuse-io.html)
* [3. FUSE-over-io-uring design documentation](fuse-io-uring.html)
  + [3.1. Limitations](fuse-io-uring.html#limitations)
  + [3.2. Fuse io-uring configuration](fuse-io-uring.html#fuse-io-uring-configuration)
* [4. FUSE Passthrough](fuse-passthrough.html)
  + [4.1. Introduction](fuse-passthrough.html#introduction)
  + [4.2. Enabling Passthrough](fuse-passthrough.html#enabling-passthrough)
  + [4.3. Privilege Requirements](fuse-passthrough.html#privilege-requirements)
