# Building libbpf

> 출처(원문): https://docs.kernel.org/bpf/libbpf/libbpf_build.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Building libbpf

libelf and zlib are internal dependencies of libbpf and thus are required to link
against and must be installed on the system for applications to work.
pkg-config is used by default to find libelf, and the program called
can be overridden with PKG\_CONFIG.

If using pkg-config at build time is not desired, it can be disabled by
setting NO\_PKG\_CONFIG=1 when calling make.

To build both static libbpf.a and shared libbpf.so:

```
$ cd src
$ make
```

To build only static libbpf.a library in directory build/ and install them
together with libbpf headers in a staging directory root/:

```
$ cd src
$ mkdir build root
$ BUILD_STATIC_ONLY=y OBJDIR=build DESTDIR=root make install
```

To build both static libbpf.a and shared libbpf.so against a custom libelf
dependency installed in /build/root/ and install them together with libbpf
headers in a build directory /build/root/:

```
$ cd src
$ PKG_CONFIG_PATH=/build/root/lib64/pkgconfig DESTDIR=/build/root make
```
